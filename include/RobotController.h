#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "ServoController.h"
#include "DisplayManager.h"
#include "GaitController.h"
#include "Kinematics.h"

class RobotController {
public:
    RobotController();

    void begin();
    void update();
    void updateHardware();
    
    // IK State setters
    void setIK(float tx, float ty, float tz, float oc, float of, float ot);
    void setRC(float t, float y, float p, float r, float s);

    // Getters for display
    float getCoxaAngle(int legIndex) const;
    float getFemurAngle(int legIndex) const;
    float getTibiaAngle(int legIndex) const;

private:
    ServoController servoController;
    DisplayManager displayManager;
    GaitController gaitController;

    // Current global IK state
    float tX, tY, tZ;
    float oC, oF, oT;

    // Cached angles for display
    LegAngles currentAngles[4];
};
