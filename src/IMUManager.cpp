#include "IMUManager.h"
#include <math.h>

IMUManager::IMUManager() : pitch(0), roll(0), pitchOffset(0), rollOffset(0), 
                           gyroPitchRate(0), gyroRollRate(0), connected(false), lastUpdateTime(0) {}

bool IMUManager::init() {
    // Start I2C explicitly on pins 21/22
    Wire.begin(21, 22);

    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        return false;
    }
    Serial.println("MPU6050 Found!");

    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ); // Smooth out vibrations

    // Load calibration from permanent memory
    preferences.begin("robot", false);
    pitchOffset = preferences.getFloat("imu_p_off", 0.0f);
    rollOffset  = preferences.getFloat("imu_r_off", 0.0f);

    connected = true;
    lastUpdateTime = millis();
    return true;
}

void IMUManager::update() {
    if (!connected) return;

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    unsigned long now = millis();
    float dt = (now - lastUpdateTime) / 1000.0f;
    lastUpdateTime = now;

    if (dt > 0.1f) dt = 0.1f; // Cap dt on long stalls

    // --- AXIS MAPPING FOR PINS ON LEFT ---
    // When the MPU6050 is mounted with pins on the left:
    // MPU X-axis points RIGHT.
    // MPU Y-axis points FORWARD.
    // MPU Z-axis points UP.

    // PITCH (Rotation around X-axis / Left-Right axis)
    // Positive pitch should make front legs extend. So tilting DOWN (front drops) needs positive pitch.
    float accelPitch = atan2(a.acceleration.y, a.acceleration.z) * 180.0f / PI;
    gyroPitchRate = -g.gyro.x * 180.0f / PI;

    // ROLL (Rotation around Y-axis / Forward-Back axis)
    // Positive roll should make left legs extend. So tilting LEFT (left drops) needs positive roll.
    float accelRoll  = -atan2(a.acceleration.x, a.acceleration.z) * 180.0f / PI;
    gyroRollRate  = -g.gyro.y * 180.0f / PI;

    // --- COMPLEMENTARY FILTER ---
    // Blend the Gyroscope (fast, drifts over time) with the Accelerometer (noisy, but absolute reference)
    pitch = alpha * (pitch + gyroPitchRate * dt) + (1.0f - alpha) * accelPitch;
    roll  = alpha * (roll  + gyroRollRate  * dt) + (1.0f - alpha) * accelRoll;
}

void IMUManager::calibrate() {
    if (!connected) return;

    pitchOffset = pitch;
    rollOffset = roll;

    // Save to permanent memory
    preferences.putFloat("imu_p_off", pitchOffset);
    preferences.putFloat("imu_r_off", rollOffset);
}
