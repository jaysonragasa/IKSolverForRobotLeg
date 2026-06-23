#include "RobotController.h"
#include "Config.h"
#include <WiFi.h>

RobotController::RobotController() 
    : gaitController(servoController), tX(0), tY(-100), tZ(28) {}

void RobotController::begin() {
    // Explicitly start I2C on pins 21 and 22 for ESP32
    // Wire.begin(21, 22); is already called inside imuManager.init() now, 
    // but doing it here or there is fine.
    imuManager.init();
    
    Wire.setClock(400000); // Fast I2C for faster OLED updates

    displayManager.begin();
    servoController.begin();

    // Load PID values from NVS
    preferences.begin("robot", false);
    float p = preferences.getFloat("pid_p", 1.0f);
    float i = preferences.getFloat("pid_i", 0.0f);
    float d = preferences.getFloat("pid_d", 0.0f);
    gaitController.setPID(p, i, d);

    float db = preferences.getFloat("imu_db", 0.0f);
    gaitController.setIMUDeadband(db);

    bool ab = preferences.getBool("ab_en", false);
    bool pidEn = preferences.getBool("pid_en", false);
    gaitController.setToggles(ab, pidEn);

    for (int ch = 0; ch < 16; ch++) {
        String key = "off_" + String(ch);
        float off = preferences.getFloat(key.c_str(), 0.0f);
        servoController.setOffset(ch, off);
    }

    updateHardware(); // Initial pose
}

void RobotController::update() {
    imuManager.update();
    
    // Pass the calculated absolute body orientation down to the physics engine
    gaitController.setIMU(imuManager.getPitch(), imuManager.getRoll());
    gaitController.setIMUGyro(imuManager.getGyroPitchRate(), imuManager.getGyroRollRate());
    
    gaitController.update(tX, tY, tZ);
}

void RobotController::setIK(float tx, float ty, float tz) {
    tX = tx; tY = ty; tZ = tz;
    // The gaitController will constantly calculate stance even when standing still.
}

void RobotController::setRC(float t, float y, float p, float r, float s) {
    gaitController.setRC(t, y, p, r, s);
}

void RobotController::setPID(float p, float i, float d) {
    preferences.putFloat("pid_p", p);
    preferences.putFloat("pid_i", i);
    preferences.putFloat("pid_d", d);
    gaitController.setPID(p, i, d);
}

void RobotController::setIMUDeadband(float db) {
    preferences.putFloat("imu_db", db);
    gaitController.setIMUDeadband(db);
}

void RobotController::setToggles(bool autoBal, bool pidEn) {
    gaitController.setToggles(autoBal, pidEn);
    preferences.putBool("ab_en", autoBal);
    preferences.putBool("pid_en", pidEn);
}

void RobotController::setServoOffset(int channel, float offset) {
    servoController.setOffset(channel, offset);
    String key = "off_" + String(channel);
    preferences.putFloat(key.c_str(), offset);
}

void RobotController::calibrateIMU() {
    imuManager.calibrate();
}

void RobotController::updateHardware() {
    LegAngles angles = IKSolver::calculate(tX, tY, tZ, 0, 0, 0);

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
