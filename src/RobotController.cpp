#include "RobotController.h"
#include "Config.h"
#include <WiFi.h>

/**
 * @brief Construct a new RobotController, initializing subsystem references
 * and zeroing out the initial IK pose.
 */
RobotController::RobotController() 
    : gaitController(servoController), tX(0), tY(-100), tZ(28) {}

/**
 * @brief System boot sequence. Initializes I2C, starts all hardware managers,
 * and restores the exact previous state and calibration from NVS.
 */
void RobotController::begin() {
    // Explicitly start I2C on pins 21 and 22 for ESP32
    // Wire.begin(21, 22); is already called inside imuManager.init() now, 
    // but doing it here or there is fine.
    imuManager.init();
    
    Wire.setClock(400000); // Fast I2C for faster OLED updates

    displayManager.begin();
    servoController.begin();

    // Load IK Target Pose
    preferences.begin("robot", false);
    tX = preferences.getFloat("ik_tx", 0.0f);
    tY = preferences.getFloat("ik_ty", -100.0f);
    tZ = preferences.getFloat("ik_tz", 28.0f);

    // Load PID values from NVS
    float p = preferences.getFloat("pid_p", 1.0f);
    float i = preferences.getFloat("pid_i", 0.0f);
    float d = preferences.getFloat("pid_d", 0.0f);
    gaitController.setPID(p, i, d);

    float db = preferences.getFloat("imu_db", 0.0f);
    gaitController.setIMUDeadband(db);

    bool ab = preferences.getBool("ab_en", false);
    bool pidEn = preferences.getBool("pid_en", false);
    gaitController.setToggles(ab, pidEn);

    // Load Logical Offsets
    for (int i = 0; i < 4; i++) {
        String prefix = "off_l" + String(i);
        
        float coxaOff = preferences.getFloat((prefix + "_c").c_str(), 0.0f);
        servoController.setOffset(Config::LEGS[i].coxa, coxaOff);

        float femurOff = preferences.getFloat((prefix + "_f").c_str(), 0.0f);
        servoController.setOffset(Config::LEGS[i].femur, femurOff);

        float tibiaOff = preferences.getFloat((prefix + "_t").c_str(), 0.0f);
        servoController.setOffset(Config::LEGS[i].tibia, tibiaOff);
    }

    updateHardware(); // Initial pose
}

/**
 * @brief The primary execution loop. Must be called rapidly.
 * Polls sensors, feeds the physics engine, and manages OLED telemetry.
 */
void RobotController::update() {
    imuManager.update();
    
    // Pass the calculated absolute body orientation down to the physics engine
    gaitController.setIMU(imuManager.getPitch(), imuManager.getRoll());
    gaitController.setIMUGyro(imuManager.getGyroPitchRate(), imuManager.getGyroRollRate());
    
    gaitController.update(tX, tY, tZ);

    // Track Loop Hz and update OLED every 1 second
    frameCount++;
    uint32_t now = millis();
    if (now - lastDisplayUpdate >= 1000) {
        loopHz = (float)frameCount / ((now - lastDisplayUpdate) / 1000.0f);
        uint32_t freeRam = ESP.getFreeHeap();
        bool isConnected = (WiFi.status() == WL_CONNECTED);
        
        displayManager.update(loopHz, freeRam, isConnected, WiFi.localIP().toString().c_str());
        
        frameCount = 0;
        lastDisplayUpdate = now;
    }
}

void RobotController::setIK(float tx, float ty, float tz) {
    tX = tx; tY = ty; tZ = tz;
    preferences.putFloat("ik_tx", tx);
    preferences.putFloat("ik_ty", ty);
    preferences.putFloat("ik_tz", tz);
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

void RobotController::setLogicalOffset(int leg, int joint, float offset) {
    uint8_t pin = 0;
    String suffix = "";
    if (joint == 0) { pin = Config::LEGS[leg].coxa; suffix = "_c"; }
    else if (joint == 1) { pin = Config::LEGS[leg].femur; suffix = "_f"; }
    else if (joint == 2) { pin = Config::LEGS[leg].tibia; suffix = "_t"; }

    servoController.setOffset(pin, offset);
    String key = "off_l" + String(leg) + suffix;
    preferences.putFloat(key.c_str(), offset);
}

float RobotController::getLogicalOffset(int leg, int joint) {
    String suffix = "";
    if (joint == 0) suffix = "_c";
    else if (joint == 1) suffix = "_f";
    else if (joint == 2) suffix = "_t";
    
    String key = "off_l" + String(leg) + suffix;
    return preferences.getFloat(key.c_str(), 0.0f);
}

void RobotController::calibrateIMU() {
    imuManager.calibrate();
}

/**
 * @brief Bypasses the GaitController and instantly computes Inverse Kinematics
 * for the current target coordinates. Used primarily during startup or static posing.
 */
void RobotController::updateHardware() {
    LegAngles angles = IKSolver::calculate(tX, tY, tZ, 0, 0, 0);

    for (int i = 0; i < 4; i++) {
        servoController.setAngle(Config::LEGS[i].coxa, angles.coxa);
        servoController.setAngle(Config::LEGS[i].femur, angles.femur);
        servoController.setAngle(Config::LEGS[i].tibia, angles.tibia);
        currentAngles[i] = angles;
    }
}

float RobotController::getCoxaAngle(int legIndex) const { return currentAngles[legIndex].coxa; }
float RobotController::getFemurAngle(int legIndex) const { return currentAngles[legIndex].femur; }
float RobotController::getTibiaAngle(int legIndex) const { return currentAngles[legIndex].tibia; }
