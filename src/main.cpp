#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>

#include "Config.h"
#include "DisplayManager.h"
#include "Kinematics.h"
#include "ServoController.h"
#include "WebInterface.h"

// --- Global Objects ---
WebServer server(80);
ServoController servoController;
DisplayManager displayManager;

// Initial Safe State
float tX = 0, tY = -80, tZ = 28;
float oC = 0, oF = 0, oT = 0;

void updateHardware() {
  LegAngles angles = IKSolver::calculate(tX, tY, tZ, oC, oF, oT);

  Serial.printf("Calculated Angles: Coxa=%.1f Femur=%.1f Tibia=%.1f\n", angles.coxa, angles.femur, angles.tibia);

  // Send the ideal mathematical angles directly to all 4 legs.
  // The physical inversion is handled internally by the ServoController.
  for (int i = 0; i < 4; i++) {
    servoController.setAngle(Config::LEGS[i].coxa, angles.coxa);
    servoController.setAngle(Config::LEGS[i].femur, angles.femur);
    servoController.setAngle(Config::LEGS[i].tibia, angles.tibia);
  }

  bool isConnected = (WiFi.status() == WL_CONNECTED);
  // displayManager.update(angles.coxa, angles.femur, angles.tibia, isConnected,
  //                       WiFi.localIP().toString().c_str());
}

// --- Web Server Endpoints ---
void handleRoot() { server.send(200, "text/html", index_html); }

void handleIK() {
  // Read parameters from the browser's HTTP fetch
  if (server.hasArg("x"))
    tX = server.arg("x").toFloat();
  if (server.hasArg("y"))
    tY = server.arg("y").toFloat();
  if (server.hasArg("z"))
    tZ = server.arg("z").toFloat();
  if (server.hasArg("cx"))
    oC = server.arg("cx").toFloat();
  if (server.hasArg("fm"))
    oF = server.arg("fm").toFloat();
  if (server.hasArg("tb"))
    oT = server.arg("tb").toFloat();

  // Serial.printf("Received IK Command: x=%.1f y=%.1f z=%.1f | oC=%.1f oF=%.1f oT=%.1f\n", tX, tY, tZ, oC, oF, oT);

  // Trigger hardware update
  updateHardware();

  server.send(200, "text/plain", "OK");
}

// C:\Users\aragasa\.platformio\penv\Scripts\pio.exe run -t upload

// --- Main Setup ---
void setup() {
  Serial.begin(115200);

  // Explicitly start I2C on pins 21 and 22 for ESP32
  Wire.begin(21, 22);
  Wire.setClock(400000); // Fast I2C for faster OLED updates

  displayManager.begin();
  servoController.begin();

  // // --- Configure Inversions ---
  // // Front-Left
  // servoController.setInvert(Config::LEGS[0].coxa, -1);
  // servoController.setInvert(Config::LEGS[0].femur, 1);
  // servoController.setInvert(Config::LEGS[0].tibia, -1);

  // // Front-Right
  // servoController.setInvert(Config::LEGS[1].coxa, 1);
  // servoController.setInvert(Config::LEGS[1].femur, -1);
  // servoController.setInvert(Config::LEGS[1].tibia, 1);

  // // Hind-Left
  // servoController.setInvert(Config::LEGS[2].coxa, 1);
  // servoController.setInvert(Config::LEGS[2].femur, 1);
  // servoController.setInvert(Config::LEGS[2].tibia, -1);

  // // Hind-Right
  // servoController.setInvert(Config::LEGS[3].coxa, -1);
  // servoController.setInvert(Config::LEGS[3].femur, -1);
  // servoController.setInvert(Config::LEGS[3].tibia, 1);

  // Initial positioning
  // > 
  tX = 0, 
  tY = -80, 
  tZ = 28;
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

void loop() { server.handleClient(); }