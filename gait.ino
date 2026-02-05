/**
 * M5StickC Plus2 GaitOS V2.0 (Complete Rewrite)
 *
 * ANKLE-MOUNTED Gait Analysis System
 * - Madgwick quaternion-based orientation tracking
 * - Adaptive ZUPT with variance-based detection
 * - Auto-calibration on stillness
 * - Real stride length integration
 * - Battery management
 *
 * Architecture: M5Unified + LittleFS + WiFi + WebServer
 *
 * BREAKING CHANGES from V1.3:
 * - Removed HFC metric (no validation)
 * - Changed CSV format (added quaternions)
 * - Ankle mounting only (not foot)
 * - New ZUPT algorithm (adaptive thresholds)
 */

#include <Arduino.h>
#include <LittleFS.h>
#include <M5Unified.h>
#include <WebServer.h>
#include <WiFi.h>

#include "MadgwickFilter.h"
#include "web_page.h"


// =============================================================================
// TYPES
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
// CONFIG - OPTIMIZED FOR ANKLE MOUNTING
// =============================================================================
const char *WIFI_SSID = "GAIT-LOGGER";
const char *WIFI_PASS = "circumduct123";
const int SAMPLE_INTERVAL_MS = 10; // 100Hz

// Tuning Parameters - Ankle-specific thresholds
float ZUPT_THRESH_DPS = 35.0f;   // Lower for ankle (more rotation than foot)
float ZUPT_ACCEL_G = 0.25f;      // Higher for ankle (more shock)
float MIN_SWING_ACCEL = 1.5f;    // Higher peaks at ankle
float MIN_STEP_TIME_MS = 280.0f; // Slightly faster detection

// =============================================================================
// GLOBAL STATE
// =============================================================================
// IMU Raw Data
float accX, accY, accZ;
float gyroX, gyroY, gyroZ;

// Orientation (Madgwick filter output)
float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f; // Quaternion
float roll = 0, pitch = 0, yaw = 0;               // Euler angles (derived)
MadgwickFilter madgwick;

// Kinematics (Navigation Frame)
Vector3 vel = {0, 0, 0};
Vector3 pos = {0, 0, 0}; // x=forward, y=lateral, z=vertical

// Algorithm State
bool isStance = false;
unsigned long lastStepTime = 0;
float distanceTotal = 0;
float stepCount = 0;
float currentCadence = 0;
float stabilityIndex = 100.0f;

// Stride tracking
float lastStanceX = 0;
float recentStrides[10] = {0};
int strideIdx = 0;

// Auto-calibration
unsigned long stillStartTime = 0;
bool isStill = false;
bool isCalibrated = false;
float gyroBiasX = 0, gyroBiasY = 0, gyroBiasZ = 0;

// Battery
float batteryVoltage = 0;
int batteryPercent = 100;
unsigned long lastBatteryCheck = 0;

// Data Structures (Ring Buffers)
#define TRAJECTORY_LEN 256
Point trajectory[TRAJECTORY_LEN];
int trajHead = 0;
int trajCount = 0;

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
// ADAPTIVE ZUPT DETECTOR
// =============================================================================
class ZUPTDetector {
private:
  float accelWindow[20]; // 200ms window at 100Hz
  float gyroWindow[20];
  int windowIdx = 0;
  float currentThreshold = 0.25f;

public:
  void init() {
    for (int i = 0; i < 20; i++) {
      accelWindow[i] = 1.0f;
      gyroWindow[i] = 0.0f;
    }
    windowIdx = 0;
  }

  bool detectStance(float ax, float ay, float az, float gx, float gy, float gz,
                    float currentCadence) {
    // 1. Update sliding window
    accelWindow[windowIdx] = sqrt(ax * ax + ay * ay + az * az);
    gyroWindow[windowIdx] = sqrt(gx * gx + gy * gy + gz * gz);
    windowIdx = (windowIdx + 1) % 20;

    // 2. Adaptive threshold based on cadence (gait frequency)
    if (currentCadence > 120) {
      currentThreshold = 0.15f; // Stricter for fast walking/running
      ZUPT_THRESH_DPS = 45.0f;
    } else if (currentCadence < 60) {
      currentThreshold = 0.35f; // Looser for slow gait
      ZUPT_THRESH_DPS = 25.0f;
    } else {
      currentThreshold = 0.25f; // Normal walking
      ZUPT_THRESH_DPS = 35.0f;
    }

    // 3. Variance check over window
    float accelVar = calculateVariance(accelWindow, 20);
    float gyroMag = avg(gyroWindow, 20);

    // 4. Multi-criteria detection
    bool lowAccelVar = (accelVar < currentThreshold);
    bool lowGyro = (gyroMag < ZUPT_THRESH_DPS);
    bool nearGravity = (abs(sqrt(ax * ax + ay * ay + az * az) - 1.0f) < 0.3f);

    return (lowAccelVar && lowGyro && nearGravity);
  }

private:
  float calculateVariance(float *data, int len) {
    float mean = avg(data, len);
    float variance = 0;
    for (int i = 0; i < len; i++) {
      variance += (data[i] - mean) * (data[i] - mean);
    }
    return variance / len;
  }

  float avg(float *data, int len) {
    float sum = 0;
    for (int i = 0; i < len; i++)
      sum += data[i];
    return sum / len;
  }
};

ZUPTDetector zuptDetector;

// =============================================================================
// AUTO-CALIBRATION
// =============================================================================
void checkAutoCalibration() {
  // Detect stillness: low accel variance + near gravity + low gyro
  float accelMag = sqrt(accX * accX + accY * accY + accZ * accZ);
  float gyroMag = sqrt(gyroX * gyroX + gyroY * gyroY + gyroZ * gyroZ);

  bool currentlyStill = (abs(accelMag - 1.0f) < 0.05f) && (gyroMag < 5.0f);

  if (currentlyStill) {
    if (!isStill) {
      // Just became still
      stillStartTime = millis();
      isStill = true;
    } else if ((millis() - stillStartTime > 3000) && !isCalibrated) {
      // Still for 3 seconds -> auto-calibrate
      gyroBiasX = gyroX;
      gyroBiasY = gyroY;
      gyroBiasZ = gyroZ;

      // Reset navigation state
      pos = {0, 0, 0};
      vel = {0, 0, 0};
      stepCount = 0;
      distanceTotal = 0;
      lastStanceX = 0;
      madgwick.reset();

      isCalibrated = true;
      showToast("Auto-Cal!", 2000);
    }
  } else {
    isStill = false;
    stillStartTime = 0;
  }
}

// =============================================================================
// BATTERY MANAGEMENT
// =============================================================================
void updateBattery() {
  if (millis() - lastBatteryCheck < 5000)
    return; // Check every 5s

  // Read voltage from GPIO38 (voltage divider: Vbat -> 2:1 -> ADC)
  int adcValue = analogRead(38);
  batteryVoltage = (adcValue / 4095.0f) * 3.3f * 2.0f; // Vbat = ADC * 2

  // Convert to percentage (LiPo: 4.2V=100%, 3.0V=0%)
  batteryPercent = constrain(((batteryVoltage - 3.0f) / 1.2f) * 100, 0, 100);

  lastBatteryCheck = millis();

  // Low battery warning
  if (batteryPercent < 15 && batteryPercent > 13) {
    showToast("Low Battery!", 3000);
  }
}

// =============================================================================
// DATA VALIDATION
// =============================================================================
void validateData() {
  // Catch obviously wrong values
  if (abs(vel.x) > 5.0f) { // 5 m/s = 18 km/h (max running speed)
    vel.x = 0;
    showToast("Vel overflow!");
  }

  if (abs(vel.z) > 3.0f) {
    vel.z = 0;
  }

  if (pos.z > 0.5f) { // Max clearance 50cm
    pos.z = 0;
  }

  if (pos.z < -0.1f) { // Don't go below ground
    pos.z = 0;
  }
}

// =============================================================================
// ZUPT-INS UPDATE (COMPLETE REWRITE)
// =============================================================================
void ZUPT_INS_Update(float dt) {
  // 1. Rotate body-frame acceleration to navigation frame using quaternion
  float accel_body[3] = {accX, accY, accZ};
  float accel_nav[3];
  madgwick.rotateVector(accel_body[0], accel_body[1], accel_body[2], accel_nav);

  // 2. Remove gravity (navigation frame: Z-down convention, gravity = +1g on Z)
  accel_nav[0] -= 0.0f; // No gravity on X
  accel_nav[1] -= 0.0f; // No gravity on Y
  accel_nav[2] -= 1.0f; // Remove 1g on Z

  // 3. Convert to m/s²
  float ax_world = accel_nav[0] * 9.81f;
  float ay_world = accel_nav[1] * 9.81f;
  float az_world = accel_nav[2] * 9.81f;

  // 4. Adaptive ZUPT Detection
  bool stanceDetected = zuptDetector.detectStance(accX, accY, accZ, gyroX,
                                                  gyroY, gyroZ, currentCadence);

  if (stanceDetected) {
    // Zero Velocity Update (Clamp drift)
    vel = {0, 0, 0};
    isStance = true;
  } else {
    isStance = false;

    // 5. Integration (Navigation Frame)
    vel.x += ax_world * dt;
    vel.y += ay_world * dt;
    vel.z += az_world * dt;

    pos.x += vel.x * dt;
    pos.y += vel.y * dt;
    pos.z += vel.z * dt;

    // Dampen Z to ground
    if (pos.z < 0)
      pos.z = 0;
  }

  // 6. Step Detection & Cadence
  static float maxSwing = 0;
  if (!isStance && abs(accZ) > maxSwing)
    maxSwing = abs(accZ);

  if (isStance && maxSwing > MIN_SWING_ACCEL &&
      (millis() - lastStepTime > MIN_STEP_TIME_MS)) {

    // Step detected!
    unsigned long dur = millis() - lastStepTime;
    lastStepTime = millis();
    stepCount++;

    // Stride length from integration (not hardcoded!)
    float strideLength = abs(pos.x - lastStanceX);
    if (strideLength > 0.2f && strideLength < 2.0f) { // Sanity check
      distanceTotal += strideLength;
      recentStrides[strideIdx] = strideLength;
      strideIdx = (strideIdx + 1) % 10;
    }
    lastStanceX = pos.x;

    // Cadence calculation
    float instCadence = 60000.0f / dur;
    currentCadence = (currentCadence * 0.8f) + (instCadence * 0.2f);

    // Stability Index
    float deviation =
        abs(instCadence - currentCadence) / (currentCadence + 1.0f);
    float instStability = constrain(100.0f * (1.0f - deviation), 0.0f, 100.0f);
    stabilityIndex = (stabilityIndex * 0.9f) + (instStability * 0.1f);

    maxSwing = 0;
  }

  // 7. Trajectory Buffer
  trajectory[trajHead].x = (int)(pos.x * 100);
  trajectory[trajHead].y = (int)(pos.z * 100);
  trajHead = (trajHead + 1) % TRAJECTORY_LEN;
  if (trajCount < TRAJECTORY_LEN)
    trajCount++;

  // 8. Data validation
  validateData();
}

// =============================================================================
// JSON API
// =============================================================================
void getStatusJSON() {
  String json = "{";
  json += "\"recording\":" + String(isRecording) + ",";
  json += "\"step_count\":" + String((int)stepCount) + ",";
  json += "\"dist_m\":" + String(distanceTotal, 2) + ",";
  json += "\"cad\":" + String(currentCadence, 1) + ",";
  json += "\"stab\":" + String(stabilityIndex, 1) + ",";
  json += "\"px\":" + String(pos.x, 3) + ",";
  json += "\"py\":" + String(pos.y, 3) + ",";
  json += "\"pz\":" + String(pos.z, 3) + ",";
  json += "\"phase\":" + String(isStance ? 0 : 1) + ",";
  json += "\"pitch\":" + String(pitch, 1) + ",";
  json += "\"roll\":" + String(roll, 1) + ",";
  json += "\"yaw\":" + String(yaw, 1) + ",";
  json += "\"is_stat\":" + String(isStance) + ",";
  json += "\"battery_pct\":" + String(batteryPercent) + ",";
  json += "\"battery_v\":" + String(batteryVoltage, 2) + ",";
  json += "\"calibrated\":" + String(isCalibrated);
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
  c.drawCircle(x, y, haloR, color);
}

// 1. LAUNCHER
class LauncherApp : public App {
  int sel = 0;

public:
  void onDraw(M5Canvas &c) override {
    c.fillRect(0, 0, 240, 135, BLACK);
    c.setTextColor(WHITE);
    c.drawCenterString("GaitOS V2.0", 120, 20, 2);

    // Battery indicator
    c.setTextSize(1);
    c.setCursor(180, 5);
    c.printf("Bat:%d%%", batteryPercent);

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

    // Battery
    c.setTextSize(1);
    c.setCursor(10, 5);
    c.printf("Bat: %d%%", batteryPercent);

    // Calibration status
    c.setCursor(150, 5);
    if (isCalibrated) {
      c.setTextColor(GREEN);
      c.print("CAL OK");
    } else {
      c.setTextColor(RED);
      c.print("NO CAL");
    }

    c.setTextColor(WHITE);
    c.setTextSize(2);
    c.setCursor(10, 25);
    c.printf("Steps: %.0f", stepCount);
    c.setCursor(10, 50);
    c.printf("Cad: %.0f", currentCadence);
    c.setTextSize(1);
    c.setCursor(10, 75);
    c.printf("Dist: %.1fm", distanceTotal);
    c.setCursor(10, 90);
    c.printf("Stability: %.0f%%", stabilityIndex);
    c.setCursor(10, 105);
    c.printf("Height: %.2fm", pos.z);

    if (isRecording)
      c.fillCircle(220, 20, 8, RED);
  }
  void onBtnA() override {
    isRecording = !isRecording;
    if (isRecording) {
      // Enhanced CSV header with metadata
      logFile = LittleFS.open("/log_" + String(millis()) + ".csv", FILE_WRITE);
      logFile.println("# GaitOS V2.0 - Ankle Mounted");
      logFile.println("# Sample Rate: 100Hz");
      logFile.println("# ZUPT Threshold: " + String(ZUPT_THRESH_DPS) +
                      " deg/s");
      logFile.println("# Calibration: " +
                      String(isCalibrated ? "Auto" : "Manual"));
      logFile.println("#");
      logFile.println("# Columns:");
      logFile.println(
          "# t(ms), ax(g), ay(g), az(g), gx(dps), gy(dps), gz(dps),");
      logFile.println("# q0, q1, q2, q3 (quaternion),");
      logFile.println("# roll(deg), pitch(deg), yaw(deg),");
      logFile.println("# vx(m/s), vy(m/s), vz(m/s),");
      logFile.println("# px(m), py(m), pz(m),");
      logFile.println("# phase(0=stance,1=swing), cadence(spm), stability(%)");
      logFile.println("#");
      logFile.println("t,ax,ay,az,gx,gy,gz,q0,q1,q2,q3,roll,pitch,yaw,vx,vy,vz,"
                      "px,py,pz,phase,cadence,stability");
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
    c.setTextColor(WHITE);
    c.setTextSize(1);
    c.setCursor(5, 5);
    c.printf("Trajectory (Z vs X)");

    int cx = 20, cy = 120;
    // Draw Trajectory from ring buffer
    for (int i = 0; i < trajCount - 1 && i < TRAJECTORY_LEN - 1; i++) {
      int idx = (trajHead - trajCount + i + TRAJECTORY_LEN) % TRAJECTORY_LEN;
      Point p1 = trajectory[idx];
      Point p2 = trajectory[(idx + 1) % TRAJECTORY_LEN];

      int x1 = cx + p1.x;
      int y1 = cy - p1.y;
      int x2 = cx + p2.x;
      int y2 = cy - p2.y;

      // Bounds check
      if (x1 >= 0 && x1 < 240 && x2 >= 0 && x2 < 240 && y1 >= 0 && y1 < 135 &&
          y2 >= 0 && y2 < 135) {
        c.drawLine(x1, y1, x2, y2, CYAN);
      }
    }
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
// --- CONFIG API ---
void handleConfig() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");

    // Manual JSON Parsing
    int idxStep = body.indexOf("step_time");
    if (idxStep > 0) {
      int valStart = body.indexOf(":", idxStep) + 1;
      int valEnd = body.indexOf(",", valStart);
      if (valEnd == -1)
        valEnd = body.indexOf("}", valStart);
      MIN_STEP_TIME_MS = body.substring(valStart, valEnd).toFloat();
    }

    int idxZupt = body.indexOf("zupt_acc");
    if (idxZupt > 0) {
      int valStart = body.indexOf(":", idxZupt) + 1;
      int valEnd = body.indexOf(",", valStart);
      if (valEnd == -1)
        valEnd = body.indexOf("}", valStart);
      ZUPT_ACCEL_G = body.substring(valStart, valEnd).toFloat();
    }
    showToast("Saved!", 1000);
  }

  String json = "{";
  json += "\"step_time\":" + String(MIN_STEP_TIME_MS) + ",";
  json += "\"zupt_acc\":" + String(ZUPT_ACCEL_G);
  json += "}";
  server.send(200, "application/json", json);
}

// --- LOGS API ---
void handleLogsList() {
  File root = LittleFS.open("/");
  String output = "[";
  if (root) {
    File file = root.openNextFile();
    bool first = true;
    while (file) {
      String name = String(file.name());
      if (name.endsWith(".csv")) {
        if (!first)
          output += ",";
        output +=
            "{\"name\":\"" + name + "\",\"size\":" + String(file.size()) + "}";
        first = false;
      }
      file = root.openNextFile();
    }
  }
  output += "]";
  server.send(200, "application/json", output);
}

// --- FILE DOWNLOAD ---
bool handleFileRead(String path) {
  if (LittleFS.exists(path)) {
    File file = LittleFS.open(path, "r");
    server.streamFile(file, "text/csv");
    file.close();
    return true;
  }
  return false;
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(3);
  M5.Display.setBrightness(50); // Power saving: 30% brightness

  canvas.createSprite(M5.Display.width(), M5.Display.height());
  LittleFS.begin(true);

  // Initialize Madgwick filter
  madgwick.begin(0.1f); // Beta tuned for ankle mounting

  // Initialize ZUPT detector
  zuptDetector.init();

  WiFi.softAP(WIFI_SSID, WIFI_PASS);
  server.on("/", HTTP_GET,
            []() { server.send_P(200, "text/html", index_html); });
  server.on("/api/status", HTTP_GET, []() { getStatusJSON(); });

  // Tuning API
  server.on("/api/config", HTTP_POST, handleConfig);

  // Calibration API
  server.on("/api/calibrate", HTTP_POST, []() {
    pos = {0, 0, 0};
    vel = {0, 0, 0};
    distanceTotal = 0;
    stepCount = 0;
    currentCadence = 0;
    stabilityIndex = 100.0f;
    lastStanceX = 0;
    isCalibrated = true;
    madgwick.reset();
    showToast("Zeroed!", 1000);
    server.send(200);
  });

  // Basic API for Rec
  server.on("/api/record/start", HTTP_POST, []() {
    isRecording = true;
    logFile = LittleFS.open("/webrec_" + String(millis()) + ".csv", FILE_WRITE);
    logFile.println("# GaitOS V2.0 - Ankle Mounted");
    logFile.println("# Sample Rate: 100Hz");
    logFile.println("#");
    logFile.println("t,ax,ay,az,gx,gy,gz,q0,q1,q2,q3,roll,pitch,yaw,vx,vy,vz,"
                    "px,py,pz,phase,cadence,stability");
    server.send(200);
  });
  server.on("/api/record/stop", HTTP_POST, []() {
    isRecording = false;
    if (logFile)
      logFile.close();
    server.send(200);
  });

  // NEW: Logs & Downloads
  server.on("/api/logs", HTTP_GET, handleLogsList);
  server.onNotFound([]() {
    if (!handleFileRead(server.uri()))
      server.send(404, "text/plain", "404: Not Found");
  });

  server.begin();

  // Zero Sensors Init
  pos = {0, 0, 0};
  vel = {0, 0, 0};

  showToast("GaitOS V2.0", 2000);
}

void loop() {
  M5.update();
  server.handleClient();

  unsigned long now = millis();
  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    float dt = (now - lastSampleTime) / 1000.0f;
    lastSampleTime = now;

    // Read IMU
    M5.Imu.getAccel(&accX, &accY, &accZ);
    M5.Imu.getGyro(&gyroX, &gyroY, &gyroZ);

    // Auto-calibration check
    checkAutoCalibration();

    // Apply gyro bias correction
    gyroX -= gyroBiasX;
    gyroY -= gyroBiasY;
    gyroZ -= gyroBiasZ;

    // Update Madgwick filter (gyro in rad/s)
    madgwick.update(gyroX * DEG_TO_RAD, gyroY * DEG_TO_RAD, gyroZ * DEG_TO_RAD,
                    accX, accY, accZ, dt);
    madgwick.getQuaternion(&q0, &q1, &q2, &q3);
    madgwick.getEuler(&roll, &pitch, &yaw);

    // ZUPT-INS Update
    ZUPT_INS_Update(dt);

    // Battery check
    updateBattery();

    // Data recording
    if (isRecording && logFile) {
      logFile.printf(
          "%lu,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%.4f,%.4f,%.4f,%.4f,%.1f,%.1f,%."
          "1f,%.3f,%.3f,%.3f,%.4f,%.4f,%.4f,%d,%.1f,%.1f\n",
          now, accX, accY, accZ, gyroX, gyroY, gyroZ, q0, q1, q2, q3, roll,
          pitch, yaw, vel.x, vel.y, vel.z, pos.x, pos.y, pos.z,
          isStance ? 0 : 1, currentCadence, stabilityIndex);
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
