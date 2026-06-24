#pragma once

#include <Adafruit_PWMServoDriver.h>
#include <Arduino.h>

class ServoController {
public:
    /**
     * @brief Constructs a new ServoController object and initializes default arrays.
     */
    ServoController();

    /**
     * @brief Initializes the PCA9685 I2C communication and sets the 50Hz PWM frequency.
     */
    void begin();
    
    /**
     * @brief Sets the maximum physical angle limit for a specific servo to prevent damage.
     * @param pin The PCA9685 channel (0-15).
     * @param max_angle The maximum safe geometric angle.
     */
    void setMaxAngle(uint8_t pin, float max_angle);
    float getMaxAngle(uint8_t pin) const;
    
    /**
     * @brief Reverses the mathematical direction of a servo.
     * @param pin The PCA9685 channel (0-15).
     * @param invert 1 for normal, -1 for inverted.
     */
    void setInvert(uint8_t pin, int8_t invert);

    /**
     * @brief Enables or disables a servo. A disabled servo goes limp (0 PWM).
     * @param pin The PCA9685 channel (0-15).
     * @param enabled 1 for enabled, 0 for disabled.
     */
    void setEnabled(uint8_t pin, int8_t enabled);

    /**
     * @brief Sets a software calibration offset to square up misaligned servo splines.
     * @param pin The PCA9685 channel (0-15).
     * @param offset The offset in degrees.
     */
    void setOffset(uint8_t pin, float offset);
    float getOffset(uint8_t pin) const;
    
    /**
     * @brief Safely bounds the requested angle, applies calibration/inversion, 
     * and transmits the resulting PWM pulse to the PCA9685.
     * @param pin The PCA9685 channel (0-15).
     * @param angle The requested angle in degrees.
     */
    void setAngle(uint8_t pin, float angle);

private:
    Adafruit_PWMServoDriver _pwm; ///< Adafruit PCA9685 driver instance
    
    float _maxAngles[16];   ///< Physical angle limits per channel
    int8_t _inverts[16];    ///< Inversion flags per channel
    int8_t _enabled[16];    ///< Enable flags per channel
    float _servoOffsets[16];///< Zero-point calibration offsets per channel
};
