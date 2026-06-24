# ASKALBOT RC: DisplayManager Deep-Dive

This document details the architectural role of the `DisplayManager` (`src/DisplayManager.cpp`), which provides head-mounted hardware-level debugging and telemetry tracking.

## 1. Architectural Role
The `DisplayManager` is a non-blocking wrapper for the Adafruit SSD1306 OLED driver. Because it operates on the same 400kHz I2C bus as the IMU and the Servo Controller, its execution time directly impacts the maximum theoretical Hz of the physics engine.

To prevent the OLED from bottlenecking the robot's balance calculations, the `DisplayManager` is strictly rate-limited by the `RobotController` to update only once per second (1Hz).

## 2. Boot & Network Discovery
```cpp
void DisplayManager::showInit() {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("ASKALBOT RC");
    display.println("Booting Core...");
    display.display();
}
```
During a cold boot, the Web Server requires an IP address to be accessible. Since the ESP32 is headless, the user has no way of knowing what IP address their local router assigned via DHCP. The `DisplayManager` displays the SSID and the resulting IP address, serving as the critical physical bridge to the web interface.

## 3. Real-Time Telemetry Dashboard
Once booted, the OLED transitions to a real-time system monitor displaying four critical health metrics:

### A. Wi-Fi Status
Indicates whether the robot has disconnected from the router, explaining sudden unresponsiveness to joystick commands without requiring the user to plug in a USB serial monitor.

### B. DHCP IP Address
Continuously displays the local IP so the operator can always connect their browser to the control panel.

### C. Available Heap RAM
```cpp
display.print("RAM: ");
display.print(freeRam / 1024);
display.println(" KB");
```
The ESP32 possesses ~320KB of usable SRAM. Hosting a Web Server and parsing JSON string telemetry can lead to memory fragmentation. By exposing `ESP.getFreeHeap()`, the system allows the architect to monitor for memory leaks in real-time. If the RAM steadily drops over an hour of operation, a memory leak exists in the HTTP handler.

### D. System Loop Frequency (Hz)
```cpp
display.print("Loop: ");
display.print(loopHz);
display.println(" Hz");
```
This is the most critical metric for the physics engine. 
*   The `RobotController` increments a counter every cycle. Once per second, it divides the counter by time to calculate the Hz.
*   For the Complementary Filter and PID auto-balancing loops to function smoothly, the system must run at >100Hz.
*   If a blocking function (like a slow Wi-Fi `fetch` or a `delay()`) is accidentally introduced into the codebase, the OLED's Hz readout will instantly plummet, immediately alerting the developer to a performance regression.
