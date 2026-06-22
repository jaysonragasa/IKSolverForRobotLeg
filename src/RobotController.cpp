#include "RobotController.h"
#include "Config.h"
#include <WiFi.h>

RobotController::RobotController() 
    : gaitController(servoController), tX(0), tY(-80), tZ(28), oC(0), oF(0), oT(0) {}

void RobotController::begin() {
    // Explicitly start I2C on pins 21 and 22 for ESP32
    // Wire.begin(21, 22); is already called inside imuManager.init() now, 
    // but doing it here or there is fine.
    imuManager.init();
    
    Wire.setClock(400000); // Fast I2C for faster OLED updates

    displayManager.begin();
    servoController.begin();

    updateHardware(); // Initial pose
}

void RobotController::update() {
    imuManager.update();
    
    // Pass the calculated absolute body orientation down to the physics engine
    gaitController.setIMU(imuManager.getPitch(), imuManager.getRoll());
    
    gaitController.update(tX, tY, tZ, oC, oF, oT);
}

void RobotController::setIK(float tx, float ty, float tz, float oc, float of, float ot) {
    tX = tx; tY = ty; tZ = tz;
    oC = oc; oF = of; oT = ot;
    // The gaitController will constantly calculate stance even when standing still.
}

void RobotController::setRC(float t, float y, float p, float r, float s) {
    gaitController.setRC(t, y, p, r, s);
}

void RobotController::calibrateIMU() {
    imuManager.calibrate();
}

void RobotController::updateHardware() {
    LegAngles angles = IKSolver::calculate(tX, tY, tZ, oC, oF, oT);

    for (int i = 0; i < 4; i++) {
        servoController.setAngle(Config::LEGS[i].coxa, angles.coxa);
        servoController.setAngle(Config::LEGS[i].femur, angles.femur);
        servoController.setAngle(Config::LEGS[i].tibia, angles.tibia);
        currentAngles[i] = angles;
    }

    bool isConnected = (WiFi.status() == WL_CONNECTED);
    displayManager.update(angles.coxa, angles.femur, angles.tibia, isConnected,
                          WiFi.localIP().toString().c_str());
}

float RobotController::getCoxaAngle(int legIndex) const { return currentAngles[legIndex].coxa; }
float RobotController::getFemurAngle(int legIndex) const { return currentAngles[legIndex].femur; }
float RobotController::getTibiaAngle(int legIndex) const { return currentAngles[legIndex].tibia; }
