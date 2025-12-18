/**
 * M5StickC Plus2 GaitOS v13.0 (Hybrid Refinement)
 *
 * "Steve & Woz" Edition - Refined
 * - Robustness: Step Time & Amplitude Gating (Anti-Sensitivity).
 * - features: Cadence (SPM), Stability Index, Swing Time.
 * - Hybrid: ZUPT Physics + Empirical Limits.
 */

#include <Arduino.h>
#include <LittleFS.h>
#include <M5Unified.h>
#include <WebServer.h>
#include <WiFi.h>

#include "web_page.h"

// =============================================================================
// CONFIG
// =============================================================================
const char *WIFI_SSID = "GAIT-LOGGER";
const char *WIFI_PASS = "circumduct123";
const int SAMPLE_RATE_HZ = 100;
const int SAMPLE_INTERVAL_MS = 10;
const int TRAJ_BUF_SIZE = 256;

// Tuning Parameters (The "Woz" Tweaks)
const float MIN_STEP_TIME_MS = 300.0f; // Max 200 SPM (Running)
const float MIN_SWING_ACCEL = 1.2f;    // Must accelerate to be a step
const float ZUPT_THRESH_DPS = 40.0f;   // Gyro threshold for stance

// =============================================================================
// TYPES & GLOBALS
// =============================================================================

enum AppID { APP_LAUNCHER, APP_GAITLAB, APP_SCOPE, APP_CONNECT, APP_SETTINGS };
enum GaitPhase { PHASE_STANCE, PHASE_SWING };
struct Vector3 {
  float x, y, z;
};
struct Point {
  int16_t x, z;
};

struct RingBuffer {
  Point buffer[TRAJ_BUF_SIZE];
  int head = 0;
  int count = 0;
  void push(Point p) {
    buffer[head] = p;
    head = (head + 1) & (TRAJ_BUF_SIZE - 1);
    if (count < TRAJ_BUF_SIZE)
      count++;
  }
  Point get(int idx) {
    int start = (count < TRAJ_BUF_SIZE) ? 0 : head;
    return buffer[(start + idx) & (TRAJ_BUF_SIZE - 1)];
  }
};

class App {
public:
  virtual void onOpen() {}
  virtual void onClose() {}
  virtual void onDraw(M5Canvas &c) {}
  virtual void onBtnA() {}
  virtual void onBtnB() {}
};

WebServer server(80);
M5Canvas canvas(&M5.Display);

AppID currentAppID = APP_LAUNCHER;
App *currentApp = nullptr;
bool isRecording = false;
File logFile;
unsigned long lastSampleTime = 0;
String toastMsg = "";
unsigned long toastEndTime = 0;

// Physics
float ax, ay, az, gx, gy, gz;
float gbx = 0, gby = 0, gbz = 0;
float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;
float beta = 0.5f;
Vector3 vel = {0, 0, 0}, pos = {0, 0, 0};
GaitPhase currentPhase = PHASE_STANCE;
float roll = 0, pitch = 0, yaw = 0, yaw_offset = 0;

// Metrics (The "Packed" Features)
unsigned long stepCount = 0;
float distanceTotal = 0.0f;
float lastClearance = 0.0f;
bool isStationary = true;
float cadence = 0.0f;          // Steps Per Minute
float stabilityIndex = 100.0f; // 100% = Perfect, 0% = Unstable
unsigned long lastStepTime = 0;
float currentSwingMaxAccel = 0.0f;

RingBuffer trajectory;

// =============================================================================
// HELPERS
// =============================================================================

void showToast(String msg, int durationMs = 1500) {
  toastMsg = msg;
  toastEndTime = millis() + durationMs;
}

void drawIcon(M5Canvas &c, int id, int x, int y, uint16_t color) {
  if (id == 0) { // Foot
    c.fillEllipse(x, y + 5, 12, 6, color);
    c.fillEllipse(x + 15, y, 10, 8, color);
    c.fillTriangle(x - 5, y + 5, x + 15, y + 5, x + 10, y - 5, color);
  } else if (id == 1) { // Wave
    for (int i = -15; i < 15; i++)
      c.drawPixel(x + i, y + sin(i * 0.3) * 10, color);
  } else if (id == 2) { // QR
    c.drawRect(x - 12, y - 12, 24, 24, color);
    c.fillRect(x - 8, y - 8, 16, 16, color);
  } else if (id == 3) { // Gear
    c.drawCircle(x, y, 12, color);
    c.drawCircle(x, y, 8, color);
  }
}

Vector3 rotateVector(float x, float y, float z) {
  float _2q0 = 2 * q0, _2q1 = 2 * q1, _2q2 = 2 * q2, _2q3 = 2 * q3;
  float q0q0 = q0 * q0, q1q1 = q1 * q1, q2q2 = q2 * q2, q3q3 = q3 * q3;
  return {(q0q0 + q1q1 - q2q2 - q3q3) * x + (_2q1 * q2 - _2q0 * q3) * y +
              (_2q1 * q3 + _2q0 * q2) * z,
          (_2q1 * q2 + _2q0 * q3) * x + (q0q0 - q1q1 + q2q2 - q3q3) * y +
              (_2q2 * q3 - _2q0 * q1) * z,
          (_2q1 * q3 - _2q0 * q2) * x + (_2q2 * q3 + _2q0 * q1) * y +
              (q0q0 - q1q1 - q2q2 + q3q3) * z};
}

void QuaternionToEuler() {
  float sinr_cosp = 2 * (q0 * q1 + q2 * q3),
        cosr_cosp = 1 - 2 * (q1 * q1 + q2 * q2);
  roll = atan2(sinr_cosp, cosr_cosp) * 57.29f;
  float sinp = 2 * (q0 * q2 - q3 * q1);
  pitch = (abs(sinp) >= 1) ? copysign(90.f, sinp) : asin(sinp) * 57.29f;
  float siny_cosp = 2 * (q0 * q3 + q1 * q2),
        cosy_cosp = 1 - 2 * (q2 * q2 + q3 * q3);
  yaw = atan2(siny_cosp, cosy_cosp) * 57.29f - yaw_offset;
}

void ZeroSensors() {
  vel = {0, 0, 0};
  pos = {0, 0, 0};
  stepCount = 0;
  distanceTotal = 0;
  trajectory.count = 0;
  trajectory.head = 0;

  float sumX = 0, sumY = 0, sumZ = 0;
  for (int i = 0; i < 200; i++) {
    M5.Imu.getGyro(&gx, &gy, &gz);
    sumX += gx;
    sumY += gy;
    sumZ += gz;
    delay(2);
  }
  gbx = sumX / 200.0f;
  gby = sumY / 200.0f;
  gbz = sumZ / 200.0f;
  yaw_offset = 0;
  QuaternionToEuler();
  yaw_offset = yaw;
  showToast("Precision Zeroed");
}

// HYBRID ENGINE LOGIC
void UpdatePhysics(float dt, unsigned long now) {
  // 1. Bias Correction
  float _gx = (gx - gbx) * 0.01745f;
  float _gy = (gy - gby) * 0.01745f;
  float _gz = (gz - gbz) * 0.01745f;

  // 2. Madgwick
  // (Condensed for space - assuming standard Madgwick math here)
  float recip, s0, s1, s2, s3, qD1, qD2, qD3, qD4;
  // ...
  if (!((ax == 0) && (ay == 0) && (az == 0))) {
    recip = 1.0f / sqrt(ax * ax + ay * ay + az * az);
    float _ax = ax * recip, _ay = ay * recip, _az = az * recip;
    float _2q0 = 2 * q0, _2q1 = 2 * q1, _2q2 = 2 * q2, _4q0 = 4 * q0,
          _4q1 = 4 * q1, _4q2 = 4 * q2, _8q1 = 8 * q1, _8q2 = 8 * q2;
    float q0q0 = q0 * q0, q1q1 = q1 * q1, q2q2 = q2 * q2, q3q3 = q3 * q3;
    // Gradient Descent (simplified)
    s0 = _4q0 * q2q2 + _2q2 * _ax + _4q0 * q1q1 - _2q1 * _ay;
    s1 = _4q1 * q3q3 - 2 * q3 * _ax + 4 * q0q0 * q1 - _2q0 * _ay - _4q1 +
         _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * _az;
    s2 = 4 * q0q0 * q2 + _2q0 * _ax + _4q2 * q3q3 - 2 * q3 * _ay - _4q2 +
         _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * _az;
    s3 = 4 * q1q1 * q3 - _2q1 * _ax + 4 * q2q2 * q3 - _2q2 * _ay;
    recip = 1.f / sqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);
    s0 *= recip;
    s1 *= recip;
    s2 *= recip;
    s3 *= recip;
    qD1 = 0.5f * (-q1 * _gx - q2 * _gy - q3 * _gz) - beta * s0;
    qD2 = 0.5f * (q0 * _gx + q2 * _gz - q3 * _gy) - beta * s1;
    qD3 = 0.5f * (q0 * _gy - q1 * _gz + q3 * _gx) - beta * s2;
    qD4 = 0.5f * (q0 * _gz + q1 * _gy - q2 * _gx) - beta * s3;
    q0 += qD1 * dt;
    q1 += qD2 * dt;
    q2 += qD3 * dt;
    q3 += qD4 * dt;
    recip = 1.f / sqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    q0 *= recip;
    q1 *= recip;
    q2 *= recip;
    q3 *= recip;
  }
  QuaternionToEuler();

  // 3. ZUPT - Tuned for Hybrid
  float gMag = abs(gx) + abs(gy) + abs(gz);
  float aMag = sqrt(ax * ax + ay * ay + az * az);
  isStationary = (gMag < ZUPT_THRESH_DPS);

  if (currentPhase == PHASE_STANCE) {
    if (!isStationary) {
      // START SWING
      currentPhase = PHASE_SWING;
      currentSwingMaxAccel = 0; // Reset metrics
    } else {
      vel = {0, 0, 0}; // Clamp Velocity
    }
  } else { // Swing
    if (isStationary) {
      // END SWING -> Potential Step
      currentPhase = PHASE_STANCE;

      // VALIDATION ALGORITHM (The "Refinement")
      float stepDur = (now - lastStepTime);
      if (stepDur > MIN_STEP_TIME_MS &&
          currentSwingMaxAccel > MIN_SWING_ACCEL) {
        // Valid Step
        stepCount++;
        float stepDist = sqrt(pos.x * pos.x + pos.y * pos.y);
        if (stepDist > 1.5f)
          stepDist = 1.0f; // Clamp Clumsy GPS-like jumps
        distanceTotal += stepDist;

        // Calculate Cadence (Steps/Min)
        float instCadence = 60000.0f / stepDur;
        cadence = (cadence * 0.8f) + (instCadence * 0.2f); // Smooth it

        // Stability: Variance in step time (Simple Metric)
        float var = abs(instCadence - cadence);
        stabilityIndex = constrain(100.0f - var, 0, 100);

        lastStepTime = now;
      } else {
        // Invalid (Noise/Shuffle) - Don't count, maybe noise
      }
      // Reset for next step, keeping global pos relative
      pos = {0, 0, 0};

    } else {
      // Integrate
      Vector3 acc = rotateVector(ax, ay, az);
      acc.z -= 1.0f;
      acc.x *= 9.81f;
      acc.y *= 9.81f;
      acc.z *= 9.81f;
      vel.x += acc.x * dt;
      vel.y += acc.y * dt;
      vel.z += acc.z * dt;
      pos.x += vel.x * dt;
      pos.y += vel.y * dt;
      pos.z += vel.z * dt;

      if (aMag > currentSwingMaxAccel)
        currentSwingMaxAccel = aMag;
      if (pos.z * 100 > lastClearance)
        lastClearance = pos.z * 100;

      // Visualization Push
      Point p = {(int16_t)(pos.x * 100), (int16_t)(pos.z * 100)};
      if (trajectory.count == 0 ||
          abs(p.x -
              trajectory.buffer[(trajectory.head - 1) & (TRAJ_BUF_SIZE - 1)]
                  .x) > 1) {
        trajectory.push(p);
      }
    }
  }
}

// =============================================================================
// APPS
// =============================================================================

class LauncherApp : public App {
  int sel = 0;

public:
  void onDraw(M5Canvas &c) override {
    c.fillRect(0, 0, 240, 135, BLACK);
    c.setTextSize(1);
    c.setTextColor(0x52AA);
    c.drawCenterString("GaitOS V13.0", 120, 120, 1);
    int spacing = 60, startX = 30;
    uint16_t cols[4] = {0x07E0, 0x041F, WHITE, 0xF800};
    const char *names[4] = {"Lab", "Scope", "Connect", "System"};
    for (int i = 0; i < 4; i++) {
      int x = startX + i * spacing;
      bool active = (i == sel);
      int r = active ? 24 + sin(millis() / 150.0) * 2 : 20;
      c.fillCircle(x, 55, r, active ? cols[i] : 0x2124);
      drawIcon(c, i, x, 55, active ? BLACK : WHITE);
      if (active) {
        c.setTextColor(WHITE);
        c.drawCenterString(names[i], 120, 20, 2);
      }
    }
  }
  void onBtnB() override { sel = (sel + 1) % 4; }
  void onBtnA() override;
};

class ConnectApp : public App {
public:
  void onDraw(M5Canvas &c) override {
    c.fillScreen(WHITE);
    String url = "http://" + WiFi.softAPIP().toString();
    c.qrcode(url, 65, 10, 115, 6);
    c.setTextColor(BLACK);
    c.drawCenterString(url, 120, 125, 1);
  }
};

class GaitLabApp : public App {
public:
  void onDraw(M5Canvas &c) override {
    c.fillScreen(BLACK);
    c.setTextColor(LIGHTGREY);
    c.setCursor(5, 5);
    c.print("HYBRID ENGINE");
    if (isRecording)
      c.fillCircle(230, 10, 6, RED);

    // Refined layout: Show Cadence (SPM)
    c.setTextSize(3);
    c.setTextColor(WHITE);
    c.drawCenterString(String(stepCount), 60, 45, 1);
    c.drawCenterString(String((int)cadence), 180, 45, 1);

    c.setTextSize(1);
    c.setTextColor(BLUE);
    c.drawCenterString("STEPS", 60, 80, 1);
    c.drawCenterString("SPM (CADENCE)", 180, 80, 1);

    // Bottom Bar: Distance + Stability
    c.drawFastHLine(20, 95, 200, 0x3333);
    c.setCursor(25, 105);
    c.setTextColor(WHITE);
    c.printf("DIST: %.1fm", distanceTotal);
    c.setCursor(140, 105);
    c.setTextColor(stabilityIndex > 80 ? GREEN : ORANGE);
    c.printf("STAB: %d%%", (int)stabilityIndex);
  }
  void onBtnA() override {
    if (isRecording) {
      isRecording = false;
      if (logFile)
        logFile.close();
      showToast("Saved");
    } else {
      logFile = LittleFS.open("/gait_" + String(millis()) + ".csv", FILE_WRITE);
      if (logFile) {
        isRecording = true;
        logFile.println("t,ax,ay,az,gx,gy,gz,px,pz,phase,roll,pitch,yaw");
        showToast("Rec...");
      }
    }
  }
};

class ScopeApp : public App {
public:
  void onDraw(M5Canvas &c) override {
    c.fillScreen(BLACK);
    int ox = 20, gy = 120;
    for (int i = 1; i < trajectory.count; i++) {
      Point p1 = trajectory.get(i - 1);
      Point p2 = trajectory.get(i);
      int x1 = (ox + p1.x) % 200;
      int z1 = gy - p1.z;
      int x2 = (ox + p2.x) % 200;
      int z2 = gy - p2.z;
      if (abs(x2 - x1) < 20)
        c.drawLine(x1, z1, x2, z2, ORANGE);
    }
    c.fillCircle((ox + (int)(pos.x * 100)) % 200, gy - (int)(pos.z * 100), 4,
                 CYAN);
    c.setTextSize(1);
    c.setTextColor(WHITE);
    c.setCursor(5, 5);
    c.printf("Z:%d cm", (int)(pos.z * 100));
  }
};
class SettingsApp : public App {
public:
  void onDraw(M5Canvas &c) override {
    c.fillScreen(BLACK);
    c.setTextColor(WHITE);
    c.drawCenterString("SYSTEM", 120, 20, 2);
    c.drawRoundRect(20, 50, 200, 40, 10, WHITE);
    c.drawCenterString("ZERO SENSORS [A]", 120, 65, 1);
  }
  void onBtnA() override { ZeroSensors(); }
};

GaitLabApp appGaitLab;
ScopeApp appScope;
ConnectApp appConnect;
SettingsApp appSettings;
LauncherApp appLauncher;

void LauncherApp::onBtnA() {
  if (sel == 0) {
    currentApp = &appGaitLab;
    currentAppID = APP_GAITLAB;
  }
  if (sel == 1) {
    currentApp = &appScope;
    currentAppID = APP_SCOPE;
  }
  if (sel == 2) {
    currentApp = &appConnect;
    currentAppID = APP_CONNECT;
  }
  if (sel == 3) {
    currentApp = &appSettings;
    currentAppID = APP_SETTINGS;
  }
  if (currentApp)
    currentApp->onOpen();
}

// =============================================================================
// API
// =============================================================================

void setupAPI() {
  server.on("/api/status", HTTP_GET, []() {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"recording\":%d,\"step_count\":%lu,\"dist_m\":%.1f,\"phase\":%"
             "d,\"px\":%.3f,\"pz\":%.3f,\"pitch\":%.1f}",
             isRecording, stepCount, distanceTotal, currentPhase, pos.x, pos.z,
             pitch);
    server.send(200, "application/json", buf);
  });

  server.on("/api/record/start", HTTP_POST, []() {
    if (!isRecording) {
      logFile = LittleFS.open("/gait_" + String(millis()) + ".csv", FILE_WRITE);
      if (logFile) {
        isRecording = true;
        logFile.println(
            "t,ax,ay,az,gx,gy,gz,px,pz,phase,roll,pitch,yaw,cadence");
        showToast("Remote REC");
      }
    }
    server.send(200, "text/plain", "OK");
  });
  server.on("/api/record/stop", HTTP_POST, []() {
    isRecording = false;
    if (logFile)
      logFile.close();
    showToast("Stopped");
    server.send(200, "text/plain", "OK");
  });
  server.on("/api/calibrate", HTTP_POST, []() {
    ZeroSensors();
    server.send(200, "text/plain", "OK");
  });

  server.on("/api/logs", HTTP_GET, []() {
    String json = "[";
    File root = LittleFS.open("/");
    File f = root.openNextFile();
    bool first = true;
    while (f) {
      if (String(f.name()).endsWith(".csv")) {
        if (!first)
          json += ",";
        json += "{\"name\":\"" + String(f.name()) +
                "\",\"size\":" + String(f.size()) + "}";
        first = false;
      }
      f = root.openNextFile();
    }
    json += "]";
    server.send(200, "application/json", json);
  });
  server.on("/api/format", HTTP_POST, []() {
    LittleFS.format();
    server.send(200, "text/plain", "Formatted");
  });
  server.onNotFound([]() {
    String uri = server.uri();
    if (uri.startsWith("/logs"))
      uri = uri.substring(5);
    if (LittleFS.exists(uri)) {
      File f = LittleFS.open(uri, "r");
      server.streamFile(f, "text/csv");
      f.close();
    } else
      server.send(404, "text/plain", "404");
  });
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(3);
  canvas.createSprite(M5.Display.width(), M5.Display.height());
  if (!LittleFS.begin(true))
    LittleFS.begin(true);

  WiFi.softAP(WIFI_SSID, WIFI_PASS);
  server.on("/", HTTP_GET,
            []() { server.send_P(200, "text/html", index_html); });
  setupAPI();
  server.begin();

  ZeroSensors();
  currentApp = &appLauncher;
}

void loop() {
  M5.update();
  server.handleClient();
  unsigned long now = millis();
  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    float dt = (now - lastSampleTime) / 1000.0f;
    lastSampleTime = now;
    M5.Imu.getAccel(&ax, &ay, &az);
    M5.Imu.getGyro(&gx, &gy, &gz);
    UpdatePhysics(dt, now);
    if (isRecording && logFile)
      logFile.printf("%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d,%.1f,%.1f,"
                     "%.1f,%.1f\n",
                     now, ax, ay, az, gx, gy, gz, pos.x, pos.z, currentPhase,
                     roll, pitch, yaw, cadence);
  }
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
  currentApp->onDraw(canvas);
  if (millis() < toastEndTime) {
    canvas.fillRoundRect(60, 100, 120, 25, 12, 0x2124);
    canvas.setTextColor(WHITE);
    canvas.drawCenterString(toastMsg, 120, 108, 1);
  }
  canvas.pushSprite(0, 0);
}
