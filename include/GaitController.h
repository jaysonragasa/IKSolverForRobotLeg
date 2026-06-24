#pragma once
#include <Arduino.h>
#include "ServoController.h"

/**
 * @class GaitController
 * @brief Handles the physics engine, autonomous gait generation, and PID auto-balancing.
 */
class GaitController {
public:
    /**
     * @brief Constructs a new GaitController object.
     * @param servoController Reference to the hardware ServoController.
     */
    GaitController(ServoController& servoController);

    /**
     * @brief Injects continuous remote control inputs from the WebUI joystick.
     * @param t Throttle (forward/back)
     * @param y Yaw (rotation)
     * @param p Pitch (tilt)
     * @param r Roll (tilt)
     * @param s Strafe (side-to-side)
     */
    void setRC(float t, float y, float p, float r, float s);

    /**
     * @brief Feeds absolute orientation telemetry from the IMU.
     * @param pitch Absolute pitch in degrees.
     * @param roll Absolute roll in degrees.
     */
    void setIMU(float pitch, float roll);

    /**
     * @brief Feeds instantaneous rotational velocity from the IMU for the PID Derivative term.
     * @param pitchRate Pitch velocity in degrees per second.
     * @param rollRate Roll velocity in degrees per second.
     */
    void setIMUGyro(float pitchRate, float rollRate);

    /**
     * @brief Updates the PID tuning constants.
     * @param p Proportional gain.
     * @param i Integral gain.
     * @param d Derivative gain.
     */
    void setPID(float p, float i, float d);

    /**
     * @brief Sets the noise floor threshold for the IMU to prevent PID jitter.
     * @param db Deadband threshold in degrees.
     */
    void setIMUDeadband(float db);

    /**
     * @brief Enables or disables active balance assistance.
     * @param autoBal True to enable Auto-Balance mode.
     * @param pid True to enable dynamic PID correction.
     */
    void setToggles(bool autoBal, bool pid);

    /**
     * @brief The core physics calculation loop. Applies IK targets, mixes RC input,
     * calculates auto-balance vectors, processes active gaits, and commands servos.
     * @param baseX The base IK target X for all legs.
     * @param baseY The base IK target Y for all legs.
     * @param baseZ The base IK target Z for all legs.
     */
    void update(float baseX, float baseY, float baseZ);

private:
    ServoController& servoController;
    float rcThrottle, rcYaw, rcPitch, rcRoll, rcStrafe;
    float imuPitch, imuRoll;
    float gyroPitchRate, gyroRollRate;
    float imuDeadband;
    
    // PID Settings & State
    float kp, ki, kd;
    float integralPitch, integralRoll;
    bool autoBalanceEnabled;
    bool pidEnabled;

    float gaitPhase;
    unsigned long lastGaitTime;
    float pidMultiplier;
};
