# ASKALBOT RC: ServoController Deep-Dive

This document provides a senior-level architectural breakdown of the `ServoController` module located in `src/ServoController.cpp`, responsible for translating high-level angles into hardware-specific PWM signals.

## 1. Architectural Role
The ESP32 is a powerful microcontroller, but generating 12 to 16 simultaneous, jitter-free hardware PWM signals in software is resource-prohibitive and prone to interrupt collisions (especially when handling Wi-Fi).

The `ServoController` acts as a driver wrapper for the Adafruit PCA9685 16-channel 12-bit PWM I2C expander. It isolates the rest of the firmware from PCA-specific register math and physical servo limitations.

## 2. Initialization and I2C Configuration
```cpp
void ServoController::begin() {
  _pwm.begin();
  _pwm.setOscillatorFrequency(27000000);
  _pwm.setPWMFreq(Config::SERVO_FREQ);
}
```
*   **Oscillator Frequency**: The PCA9685 contains an internal 25MHz oscillator. However, real-world chips often drift closer to 27MHz. Setting this accurately prevents scaling errors in pulse-width outputs.
*   **PWM Frequency**: RC hobby servos (like the SG90 or MG90S) expect a 50Hz update rate (one pulse every 20ms). This is hardcoded via `Config::SERVO_FREQ`.

## 3. The Angle-to-Pulse Transformation pipeline
The core function `setAngle(uint8_t pin, float angle)` handles a strict transformation pipeline to ensure mechanical safety before writing to the I2C bus.

### Stage A: Software Offsets
```cpp
angle += _servoOffsets[pin];
```
In a quadruped, mechanical assembly is never perfect. The splines on a servo horn only allow 5-10 degree adjustment increments. To achieve a perfectly squared stance, the firmware injects a sub-degree float offset (loaded from NVS) into the incoming angle request.

### Stage B: Inversion Mathematics
```cpp
if (_inverts[pin] == -1) {
    angle = _maxAngles[pin] - angle;
}
```
Robot legs are mirrored across the sagittal plane (left vs right). To allow the `GaitController` to use identical math for all legs, the `ServoController` automatically flips the geometry for specified servos based on the `Config::LEG_CALIBRATIONS` struct.

### Stage C: Mechanical Boundary Enforcement
```cpp
angle = angle * (_maxAngles[pin] / 180.0); 
if (angle < 0.0f) angle = 0.0f;
if (angle > _maxAngles[pin]) angle = _maxAngles[pin];
```
Commanding a servo to push past its physical mechanical stop draws extreme stall currents (risking brownouts) and quickly strips nylon or metal gears. The logic strictly clamps the command before execution.

### Stage D: Pulse Width Modulation (PWM) Mapping
```cpp
float pulseF = Config::SERVO_MIN + (angle / _maxAngles[pin]) * (Config::SERVO_MAX - Config::SERVO_MIN);
_pwm.setPWM(pin, 0, (uint16_t)pulseF);
```
Standard servos use pulse widths from ~500µs to ~2500µs to determine position. The PCA9685 has a 12-bit resolution per cycle (0 to 4095 ticks).
*   `Config::SERVO_MIN`: The PCA tick value corresponding to 0 degrees.
*   `Config::SERVO_MAX`: The PCA tick value corresponding to 180 degrees.
The mapped tick value is sent over I2C to the PCA9685 registers, completing the execution.

## 4. Disabling Servos (Going Limp)
```cpp
if (_enabled[pin] == 0) {
    _pwm.setPWM(pin, 0, 0);
    return;
}
```
Setting the PWM high/low pulse sequence to 0 effectively turns off the signal line to the servo. Without a signal, analog servos shut down their internal H-bridge motor drivers, allowing the joint to go entirely limp. This is crucial for safely depowering the robot without fighting active torque.
