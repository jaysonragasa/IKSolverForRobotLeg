#include "Kinematics.h"
#include "Config.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

LegAngles IKSolver::calculate(float x, float y, float z, float offC, float offF, float offT) {
    // 1. Roll Plane (Y-Z)
    float L_yz = sqrt(y * y + z * z);
    if (L_yz < Config::L_COXA) L_yz = Config::L_COXA; // Physical clamp

    float L_drop = sqrt(L_yz * L_yz - Config::L_COXA * Config::L_COXA);

    float alpha = atan2(z, abs(y));
    float beta = asin(Config::L_COXA / L_yz);
    float theta_c = alpha - beta;

    // 2. Pitch Plane (X-drop)
    float D = sqrt(x * x + L_drop * L_drop);
    if (D > (Config::L_FEMUR + Config::L_TIBIA)) D = Config::L_FEMUR + Config::L_TIBIA;
    if (D < 0.1f) D = 0.1f;

    float angle_to_target = atan2(x, L_drop);

    float cos_femur = (Config::L_FEMUR * Config::L_FEMUR + D * D - Config::L_TIBIA * Config::L_TIBIA) / (2.0f * Config::L_FEMUR * D);
    float cos_tibia = (Config::L_FEMUR * Config::L_FEMUR + Config::L_TIBIA * Config::L_TIBIA - D * D) / (2.0f * Config::L_FEMUR * Config::L_TIBIA);

    // Safety clamp before acos
    cos_femur = fmax(-1.0f, fmin(1.0f, cos_femur));
    cos_tibia = fmax(-1.0f, fmin(1.0f, cos_tibia));

    float inner_femur = acos(cos_femur);
    float inner_tibia = acos(cos_tibia);

    // 3. Convert to Degrees
    float coxa_deg = theta_c * (180.0f / M_PI);
    float femur_deg = (angle_to_target + inner_femur) * (180.0f / M_PI);
    float tibia_inner_deg = inner_tibia * (180.0f / M_PI);

    // 4. Calculate Ideal 180-degree geometric positions
    float idealC = fmax(0.0f, fmin(180.0f, 90.0f + coxa_deg + offC));
    float idealF = fmax(0.0f, fmin(180.0f, 90.0f + femur_deg + offF));
    float idealT = fmax(0.0f, fmin(180.0f, tibia_inner_deg + offT));

    LegAngles angles;
    angles.coxa = idealC;
    angles.femur = idealF;
    angles.tibia = idealT;

    return angles;
}
