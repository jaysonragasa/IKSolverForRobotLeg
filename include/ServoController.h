#pragma once

#include <Adafruit_PWMServoDriver.h>
#include <Arduino.h>

class ServoController {
public:
    ServoController();
    void begin();
    
    // Set the physical max angle for a specific servo index (0-15).
    void setMaxAngle(uint8_t pin, float max_angle);
    float getMaxAngle(uint8_t pin) const;
    
    // Set the target angle of the servo on a specific pin in physical degrees.
    void setAngle(uint8_t pin, float angle);

private:
    Adafruit_PWMServoDriver _pwm;
    float _maxAngles[16];
};
