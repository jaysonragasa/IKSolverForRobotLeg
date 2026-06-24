#pragma once
#include <WebServer.h>
#include "RobotController.h"

class WebManager {
public:
    WebManager(RobotController& robot);

    void begin();
    void update();

private:
    RobotController& robot;
    WebServer server;

    void connectWiFi();
    void setupRoutes();

    void handleRoot();
    void handleIK();
    void handleRC();
    void handleGait();
    void handlePID();
    void handleDeadband();
    void handleCalibrate();
    void handleOffset();
    void handleToggle();
    void handleState();
};
