#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class DisplayManager {
public:
    DisplayManager();
    void begin();
    
    // Updates the OLED with IP and system stats
    void update(float loopHz, uint32_t freeRam, bool wifiConnected, const char* ipAddress);

private:
    Adafruit_SSD1306 _display;
};
