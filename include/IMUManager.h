#ifndef IMU_MANAGER_H
#define IMU_MANAGER_H

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Preferences.h>

/**
 * @class IMUManager
 * @brief Manages the MPU6050 sensor, providing filtered pitch/roll and gyro rates.
 */
class IMUManager {
public:
    /**
     * @brief Constructs a new IMUManager object.
     */
    IMUManager();

    /**
     * @brief Initializes the I2C bus and the MPU6050 hardware.
     * @return true if MPU6050 is successfully detected and configured.
     * @return false if initialization fails.
     */
    bool init();

    /**
     * @brief Polls the hardware and runs the Complementary Filter.
     * Must be called frequently in the main loop.
     */
    void update();

    /**
     * @brief Gets the absolute Pitch angle in degrees, corrected by the calibration offset.
     * @return float Pitch angle.
     */
    float getPitch() const { return pitch - pitchOffset; }

    /**
     * @brief Gets the absolute Roll angle in degrees, corrected by the calibration offset.
     * @return float Roll angle.
     */
    float getRoll() const { return roll - rollOffset; }

    /**
     * @brief Gets the instantaneous Pitch rotational velocity.
     * @return float Degrees per second.
     */
    float getGyroPitchRate() const { return gyroPitchRate; }

    /**
     * @brief Gets the instantaneous Roll rotational velocity.
     * @return float Degrees per second.
     */
    float getGyroRollRate() const { return gyroRollRate; }

    /**
     * @brief Checks if the MPU6050 is successfully connected and responding.
     * @return true if connected.
     */
    bool isConnected() const { return connected; }

    /**
     * @brief Captures the current pitch and roll as the new zero-point offsets,
     * and saves them persistently to Non-Volatile Storage (NVS).
     */
    void calibrate();

private:
    Adafruit_MPU6050 mpu;   ///< Adafruit MPU6050 driver instance
    Preferences preferences; ///< NVS Preferences instance

    float pitch;            ///< Raw calculated pitch
    float roll;             ///< Raw calculated roll
    float pitchOffset;      ///< Persistent zero-point pitch offset
    float rollOffset;       ///< Persistent zero-point roll offset
    float gyroPitchRate;    ///< Instantaneous pitch velocity
    float gyroRollRate;     ///< Instantaneous roll velocity
    
    bool connected;         ///< Hardware connection status flag
    unsigned long lastUpdateTime; ///< Timestamp of the last update for delta-T calculation

    const float alpha = 0.96f; ///< Complementary filter blending coefficient
};

#endif
