#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>

#include "Config.h"
#include "DisplayManager.h"
#include "Kinematics.h"
#include "ServoController.h"
#include "WebInterface.h"

#include "ServoController.h"
#include "WebInterface.h"

// --- Gait State Machine ---
enum GaitState {
  STOP,
  FORWARD,
  BACKWARD,
  STRAFE_L,
  STRAFE_R,
  ROTATE_L,
  ROTATE_R
};
GaitState currentGait = STOP;
float gaitPhase = 0.0f;
unsigned long lastGaitTime = 0;

// --- Global Objects ---
WebServer server(80);
ServoController servoController;
DisplayManager displayManager;

// Initial Safe State
float tX = 0, tY = -80, tZ = 28;
float oC = 0, oF = 0, oT = 0;

void updateHardware() {
  LegAngles angles = IKSolver::calculate(tX, tY, tZ, oC, oF, oT);

  // Serial.printf("Calculated Angles: Coxa=%.1f Femur=%.1f Tibia=%.1f\n", angles.coxa, angles.femur, angles.tibia);

  // Send the ideal mathematical angles directly to all 4 legs.
  // The physical inversion is handled internally by the ServoController.
  for (int i = 0; i < 4; i++) {
    servoController.setAngle(Config::LEGS[i].coxa, angles.coxa);
    servoController.setAngle(Config::LEGS[i].femur, angles.femur);
    servoController.setAngle(Config::LEGS[i].tibia, angles.tibia);
  }

  bool isConnected = (WiFi.status() == WL_CONNECTED);
  displayManager.update(angles.coxa, angles.femur, angles.tibia, isConnected,
                        WiFi.localIP().toString().c_str());
}

void updateGait() {
  if (currentGait == STOP) return;

  unsigned long now = millis();
  float dt = (now - lastGaitTime) / 1000.0f;
  lastGaitTime = now;

  float gaitSpeed = 1.5f; // 1 cycle per second
  gaitPhase += dt * gaitSpeed;
  if (gaitPhase >= 1.0f) gaitPhase -= 1.0f;

  // Adjusted to be longer and lower so it's a stride rather than a march
  float stepLength = 80.0f; 
  float stepHeight = 15.0f;
  
  // Base posture
  float baseX = tX; // Base X (typically 0)
  float baseY = tY; // Base Y (typically -80)
  float baseZ = tZ; // Base Z (typically 28)

  for (int i = 0; i < 4; i++) {
    // Trot gait phases: FL(0) and HR(3) are phase 0, FR(1) and HL(2) are phase 0.5
    float legPhase = gaitPhase;
    if (i == 1 || i == 2) {
      legPhase += 0.5f;
      if (legPhase >= 1.0f) legPhase -= 1.0f;
    }

    float x = baseX;
    float y = baseY;
    float z = baseZ;

    float swingProgress = 0.0f;
    float stanceProgress = 0.0f;

    if (legPhase < 0.5f) {
      swingProgress = legPhase * 2.0f; // 0 to 1
      y = baseY + sin(swingProgress * PI) * stepHeight;
    } else {
      stanceProgress = (legPhase - 0.5f) * 2.0f; // 0 to 1
      y = baseY;
    }

    // Direction modifiers (Inverted physically)
    float dx = 0, dz = 0;
    if (currentGait == FORWARD) { dx = -1; }
    else if (currentGait == BACKWARD) { dx = 1; }
    else if (currentGait == STRAFE_L) { dz = -1; }
    else if (currentGait == STRAFE_R) { dz = 1; }
    else if (currentGait == ROTATE_L) {
      if (i == 0) { dx = 1; dz = -1; }
      if (i == 1) { dx = -1; dz = -1; }
      if (i == 2) { dx = 1; dz = 1; }
      if (i == 3) { dx = -1; dz = 1; }
    }
    else if (currentGait == ROTATE_R) {
      if (i == 0) { dx = -1; dz = 1; }
      if (i == 1) { dx = 1; dz = 1; }
      if (i == 2) { dx = -1; dz = -1; }
      if (i == 3) { dx = 1; dz = -1; }
    }

    // Apply dx and dz based on phase using cosine for smoother acceleration/deceleration
    if (legPhase < 0.5f) {
      // Swing phase: move from -1 to 1 smoothly
      float move = -cos(swingProgress * PI); 
      x += move * dx * stepLength / 2.0f;
      z += move * dz * stepLength / 2.0f;
    } else {
      // Stance phase: move from 1 to -1 smoothly
      float move = cos(stanceProgress * PI); 
      x += move * dx * stepLength / 2.0f;
      z += move * dz * stepLength / 2.0f;
    }

    LegAngles angles = IKSolver::calculate(x, y, z, oC, oF, oT);
    servoController.setAngle(Config::LEGS[i].coxa, angles.coxa);
    servoController.setAngle(Config::LEGS[i].femur, angles.femur);
    servoController.setAngle(Config::LEGS[i].tibia, angles.tibia);
  }
}

// --- Web Server Endpoints ---
void handleRoot() { server.send(200, "text/html", index_html); }

void handleIK() {
  // Read parameters from the browser's HTTP fetch
  if (server.hasArg("x"))
    tX = server.arg("x").toFloat();
  if (server.hasArg("y"))
    tY = server.arg("y").toFloat();
  if (server.hasArg("z"))
    tZ = server.arg("z").toFloat();
  if (server.hasArg("cx"))
    oC = server.arg("cx").toFloat();
  if (server.hasArg("fm"))
    oF = server.arg("fm").toFloat();
  if (server.hasArg("tb"))
    oT = server.arg("tb").toFloat();

  // Serial.printf("Received IK Command: x=%.1f y=%.1f z=%.1f | oC=%.1f oF=%.1f oT=%.1f\n", tX, tY, tZ, oC, oF, oT);

  // Trigger hardware update
  if (currentGait == STOP) {
    updateHardware();
  }

  server.send(200, "text/plain", "OK");
}

void handleGait() {
  if (server.hasArg("cmd")) {
    String cmd = server.arg("cmd");
    if (cmd == "stop") {
      currentGait = STOP;
      updateHardware(); // go back to default stance
    }
    else if (cmd == "forward") currentGait = FORWARD;
    else if (cmd == "backward") currentGait = BACKWARD;
    else if (cmd == "strafe_left") currentGait = STRAFE_L;
    else if (cmd == "strafe_right") currentGait = STRAFE_R;
    else if (cmd == "rotate_left") currentGait = ROTATE_L;
    else if (cmd == "rotate_right") currentGait = ROTATE_R;
    
    lastGaitTime = millis();
  }
  server.send(200, "text/plain", "OK");
}

void TestServoInversion() {
  // // --- Configure Inversions ---
  // // Front-Left
  // servoController.setInvert(Config::LEGS[0].coxa, -1);
  // servoController.setInvert(Config::LEGS[0].femur, 1);
  // servoController.setInvert(Config::LEGS[0].tibia, -1);

  // // Front-Right
  // servoController.setInvert(Config::LEGS[1].coxa, 1);
  // servoController.setInvert(Config::LEGS[1].femur, -1);
  // servoController.setInvert(Config::LEGS[1].tibia, 1);

  // // Hind-Left
  // servoController.setInvert(Config::LEGS[2].coxa, 1);
  // servoController.setInvert(Config::LEGS[2].femur, 1);
  // servoController.setInvert(Config::LEGS[2].tibia, -1);

  // // Hind-Right
  // servoController.setInvert(Config::LEGS[3].coxa, -1);
  // servoController.setInvert(Config::LEGS[3].femur, -1);
  // servoController.setInvert(Config::LEGS[3].tibia, 1);
}

void InitialPose() {
  tX = 0;
  tY = -80;
  tZ = 28;
  updateHardware();
}

// C:\Users\aragasa\.platformio\penv\Scripts\pio.exe run -t upload

// --- Main Setup ---
void setup() {
  Serial.begin(115200);

  // Explicitly start I2C on pins 21 and 22 for ESP32
  Wire.begin(21, 22);
  Wire.setClock(400000); // Fast I2C for faster OLED updates

  displayManager.begin();
  servoController.begin();

  // Initial positioning
  InitialPose();

  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(Config::SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(Config::SSID, Config::PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.print("Access the simulator at: http://");
  Serial.println(WiFi.localIP());

  // Update OLED with new IP
  updateHardware();

  // Start Server
  server.on("/", handleRoot);
  server.on("/ik", handleIK);
  server.on("/gait", handleGait);
  server.begin();
}

void loop() { 
  server.handleClient(); 
  if (currentGait != STOP) {
    updateGait();
    delay(10);
  }
}