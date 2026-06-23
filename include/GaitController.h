#pragma once
#include <Arduino.h>
#include "ServoController.h"

class GaitController {
public:
    GaitController(ServoController& servoController);

    void setRC(float t, float y, float p, float r, float s);
    void setIMU(float pitch, float roll);
    void setIMUGyro(float pitchRate, float rollRate);
    void setPID(float p, float i, float d);
    void setIMUDeadband(float db);
    void setToggles(bool autoBal, bool pid);
    void update(float baseX, float baseY, float baseZ);

private:
    ServoController& servoController;
    float rcThrottle, rcYaw, rcPitch, rcRoll, rcStrafe;
    float imuPitch, imuRoll;
    float gyroPitchRate, gyroRollRate;
    float imuDeadband;
    
    // PID Settings & State
    float kp, ki, kd;
    float integralPitch, integralRoll;
    bool autoBalanceEnabled;
    bool pidEnabled;

    float gaitPhase;
    unsigned long lastGaitTime;
    float pidMultiplier;
};
