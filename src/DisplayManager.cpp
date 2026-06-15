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

void DisplayManager::drawGauge(int x, int y, int r, float angle, const char *label) {
    _display.drawCircle(x, y, r, SSD1306_WHITE);
    _display.fillRect(x - r - 1, y, r * 2 + 3, r + 2, SSD1306_BLACK); // Erase bottom half
    _display.drawLine(x - r, y, x + r, y, SSD1306_WHITE);             // Base line

    float rad = angle * PI / 180.0f;
    int nx = x + r * cos(rad);
    int ny = y - r * sin(rad);
    _display.drawLine(x, y, nx, ny, SSD1306_WHITE);

    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    int labelLen = strlen(label) * 6; // 6 pixels per char in size 1
    _display.setCursor(x - labelLen / 2, y + 4);
    _display.print(label);

    String valStr = String((int)angle) + (char)247; // 247 is approx degree symbol
    int valLen = valStr.length() * 6;
    _display.setCursor(x - valLen / 2, y - r - 12);
    _display.print(valStr);
}

void DisplayManager::update(float coxa, float femur, float tibia, bool wifiConnected, const char* ipAddress) {
    _display.clearDisplay();

    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(0, 0);
    if (wifiConnected) {
        _display.print("IP: ");
        _display.print(ipAddress);
    } else {
        _display.print("WiFi: Disconnected");
    }

    int y = 45;
    int r = 16;
    drawGauge(21, y, r, coxa, "COXA");
    drawGauge(64, y, r, femur, "FEMUR");
    drawGauge(107, y, r, tibia, "TIBIA");

    _display.display();
}
