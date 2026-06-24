#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "ServoController.h"
#include "DisplayManager.h"
#include "GaitController.h"
#include "Kinematics.h"
#include "IMUManager.h"
#include <Preferences.h>

/**
 * @class RobotController
 * @brief Central state machine and hardware orchestration layer.
 * Manages NVS persistence, physics updates, and subsystem bridging.
 */
class RobotController {
public:
    /**
     * @brief Constructs the RobotController and initializes subsystems.
     */
    RobotController();

    /**
     * @brief Boots hardware buses (I2C), initializes components, and loads NVS state.
     */
    void begin();

    /**
     * @brief Main heartbeat loop. Polls IMU, drives Physics, updates Display.
     */
    void update();

    /**
     * @brief Computes immediate IK for the current static pose and pushes to servos.
     */
    void updateHardware();
    
    /**
     * @brief Sets and saves a static absolute IK pose.
     */
    void setIK(float tx, float ty, float tz);

    /**
     * @brief Passes RC joystick data down to the GaitController.
     */
    void setRC(float t, float y, float p, float r, float s);

    /**
     * @brief Updates and permanently saves Auto-Balance PID tuning.
     */
    void setPID(float p, float i, float d);

    /**
     * @brief Updates and permanently saves the IMU noise deadband threshold.
     */
    void setIMUDeadband(float db);

    /**
     * @brief Toggles and saves Auto-Balance active states.
     */
    void setToggles(bool autoBalance, bool pidEnabled);

    /**
     * @brief Sets and saves a mechanical zero-point calibration for a specific joint.
     * @param leg The leg index (0-3).
     * @param joint The joint index (0=Coxa, 1=Femur, 2=Tibia).
     * @param offset The zero-point offset in degrees.
     */
    void setLogicalOffset(int leg, int joint, float offset);

    /**
     * @brief Retrieves the saved calibration offset for a joint.
     */
    float getLogicalOffset(int leg, int joint);

    /**
     * @brief Triggers the IMU zero-point leveling calibration routine.
     */
    void calibrateIMU();

    // Getters for display
    float getCoxaAngle(int legIndex) const;
    float getFemurAngle(int legIndex) const;
    float getTibiaAngle(int legIndex) const;

    // Getters for Web UI State
    float getTX() { return tX; }
    float getTY() { return tY; }
    float getTZ() { return tZ; }
    
    bool getAutoBalance() { return preferences.getBool("ab_en", false); }
    bool getPIDEnabled() { return preferences.getBool("pid_en", false); }
    float getKp() { return preferences.getFloat("pid_p", 1.0f); }
    float getKi() { return preferences.getFloat("pid_i", 0.0f); }
    float getKd() { return preferences.getFloat("pid_d", 0.0f); }
    float getDeadband() { return preferences.getFloat("imu_db", 0.0f); }

private:
    ServoController servoController;
    DisplayManager displayManager;
    GaitController gaitController;
    IMUManager imuManager;
    Preferences preferences;

    // Current global IK state
    float tX, tY, tZ;
    float oC, oF, oT;

    // Cached angles for display
    LegAngles currentAngles[4];

    // System stats tracking
    uint32_t lastDisplayUpdate = 0;
    uint32_t frameCount = 0;
    float loopHz = 0.0f;
};
