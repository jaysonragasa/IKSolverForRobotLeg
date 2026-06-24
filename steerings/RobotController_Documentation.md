# ASKALBOT RC: RobotController Deep-Dive

This document provides a senior-level architectural breakdown of the `RobotController` module (`src/RobotController.cpp`), which acts as the central state machine and hardware orchestration layer of the firmware.

## 1. Architectural Role
The `RobotController` is the "Main Thread" of the robotics logic. It is responsible for bridging the physical hardware domain (I2C, OLED, Servos, IMU), the physics domain (`GaitController`, Inverse Kinematics), and the state persistence domain (NVS).

By consolidating all subsystem calls into `RobotController::update()`, the system guarantees deterministic execution order: Read Sensors -> Calculate Physics -> Command Hardware -> Report Telemetry.

## 2. Boot & Non-Volatile Storage (NVS)
When the robot powers on, it must recall its exact calibration and physical stance from its previous session. The `RobotController` utilizes the ESP32's NVS flash memory partition (via the `Preferences` library) to permanently store floats and booleans.

```cpp
// Load IK Target Pose
preferences.begin("robot", false);
tX = preferences.getFloat("ik_tx", 0.0f);
tY = preferences.getFloat("ik_ty", -100.0f);
tZ = preferences.getFloat("ik_tz", 28.0f);
```
### Engineering Decision: Safe Start
Because standard hobby servos do not have positional feedback (potentiometer readings are internal to the servo), the ESP32 has no idea where the legs are physically located upon boot.
1. The `RobotController` reads the last requested `(tX, tY, tZ)` from NVS.
2. It solves the Inverse Kinematics for that pose.
3. It commands the servos to that pose.
This ensures that if the robot was powered off while lying down, it powers back on in the exact same lying down pose, preventing the legs from violently snapping to a default standing pose and damaging the chassis.

## 3. The Core Update Loop
The `RobotController::update()` function runs unblocked in the main Arduino `loop()`, acting as the central heartbeat.

### Stage A: Telemetry Acquisition
```cpp
imuManager.update();
```
Polls the MPU6050 over I2C and runs the Complementary Filter to generate absolute Pitch/Roll angles.

### Stage B: Physics Injection
```cpp
gaitController.setIMU(imuManager.getPitch(), imuManager.getRoll());
gaitController.setIMUGyro(imuManager.getGyroPitchRate(), imuManager.getGyroRollRate());
```
The absolute angles are fed to the `GaitController` for PID Proportional/Integral calculations, while the raw Gyro rates are fed separately to drive the PID Derivative calculation for instant damping.

### Stage C: Kinematic Resolution
```cpp
gaitController.update(tX, tY, tZ);
```
The physics engine calculates auto-balancing offsets, interpolates smooth movement towards the target `(tX, tY, tZ)` pose, overrides with macros if active, calculates Inverse Kinematics, and transmits the resulting 12-channel PWM matrix to the PCA9685 driver.

### Stage D: System Profiling
```cpp
if (now - lastDisplayUpdate >= 1000) {
    loopHz = (float)frameCount / ((now - lastDisplayUpdate) / 1000.0f);
    // ... pushes to displayManager
}
```
The controller profiles its own execution time. By tracking `frameCount` over a 1000ms delta, it calculates its exact loop frequency. This allows developers to instantly detect execution blocking or I2C bus saturation.

## 4. State Delegation (The Setter Methods)
The `RobotController` exposes safe setter methods for the asynchronous `WebManager` to call when HTTP requests arrive.
*   `setLogicalOffset(leg, joint, offset)`: Modifies a servo's permanent zero-point calibration. The `RobotController` applies it to the `ServoController` instantly and writes it to NVS flash memory to survive the next reboot.
*   `setPID()`, `setIMUDeadband()`: Directly mutate the variables inside the `GaitController` memory space while ensuring permanent persistence.
