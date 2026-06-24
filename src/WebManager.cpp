#include "WebManager.h"
#include "WebInterface.h"
#include "Config.h"
#include <WiFi.h>

/**
 * @brief Construct a new WebManager and binds to the RobotController on port 80.
 */
WebManager::WebManager(RobotController& robot) : robot(robot), server(80) {}

/**
 * @brief Attempts connection to the router and configures the HTTP routes.
 */
void WebManager::begin() {
    connectWiFi();
    setupRoutes();
    server.begin();
}

/**
 * @brief Polls the HTTP server to handle incoming client requests.
 * Must be called rapidly in the main loop to prevent request timeouts.
 */
void WebManager::update() {
    server.handleClient();
}

/**
 * @brief Blocks and attempts to connect to the configured SSID.
 */
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

/**
 * @brief Binds all URI endpoints to the specific handler methods.
 */
void WebManager::setupRoutes() {
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/ik", HTTP_GET, [this]() { handleIK(); });
    server.on("/rc", HTTP_GET, [this]() { handleRC(); });
    server.on("/gait", HTTP_GET, [this]() { handleGait(); });
    server.on("/pid", HTTP_GET, [this]() { handlePID(); });
    server.on("/deadband", HTTP_GET, [this]() { handleDeadband(); });
    server.on("/calibrate", HTTP_GET, [this]() { handleCalibrate(); });
    server.on("/offset", HTTP_GET, [this]() { handleOffset(); });
    server.on("/toggle", HTTP_GET, [this]() { handleToggle(); });
    server.on("/state", HTTP_GET, [this]() { handleState(); });
}

/**
 * @brief Endpoint `/`. Sends the full `WebInterface.h` HTML/JS single-page app.
 */
void WebManager::handleRoot() { 
    server.send(200, "text/html", index_html); 
}

/**
 * @brief Endpoint `/pose` and `/ik`. Updates the permanent IK targets.
 */
void WebManager::handleIK() {
    float tx = 0, ty = -80, tz = 28;

    if (server.hasArg("x")) tx = server.arg("x").toFloat();
    if (server.hasArg("y")) ty = server.arg("y").toFloat();
    if (server.hasArg("z")) tz = server.arg("z").toFloat();

    robot.setIK(tx, ty, tz);
    server.send(200, "text/plain", "OK");
}

/**
 * @brief Endpoint `/rc`. Receives joystick vectors and applies them to the GaitController.
 */
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
        server.send(200, "text/plain", "PID OK");
    } else {
        server.send(400, "text/plain", "Bad Args");
    }
}

void WebManager::handleDeadband() {
    if (server.hasArg("v")) {
        float db = server.arg("v").toFloat();
        robot.setIMUDeadband(db);
        server.send(200, "text/plain", "Deadband OK");
    } else {
        server.send(400, "text/plain", "Bad Args");
    }
}

void WebManager::handleCalibrate() {
    robot.calibrateIMU();
    server.send(200, "text/plain", "OK");
}

void WebManager::handleOffset() {
    if (server.hasArg("leg") && server.hasArg("joint") && server.hasArg("val")) {
        int leg = server.arg("leg").toInt();
        int joint = server.arg("joint").toInt();
        float val = server.arg("val").toFloat();
        robot.setLogicalOffset(leg, joint, val);
    }
    server.send(200, "text/plain", "OK");
}

void WebManager::handleToggle() {
    if (server.hasArg("ab") && server.hasArg("pid")) {
        bool ab = (server.arg("ab") == "1");
        bool pid = (server.arg("pid") == "1");
        robot.setToggles(ab, pid);
    }
    server.send(200, "text/plain", "OK");
}

/**
 * @brief Endpoint `/state`. Returns a JSON string containing current physical telemetry.
 * Used by the WebGL canvas to sync the 3D model with reality.
 */
void WebManager::handleState() {
    String json = "{";
    
    json += "\"ik\":[" + String(robot.getTX()) + "," + String(robot.getTY()) + "," + String(robot.getTZ()) + "],";
    json += "\"ab\":" + String(robot.getAutoBalance() ? "true" : "false") + ",";
    json += "\"pid_en\":" + String(robot.getPIDEnabled() ? "true" : "false") + ",";
    json += "\"pid\":[" + String(robot.getKp()) + "," + String(robot.getKi()) + "," + String(robot.getKd()) + "],";
    json += "\"db\":" + String(robot.getDeadband()) + ",";

    json += "\"offsets\":[";
    for (int leg = 0; leg < 4; leg++) {
        json += "[";
        for (int joint = 0; joint < 3; joint++) {
            json += String(robot.getLogicalOffset(leg, joint));
            if (joint < 2) json += ",";
        }
        json += "]";
        if (leg < 3) json += ",";
    }
    json += "]}";
    
    server.send(200, "application/json", json);
}
