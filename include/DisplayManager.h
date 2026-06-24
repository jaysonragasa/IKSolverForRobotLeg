#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/**
 * @class DisplayManager
 * @brief Manages the 128x64 OLED display for real-time telemetry output.
 */
class DisplayManager {
public:
    /**
     * @brief Constructs a new DisplayManager object.
     */
    DisplayManager();

    /**
     * @brief Initializes the I2C OLED display.
     */
    void begin();
    
    /**
     * @brief Updates the OLED screen with real-time system statistics.
     * 
     * @param loopHz The current calculation speed of the main loop.
     * @param freeRam The amount of free Heap RAM available on the ESP32.
     * @param wifiConnected True if Wi-Fi is actively connected to the router.
     * @param ipAddress The DHCP IP address string assigned to the robot.
     */
    void update(float loopHz, uint32_t freeRam, bool wifiConnected, const char* ipAddress);

    /**
     * @brief Displays the boot splash screen indicating initialization progress.
     */
    void showInit();

private:
    Adafruit_SSD1306 _display; ///< Adafruit SSD1306 driver instance
};
