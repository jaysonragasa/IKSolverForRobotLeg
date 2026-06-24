#pragma once
#include <WebServer.h>
#include "RobotController.h"

/**
 * @class WebManager
 * @brief Handles Wi-Fi connection, HTTP routing, and the Web UI API.
 */
class WebManager {
public:
    /**
     * @brief Constructs the WebManager.
     * @param robot Reference to the central RobotController.
     */
    WebManager(RobotController& robot);

    /**
     * @brief Initializes Wi-Fi and starts the asynchronous HTTP server.
     */
    void begin();

    /**
     * @brief Polls the web server for incoming client requests.
     */
    void update();

private:
    RobotController& robot;
    WebServer server;

    /**
     * @brief Attempts connection to the configured Wi-Fi network.
     */
    void connectWiFi();

    /**
     * @brief Binds all URI endpoints to their respective handler functions.
     */
    void setupRoutes();

    // Endpoints
    void handleRoot();      ///< Serves the HTML/JS Web Interface
    void handleIK();        ///< Handles static pose targets
    void handleRC();        ///< Handles joystick commands
    void handleGait();      ///< Handles macro gait commands
    void handlePID();       ///< Handles PID tuning adjustments
    void handleDeadband();  ///< Handles IMU sensitivity
    void handleCalibrate(); ///< Handles IMU leveling trigger
    void handleOffset();    ///< Handles mechanical offsets
    void handleToggle();    ///< Handles enable/disable states
    void handleState();     ///< Serves JSON telemetry to the WebUI
};
