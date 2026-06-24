# ASKALBOT RC: GaitController Deep-Dive

This document provides a senior-level breakdown of the `GaitController` (`src/GaitController.cpp`), which serves as the core Physics, Kinematics, and Auto-Balancing engine of the robot.

## 1. Architectural Role
The `GaitController` operates continuously in the `update()` loop. It takes in asynchronous high-level intents (joystick inputs, desired gaits, PID configurations) and absolute telemetry (IMU pitch/roll), and outputs a synchronized matrix of 12 joint angles to the `ServoController` every cycle.

## 2. PID Auto-Balancing Engine
Quadrupeds are inherently unstable, especially when walking on uneven terrain or transitioning weight. To prevent the chassis from drifting or tipping, an active PID loop injects real-time counter-movements.

```cpp
void GaitController::update(float baseTx, float baseTy, float baseTz) {
    // 1. Calculate Error (Target is 0 degrees perfectly level)
    float pitchError = 0.0f - currentPitch;
    float rollError  = 0.0f - currentRoll;

    // 2. Deadband Filter
    if (abs(pitchError) < imuDeadband) pitchError = 0.0f;
    if (abs(rollError) < imuDeadband)  rollError = 0.0f;
```
### Deadbanding
Micro-vibrations from servos create constant IMU noise. Without a deadband, the PID loop enters a feedback resonance loop ("jitter"). The deadband filter ensures the PID engine ignores microscopic errors and only responds to macroscopic tilting.

### The PID Calculation
```cpp
// P (Proportional)
float pOutPitch = pidKp * pitchError;

// I (Integral)
pidIntegralPitch += pitchError * 0.01f;
float iOutPitch = pidKi * pidIntegralPitch;

// D (Derivative) - Utilizes pure Gyro rate for lowest latency
float dOutPitch = pidKd * (-currentGyroPitchRate);
```
1.  **Proportional (P)**: Applies an immediate counter-rotation proportional to how far the robot is tilting.
2.  **Integral (I)**: Accumulates small, persistent errors over time. If the robot has a heavy battery strapped to its rear causing a constant 2° sag that the P-term isn't strong enough to fix, the I-term builds up power until the sag is corrected.
3.  **Derivative (D)**: Acts as a damper. Instead of deriving the noisy pitch angle over time, it directly reads the raw Gyroscope rate (`currentGyroPitchRate`), providing instantaneous resistance to sudden rotational velocity. This prevents overshooting and oscillations.

## 3. Autonomous Gait Generation
When a walking macro is triggered (e.g., Forward, Strafe), the `GaitController` detaches from static IK poses and begins a continuous cyclic trajectory generator.

### Phase Management
```cpp
float legPhase = fmod(gaitPhase + Config::LEG_PHASES[i], 1.0f);
```
The gait relies on a master clock `gaitPhase` that scales from 0.0 to 1.0. Each of the 4 legs has a phase offset defined in `Config::LEG_PHASES`. For a standard "Trot" gait, diagonal pairs of legs share the same phase offset (e.g., Front-Left and Back-Right lift together).

### Swing vs. Stance
A full leg cycle is divided into two phases:
1.  **Swing Phase (legPhase < 0.5):** The leg is lifted into the air and moved rapidly forward to a new anchor point.
    ```cpp
    y += sin(swingProgress * PI) * 50.0f; // Lift foot in a parabola
    float move = -cos(swingProgress * PI); // Accelerate X/Z vector
    ```
2.  **Stance Phase (legPhase >= 0.5):** The foot is planted on the ground. The leg pushes backward relative to the chassis, propelling the robot forward.
    ```cppe
    float move = cos(stanceProgress * PI); // Sweep backward
    ```

### Steering Mixing (Omnidirectional Vectoring)
The user's joystick inputs (`s_throttle`, `s_strafe`, `s_yaw`) are dynamically injected into the stride calculations. By vectoring the `dx` and `dz` targets while the leg is in the air, the robot achieves seamless, omnidirectional translation without needing to pause or rotate in place.

## 4. Animation Overrides
The architecture supports overriding the entire IK and Gait pipeline with hardcoded procedural animations.

```cpp
if (currentAnimMode == 1) { // 1 = WAVE
    if (i == 1) { // Front Right Leg
        y += -60.0f; // Lift high
        x += sin(millis() / 150.0f) * 35.0f; // Wave forward and backward
    }
}
```
Because the override sits *after* the base pose calculation but *before* the Inverse Kinematics solver, the custom animation inherits Auto-Balancing for free, allowing the robot to actively balance on 3 legs while waving the 4th.
