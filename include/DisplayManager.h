#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class DisplayManager {
public:
    DisplayManager();
    void begin();
    
    // Updates the OLED with IP and three gauge values.
    void update(float coxa, float femur, float tibia, bool wifiConnected, const char* ipAddress);

private:
    void drawGauge(int x, int y, int r, float angle, const char *label);
    Adafruit_SSD1306 _display;
};
