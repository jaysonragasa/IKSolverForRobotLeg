#pragma once

/**
 * @struct LegAngles
 * @brief Represents the geometric angles of a single 3-DOF robot leg.
 */
struct LegAngles {
    float coxa;   ///< Shoulder Yaw angle in degrees (0-180)
    float femur;  ///< Shoulder Pitch angle in degrees (0-180)
    float tibia;  ///< Knee Pitch angle in degrees (0-180)
};

/**
 * @class IKSolver
 * @brief Provides a static geometric Inverse Kinematics solver for 3-DOF legs.
 */
class IKSolver {
public:
    /**
     * @brief Calculates the exact servo angles required to place the foot at a specific 3D coordinate.
     * 
     * @param x The forward/backward distance in mm (negative is forward).
     * @param y The vertical drop in mm (negative is towards the ground).
     * @param z The lateral spread in mm (negative is left).
     * @param offC The mechanical calibration offset for the Coxa servo in degrees.
     * @param offF The mechanical calibration offset for the Femur servo in degrees.
     * @param offT The mechanical calibration offset for the Tibia servo in degrees.
     * @return LegAngles The resulting target angles clamped safely between 0 and 180 degrees.
     */
    static LegAngles calculate(float x, float y, float z, float offC, float offF, float offT);
};
