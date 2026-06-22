#include <Arduino.h>
#include "RobotController.h"
#include "WebManager.h"

// Instantiate the global managers
RobotController robot;
WebManager webManager(robot);

void setup() {
    Serial.begin(115200);

    // Initialize hardware (I2C, Servos, OLED, initial pose)
    robot.begin();

    // Initialize networking (WiFi, HTTP routes)
    webManager.begin();
}

void loop() {
    // Process incoming HTTP requests
    webManager.update();

    // Process procedural animation updates
    robot.update();

    delay(10);
}