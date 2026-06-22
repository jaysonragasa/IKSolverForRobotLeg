#ifndef IMU_MANAGER_H
#define IMU_MANAGER_H

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Preferences.h>

class IMUManager {
public:
    IMUManager();
    bool init();
    void update();

    // Returns the corrected angles in degrees
    float getPitch() const { return pitch - pitchOffset; }
    float getRoll() const { return roll - rollOffset; }

    // Returns raw rotation speeds for Derivative PID
    float getGyroPitchRate() const { return gyroPitchRate; }
    float getGyroRollRate() const { return gyroRollRate; }

    void calibrate();

private:
    Adafruit_MPU6050 mpu;
    Preferences preferences;

    float pitch;
    float roll;
    float pitchOffset;
    float rollOffset;
    float gyroPitchRate;
    float gyroRollRate;
    
    unsigned long lastUpdateTime;

    // Complementary filter constants
    const float alpha = 0.96f; 
};

#endif
