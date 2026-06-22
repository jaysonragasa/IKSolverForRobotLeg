#include "WebManager.h"
#include "WebInterface.h"
#include "Config.h"
#include <WiFi.h>

WebManager::WebManager(RobotController& robot) : robot(robot), server(80) {}

void WebManager::begin() {
    connectWiFi();
    setupRoutes();
    server.begin();
}

void WebManager::update() {
    server.handleClient();
}

void WebManager::connectWiFi() {
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
    
    // Tell the robot to update its display with the new IP
    robot.updateHardware(); 
}

void WebManager::setupRoutes() {
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/ik", HTTP_GET, [this]() { handleIK(); });
    server.on("/rc", HTTP_GET, [this]() { handleRC(); });
    server.on("/gait", HTTP_GET, [this]() { handleGait(); });
    server.on("/pid", HTTP_GET, [this]() { handlePID(); });
    server.on("/calibrate", HTTP_GET, [this]() { handleCalibrate(); });
}

void WebManager::handleRoot() { 
    server.send(200, "text/html", index_html); 
}

void WebManager::handleIK() {
    float tx = 0, ty = -80, tz = 28;
    float oc = 0, of = 0, ot = 0;

    if (server.hasArg("x")) tx = server.arg("x").toFloat();
    if (server.hasArg("y")) ty = server.arg("y").toFloat();
    if (server.hasArg("z")) tz = server.arg("z").toFloat();
    if (server.hasArg("cx")) oc = server.arg("cx").toFloat();
    if (server.hasArg("fm")) of = server.arg("fm").toFloat();
    if (server.hasArg("tb")) ot = server.arg("tb").toFloat();

    robot.setIK(tx, ty, tz, oc, of, ot);
    server.send(200, "text/plain", "OK");
}

void WebManager::handleRC() {
    float t = 0, y = 0, p = 0, r = 0, s = 0;
    if (server.hasArg("t")) t = server.arg("t").toFloat();
    if (server.hasArg("y")) y = server.arg("y").toFloat();
    if (server.hasArg("p")) p = server.arg("p").toFloat();
    if (server.hasArg("r")) r = server.arg("r").toFloat();
    if (server.hasArg("s")) s = server.arg("s").toFloat();

    robot.setRC(t, y, p, r, s);
    server.send(200, "text/plain", "OK");
}

void WebManager::handleGait() {
    if (server.hasArg("cmd")) {
        String cmd = server.arg("cmd");
        if (cmd == "stop") robot.setRC(0, 0, 0, 0, 0);
        else if (cmd == "forward") robot.setRC(1.0, 0, 0, 0, 0);
        else if (cmd == "backward") robot.setRC(-1.0, 0, 0, 0, 0);
        else if (cmd == "strafe_left") robot.setRC(0, 0, 0, 0, 1.0);
        else if (cmd == "strafe_right") robot.setRC(0, 0, 0, 0, -1.0);
        else if (cmd == "rotate_left") robot.setRC(0, -1.0, 0, 0, 0);
        else if (cmd == "rotate_right") robot.setRC(0, 1.0, 0, 0, 0);
    }
    server.send(200, "text/plain", "OK");
}

void WebManager::handlePID() {
    if (server.hasArg("p") && server.hasArg("i") && server.hasArg("d")) {
        float p = server.arg("p").toFloat();
        float i = server.arg("i").toFloat();
        float d = server.arg("d").toFloat();
        robot.setPID(p, i, d);
    }
    server.send(200, "text/plain", "OK");
}

void WebManager::handleCalibrate() {
    robot.calibrateIMU();
    server.send(200, "text/plain", "OK");
}
