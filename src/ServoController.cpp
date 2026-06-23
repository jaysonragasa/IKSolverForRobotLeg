#include "ServoController.h"
#include "Config.h"

ServoController::ServoController() : _pwm(Adafruit_PWMServoDriver()) {
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
  if (pin < 16)
    return _maxAngles[pin];
  return 180.0f;
}

void ServoController::setInvert(uint8_t pin, int8_t invert) {
  if (pin < 16) {
    _inverts[pin] = invert;
  }
}

void ServoController::setEnabled(uint8_t pin, int8_t enabled) {
  if (pin < 16) {
    _enabled[pin] = enabled;
  }
}

void ServoController::setOffset(uint8_t pin, float offset) {
  if (pin < 16) {
    _servoOffsets[pin] = offset;
  }
}

float ServoController::getOffset(uint8_t pin) const {
  if (pin < 16)
    return _servoOffsets[pin];
  return 0.0f;
}

void ServoController::setAngle(uint8_t pin, float angle) {
  if (pin >= 16)
    return;

  // Apply calibration offset
  angle += _servoOffsets[pin];

  // Check if servo is disabled
  if (_enabled[pin] == 0) {
    // Send 0 pulse to turn off PWM (makes servo go limp)
    _pwm.setPWM(pin, 0, 0);
    return;
  }

  // Automatically apply inversion math if requested
  if (_inverts[pin] == -1) {
    angle = _maxAngles[pin] - angle;
  }

  // constrain angle to physical limits of the servo
  angle = angle * (_maxAngles[pin] / 180.0); 

  // Prevent out of bounds
  if (angle < 0.0f)
    angle = 0.0f;
  if (angle > _maxAngles[pin])
    angle = _maxAngles[pin];

  // Map 0 -> maxAngle directly to SERVOMIN -> SERVOMAX
  float pulseF =
      Config::SERVO_MIN +
      (angle / _maxAngles[pin]) * (Config::SERVO_MAX - Config::SERVO_MIN);

  // DEBUG: Print to serial monitor
  // Serial.print("Pin: "); Serial.print(pin);
  // Serial.print(" | Angle: "); Serial.print(angle);
  // Serial.print(" | Pulse: "); Serial.println(pulseF);

  //Serial.printf("Setting servo on pin %d to angle %.2f degrees (pulse: %.2f)\n", pin, angle, pulseF);

  _pwm.setPWM(pin, 0, (uint16_t)pulseF);
}
