#include "DisplayManager.h"
#include "Config.h"

DisplayManager::DisplayManager() 
    : _display(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, &Wire, Config::OLED_RESET) {}

void DisplayManager::begin() {
    if (!_display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
    } else {
        _display.clearDisplay();
        _display.setTextColor(SSD1306_WHITE);
        _display.setCursor(0, 0);
        _display.println("Booting System...");
        _display.display();
    }
}

void DisplayManager::update(float loopHz, uint32_t freeRam, bool wifiConnected, const char* ipAddress) {
    _display.clearDisplay();

    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    
    // Header
    _display.setCursor(0, 0);
    if (wifiConnected) {
        _display.print("IP: ");
        _display.println(ipAddress);
    } else {
        _display.println("WiFi: Disconnected");
    }
    
    _display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

    // System Stats
    _display.setCursor(0, 18);
    _display.print("Loop Freq : ");
    _display.print(loopHz, 1);
    _display.println(" Hz");

    _display.setCursor(0, 32);
    _display.print("Free RAM  : ");
    _display.print(freeRam / 1024);
    _display.println(" KB");

    _display.display();
}
