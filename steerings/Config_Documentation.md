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
RC servos are "dumb" actuators. If commanded to move beyond their physical capabilities or geometric limits, they will draw extreme stall currents (potentially browning-out the ESP32) and quickly strip their internal gears.

The `LEG_CALIBRATIONS` array enforces strict mechanical boundaries *per joint*:
1.  **`maxAngle`**: Clamps the output before it hits the mechanical stop of the servo casing or linkage.
2.  **`invert`**: Robot legs are mirrored across the sagittal plane. Instead of the `GaitController` writing complex `if (isLeftSide) { angle = 180 - angle }` logic, the `invert` flag allows the physics engine to calculate everything symmetrically. The `ServoController` driver layer handles the hardware-specific inversions transparently.
3.  **`offset`**: A zero-point calibration trim. Servo splines are physical teeth, meaning a servo horn can rarely be attached at exactly 90.0 degrees perfectly square to the chassis. This offset provides sub-degree software trim compensation.

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
