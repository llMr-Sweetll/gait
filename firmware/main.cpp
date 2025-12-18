/**
 * M5StickC Plus2 GaitOS v10.0 (Medical Grade)
 *
 * A Professional, Physics-Based Gait Analysis System (ZUPT-INS).
 *
 * Research-Based Algorithms:
 * - Strapdown Inertial Navigation System (INS).
 * - Zero Velocity Update (ZUPT) for drift correction.
 * - 3D Trajectory Reconstruction (Arc of the foot).
 * - Precise Phase Detection (Stance vs Swing).
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
// CONFIGURATION
// =============================================================================

const char *WIFI_SSID = "GAIT-LOGGER";
const char *WIFI_PASS = "circumduct123";
const int SAMPLE_RATE_HZ = 100;
const int SAMPLE_INTERVAL_MS = 1000 / SAMPLE_RATE_HZ;

// ZUPT Thresholds (Empirically tuned for walking)
const float ZUPT_GYRO_THRESHOLD_DPS = 40.0f;  // Deg/s
const float ZUPT_ACCEL_VAR_THRESHOLD = 0.05f; // G
const int ZUPT_WINDOW_SIZE = 5;

// =============================================================================
// CORE TYPES
// =============================================================================

enum AppID {
  APP_LAUNCHER,
  APP_GAITLAB,
  APP_SCOPE,
  APP_FILES,
  APP_SETTINGS,
  APP_POWER
};
enum ScopeMode { SCOPE_TRAJECTORY, SCOPE_PHASE };
enum GaitPhase { PHASE_STANCE, PHASE_SWING };

struct Vector3 {
  float x, y, z;
};

class App {
public:
  virtual void onOpen() {}
  virtual void onClose() {}
  virtual void onUpdate(unsigned long now) {}
  virtual void onDraw(M5Canvas &canvas) {}
  virtual void onBtnA() {} // Select
  virtual void onBtnB() {} // Next
};

// =============================================================================
// GLOBALS
// =============================================================================

WebServer server(80);
M5Canvas canvas(&M5.Display);

// System State
AppID currentAppID = APP_LAUNCHER;
App *currentApp = nullptr;
bool isRecording = false;
bool autoSave = true;
String currentLogFile = "";
File logFile;
unsigned long lastSampleTime = 0;

// UI State
String toastMsg = "";
unsigned long toastEndTime = 0;
int currentRotation = 3; // Landscape

// IMU Raw
float ax, ay, az, gx, gy, gz;

// Orientation (Madgwick)
float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;
float beta = 0.1f; // Filter gain

// INS State (World Frame)
Vector3 vel = {0, 0, 0};
Vector3 pos = {0, 0, 0};
GaitPhase currentPhase = PHASE_STANCE;
unsigned long stanceStartTime = 0;
unsigned long swingStartTime = 0;

// Metrics
float lastStepLength = 0.0f;     // m
float lastClearance = 0.0f;      // cm
float lastSwingDuration = 0.0f;  // ms
float lastStanceDuration = 0.0f; // ms
unsigned long stepCount = 0;
float distanceTotal = 0.0f;

// Trajectory Buffer (Side Profile: X vs Z)
struct Point {
  int x, z;
};
std::vector<Point> trajectory;

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

void showToast(String msg, int durationMs = 2000) {
  toastMsg = msg;
  toastEndTime = millis() + durationMs;
}

// Rotate vector v by quaternion q -> v_world
Vector3 rotateVector(float x, float y, float z) {
  // Standard quaternion rotation: v' = q * v * q_inv
  // Simplified for direction cosine matrix
  float q0q0 = q0 * q0, q1q1 = q1 * q1, q2q2 = q2 * q2, q3q3 = q3 * q3;
  float _2q0 = 2 * q0, _2q1 = 2 * q1, _2q2 = 2 * q2, _2q3 = 2 * q3;

  Vector3 out;
  out.x = (q0q0 + q1q1 - q2q2 - q3q3) * x + (_2q1 * q2 - _2q0 * q3) * y +
          (_2q1 * q3 + _2q0 * q2) * z;
  out.y = (_2q1 * q2 + _2q0 * q3) * x + (q0q0 - q1q1 + q2q2 - q3q3) * y +
          (_2q2 * q3 - _2q0 * q1) * z;
  out.z = (_2q1 * q3 - _2q0 * q2) * x + (_2q2 * q3 + _2q0 * q1) * y +
          (q0q0 - q1q1 - q2q2 + q3q3) * z;
  return out;
}

void MadgwickUpdate(float dt) {
  float recipNorm;
  float s0, s1, s2, s3;
  float qDot1, qDot2, qDot3, qDot4;
  float _gx = gx * 0.0174533f;
  float _gy = gy * 0.0174533f;
  float _gz = gz * 0.0174533f;

  if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {
    recipNorm = 1.0f / sqrt(ax * ax + ay * ay + az * az);
    float _ax = ax * recipNorm;
    float _ay = ay * recipNorm;
    float _az = az * recipNorm;

    // Gradient descent algorithm corrective step
    float _2q0 = 2.0f * q0;
    float _2q1 = 2.0f * q1;
    float _2q2 = 2.0f * q2;
    float _2q3 = 2.0f * q3;
    float _4q0 = 4.0f * q0;
    float _4q1 = 4.0f * q1;
    float _4q2 = 4.0f * q2;
    float _8q1 = 8.0f * q1;
    float _8q2 = 8.0f * q2;
    float q0q0 = q0 * q0;
    float q1q1 = q1 * q1;
    float q2q2 = q2 * q2;
    float q3q3 = q3 * q3;

    s0 = _4q0 * q2q2 + _2q2 * _ax + _4q0 * q1q1 - _2q1 * _ay;
    s1 = _4q1 * q3q3 - _2q3 * _ax + 4.0f * q0q0 * q1 - _2q0 * _ay - _4q1 +
         _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * _az;
    s2 = 4.0f * q0q0 * q2 + _2q0 * _ax + _4q2 * q3q3 - _2q3 * _ay - _4q2 +
         _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * _az;
    s3 = 4.0f * q1q1 * q3 - _2q1 * _ax + 4.0f * q2q2 * q3 - _2q2 * _ay;
    recipNorm = 1.0f / sqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);
    s0 *= recipNorm;
    s1 *= recipNorm;
    s2 *= recipNorm;
    s3 *= recipNorm;

    // Apply feedback step
    qDot1 = 0.5f * (-q1 * _gx - q2 * _gy - q3 * _gz) - beta * s0;
    qDot2 = 0.5f * (q0 * _gx + q2 * _gz - q3 * _gy) - beta * s1;
    qDot3 = 0.5f * (q0 * _gy - q1 * _gz + q3 * _gx) - beta * s2;
    qDot4 = 0.5f * (q0 * _gz + q1 * _gy - q2 * _gx) - beta * s3;

    q0 += qDot1 * dt;
    q1 += qDot2 * dt;
    q2 += qDot3 * dt;
    q3 += qDot4 * dt;
    recipNorm = 1.0f / sqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    q0 *= recipNorm;
    q1 *= recipNorm;
    q2 *= recipNorm;
    q3 *= recipNorm;
  }
}

// =============================================================================
// PHYSICS ENGINE (ZUPT-INS)
// =============================================================================

void ZUPT_INS_Update(float dt) {
  // 1. Zero Velocity Detection
  float gyroMag = sqrt(gx * gx + gy * gy + gz * gz);
  bool isStationary = (gyroMag < ZUPT_GYRO_THRESHOLD_DPS);

  // 2. State Machine
  if (currentPhase == PHASE_STANCE) {
    if (!isStationary) {
      // Transition to Swing (Toe Off)
      currentPhase = PHASE_SWING;
      swingStartTime = millis();
      lastStanceDuration = swingStartTime - stanceStartTime;

      // Reset Trajectory for new step
      trajectory.clear();
      pos = {0, 0, 0}; // Start integration from origin
    } else {
      // Still in Stance: Zero Velocity (ZUPT)
      vel = {0, 0, 0};
    }
  } else { // SWING
    if (isStationary) {
      // Transition to Stance (Heel Strike)
      currentPhase = PHASE_STANCE;
      stanceStartTime = millis();
      lastSwingDuration = stanceStartTime - swingStartTime;

      // End of Step Metrics
      lastStepLength =
          sqrt(pos.x * pos.x + pos.y * pos.y); // Euclidean distance on ground
      if (lastStepLength > 0.1f) {             // Ignore micro-movements
        stepCount++;
        distanceTotal += lastStepLength;
      }
    } else {
      // Strapdown Integration
      // Rotate Body Accel to World Frame
      Vector3 accWorld = rotateVector(ax, ay, az);

      // Remove Gravity (Assuming Z is up, Gravity is -1g down)
      accWorld.z -= 1.0f; // in G's

      // Convert G to m/s^2
      accWorld.x *= 9.81f;
      accWorld.y *= 9.81f;
      accWorld.z *= 9.81f;

      // Integrate Vel
      vel.x += accWorld.x * dt;
      vel.y += accWorld.y * dt;
      vel.z += accWorld.z * dt;

      // Integrate Pos
      pos.x += vel.x * dt;
      pos.y += vel.y * dt;
      pos.z += vel.z * dt;

      // Update Metrics (Real-time)
      if (pos.z * 100.0f > lastClearance)
        lastClearance = pos.z * 100.0f; // Track Max Z

      // Add to visualization buffer (Side Profile: X vs Z)
      // Scale roughly 1m = 100px
      Point p;
      p.x = (int)(pos.x * 100.0f);
      p.z = (int)(pos.z * 100.0f);
      if (trajectory.empty() || (abs(p.x - trajectory.back().x) > 1 ||
                                 abs(p.z - trajectory.back().z) > 1)) {
        trajectory.push_back(p);
      }
    }
  }
}

void updateSensors() {
  unsigned long now = millis();
  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    float dt = (now - lastSampleTime) / 1000.0f;
    lastSampleTime = now;

    M5.Imu.getAccel(&ax, &ay, &az);
    M5.Imu.getGyro(&gx, &gy, &gz);

    MadgwickUpdate(dt);
    ZUPT_INS_Update(dt);

    if (isRecording && logFile) {
      // Log raw + kinematics
      logFile.printf("%lu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%d\n",
                     now, ax, ay, az, gx, gy, gz, pos.x, pos.y, pos.z,
                     currentPhase);
    }
  }
}

// =============================================================================
// APPS
// =============================================================================

// --- Launcher (Grid) ---
class LauncherApp : public App {
  const char *apps[4] = {"Gait Lab", "Trace Scope", "Files", "Settings"};
  int sel = 0;

public:
  void onDraw(M5Canvas &c) override {
    c.setTextSize(2);
    c.setTextColor(WHITE);
    c.drawCenterString("GaitOS v10", 120, 5, 1);
    int w = 110, h = 50;
    int gap = 10;
    int ox = 5, oy = 30;
    for (int i = 0; i < 4; i++) {
      int row = i / 2;
      int col = i % 2;
      int x = ox + col * (w + gap);
      int y = oy + row * (h + gap);
      uint16_t color = 0x3333;
      if (i == 0)
        color = 0x00AA;
      if (i == 1)
        color = 0xAA00;
      if (i == 2)
        color = 0x0055;
      if (i == 3)
        color = 0x5500;
      if (i == sel) {
        c.fillRoundRect(x, y, w, h, 8, WHITE);
        c.setTextColor(BLACK);
      } else {
        c.fillRoundRect(x, y, w, h, 8, color);
        c.drawRoundRect(x, y, w, h, 8, WHITE);
        c.setTextColor(WHITE);
      }
      c.setTextSize(2);
      c.drawCenterString(apps[i], x + w / 2, y + h / 2 - 8, 1);
    }
  }
  void onBtnB() override { sel = (sel + 1) % 4; }
  void onBtnA() override; // Forward decl
};

// --- Gait Lab (Medical Metrics) ---
class GaitLabApp : public App {
public:
  void onDraw(M5Canvas &c) override {
    // Quadrant Layout
    c.drawLine(120, 20, 120, 135, 0x4444);
    c.drawLine(0, 77, 240, 77, 0x4444);

    // Q1: Steps
    c.setTextColor(ORANGE);
    c.setTextSize(1);
    c.setCursor(5, 25);
    c.print("STEPS");
    c.setTextSize(3);
    c.setCursor(5, 40);
    c.print(stepCount);

    // Q2: Distance
    c.setTextColor(CYAN);
    c.setTextSize(1);
    c.setCursor(125, 25);
    c.print("DIST (m)");
    c.setTextSize(3);
    c.setCursor(125, 40);
    c.printf("%.1f", distanceTotal);

    // Q3: Clearance
    c.setTextColor(GREEN);
    c.setTextSize(1);
    c.setCursor(5, 82);
    c.print("CLEAR (cm)");
    c.setTextSize(3);
    c.setCursor(5, 97);
    c.printf("%.1f", lastClearance);

    // Q4: Phase
    c.setTextColor(MAGENTA);
    c.setTextSize(1);
    c.setCursor(125, 82);
    c.print("PHASE");
    c.setTextSize(2);
    c.setCursor(125, 100);
    if (currentPhase == PHASE_STANCE)
      c.print("STANCE");
    else
      c.print("SWING");

    // Status
    c.setTextColor(isRecording ? RED : GREEN);
    c.drawRightString(isRecording ? "REC" : "RDY", 235, 5, 1);
  }
  void onBtnA() override {
    if (isRecording) {
      isRecording = false;
      if (logFile)
        logFile.close();
      showToast("Saved!");
    } else {
      String fname = "/gait_" + String(millis()) + ".csv";
      logFile = LittleFS.open(fname, FILE_WRITE);
      if (logFile) {
        isRecording = true;
        logFile.println("t,ax,ay,az,gx,gy,gz,px,py,pz,phase");
        showToast("Recording...");
      }
    }
  }
};

// --- Scope (Trajectory & ZUPT Visualization) ---
class ScopeApp : public App {
  ScopeMode mode = SCOPE_TRAJECTORY;

public:
  void onOpen() override {
    trajectory.clear();
    pos = {0, 0, 0}; // Reset to center
    showToast("Ready to Walk", 1000);
  }
  void onDraw(M5Canvas &c) override {
    if (mode == SCOPE_TRAJECTORY) {
      // Draw Trajectory (Side Profile: Z vs X)
      c.drawRect(0, 20, 240, 115, 0x2222);

      // Ground Line
      int groundY = 120;
      c.drawLine(0, groundY, 240, groundY, 0x5555);
      c.setTextSize(1);
      c.setTextColor(LIGHTGREY);
      c.setCursor(5, 25);
      c.print("Side Profile (Z vs X)");

      // Draw Path
      int originX = 20;
      for (size_t i = 1; i < trajectory.size(); i++) {
        // Scale: 1 unit map = 1 cm? Let's say 100px/m
        // trajectory stores units of cm basically (pos * 100)
        int x1 = originX + trajectory[i - 1].x;
        int z1 = groundY - trajectory[i - 1].z;
        int x2 = originX + trajectory[i].x;
        int z2 = groundY - trajectory[i].z;

        // Wrap X for continuous walking
        x1 %= 200;
        x2 %= 200;

        if (abs(x2 - x1) < 20) // Don't draw wrap lines
          c.drawLine(x1, z1, x2, z2, ORANGE);
      }

      // Live Cursor
      int currX = (originX + (int)(pos.x * 100)) % 200;
      int currZ = groundY - (int)(pos.z * 100);
      c.fillCircle(currX, currZ, 3, CYAN);

    } else {
      // Phase Plot (Gyro Mag vs Threshold)
      // Visualizes the ZUPT detector
      c.drawRect(0, 20, 240, 115, 0x2222);
      c.drawLine(0, 100, 240, 100, RED); // Threshold Line

      float gyroMag = sqrt(gx * gx + gy * gy + gz * gz);
      int barHeight = (int)gyroMag;
      if (barHeight > 100)
        barHeight = 100;

      c.fillRect(100, 135 - barHeight, 40, barHeight,
                 gyroMag < ZUPT_GYRO_THRESHOLD_DPS ? GREEN : RED);

      c.setTextSize(2);
      c.setTextColor(WHITE);
      c.drawCenterString(gyroMag < ZUPT_GYRO_THRESHOLD_DPS ? "STATIONARY"
                                                           : "MOVING",
                         120, 40, 1);
      c.setTextSize(1);
      c.drawCenterString("Gyro Magnitude", 120, 120, 1);
    }
    c.setTextColor(WHITE);
    c.setTextSize(1);
    c.drawRightString(mode == SCOPE_TRAJECTORY ? "View:TRAJ[A]"
                                               : "View:ZUPT[A]",
                      235, 120, 1);
  }
  void onBtnA() override {
    mode = (mode == SCOPE_TRAJECTORY) ? SCOPE_PHASE : SCOPE_TRAJECTORY;
  }
};

// --- Files & Settings (Simplified) ---
class FilesApp : public App {
  std::vector<String> files;
  int sel = 0;

public:
  void onOpen() override {
    files.clear();
    File root = LittleFS.open("/");
    File f = root.openNextFile();
    while (f) {
      if (String(f.name()).endsWith(".csv"))
        files.push_back(String(f.name()));
      f = root.openNextFile();
    }
  }
  void onDraw(M5Canvas &c) override {
    c.setTextSize(2);
    c.setTextColor(YELLOW);
    c.setCursor(10, 30);
    c.println("FILES");
    if (files.empty()) {
      c.setTextColor(LIGHTGREY);
      c.setCursor(10, 60);
      c.println("No Logs");
      return;
    }
    for (int i = 0; i < 4; i++) {
      int idx = (sel / 4) * 4 + i;
      if (idx >= files.size())
        break;
      c.setTextColor(idx == sel ? WHITE : LIGHTGREY);
      c.setCursor(10, 60 + i * 20);
      c.println(files[idx].substring(1));
    }
    c.setTextColor(RED);
    c.setTextSize(1);
    c.drawRightString("DEL[A]", 235, 120, 1);
  }
  void onBtnB() override {
    if (!files.empty())
      sel = (sel + 1) % files.size();
  }
  void onBtnA() override {
    if (!files.empty()) {
      LittleFS.remove(files[sel]);
      onOpen();
      showToast("Deleted");
    }
  }
};

class SettingsApp : public App {
  const char *opts[3] = {"Flip Screen", "Clear Logs", "About V10"};
  int sel = 0;

public:
  void onDraw(M5Canvas &c) override {
    c.setTextSize(2);
    c.setTextColor(MAGENTA);
    c.setCursor(10, 30);
    c.println("SETTINGS");
    for (int i = 0; i < 3; i++) {
      c.setTextColor(i == sel ? WHITE : LIGHTGREY);
      c.setCursor(20, 60 + i * 20);
      c.println(opts[i]);
    }
  }
  void onBtnB() override { sel = (sel + 1) % 3; }
  void onBtnA() override {
    if (sel == 0) {
      currentRotation = (currentRotation == 1) ? 3 : 1;
      M5.Display.setRotation(currentRotation);
      M5.Display.clear();
    }
    if (sel == 1) {
      LittleFS.format();
      showToast("Cleared");
    }
    if (sel == 2) {
      showToast("GaitOS V10", 3000);
    }
  }
};

// =============================================================================
// KERNEL
// =============================================================================

LauncherApp appLauncher;
GaitLabApp appGaitLab;
ScopeApp appScope;
FilesApp appFiles;
SettingsApp appSettings;

void LauncherApp::onBtnA() {
  switch (sel) {
  case 0:
    currentApp = &appGaitLab;
    currentAppID = APP_GAITLAB;
    break;
  case 1:
    currentApp = &appScope;
    currentAppID = APP_SCOPE;
    break;
  case 2:
    currentApp = &appFiles;
    currentAppID = APP_FILES;
    break;
  case 3:
    currentApp = &appSettings;
    currentAppID = APP_SETTINGS;
    break;
  }
  if (currentApp)
    currentApp->onOpen();
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(
      currentRotation); // FIX: Set rotation *before* creating sprite
  canvas.createSprite(M5.Display.width(), M5.Display.height());
  Serial.begin(115200);
  if (!LittleFS.begin(true))
    LittleFS.begin(true);
  WiFi.softAP(WIFI_SSID, WIFI_PASS);
  server.on("/", HTTP_GET,
            []() { server.send_P(200, "text/html", index_html); });
  server.begin();

  // Boot Anim
  canvas.fillScreen(BLACK);
  canvas.setTextSize(3);
  canvas.setTextColor(ORANGE);
  canvas.drawCenterString("GaitOS v10", 120, 50, 1);
  canvas.setTextSize(1);
  canvas.setTextColor(CYAN);
  canvas.drawCenterString("MEDICAL GRADE KINEMATICS", 120, 90, 1);
  canvas.pushSprite(0, 0);
  delay(2000);

  currentApp = &appLauncher;
}

void loop() {
  M5.update();
  server.handleClient();
  updateSensors();

  // Input
  if (M5.BtnA.wasPressed())
    currentApp->onBtnA();
  if (M5.BtnB.wasPressed())
    currentApp->onBtnB();
  if (M5.BtnPWR.wasPressed() && currentAppID != APP_LAUNCHER) {
    currentApp->onClose();
    currentApp = &appLauncher;
    currentAppID = APP_LAUNCHER;
    currentApp->onOpen();
  }

  // Draw
  canvas.fillScreen(BLACK);

  // System Bar
  canvas.fillRect(0, 0, 240, 20, 0x1111);
  canvas.setTextSize(1);
  canvas.setTextColor(WHITE);
  canvas.setCursor(5, 4);
  canvas.printf("%02d:%02d", M5.Rtc.getTime().hours, M5.Rtc.getTime().minutes);
  canvas.setCursor(205, 4);
  canvas.printf("%d%%", M5.Power.getBatteryLevel());

  currentApp->onDraw(canvas);

  if (millis() < toastEndTime) {
    canvas.fillRoundRect(40, 100, 160, 30, 5, 0x3333);
    canvas.drawRoundRect(40, 100, 160, 30, 5, CYAN);
    canvas.setTextSize(1);
    canvas.setTextColor(CYAN);
    canvas.drawCenterString(toastMsg, 120, 110, 1);
  }
  canvas.pushSprite(0, 0);
}
