#pragma once

#include <Arduino.h>

/**
 * @namespace Config
 * @brief Contains all static hardware definitions, Wi-Fi credentials,
 * physical chassis measurements, and default servo mapping calibrations.
 */
namespace Config {
    // --- WiFi Configuration ---
    constexpr const char* SSID = "corp-wifi";
    constexpr const char* PASSWORD = "1nf1ni8m@InF0r";

    // --- Hardware Setup ---
    constexpr uint16_t SERVO_MIN = 150;
    constexpr uint16_t SERVO_MAX = 600;
    constexpr uint16_t SERVO_FREQ = 50;

    /**
     * @struct LegPins
     * @brief Maps the 3 joints of a leg to their physical PCA9685 pin indices.
     */
    struct LegPins {
        uint8_t coxa;   ///< Pin index for Shoulder Yaw
        uint8_t femur;  ///< Pin index for Shoulder Pitch
        uint8_t tibia;  ///< Pin index for Knee Pitch
    };

    // Pin Mappings: Front-Left, Front-Right, Hind-Left, Hind-Right
    constexpr LegPins LEGS[4] = {
        {8, 9, 10},   // Front-Left (Group 3)
        {4, 5, 6},    // Front-Right (Group 2)
        {12, 13, 14}, // Hind-Left (Group 4)
        {0, 1, 2}     // Hind-Right (Group 1)
    };

// Original config
// {0, 1, 2},    // Front-Left
// {4, 5, 6},    // Front-Right
// {8, 9, 10},   // Hind-Left
// {12, 13, 14}  // Hind-Right

    /**
     * @struct ServoCalib
     * @brief Stores the physical bounds and hardware calibration for a single servo.
     */
    struct ServoCalib {
        float maxAngle; ///< Physical geometric limit of the servo
        int8_t invert;  ///< 1 for normal rotation, -1 for mirrored rotation
        int8_t enabled; ///< 1 to actively drive PWM, 0 to shut off power (go limp)
        float offset;   ///< Hardcoded physical spline offset in degrees
    };

    /**
     * @struct LegCalib
     * @brief Groups the 3 joint calibrations for a single leg.
     */
    struct LegCalib {
        ServoCalib coxa;  ///< Calibration for Coxa joint
        ServoCalib femur; ///< Calibration for Femur joint
        ServoCalib tibia; ///< Calibration for Tibia joint
    };

    // --- Servo Calibration ---
    // Mapped by Leg, NOT by Pin! You can freely change the LEGS pin mapping above,
    // and the calibrations will automatically follow the correct leg.
    constexpr LegCalib LEG_CALIBRATIONS[4] = {
        // Front-Left
        { 
            {148.0f, -1, 1, 0.0f}, // Coxa
            {148.0f,  1, 1, 0.0f}, // Femur
            {149.0f, -1, 1, 0.0f}  // Tibia
        },
        // Front-Right
        { 
            {148.0f,  1, 1, 0.0f}, // Coxa
            {147.0f, -1, 1, 0.0f}, // Femur
            {147.0f,  1, 1, 0.0f}  // Tibia
        },
        // Hind-Left
        { 
            {149.0f,  1, 1, 0.0f}, // Coxa
            {148.0f,  1, 1, 0.0f}, // Femur
            {149.0f, -1, 1, 0.0f}  // Tibia
        },
        // Hind-Right
        { 
            {148.0f, -1, 1, 0.0f}, // Coxa
            {149.0f, -1, 1, 0.0f}, // Femur
            {149.0f,  1, 1, 0.0f}  // Tibia
        }
    };

    // --- Link Lengths (mm) ---
    constexpr float L_COXA = 28.0f;
    constexpr float L_FEMUR = 50.0f;
    constexpr float L_TIBIA = 72.0f;

    // --- OLED Setup ---
    constexpr uint8_t SCREEN_WIDTH = 128;
    constexpr uint8_t SCREEN_HEIGHT = 64;
    constexpr int8_t OLED_RESET = -1;
}
