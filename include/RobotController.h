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
    
    // IK State setters
    void setIK(float tx, float ty, float tz);
    void setRC(float t, float y, float p, float r, float s);
    void setPID(float p, float i, float d);
    void setIMUDeadband(float db);
    void setToggles(bool autoBal, bool pid);
    void setServoOffset(int channel, float offset);

    void calibrateIMU();

    // Getters for display
    float getCoxaAngle(int legIndex) const;
    float getFemurAngle(int legIndex) const;
    float getTibiaAngle(int legIndex) const;

private:
    ServoController servoController;
    DisplayManager displayManager;
    GaitController gaitController;
    IMUManager imuManager;
    Preferences preferences;

    // Current global IK state
    float tX, tY, tZ;
    float oC, oF, oT;

    // Cached angles for display
    LegAngles currentAngles[4];
};
