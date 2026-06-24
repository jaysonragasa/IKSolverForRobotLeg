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
    server.on("/deadband", HTTP_GET, [this]() { handleDeadband(); });
    server.on("/calibrate", HTTP_GET, [this]() { handleCalibrate(); });
    server.on("/offset", HTTP_GET, [this]() { handleOffset(); });
    server.on("/toggle", HTTP_GET, [this]() { handleToggle(); });
    server.on("/anim", HTTP_GET, [this]() { handleAnim(); });
    server.on("/state", HTTP_GET, [this]() { handleState(); });
}

void WebManager::handleRoot() { 
    server.send(200, "text/html", index_html); 
}

void WebManager::handleIK() {
    float x = server.hasArg("x") ? server.arg("x").toFloat() : robot.getTX();
    float y = server.hasArg("y") ? server.arg("y").toFloat() : robot.getTY();
    float z = server.hasArg("z") ? server.arg("z").toFloat() : robot.getTZ();
    float p = server.hasArg("p") ? server.arg("p").toFloat() : robot.getTPitch();
    float r = server.hasArg("r") ? server.arg("r").toFloat() : robot.getTRoll();
    robot.setIK(x, y, z, p, r);
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

void WebManager::handleAnim() {
    if (server.hasArg("mode")) {
        int mode = server.arg("mode").toInt();
        robot.setAnimation(mode);
    }
    server.send(200, "text/plain", "OK");
}

void WebManager::handleState() {
    String json = "{";
    
    json += "\"ik\":[" + String(robot.getTX()) + "," + String(robot.getTY()) + "," + String(robot.getTZ()) + "," + String(robot.getTPitch()) + "," + String(robot.getTRoll()) + "],";
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
