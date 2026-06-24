#include "ServoController.h"
#include "Config.h"

/**
 * @brief Constructs the ServoController and loads hardware calibration bounds
 * from the Config mappings to ensure startup safety.
 */
ServoController::ServoController() : _pwm(Adafruit_PWMServoDriver()) {
    // Default everything to basic values
    for (int i = 0; i < 16; i++) {
        _maxAngles[i] = 180.0f;
        _inverts[i] = 1;
        _enabled[i] = 1;
        _servoOffsets[i] = 0.0f;
    }

    // Apply calibrations according to the logical Leg to Pin mapping
    for (int i = 0; i < 4; i++) {
        uint8_t coxaPin = Config::LEGS[i].coxa;
        uint8_t femurPin = Config::LEGS[i].femur;
        uint8_t tibiaPin = Config::LEGS[i].tibia;

        // Coxa
        if (coxaPin < 16) {
            _maxAngles[coxaPin] = Config::LEG_CALIBRATIONS[i].coxa.maxAngle;
            _inverts[coxaPin] = Config::LEG_CALIBRATIONS[i].coxa.invert;
            _enabled[coxaPin] = Config::LEG_CALIBRATIONS[i].coxa.enabled;
            _servoOffsets[coxaPin] = Config::LEG_CALIBRATIONS[i].coxa.offset;
        }

        // Femur
        if (femurPin < 16) {
            _maxAngles[femurPin] = Config::LEG_CALIBRATIONS[i].femur.maxAngle;
            _inverts[femurPin] = Config::LEG_CALIBRATIONS[i].femur.invert;
            _enabled[femurPin] = Config::LEG_CALIBRATIONS[i].femur.enabled;
            _servoOffsets[femurPin] = Config::LEG_CALIBRATIONS[i].femur.offset;
        }

        // Tibia
        if (tibiaPin < 16) {
            _maxAngles[tibiaPin] = Config::LEG_CALIBRATIONS[i].tibia.maxAngle;
            _inverts[tibiaPin] = Config::LEG_CALIBRATIONS[i].tibia.invert;
            _enabled[tibiaPin] = Config::LEG_CALIBRATIONS[i].tibia.enabled;
            _servoOffsets[tibiaPin] = Config::LEG_CALIBRATIONS[i].tibia.offset;
        }
    }
}

/**
 * @brief Configures the PCA9685 IC.
 * Sets the oscillator to 27MHz (for precision) and the PWM rate to 50Hz (for RC servos).
 */
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

/**
 * @brief Master command function for servos.
 * Modifies the angle based on physical limits, logical offsets, and inversion rules,
 * then maps the 0-180 degree angle to the PCA9685's 12-bit tick length, transmitting via I2C.
 * 
 * @param pin The PCA9685 channel (0-15).
 * @param angle The target angle in degrees.
 */
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
