/**
 * M5StickC Plus2 GaitOS V13.0 (Fixed & Refined)
 *
 * "Professional" Edition
 * - Hybrid: ZUPT Physics + Empirical Limits.
 * - Architecture: M5Unified + LittleFS + WiFi + WebServer.
 */

#include <Arduino.h>
#include <LittleFS.h>
#include <M5Unified.h>
#include <WebServer.h>
#include <WiFi.h>
#include <numeric>
#include <vector>

#include "web_page.h"

// =============================================================================
// TYPES (MOVED TO TOP)
// =============================================================================
struct Vector3 {
  float x, y, z;
};

struct Point {
  int x;
  int y;
};

enum SiteState { STANCE, SWING };

// =============================================================================
// CONFIG
// =============================================================================
const char *WIFI_SSID = "GAIT-LOGGER";
const char *WIFI_PASS = "circumduct123";
const int SAMPLE_INTERVAL_MS = 10; // 100Hz

// Tuning Parameters (Core Tweaks)
// Removed duplicates, keeping Dissertation-Grade Consistencies
constexpr float ZUPT_THRESH_DPS = 40.0f;   // Angular rate threshold for Stance
constexpr float ZUPT_ACCEL_G = 0.2f;       // Linear Accel threshold for Stance
constexpr float MIN_SWING_ACCEL = 1.2f;    // Minimum energy to valid swing
constexpr float MIN_STEP_TIME_MS = 300.0f; // Minimum time between steps

// =============================================================================
// GLOBAL STATE
// =============================================================================
float accX, accY, accZ;
float gyroX, gyroY, gyroZ;
float pitch = 0, roll = 0, yaw = 0;

// Kinematics (World Frame)
Vector3 vel = {0, 0, 0};
Vector3 pos = {0, 0, 0}; // x=forward, y=lateral, z=vertical

// Algorithm State
bool isStance = false;
unsigned long lastStepTime = 0;
float distanceTotal = 0;
float stepCount = 0;
float currentCadence = 0;
float stabilityIndex = 100.0f;
float hipFootCoupling = 0.0f;

// Data Structures (Ring Buffers)
#define TRAJECTORY_LEN 256
Point trajectory[TRAJECTORY_LEN];
int trajHead = 0;
int trajCount = 0; // Track valid points

// Web Server
WebServer server(80);
M5Canvas canvas(&M5.Display);
bool isRecording = false;
File logFile;
unsigned long lastSampleTime = 0;

// Toast
String toastMsg = "";
unsigned long toastEndTime = 0;
void showToast(String msg, int durationMs = 1500) {
  toastMsg = msg;
  toastEndTime = millis() + durationMs;
}

// =============================================================================
// HELPER MATH
// =============================================================================
// Simple Madgwick or Complementary Filter Component
// For simplicity in V13, we focus on Physics + ZUPT.
// We assume M5Unified provides decent IMU abstraction, but we do raw
// accumulation.

void ZUPT_INS_Update(float dt) {
  // 1. Magnitude Check
  float gMag = sqrt(gyroX * gyroX + gyroY * gyroY + gyroZ * gyroZ);
  float aMag = sqrt(accX * accX + accY * accY + accZ * accZ);

  // 2. Stance Detection
  bool stanceDetected =
      (gMag < ZUPT_THRESH_DPS) && (abs(aMag - 1.0f) < ZUPT_ACCEL_G);

  if (stanceDetected) {
    // Zero Velocity Update (Clamp)
    vel = {0, 0, 0};
    isStance = true;
  } else {
    isStance = false;

    // 3. Integration (World Frame Approx)
    // We assume sensor is flat on foot for simple Z local frame projection
    // In full implementation, proper Quaternion rotation is needed.
    // Here we use a simplified "Vertical is AccZ - 1g" assumption for
    // robustness on simple walks.

    float az_world = accZ - 1.0f; // Remove Gravity
    vel.z += az_world * 9.81f * dt;
    pos.z += vel.z * dt;

    // Dampen Z drift to ground
    if (pos.z < 0)
      pos.z = 0;

    // Integrate Forward (X) - approximated by raw AccX
    vel.x += accX * 9.81f * dt;
    pos.x += vel.x * dt;
  }

  // 4. Cadence & Stability Logic independent of Stance
  // Check for Swing Peak
  static float maxSwing = 0;
  if (!isStance && abs(accZ) > maxSwing)
    maxSwing = abs(accZ);

  if (isStance && maxSwing > MIN_SWING_ACCEL &&
      (millis() - lastStepTime > 300)) {
    // Heel Strike Event Just Happened (or transition to stance)
    unsigned long dur = millis() - lastStepTime;
    lastStepTime = millis();
    stepCount++;

    // Metrics
    float instCadence = 60000.0f / dur;
    currentCadence = (currentCadence * 0.8f) + (instCadence * 0.2f);

    float deviation =
        abs(instCadence - currentCadence) / (currentCadence + 1.0f);
    float instStability = constrain(100.0f * (1.0f - deviation), 0.0f, 100.0f);
    stabilityIndex = (stabilityIndex * 0.9f) + (instStability * 0.1f);

    // Distance Approx
    distanceTotal += 0.7f; // Avg Step length placeholder or integral
    maxSwing = 0;
  }

  // Hip Foot Coupling (Proxy)
  // High VelX + Low Pitch Change ~ Hip Hike.
  // We simulate "Pitch" change via GyroY integration or just raw GyroY.
  hipFootCoupling = (abs(vel.x) * 10.0f) / (abs(gyroY) + 1.0f);

  // Trajectory Buffer
  trajectory[trajHead].x = (int)(pos.x * 100);
  trajectory[trajHead].y = (int)(pos.z * 100); // Screen Y is World Z
  trajHead = (trajHead + 1) % TRAJECTORY_LEN;
  if (trajCount < TRAJECTORY_LEN)
    trajCount++;
}

float calculateHipProbe() { return hipFootCoupling; }

// =============================================================================
// JSON API
// =============================================================================
void getStatusJSON() {
  String json = "{";
  json += "\"recording\":" + String(isRecording) + ",";
  json += "\"step_count\":" + String((int)stepCount) + ",";
  json += "\"dist_m\":" + String(distanceTotal) + ",";
  json += "\"cad\":" + String(currentCadence) + ",";
  json += "\"stab\":" + String(stabilityIndex) + ",";
  json += "\"hfc\":" + String(hipFootCoupling) + ",";
  json += "\"px\":" + String(pos.x, 3) + ",";
  json += "\"pz\":" + String(pos.z, 3) + ",";
  json += "\"phase\":" + String(isStance ? 0 : 1) + ",";
  json += "\"pitch\":" + String(pitch) + ",";
  json += "\"is_stat\":" + String(isStance);
  json += "}";
  server.send(200, "application/json", json);
}

// =============================================================================
// APPS SYSTEM
// =============================================================================
class App {
public:
  virtual void onOpen() {}
  virtual void onClose() {}
  virtual void onDraw(M5Canvas &c) {}
  virtual void onBtnA() {}
  virtual void onBtnB() {}
};

void drawPulse(M5Canvas &c, int x, int y, int r, uint16_t color) {
  float phase = (millis() % 2000) / 2000.0f;
  float breath = (sin(phase * 6.28f) + 1.0f) * 0.5f;
  c.fillCircle(x, y, r, color);
  int haloR = r + 2 + (breath * 6);
  uint8_t alpha = 255 - (breath * 200);
  // Fixed: Use M5.Display.alphaBlend explicitly or manual color
  // Since c.alphaBlend might not exist depending on M5Unified version
  // We just draw a ring with dimmer color for compatibility
  c.drawCircle(x, y, haloR, color);
}

// 1. LAUNCHER
class LauncherApp : public App {
  int sel = 0;

public:
  void onDraw(M5Canvas &c) override {
    c.fillRect(0, 0, 240, 135, BLACK);
    c.setTextColor(WHITE);
    c.drawCenterString("GaitOS V13", 120, 20, 2);

    const char *names[3] = {"Lab", "Scope", "Net"};
    int startX = 60, gap = 60;
    for (int i = 0; i < 3; i++) {
      int x = startX + i * gap;
      bool act = (sel == i);
      int r = act ? 22 : 15;
      drawPulse(c, x, 70, r, act ? GREEN : DARKGREY);
      c.drawCenterString(names[i], x, 100, 1);
    }
  }
  void onBtnB() override { sel = (sel + 1) % 3; }
  void onBtnA() override; // Defined below classes
};

// 2. GAIT LAB
class GaitLabApp : public App {
public:
  void onDraw(M5Canvas &c) override {
    c.fillScreen(BLACK);
    c.setTextColor(WHITE);
    c.setTextSize(2);
    c.setCursor(10, 20);
    c.printf("Steps: %.0f", stepCount);
    c.setCursor(10, 50);
    c.printf("Cadence: %.0f", currentCadence);
    c.setTextSize(1);
    c.setCursor(10, 90);
    c.printf("Stability: %.0f%%", stabilityIndex);
    c.setCursor(10, 110);
    c.printf("HFC: %.1f", hipFootCoupling);

    if (isRecording)
      c.fillCircle(220, 20, 8, RED);
  }
  void onBtnA() override {
    isRecording = !isRecording;
    if (isRecording) {
      logFile = LittleFS.open("/log_" + String(millis()) + ".csv", FILE_WRITE);
      logFile.println("t,ax,ay,az,gx,gy,gz,px,pz");
      showToast("Rec Start");
    } else {
      if (logFile)
        logFile.close();
      showToast("Rec Stop");
    }
  }
};

// 3. SCOPE
class ScopeApp : public App {
public:
  void onDraw(M5Canvas &c) override {
    c.fillScreen(BLACK);
    int cx = 20, cy = 120;
    // Draw Trajectory from ring buffer
    for (int i = 0; i < TRAJECTORY_LEN - 1; i++) {
      int idx = (trajHead + i) % TRAJECTORY_LEN;
      Point p1 = trajectory[idx];
      Point p2 = trajectory[(idx + 1) % TRAJECTORY_LEN];

      // Simple pixel wrap logic
      int x1 = (cx + p1.x) % 240;
      int y1 = cy - p1.y;
      int x2 = (cx + p2.x) % 240;
      int y2 = cy - p2.y;

      // Avoid wrap-around lines
      if (abs(x2 - x1) < 20)
        c.drawLine(x1, y1, x2, y2, CYAN);
    }
    c.drawCenterString("Trajectory (Z vs X)", 120, 5, 1);
  }
};

// 4. NET
class NetApp : public App {
public:
  void onDraw(M5Canvas &c) override {
    c.fillScreen(BLACK);
    c.setTextColor(WHITE);
    c.drawCenterString(WiFi.softAPIP().toString(), 120, 60, 2);
    c.drawCenterString("SSID: GAIT-LOGGER", 120, 90, 1);
  }
};

LauncherApp appLauncher;
GaitLabApp appGaitLab;
ScopeApp appScope;
NetApp appNet;

App *currentApp = &appLauncher;

void LauncherApp::onBtnA() {
  if (sel == 0)
    currentApp = &appGaitLab;
  if (sel == 1)
    currentApp = &appScope;
  if (sel == 2)
    currentApp = &appNet;
  currentApp->onOpen();
}

// =============================================================================
// SETUP & LOOP
// =============================================================================
void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(3);
  canvas.createSprite(M5.Display.width(), M5.Display.height());
  LittleFS.begin(true);

  WiFi.softAP(WIFI_SSID, WIFI_PASS);
  server.on("/", HTTP_GET,
            []() { server.send_P(200, "text/html", index_html); });
  server.on("/api/status", HTTP_GET,
            []() { getStatusJSON(); }); // Fix missing ref

  // Basic API for Rec
  server.on("/api/record/start", HTTP_POST, []() {
    isRecording = true;
    logFile = LittleFS.open("/webrec.csv", FILE_WRITE);
    server.send(200);
  });
  server.on("/api/record/stop", HTTP_POST, []() {
    isRecording = false;
    if (logFile)
      logFile.close();
    server.send(200);
  });

  server.begin();

  // Zero Sensors Init
  pos = {0, 0, 0};
  vel = {0, 0, 0};
}

void loop() {
  M5.update();
  server.handleClient();

  unsigned long now = millis();
  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    float dt = (now - lastSampleTime) / 1000.0f;
    lastSampleTime = now;
    M5.Imu.getAccel(&accX, &accY, &accZ); // Correct vars
    M5.Imu.getGyro(&gyroX, &gyroY, &gyroZ);

    ZUPT_INS_Update(dt);

    if (isRecording && logFile) {
      logFile.printf("%lu,%.2f,%.2f,%.2f,%.2f,%.2f\n", now, accX, pos.z,
                     stabilityIndex, stepCount);
    }
  }

  // App Input
  if (M5.BtnA.wasPressed())
    currentApp->onBtnA();
  if (M5.BtnB.wasPressed())
    currentApp->onBtnB();
  if (M5.BtnPWR.wasPressed()) {
    currentApp = &appLauncher;
    currentApp->onOpen();
  }

  // Draw
  currentApp->onDraw(canvas);
  // Toast overlay
  if (millis() < toastEndTime) {
    canvas.fillRect(60, 100, 120, 30, DARKGREY);
    canvas.setTextColor(WHITE);
    canvas.drawCenterString(toastMsg, 120, 110, 1);
  }
  canvas.pushSprite(0, 0);
}
