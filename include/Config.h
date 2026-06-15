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

    constexpr uint8_t COXA_PIN = 0;
    constexpr uint8_t FEMUR_PIN = 1;
    constexpr uint8_t TIBIA_PIN = 2;

    // --- Link Lengths (mm) ---
    constexpr float L_COXA = 28.0f;
    constexpr float L_FEMUR = 50.0f;
    constexpr float L_TIBIA = 72.0f;

    // --- OLED Setup ---
    constexpr uint8_t SCREEN_WIDTH = 128;
    constexpr uint8_t SCREEN_HEIGHT = 64;
    constexpr int8_t OLED_RESET = -1;
}
