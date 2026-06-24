# ASKALBOT RC: Kinematics & Inverse Kinematics (IK) Deep-Dive

This document provides a senior-level architectural breakdown of the Inverse Kinematics engine located in `src/Kinematics.cpp`. 

## 1. Architectural Role
The `IKSolver` acts as the geometric translation layer between the high-level `GaitController` (which thinks in terms of Cartesian coordinates and spatial trajectories) and the low-level `ServoController` (which requires strictly bounded PWM joint angles). 

The module exposes a stateless static method `calculate()`, ensuring zero memory footprint per leg and enabling O(1) mathematical resolution.

## 2. The IK Mathematics Breakdown

The solver uses closed-form trigonometry to resolve a 3-DOF (Degree of Freedom) leg into its constituent joint angles: Coxa (shoulder yaw), Femur (shoulder pitch), and Tibia (knee pitch).

### Step 1: Solving the Coxa Angle (Y-Z Roll Plane)
The Coxa joint dictates the lateral spread of the leg. We resolve this by collapsing the 3D target `(x, y, z)` onto the 2D Coronal (Y-Z) plane.

```cpp
float L_yz = sqrt(y * y + z * z);
if (L_yz < Config::L_COXA) L_yz = Config::L_COXA; // Physical clamp

float L_drop = sqrt(L_yz * L_yz - Config::L_COXA * Config::L_COXA);
```
1. **Hypotenuse (`L_yz`)**: We calculate the direct diagonal distance from the Coxa pivot to the target foot position. 
2. **Singularity Prevention**: If the target is physically closer than the length of the Coxa itself, `L_yz` is clamped to prevent `asin()` domain errors and physical chassis collision.
3. **Vertical Projection (`L_drop`)**: Using Pythagorean theorem, we calculate the pure vertical drop from the end of the Coxa out to the foot target. This virtual drop becomes the local Y-axis for the next stage of calculations.

```cpp
float alpha = atan2(z, abs(y));
float beta = asin(Config::L_COXA / L_yz);
float theta_c = alpha - beta;
```
*   `alpha` is the angle of the full vector to the foot.
*   `beta` is the angle consumed by the Coxa linkage itself.
*   `theta_c` yields the final rotational angle required for the Coxa servo in radians.

### Step 2: Solving the Femur and Tibia (X-drop Pitch Plane)
With the Coxa resolved, the mathematical frame shifts to the Sagittal plane relative to the leg. This plane is defined by the horizontal `X` axis and the virtual `L_drop` axis calculated above.

```cpp
float D = sqrt(x * x + L_drop * L_drop);
if (D > (Config::L_FEMUR + Config::L_TIBIA)) D = Config::L_FEMUR + Config::L_TIBIA;
if (D < 0.1f) D = 0.1f;
```
1. **Reach (`D`)**: This is the direct hypotenuse from the Femur pivot to the foot.
2. **Hyper-Extension Clamp**: If `D` exceeds the combined physical lengths of the Femur and Tibia, the IK target is unreachable. We clamp `D` to prevent floating-point `acos()` exceptions (NaN generation) and to command the leg to simply "point" as far as it can in the target direction.

```cpp
float cos_femur = (pow(Config::L_FEMUR, 2) + pow(D, 2) - pow(Config::L_TIBIA, 2)) / (2.0f * Config::L_FEMUR * D);
float cos_tibia = (pow(Config::L_FEMUR, 2) + pow(Config::L_TIBIA, 2) - pow(D, 2)) / (2.0f * Config::L_FEMUR * Config::L_TIBIA);
```
Here we employ the **Law of Cosines**. Because all three sides of the triangle are known (Femur length, Tibia length, and `D`), we can deterministically solve the interior angles of the joints.

## 3. Output Normalization and Calibration Mapping
The raw angles are converted from radians to degrees and mapped into the 0-180° boundaries of standard PWM hobby servos.

```cpp
float coxa_deg = theta_c * (180.0f / M_PI);
float femur_deg = (angle_to_target + inner_femur) * (180.0f / M_PI);
float tibia_inner_deg = inner_tibia * (180.0f / M_PI);

float idealC = fmax(0.0f, fmin(180.0f, 90.0f + coxa_deg + offC));
float idealF = fmax(0.0f, fmin(180.0f, 90.0f + femur_deg + offF));
float idealT = fmax(0.0f, fmin(180.0f, tibia_inner_deg + offT));
```

### Key Engineering Decisions:
1.  **90-Degree Zero Point**: Both the Coxa and Femur expect 90° to be their mechanical neutral (straight down/straight out). The output mapping shifts the calculated variations onto this 90° center point.
2.  **Offset Injection**: The function accepts `offC`, `offF`, and `offT`. These are software-defined mechanical calibration trims loaded from Non-Volatile Storage (NVS) to compensate for servo splines that were assembled slightly off-center.
3.  **Final Safeguards**: The `fmax` and `fmin` clamps ensure that the `ServoController` is never fed a mathematically valid but physically destructive command (e.g., commanding a servo to 195°, which would strip internal gears or draw stall current).

## 4. Execution Flow
1.  `GaitController` determines the required `(x, y, z)` for a specific leg at time *t*.
2.  `IKSolver::calculate(...)` is invoked.
3.  The returned `LegAngles` struct is immediately handed to `ServoController::setAngle(...)`.
