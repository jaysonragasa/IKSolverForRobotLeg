#pragma once

#include <Arduino.h>

namespace Config {
    // --- WiFi Configuration ---
    constexpr const char* SSID = "corp-wifi";
    constexpr const char* PASSWORD = "1nf1ni8m@InF0r";

    // --- Hardware Setup ---
    constexpr uint16_t SERVO_MIN = 150;
    constexpr uint16_t SERVO_MAX = 600;
    constexpr uint16_t SERVO_FREQ = 50;

    struct LegPins {
        uint8_t coxa;
        uint8_t femur;
        uint8_t tibia;
    };

    // Pin Mappings: Front-Left, Front-Right, Hind-Left, Hind-Right
    constexpr LegPins LEGS[4] = {
        {0, 1, 2},    // Front-Left
        {4, 5, 6},    // Front-Right
        {8, 9, 10},   // Hind-Left
        {12, 13, 14}  // Hind-Right
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
