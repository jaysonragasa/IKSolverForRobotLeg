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
    void update(float baseX, float baseY, float baseZ, float oC, float oF, float oT);

private:
    ServoController& servoController;
    float rcThrottle, rcYaw, rcPitch, rcRoll, rcStrafe;
    float imuPitch, imuRoll;
    float gyroPitchRate, gyroRollRate;
    
    // PID Settings & State
    float kp, ki, kd;
    float integralPitch, integralRoll;

    float gaitPhase;
    unsigned long lastGaitTime;
    float pidMultiplier;
};
