#include "GaitController.h"
#include "Kinematics.h"
#include "Config.h"
#include <math.h>

GaitController::GaitController(ServoController& servoController)
    : servoController(servoController), rcThrottle(0), rcYaw(0), rcPitch(0), rcRoll(0), rcStrafe(0), 
      imuPitch(0), imuRoll(0), gaitPhase(0.0f), lastGaitTime(0) {}

void GaitController::setRC(float t, float y, float p, float r, float s) {
    rcThrottle = t;
    rcYaw = y;
    rcPitch = p;
    rcRoll = r;
    rcStrafe = s;
}

void GaitController::setIMU(float pitch, float roll) {
    imuPitch = pitch;
    imuRoll = roll;
}

void GaitController::update(float baseX, float baseY, float baseZ, float oC, float oF, float oT) {
    unsigned long now = millis();
    float dt = (now - lastGaitTime) / 1000.0f;
    lastGaitTime = now;
    if (dt > 0.1f) dt = 0.1f; // Cap dt to 100ms to prevent physics explosions on WiFi lag

    // --- SMOOTHING (LOW-PASS FILTER) ---
    static float s_throttle = 0, s_yaw = 0, s_pitch = 0, s_roll = 0, s_strafe = 0;
    
    float lerp = 8.0f * dt; 
    if (lerp > 1.0f) lerp = 1.0f;

    s_throttle += (rcThrottle - s_throttle) * lerp;
    s_yaw += (rcYaw - s_yaw) * lerp;
    s_pitch += (rcPitch - s_pitch) * lerp;
    s_roll += (rcRoll - s_roll) * lerp;
    s_strafe += (rcStrafe - s_strafe) * lerp;

    // --- CONTINUOUS STATE LOGIC ---
    float moveSpeed = sqrt(s_throttle * s_throttle + s_yaw * s_yaw + s_strafe * s_strafe);
    if (moveSpeed > 1.0f) moveSpeed = 1.0f;

    // Advance the gait phase ONLY if we are commanding movement
    if (moveSpeed > 0.02f) { // tighter deadzone now that it's smoothed
        gaitPhase += dt * moveSpeed * 2.0f; // max 2 cycles per sec
        if (gaitPhase >= 1.0f) gaitPhase -= 1.0f;
    } else {
        // When stopped, gently advance phase forward to the nearest resting posture (0.0 or 0.5)
        // so the legs finish their current step and touch the ground smoothly.
        if (gaitPhase > 0.01f && gaitPhase < 0.49f) {
            gaitPhase += dt * 0.5f;
            if (gaitPhase > 0.5f) gaitPhase = 0.5f;
        } else if (gaitPhase > 0.51f && gaitPhase < 0.99f) {
            gaitPhase += dt * 0.5f;
            if (gaitPhase > 1.0f) gaitPhase = 0.0f;
        } else {
            if (gaitPhase < 0.25f || gaitPhase > 0.75f) gaitPhase = 0.0f;
            else gaitPhase = 0.5f;
        }
    }

    float stepLength = 80.0f; 
    float stepHeight = 15.0f;
    
    for (int i = 0; i < 4; i++) {
        // Trot gait phases: FL(0) and HR(3) are phase 0, FR(1) and HL(2) are phase 0.5
        float legPhase = gaitPhase;
        if (i == 1 || i == 2) {
            legPhase += 0.5f;
            if (legPhase >= 1.0f) legPhase -= 1.0f;
        }

        float x = baseX;
        float y = baseY;
        float z = baseZ;

        // --- PITCH & ROLL (Active Suspension) ---
        // We combine the user's manual joystick offset with the IMU's absolute gravity offset.
        // The IMU gives angles in degrees. E.g., if tilted 10 degrees forward, imuPitch is 10.
        // We multiply this by an aggressiveness factor (e.g. 1.0) to convert degrees to mm of leg extension/compression.
        float imuAggressiveness = 1.0f; // mm per degree of tilt

        // Pitch: Front legs (0, 1) go up, Hind legs (2, 3) go down
        float pitchOffset = (s_pitch * 20.0f) + (imuPitch * imuAggressiveness);
        y += (i < 2) ? pitchOffset : -pitchOffset;

        // Roll: Left legs (0, 2) go up, Right legs (1, 3) go down
        float rollOffset = (s_roll * 20.0f) + (imuRoll * imuAggressiveness);
        y += (i % 2 == 0) ? rollOffset : -rollOffset;


        // --- WALKING STRIDE (Only applies if moving) ---
        if (moveSpeed > 0.05f || gaitPhase > 0) {
            float swingProgress = 0.0f;
            float stanceProgress = 0.0f;

            if (legPhase < 0.5f) {
                swingProgress = legPhase * 2.0f; // 0 to 1
                y += sin(swingProgress * PI) * stepHeight;
            } else {
                stanceProgress = (legPhase - 0.5f) * 2.0f; // 0 to 1
            }

            // Direction & Turn Mixing
            // Throttle dictates forward/backward (Forward = negative X physical)
            float dx = -s_throttle; 
            // Strafe dictates left/right (Left = negative Z physical)
            float dz = -s_strafe;

            // Yaw dictates rotation (Add to left, subtract from right)
            if (i % 2 == 0) { dx += s_yaw; } // Left legs
            else            { dx -= s_yaw; } // Right legs

            // Apply calculated vector dynamically based on stance/swing phase
            if (legPhase < 0.5f) {
                // Swing phase: move from -1 to 1 smoothly
                float move = -cos(swingProgress * PI); 
                x += move * dx * (stepLength / 2.0f);
                z += move * dz * (stepLength / 2.0f);
            } else {
                // Stance phase: move from 1 to -1 smoothly
                float move = cos(stanceProgress * PI); 
                x += move * dx * (stepLength / 2.0f);
                z += move * dz * (stepLength / 2.0f);
            }

            // Cheetah Centerline Tracking (Cat Walk)
            float catWalkZ = 0;
            if (legPhase < 0.5f) {
                catWalkZ = sin(swingProgress * PI) * 15.0f; // swing out 15mm
            } else {
                catWalkZ = -sin(stanceProgress * PI) * 20.0f; // track in 20mm
            }
            z += catWalkZ;
            
            // Shoulder / Spine Flexion (Pitching Bounce)
            float flexY = 0;
            if (i == 0 || i == 1) { 
                flexY = (cos(legPhase * 2.0f * PI) - 1.0f) * 10.0f * moveSpeed; 
            } else { 
                flexY = (-cos(legPhase * 2.0f * PI) - 1.0f) * 10.0f * moveSpeed; 
            }
            //y += flexY;
        }

        LegAngles angles = IKSolver::calculate(x, y, z, oC, oF, oT);
        servoController.setAngle(Config::LEGS[i].coxa, angles.coxa);
        servoController.setAngle(Config::LEGS[i].femur, angles.femur);
        servoController.setAngle(Config::LEGS[i].tibia, angles.tibia);
    }
}
