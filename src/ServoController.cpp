#include "ServoController.h"
#include "Config.h"

ServoController::ServoController() : _pwm(Adafruit_PWMServoDriver()) {
    for (int i = 0; i < 16; i++) {
        _maxAngles[i] = 180.0f; // Default max angle
    }
}

void ServoController::begin() {
    _pwm.begin();
    _pwm.setOscillatorFrequency(27000000);
    _pwm.setPWMFreq(Config::SERVO_FREQ);
}

void ServoController::setMaxAngle(uint8_t pin, float max_angle) {
    if (pin < 16) {
        _maxAngles[pin] = max_angle;
    }
}

float ServoController::getMaxAngle(uint8_t pin) const {
    if (pin < 16) return _maxAngles[pin];
    return 180.0f;
}

void ServoController::setAngle(uint8_t pin, float angle) {
    if (pin >= 16) return;
    
    // Prevent out of bounds
    if (angle < 0.0f) angle = 0.0f;
    if (angle > _maxAngles[pin]) angle = _maxAngles[pin];

    // Map 0 -> maxAngle directly to SERVOMIN -> SERVOMAX
    float pulseF = Config::SERVO_MIN + (angle / _maxAngles[pin]) * (Config::SERVO_MAX - Config::SERVO_MIN);
    _pwm.setPWM(pin, 0, (uint16_t)pulseF);
}
