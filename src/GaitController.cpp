#include "GaitController.h"
#include "Kinematics.h"
#include "Config.h"
#include <math.h>

GaitController::GaitController(ServoController& servoController)
    : servoController(servoController), rcThrottle(0), rcYaw(0), rcPitch(0), rcRoll(0), rcStrafe(0), 
      imuPitch(0), imuRoll(0), gyroPitchRate(0), gyroRollRate(0), imuDeadband(0.0f),
      kp(1.0f), ki(0.0f), kd(0.0f), integralPitch(0), integralRoll(0),
      gaitPhase(0.0f), lastGaitTime(0), pidMultiplier(1.0f) {}

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

void GaitController::setIMUGyro(float pitchRate, float rollRate) {
    gyroPitchRate = pitchRate;
    gyroRollRate = rollRate;
}

void GaitController::setPID(float p, float i, float d) {
    kp = p;
    ki = i;
    kd = d;
}

void GaitController::setIMUDeadband(float db) {
    imuDeadband = db;
}

void GaitController::setToggles(bool autoBal, bool pid) {
    autoBalanceEnabled = autoBal;
    pidEnabled = pid;
}

void GaitController::update(float baseX, float baseY, float baseZ) {
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

    // ==========================================
    // PID FADING LOGIC
    // Automatically disable auto-balance
    // when the joystick starts commanding a walk.
    // ==========================================
    if (moveSpeed > 0.05f) {
        pidMultiplier -= dt * 2.0f; 
    } else if (gaitPhase == 0.0f || gaitPhase == 0.5f) {
        pidMultiplier += dt * 1.0f; 
    }
    if (pidMultiplier < 0.0f) pidMultiplier = 0.0f;
    if (pidMultiplier > 1.0f) pidMultiplier = 1.0f;

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

        // --- PITCH & ROLL (Auto-Balance) ---
        float pitchOffset = 0;
        float rollOffset = 0;

        if (autoBalanceEnabled) {
            float targetPitch = s_pitch * 15.0f; 
            float targetRoll  = s_roll * 15.0f;  

            float errorPitch = imuPitch - targetPitch;
            float errorRoll  = imuRoll - targetRoll;

            if (fabs(errorPitch) < imuDeadband) errorPitch = 0.0f;
            if (fabs(errorRoll) < imuDeadband) errorRoll = 0.0f;

            if (pidEnabled) {
                // ==========================================
                // ACTIVE SUSPENSION PID
                // ==========================================
                integralPitch += errorPitch * dt;
                integralRoll  += errorRoll * dt;

                float maxIntegral = 20.0f;
                if (integralPitch > maxIntegral) integralPitch = maxIntegral;
                if (integralPitch < -maxIntegral) integralPitch = -maxIntegral;
                if (integralRoll > maxIntegral) integralRoll = maxIntegral;
                if (integralRoll < -maxIntegral) integralRoll = -maxIntegral;

                float derivativePitch = gyroPitchRate;
                float derivativeRoll  = gyroRollRate;

                pitchOffset = ((kp * errorPitch) + (ki * integralPitch) + (kd * derivativePitch)) * pidMultiplier;
                rollOffset  = ((kp * errorRoll) + (ki * integralRoll) + (kd * derivativeRoll)) * pidMultiplier;

                if (pidMultiplier == 0.0f) {
                    integralPitch = 0;
                    integralRoll = 0;
                }
            } else {
                // Pure proportional auto-balance using only the Kp slider (no fading, no dampening)
                pitchOffset = errorPitch * 1.0f; // Hardcoded stiffness of 1.0 when PID is off
                rollOffset  = errorRoll * 1.0f;
            }
        }

        // Apply offsets
        // Pitch: Front legs (0, 1) go up/down, Hind legs (2, 3) do opposite
        y += (i < 2) ? pitchOffset : -pitchOffset;

        // Roll: Left legs (0, 2) go up/down, Right legs (1, 3) do opposite
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

        LegAngles angles = IKSolver::calculate(x, y, z, 0, 0, 0);
        servoController.setAngle(Config::LEGS[i].coxa, angles.coxa);
        servoController.setAngle(Config::LEGS[i].femur, angles.femur);
        servoController.setAngle(Config::LEGS[i].tibia, angles.tibia);
    }
}
