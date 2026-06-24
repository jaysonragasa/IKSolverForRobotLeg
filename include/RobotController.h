#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "ServoController.h"
#include "DisplayManager.h"
#include "GaitController.h"
#include "Kinematics.h"
#include "IMUManager.h"
#include <Preferences.h>

class RobotController {
public:
    RobotController();

    void begin();
    void update();
    void updateHardware();
    
    // Set global IK target pose
    void setIK(float tx, float ty, float tz, float tpitch = 0.0f, float troll = 0.0f);
    void setRC(float t, float y, float p, float r, float s);
    void setPID(float p, float i, float d);
    void setIMUDeadband(float db);
    void setToggles(bool autoBalance, bool pidEnabled);
    void setAnimation(int mode);
    void setLogicalOffset(int leg, int joint, float offset);
    float getLogicalOffset(int leg, int joint);

    void calibrateIMU();

    // Getters for display
    float getCoxaAngle(int legIndex) const;
    float getFemurAngle(int legIndex) const;
    float getTibiaAngle(int legIndex) const;

    // Getters for Web UI State
    float getTX() { return tX; }
    float getTY() { return tY; }
    float getTZ() { return tZ; }
    float getTPitch() { return tPitch; }
    float getTRoll() { return tRoll; }
    
    bool getAutoBalance() { return preferences.getBool("ab_en", false); }
    bool getPIDEnabled() { return preferences.getBool("pid_en", false); }
    float getKp() { return preferences.getFloat("pid_p", 1.0f); }
    float getKi() { return preferences.getFloat("pid_i", 0.0f); }
    float getKd() { return preferences.getFloat("pid_d", 0.0f); }
    float getDeadband() { return preferences.getFloat("imu_db", 0.0f); }

private:
    ServoController servoController;
    DisplayManager displayManager;
    GaitController gaitController;
    IMUManager imuManager;
    Preferences preferences;

    // Current global IK state
    // Target IK pose (for smooth interpolation)
    float targetTX, targetTY, targetTZ, targetTPitch, targetTRoll;

    // Current interpolated IK pose
    float tX, tY, tZ, tPitch, tRoll;

    // Cached angles for display
    LegAngles currentAngles[4];

    // System stats tracking
    uint32_t lastDisplayUpdate = 0;
    uint32_t frameCount = 0;
    float loopHz = 0.0f;
};
