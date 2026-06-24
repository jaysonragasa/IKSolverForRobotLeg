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
    
    // Set whether the servo is inverted (1 for normal, -1 for inverted).
    void setInvert(uint8_t pin, int8_t invert);
    void setEnabled(uint8_t pin, int8_t enabled);
    void setOffset(uint8_t pin, float offset);
    float getOffset(uint8_t pin) const;
    
    // Set the target angle of the servo on a specific pin in physical degrees.
    void setAngle(uint8_t pin, float angle);

private:
    Adafruit_PWMServoDriver _pwm;
    
    float _maxAngles[16];
    int8_t _inverts[16];
    int8_t _enabled[16];
    float _servoOffsets[16];
};
