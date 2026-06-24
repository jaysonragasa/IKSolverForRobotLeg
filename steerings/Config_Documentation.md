# ASKALBOT RC: Config Deep-Dive

This document provides a senior-level breakdown of `include/Config.h`. The configuration file acts as the single source of truth for all immutable hardware parameters, mechanical measurements, and network definitions.

## 1. Architectural Role
By extracting all "magic numbers" (like pin arrays, servo boundaries, and physical link lengths) into a dedicated `Config` namespace, the codebase adheres to strict separation of concerns.

The physics engine (`GaitController`), driver layers (`ServoController`), and trigonometric solvers (`IKSolver`) contain absolutely zero hardcoded hardware values. They dynamically adapt their math based on this centralized header.

## 2. Hardware Independence via Abstraction

### Pin Mapping Matrix (`LEGS[4]`)
```cpp
constexpr LegPins LEGS[4] = {
    {8, 9, 10},   // Front-Left
    {4, 5, 6},    // Front-Right
    {12, 13, 14}, // Hind-Left
    {0, 1, 2}     // Hind-Right
};
```
In hardware prototyping, physical wire routing is rarely perfectly sequential. Re-routing 12 tightly packed servo wires on a PCA9685 board simply to satisfy software indices is a tedious and error-prone process.

The `LEGS[4]` array solves this by providing a software abstraction matrix. The physics engine always iterates sequentially through Legs 0 to 3 (FL, FR, HL, HR). When it needs to move the Front-Left Coxa, it simply asks for `LEGS[0].coxa`. The code resolves this to physical pin `8`. If the builder accidentally plugs the leg into pins `0, 1, 2`, they only need to update this matrix; the physics engine remains completely untouched.

### Safety Limits & Inversions (`LEG_CALIBRATIONS[4]`)
RC servos are "dumb" actuators. If commanded to move beyond their physical capabilities or geometric limits, they will draw extreme stall currents (potentially browning-out the ESP32) and quickly strip their internal gears. The `LEG_CALIBRATIONS` array provides a software abstraction layer over these imperfect physical actuators. 

Crucially, this calibration array is mapped by **Logical Leg**, not by Physical Pin. Because of the `LEGS` mapping matrix defined earlier, you can freely change which PCA9685 pins the legs are plugged into, and the calibrations will automatically follow the correct leg.

The array enforces strict mechanical boundaries *per joint* using the `ServoCalib` struct `{maxAngle, invert, enabled, offset}`:

1.  **`maxAngle` (e.g., `148.0f`)**: Clamps the output before it hits the mechanical stop of the servo casing or the linkage geometry. 
    * *Example*: Even if the IK solver mathematically determines the Tibia needs to bend to 160° to reach a target, the `ServoController` will intercept this and clamp it to `148.0°` to prevent the servo horn from physically crashing into the femur bracket.

2.  **`invert` (e.g., `1` or `-1`)**: Robot legs are mirrored across the sagittal (left/right) plane. 
    * *Example*: When the robot needs to walk forward, the Front-Left and Front-Right femurs both need to rotate "forward". Physically, because the servos are mounted as mirror images of each other, one servo must rotate clockwise while the other rotates counter-clockwise.
    * Instead of the `GaitController` writing complex, messy logic like `if (isLeftSide) { angle = 180 - angle; }`, the `invert` flag allows the physics engine to calculate everything symmetrically (assuming + angles always mean "forward" or "up"). The `ServoController` driver layer reads this `invert` flag and handles the hardware-specific mirroring transparently.

3.  **`enabled` (e.g., `1` or `0`)**: A simple software kill-switch for a specific joint.
    * *Example*: Setting this to `0` prevents the PCA9685 from sending a PWM signal to that specific pin. This causes the servo to go "limp" (unpowered), which is extremely useful during physical assembly when you need to manually turn a joint without fighting the motor's holding torque, or for isolating a broken servo during debugging.

4.  **`offset` (e.g., `0.0f`)**: A zero-point calibration trim. 
    * *Example*: Servo splines (the gears the horn attaches to) have discrete physical teeth. When attaching the leg horn to the servo, it is physically impossible to align it perfectly to `90.0°` relative to the chassis; it might snap into place at `87.5°` or `92.5°`. 
    * The `offset` allows the builder to assemble the robot "as close as possible" physically, and then use this floating-point value to digitally trim the joint so the robot stands perfectly level. If a leg droops slightly, setting the offset to `2.5f` will correct the physical error without needing to rebuild the leg.

## 3. Geometric Constants (`L_COXA`, `L_FEMUR`, `L_TIBIA`)
```cpp
constexpr float L_COXA = 28.0f;
constexpr float L_FEMUR = 50.0f;
constexpr float L_TIBIA = 72.0f;
```
These are the fundamental measurements of the physical chassis links in millimeters. They are consumed exclusively by the Inverse Kinematics solver (`IKSolver::calculate`).

If the user 3D-prints a larger tibia to increase the robot's ride height, they only need to update `L_TIBIA`. The Law of Cosines math in the IK engine will automatically adjust to the new geometric reality without requiring any algorithmic refactoring.

## 4. PWM Constants
```cpp
constexpr uint16_t SERVO_MIN = 150;
constexpr uint16_t SERVO_MAX = 600;
constexpr uint16_t SERVO_FREQ = 50;
```
These govern the communication over the PCA9685. Standard hobby servos expect a 50Hz pulse rate, with a pulse width typically ranging from ~500µs (0°) to ~2500µs (180°). 

The `SERVO_MIN` and `SERVO_MAX` values map the 12-bit PCA9685 tick resolution (0-4095) to these real-world microsecond pulses. `150` and `600` are the standard tested tick values for generic SG90/MG90S servos operating at 50Hz.
