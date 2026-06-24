# ASKALBOT RC: WebManager Deep-Dive

This document provides a senior-level architectural breakdown of the `WebManager` module (`src/WebManager.cpp`), which serves as the bridge between the asynchronous network stack and the deterministic physics engine.

## 1. Architectural Role
The `WebManager` operates as a lightweight, non-blocking HTTP REST API server. Its primary responsibility is to ingest high-frequency telemetry requests and low-latency control commands from the browser-based WebGL User Interface, parse them safely, and inject them into the `RobotController` state machine.

Because the ESP32 is a dual-core processor, the Web Server inherently runs on Core 0 (alongside the Wi-Fi stack), leaving Core 1 entirely free to run the high-speed `RobotController` physics engine without interruption.

## 2. API Routing & Data Transformation

The module utilizes the `WebServer` class to bind specific URI endpoints to C++ lambda functions (or `std::bind` callbacks).

### The Real-Time Command Endpoint (`/rc`)
```cpp
void WebManager::handleRC() {
    float t = server.arg("t").toFloat(); // Throttle
    float y = server.arg("y").toFloat(); // Yaw
    float p = server.arg("p").toFloat(); // Pitch
    float r = server.arg("r").toFloat(); // Roll
    float s = server.arg("s").toFloat(); // Strafe

    robot.setRC(t, y, p, r, s);
    server.send(200, "text/plain", "OK");
}
```
**Engineering Note:** The Web UI sends joystick data encoded as floating-point URL arguments at 20Hz. The `WebManager` parses these strings to floats and passes them to the `RobotController`. This decoupling allows the UI to disconnect or crash without halting the physical robot; if the `/rc` endpoint stops receiving data, the physics engine maintains its last known momentum state until a `/gait?cmd=stop` is explicitly issued or a watchdog intervenes.

### The Inverse Kinematics Endpoint (`/pose` & `/ik`)
Allows the 3D visualizer in the browser to scrub physical slider values and force the robot's IK solver to track those variables instantly.
*   It accepts raw X, Y, Z coordinates (in millimeters) and absolute Pitch/Roll overrides.
*   These are passed to `robot.setIK()`, which permanently saves the new target to Non-Volatile Storage (NVS) so the robot wakes up in the same pose.

### Configuration Injection
Endpoints like `/pid` and `/offset` allow the user to modify low-level tuning constants on the fly.
*   Instead of requiring a firmware re-flash to tune a PID controller or calibrate a misaligned servo horn, the `WebManager` exposes these internal memory addresses.
*   The `RobotController` applies the math immediately and commits the changes to NVS flash memory.

## 3. Telemetry Feedback Loop (`/state`)
To achieve the "Digital Twin" effect where the WebGL 3D dog mimics the physical robot's movements, the WebUI constantly polls the `/state` endpoint.

```cpp
void WebManager::handleState() {
    String json = "{";
    json += "\"pitch\":" + String(robot.imuManager.getPitch()) + ",";
    json += "\"roll\":" + String(robot.imuManager.getRoll()) + ",";
    // ... constructs JSON containing all 12 servo angles
    json += "}";
    server.send(200, "application/json", json);
}
```
**Engineering Note:** Constructing massive JSON strings in C++ can lead to heap fragmentation on embedded devices. The `WebManager` uses minimal `String` concatenation specifically formatted to avoid complex serialization library overhead (like `ArduinoJson`), ensuring the response resolves in microseconds and keeps the ESP32's network stack unblocked.
