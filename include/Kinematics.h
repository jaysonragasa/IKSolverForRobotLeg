#pragma once

struct LegAngles {
    float coxa;
    float femur;
    float tibia;
};

class IKSolver {
public:
    // Calculates the ideal angles (0-180) based on target coordinates and offsets
    static LegAngles calculate(float x, float y, float z, float offC, float offF, float offT);
};
