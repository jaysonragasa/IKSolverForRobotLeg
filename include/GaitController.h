#pragma once
#include <Arduino.h>
#include "ServoController.h"

class GaitController {
public:
    GaitController(ServoController& servoController);

    void setRC(float t, float y, float p, float r, float s);
    void setIMU(float pitch, float roll);
    void update(float baseX, float baseY, float baseZ, float oC, float oF, float oT);

private:
    ServoController& servoController;
    float rcThrottle, rcYaw, rcPitch, rcRoll, rcStrafe;
    float imuPitch, imuRoll;
    float gaitPhase;
    unsigned long lastGaitTime;
};
