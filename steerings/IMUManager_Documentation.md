# ASKALBOT RC: IMUManager Deep-Dive

This document details the architectural design and signal processing mathematics utilized in `src/IMUManager.cpp` to provide a robust, drift-resistant orientation telemetry stream for the robot's Auto-Balance subsystem.

## 1. Architectural Role
The `IMUManager` acts as the hardware abstraction layer (HAL) and DSP (Digital Signal Processing) engine for the MPU6050 6-DOF (Degree of Freedom) Inertial Measurement Unit.

It isolates the raw I2C polling and sensor fusion algorithms from the central `RobotController`, exposing only clean, filtered telemetry: Absolute Pitch, Absolute Roll, and rotational velocities.

## 2. Hardware Initialization and Tuning
During `init()`, the module configures the MPU6050 via the `Adafruit_MPU6050` driver.

```cpp
mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
mpu.setGyroRange(MPU6050_RANGE_250_DEG);
mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
```
### Engineering Decisions:
1.  **2G Accel Range**: A quadruped robot's movements are relatively slow. Setting the accelerometer to its most sensitive range (±2G) maximizes the resolution of the gravity vector.
2.  **250 DPS Gyro Range**: Degrees Per Second. The robot is not expected to perform high-speed acrobatics. The lowest range maximizes angular velocity precision.
3.  **21Hz Hardware Low-Pass Filter**: Servos induce significant high-frequency mechanical vibration into the chassis. Setting the internal Digital Low-Pass Filter (DLPF) to 21Hz hardware-filters out PWM hum and gear chatter before the data even reaches the ESP32.

## 3. The Complementary Filter Algorithm
The core of `update()` is the sensor fusion logic. Relying purely on an Accelerometer is impossible because chassis movements induce lateral G-forces that corrupt the gravity vector. Relying purely on a Gyroscope is impossible because integration over time inevitably results in mathematical drift. 

The **Complementary Filter** fuses the best traits of both.

```cpp
// 1. Calculate delta time (dt) for gyro integration
unsigned long now = millis();
float dt = (now - lastUpdateTime) / 1000.0f;
if (dt > 0.1f) dt = 0.1f; // Cap dt on long stalls to prevent integration explosions

// 2. Extract Absolute Accelerometer Angles
float accelPitch = atan2(a.acceleration.y, a.acceleration.z) * 180.0f / PI;
float accelRoll  = -atan2(a.acceleration.x, a.acceleration.z) * 180.0f / PI;

// 3. Extract Gyroscope Rates
gyroPitchRate = -g.gyro.x * 180.0f / PI;
gyroRollRate  = -g.gyro.y * 180.0f / PI;

// 4. Fuse Data Streams
pitch = alpha * (pitch + gyroPitchRate * dt) + (1.0f - alpha) * accelPitch;
roll  = alpha * (roll  + gyroRollRate  * dt) + (1.0f - alpha) * accelRoll;
```

### The Math Breakdown
*   **The Gyro Integration (`pitch + gyroPitchRate * dt`)**: This calculates the new angle by taking the previous angle and adding the degrees rotated in the last `dt` slice. It represents ~98% of the signal (`alpha` ≈ 0.98), providing fast, immune-to-lateral-acceleration responsiveness.
*   **The Accel Correction (`accelPitch`)**: The accelerometer calculates absolute angle relative to gravity using `atan2`. It represents ~2% of the signal `(1.0f - alpha)`. 
*   **The Result**: The gyro handles all sudden movements and balancing reactions, while the accelerometer acts as a slow, continuous "rubber band," gently pulling the gyro back to true zero to cancel out thermal drift.

## 4. Persistent Calibration
The MPU6050 is rarely mounted perfectly level with the robot's physical chassis geometry.

When `calibrate()` is called (via the Web UI), the system captures the current absolute `pitch` and `roll` and saves them to the ESP32's Non-Volatile Storage (NVS). The `RobotController` then subtracts these offsets from the telemetry stream, ensuring the physics engine treats the current stance as "perfectly level" regardless of imperfect hardware mounting.
