#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>

#include "Config.h"
#include "WebInterface.h"
#include "ServoController.h"
#include "Kinematics.h"
#include "DisplayManager.h"

// --- Global Objects ---
WebServer server(80);
ServoController servoController;
DisplayManager displayManager;

// Initial Safe State
float tX = 0, tY = -80, tZ = 28;
float oC = 0, oF = 0, oT = 0;

void updateHardware() {
    LegAngles angles = IKSolver::calculate(tX, tY, tZ, oC, oF, oT);

    // Invert COXA and TIBIA when sending to the servos.
    // We pass ideal angles (0-180), which the ServoController handles
    // internally based on each pin's configured maxAngle.
    servoController.setAngle(Config::COXA_PIN, 180.0f - angles.coxa);
    servoController.setAngle(Config::FEMUR_PIN, angles.femur);
    servoController.setAngle(Config::TIBIA_PIN, 180.0f - angles.tibia);

    bool isConnected = (WiFi.status() == WL_CONNECTED);
    displayManager.update(angles.coxa, angles.femur, angles.tibia, 
                          isConnected, WiFi.localIP().toString().c_str());
}

// --- Web Server Endpoints ---
void handleRoot() {
    server.send(200, "text/html", index_html);
}

void handleIK() {
    // Read parameters from the browser's HTTP fetch
    if (server.hasArg("x")) tX = server.arg("x").toFloat();
    if (server.hasArg("y")) tY = server.arg("y").toFloat();
    if (server.hasArg("z")) tZ = server.arg("z").toFloat();
    if (server.hasArg("cx")) oC = server.arg("cx").toFloat();
    if (server.hasArg("fm")) oF = server.arg("fm").toFloat();
    if (server.hasArg("tb")) oT = server.arg("tb").toFloat();

    // Trigger hardware update
    updateHardware();

    server.send(200, "text/plain", "OK");
}

// --- Main Setup ---
void setup() {
    Serial.begin(115200);

    // Explicitly start I2C on pins 21 and 22 for ESP32
    Wire.begin(21, 22);
    Wire.setClock(400000); // Fast I2C for faster OLED updates

    displayManager.begin();
    servoController.begin();

    // The max angles for all 16 servos default to 180.0 in the constructor.
    // If you need to configure them specifically (e.g. they only physically reach 147 deg), do it here:
    // servoController.setMaxAngle(Config::COXA_PIN, 147.0f);
    // servoController.setMaxAngle(Config::FEMUR_PIN, 147.0f);
    // servoController.setMaxAngle(Config::TIBIA_PIN, 147.0f);

    // Initial positioning
    updateHardware();

    // Connect to WiFi
    Serial.print("Connecting to WiFi: ");
    Serial.println(Config::SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(Config::SSID, Config::PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi Connected!");
    Serial.print("Access the simulator at: http://");
    Serial.println(WiFi.localIP());

    // Update OLED with new IP
    updateHardware();

    // Start Server
    server.on("/", handleRoot);
    server.on("/ik", handleIK);
    server.begin();
}

void loop() {
    server.handleClient();
}