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
#include "esp_system.h"
// esp_task_wdt removed — Arduino core manages WDT
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
float lastStanceY = 0;
float recentStrides[10] = {0};
int strideIdx = 0;

// Auto-calibration
unsigned long stillStartTime = 0;
bool isStill = false;
bool isCalibrated = false;
float gyroBiasX = 0, gyroBiasY = 0, gyroBiasZ = 0;
float accBiasX = 0, accBiasY = 0, accBiasZ = 0;
float autoCalSumGyroX = 0, autoCalSumGyroY = 0, autoCalSumGyroZ = 0;
int autoCalSamples = 0;

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

// Toast - minimum 2.5 seconds for readability
String toastMsg = "";
unsigned long toastEndTime = 0;
void showToast(String msg, int durationMs = 2500) {
  toastMsg = msg;
  toastEndTime = millis() + max(2000, durationMs); // At least 2 seconds
}

// =============================================================================
// UI POLISH: Color Palette & Helpers
// =============================================================================
// Enhanced color palette
#define UI_BG 0x0000      // Black background
#define UI_CARD 0x1082    // Dark gray for cards
#define UI_ACCENT 0x04FF  // Cyan accent
#define UI_SUCCESS 0x07E0 // Green
#define UI_WARNING 0xFD20 // Orange
#define UI_DANGER 0xF800  // Red
#define UI_TEXT 0xFFFF    // White
#define UI_MUTED 0x7BEF   // Gray text
#define UI_HEADER 0x1863  // Dark blue header
#define STATUS_BAR_H 18   // Pixel height of status bar — use as Y offset for app content

// Abnormality tracking
bool gaitAbnormal = false;
String abnormalReason = "";
unsigned long lastAbnormalTime = 0;
const unsigned long ABNORMAL_ALERT_COOLDOWN = 3000;

// Real-time alert system
bool alertBeepEnabled = true;
bool alertLedEnabled = true;
bool alertRangeEnabled = true;

float alertCadenceMin = 70.0f;
float alertCadenceMax = 150.0f;
float alertStabilityMin = 50.0f;
float alertClearanceMin = 0.02f;

const float CADENCE_OK_MIN = 90.0f;
const float CADENCE_OK_MAX = 130.0f;
const float STABILITY_OK_MIN = 80.0f;
const float CLEARANCE_OK_MIN = 0.02f;
const float CLEARANCE_WARN_MIN = 0.01f;

int alertZone = 0;
String alertZoneStr = "ok";

bool rangeAlertActive = false;
String rangeAlertReason = "";
unsigned long lastRangeAlertTime = 0;
const unsigned long RANGE_ALERT_COOLDOWN_OK = 3000;
const unsigned long RANGE_ALERT_COOLDOWN_WARN = 3000;
const unsigned long RANGE_ALERT_COOLDOWN_CRIT = 2000;

bool ledAlertState = false;
unsigned long lastLedToggle = 0;

// Draw status bar at top of screen (call at start of each onDraw)
void drawStatusBar(M5Canvas &c) {
  // Background gradient effect (dark blue)
  c.fillRect(0, 0, 240, 18, UI_HEADER);
  c.drawLine(0, 18, 240, 18, UI_CARD);

  c.setTextSize(1);

  // === LEFT: Battery with charging indicator ===
  int batX = 5;
  bool isCharging = M5.Power.isCharging();

  // Battery outline
  c.drawRect(batX, 4, 18, 10, UI_MUTED);
  c.fillRect(batX + 18, 6, 2, 6, UI_MUTED);

  // Battery fill (color based on level)
  uint16_t batColor = batteryPercent > 50
                          ? UI_SUCCESS
                          : (batteryPercent > 20 ? UI_WARNING : UI_DANGER);
  int fillWidth = map(batteryPercent, 0, 100, 0, 16);
  c.fillRect(batX + 1, 5, fillWidth, 8, batColor);

  // Charging bolt icon overlay
  if (isCharging) {
    c.setTextColor(UI_BG);
    c.drawCenterString("+", batX + 9, 4, 1);
  }

  // Percentage text
  c.setTextColor(UI_TEXT);
  c.setCursor(batX + 24, 5);
  c.printf("%d%%", batteryPercent);

  // === CENTER: Calibration/Recording status ===
  if (isRecording) {
    // Pulsing REC indicator
    int pulse = (millis() / 300) % 2;
    c.fillCircle(105, 9, pulse ? 5 : 4, UI_DANGER);
    c.setTextColor(UI_DANGER);
    c.setCursor(113, 5);
    c.print("REC");
  } else if (isCalibrated) {
    c.setTextColor(UI_SUCCESS);
    c.drawCenterString("Ready", 120, 5, 1);
  } else {
    c.setTextColor(UI_WARNING);
    // Flash between "Setup" and a hint so users know what to do
    c.drawCenterString((millis() / 1500) % 2 ? "Stand still..." : "Setup", 120, 5, 1);
  }

  // === RIGHT: Alert dot + WiFi ===
  // Alert indicator (replaces hardware LED)
  if (ledAlertState && alertLedEnabled) {
    uint16_t dotColor = (alertZone == 2 || gaitAbnormal) ? UI_DANGER
                       : (alertZone == 1) ? UI_WARNING : UI_SUCCESS;
    c.fillCircle(200, 9, 4, dotColor);
  }

  int clientCount = WiFi.softAPgetStationNum();
  if (clientCount > 0) {
    c.setTextColor(UI_SUCCESS);
    c.drawRightString(String(clientCount) + "c", 235, 5, 1);
  } else {
    c.setTextColor(UI_MUTED);
    c.drawRightString("WiFi", 235, 5, 1);
  }
}

// Sound effects helper
void playSound(const char *type) {
  if (strcmp(type, "click") == 0) {
    M5.Speaker.tone(1500, 30);
  } else if (strcmp(type, "select") == 0) {
    M5.Speaker.tone(2000, 80);
  } else if (strcmp(type, "success") == 0) {
    M5.Speaker.tone(2200, 100);
    delay(50);
    M5.Speaker.tone(2600, 100);
  } else if (strcmp(type, "error") == 0) {
    M5.Speaker.tone(400, 200);
  } else if (strcmp(type, "recstart") == 0) {
    M5.Speaker.tone(1800, 100);
    delay(80);
    M5.Speaker.tone(2200, 100);
  } else if (strcmp(type, "recstop") == 0) {
    M5.Speaker.tone(2200, 100);
    delay(80);
    M5.Speaker.tone(1800, 150);
  }
}

// Draw splash screen with welcome message
void showSplashScreen() {
  canvas.fillScreen(UI_BG);

  // Title box with accent border
  int boxW = 180, boxH = 70;
  int boxX = (240 - boxW) / 2, boxY = 20;

  // Outer glow effect
  canvas.drawRect(boxX - 2, boxY - 2, boxW + 4, boxH + 4, UI_ACCENT);
  canvas.fillRect(boxX, boxY, boxW, boxH, UI_CARD);

  // Title
  canvas.setTextColor(UI_ACCENT);
  canvas.setTextSize(2);
  canvas.drawCenterString("GAIT", 120, boxY + 10, 1);
  canvas.setTextColor(UI_TEXT);
  canvas.drawCenterString("OS", 120, boxY + 30, 1);

  // Version
  canvas.setTextSize(1);
  canvas.setTextColor(UI_MUTED);
  canvas.drawCenterString("V2.0 - Gait Analysis", 120, boxY + 52, 1);

  // Progress bar background
  int barX = 40, barY = 105, barW = 160, barH = 8;
  canvas.drawRect(barX - 1, barY - 1, barW + 2, barH + 2, UI_MUTED);

  canvas.pushSprite(0, 0);

  // Animate progress bar
  for (int i = 0; i <= barW; i += 4) {
    canvas.fillRect(barX, barY, i, barH, UI_ACCENT);
    canvas.pushSprite(0, 0);
    delay(12);
  }

  // Welcome message
  canvas.setTextSize(1);
  canvas.setTextColor(UI_SUCCESS);
  canvas.drawCenterString("Welcome!", 120, 120, 1);
  canvas.pushSprite(0, 0);
  playSound("success");
  delay(500);
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
    float gyroAvg  = avg(gyroWindow, 20);
    float gyroVar  = calculateVariance(gyroWindow, 20);

    // 4. Multi-criteria detection
    bool lowAccelVar = (accelVar < currentThreshold);
    bool lowGyro = (gyroAvg < ZUPT_THRESH_DPS) && (gyroVar < ZUPT_THRESH_DPS);
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
// GAIT ANOMALY DETECTOR (PHASE 3)
// =============================================================================
class GaitAnomalyDetector {
private:
  static const int HISTORY_SIZE = 10; // Last 10 steps
  float stepTimeHistory[HISTORY_SIZE] = {0};
  float clearanceHistory[HISTORY_SIZE] = {0};
  float strideHistory[HISTORY_SIZE] = {0};
  int historyIdx = 0;
  int sampleCount = 0;

  // Thresholds (tunable based on patient)
  const float STEP_TIME_CV_THRESH = 0.15; // 15% coefficient of variation
  const float CLEARANCE_CV_THRESH = 0.30; // 30% variation in clearance
  const float STRIDE_CV_THRESH = 0.20;    // 20% stride length variation
  const float MIN_CADENCE = 70.0f;        // Below = shuffling
  const float MAX_CADENCE = 150.0f;       // Above = rushing

public:
  void init() {
    historyIdx = 0;
    sampleCount = 0;
  }

  void addStep(float stepTime, float clearance, float strideLength) {
    stepTimeHistory[historyIdx] = stepTime;
    clearanceHistory[historyIdx] = clearance;
    strideHistory[historyIdx] = strideLength;

    historyIdx = (historyIdx + 1) % HISTORY_SIZE;
    if (sampleCount < HISTORY_SIZE)
      sampleCount++;
  }

  bool detectAbnormality(float currentCadence, String &reason) {
    if (sampleCount < 5)
      return false; // Need at least 5 steps

    // 1. Step Time Variability Check
    float stepTimeCV = calculateCV(stepTimeHistory, sampleCount);
    if (stepTimeCV > STEP_TIME_CV_THRESH) {
      reason = "Irregular rhythm";
      return true;
    }

    // 2. Clearance Consistency Check
    float clearanceCV = calculateCV(clearanceHistory, sampleCount);
    if (clearanceCV > CLEARANCE_CV_THRESH) {
      reason = "Inconsistent lift";
      return true;
    }

    // 3. Stride Length Variation Check
    float strideCV = calculateCV(strideHistory, sampleCount);
    if (strideCV > STRIDE_CV_THRESH) {
      reason = "Uneven stride";
      return true;
    }

    // 4. Cadence Bounds Check
    if (currentCadence > 0 && currentCadence < MIN_CADENCE) {
      reason = "Shuffling gait";
      return true;
    }
    if (currentCadence > MAX_CADENCE) {
      reason = "Rushing gait";
      return true;
    }

    return false; // Normal gait
  }

private:
  float calculateCV(float *data, int count) {
    // Coefficient of Variation = (stddev / mean)
    float sum = 0, mean = 0, variance = 0;

    for (int i = 0; i < count; i++) {
      sum += data[i];
    }
    mean = sum / count;

    if (mean < 0.001f)
      return 0; // Avoid divide by zero

    for (int i = 0; i < count; i++) {
      float diff = data[i] - mean;
      variance += diff * diff;
    }
    variance /= count;

    float stddev = sqrt(variance);
    return stddev / mean;
  }
};

GaitAnomalyDetector anomalyDetector;

/*
// Abnormality tracking
bool gaitAbnormal = false;
String abnormalReason = "";
unsigned long lastAbnormalTime = 0;
const unsigned long ABNORMAL_ALERT_COOLDOWN = 3000; // 3 seconds

// =============================================================================
// REAL-TIME ALERT SYSTEM (THREE-ZONE: OK / WARNING / CRITICAL)
// =============================================================================
// Toggle switches (controllable from web UI)
bool alertBeepEnabled = true;
bool alertLedEnabled = true;
bool alertRangeEnabled = true; // continuous range monitoring

// Configurable safe ranges
float alertCadenceMin = 70.0f;
float alertCadenceMax = 150.0f;
float alertStabilityMin = 50.0f;
float alertClearanceMin = 0.02f; // 2cm minimum foot lift

// Three-zone boundaries (Warning zone sits between OK and Critical)
const float CADENCE_OK_MIN = 90.0f;
const float CADENCE_OK_MAX = 130.0f;
const float STABILITY_OK_MIN = 80.0f;
const float CLEARANCE_OK_MIN = 0.02f; // 2cm
const float CLEARANCE_WARN_MIN = 0.01f; // 1cm

// Zone state: 0=OK, 1=Warning, 2=Critical
int alertZone = 0;
String alertZoneStr = "ok";

// Range alert state
bool rangeAlertActive = false;
String rangeAlertReason = "";
unsigned long lastRangeAlertTime = 0;
const unsigned long RANGE_ALERT_COOLDOWN_OK = 3000;   // 3s when returning to OK
const unsigned long RANGE_ALERT_COOLDOWN_WARN = 3000;  // 3s for warning beeps
const unsigned long RANGE_ALERT_COOLDOWN_CRIT = 2000;  // 2s for critical beeps

// LED state for non-blocking blink
bool ledAlertState = false;
unsigned long lastLedToggle = 0;
*/
// --- Non-blocking alert beeps (single tone per alert — no delay()) ---
// IMPORTANT: delay() blocks server.handleClient(), causing web UI timeouts.
// Each alert type uses a distinct single tone (frequency + duration) for audible differentiation.
void playAlertBeep(const char *alertType) {
  if (!alertBeepEnabled) return;

  if (strcmp(alertType, "cad_low") == 0) {
    M5.Speaker.tone(2000, 150);       // Low cadence: mid-high 2kHz, 150ms
  } else if (strcmp(alertType, "cad_high") == 0) {
    M5.Speaker.tone(2500, 100);       // High cadence: higher 2.5kHz, short 100ms
  } else if (strcmp(alertType, "stability") == 0) {
    M5.Speaker.tone(800, 200);        // Stability: low 800Hz, longer 200ms
  } else if (strcmp(alertType, "clearance") == 0) {
    M5.Speaker.tone(1800, 120);       // Clearance: sharp 1.8kHz, 120ms
  } else if (strcmp(alertType, "back_ok") == 0) {
    M5.Speaker.tone(2400, 100);       // Back to OK: bright high chirp
  } else if (strcmp(alertType, "warn") == 0) {
    M5.Speaker.tone(1500, 100);       // Generic warning: 1.5kHz, 100ms
  }
}

// Determine the current alert zone and trigger appropriate feedback
void checkRangeAlerts() {
  if (!alertRangeEnabled || !isCalibrated || stepCount < 3)
    return;

  unsigned long cooldown = (alertZone == 2) ? RANGE_ALERT_COOLDOWN_CRIT
                         : (alertZone == 1) ? RANGE_ALERT_COOLDOWN_WARN
                         : RANGE_ALERT_COOLDOWN_OK;
  if (millis() - lastRangeAlertTime < cooldown)
    return;

  // --- Determine zone per metric ---
  int newZone = 0; // Start at OK
  String reason = "";

  // Cadence check (only if walking)
  if (currentCadence > 0) {
    if (currentCadence < alertCadenceMin || currentCadence > alertCadenceMax) {
      // Critical
      newZone = 2;
      reason = currentCadence < alertCadenceMin ? "Cadence low" : "Cadence high";
    } else if (currentCadence < CADENCE_OK_MIN || currentCadence > CADENCE_OK_MAX) {
      // Warning (between user limit and OK zone)
      if (newZone < 1) {
        newZone = 1;
        reason = currentCadence < CADENCE_OK_MIN ? "Cadence borderline" : "Cadence fast";
      }
    }
  }

  // Stability check
  if (stabilityIndex > 0 && stabilityIndex < alertStabilityMin) {
    newZone = 2;
    reason = "Unstable gait";
  } else if (stabilityIndex > 0 && stabilityIndex < STABILITY_OK_MIN && newZone < 1) {
    newZone = 1;
    reason = "Stability borderline";
  }

  // Clearance check (only during swing)
  if (!isStance && pos.z > 0) {
    if (pos.z < CLEARANCE_WARN_MIN) {
      newZone = 2;
      reason = "Low clearance";
    } else if (pos.z < CLEARANCE_OK_MIN && newZone < 1) {
      newZone = 1;
      reason = "Clearance borderline";
    }
  }

  // --- Update zone state ---
  int prevZone = alertZone;
  alertZone = newZone;
  alertZoneStr = (newZone == 2) ? "critical" : (newZone == 1) ? "warning" : "ok";

  if (newZone >= 1) {
    rangeAlertActive = true;
    rangeAlertReason = reason;
    lastRangeAlertTime = millis();

    // Play distinct beep based on reason
    if (newZone == 2) {
      // Critical: distinct sound per metric
      if (reason == "Cadence low") playAlertBeep("cad_low");
      else if (reason == "Cadence high") playAlertBeep("cad_high");
      else if (reason == "Unstable gait") playAlertBeep("stability");
      else if (reason == "Low clearance") playAlertBeep("clearance");
      else playAlertBeep("warn");
    } else {
      // Warning: single short beep
      playAlertBeep("warn");
    }
    showToast(reason, 2000);
  } else if (newZone == 0 && prevZone >= 1) {
    // Transition back to OK
    rangeAlertActive = false;
    rangeAlertReason = "";
    playAlertBeep("back_ok");
  }
}

// Visual alert indicator — uses display status bar instead of GPIO LED
// (GPIO 10 is an SPI flash pin on the ESP32-PICO-V3-02 in the Plus 2)
void updateAlertLed() {
  // Alert state is tracked via alertZone / ledAlertState for the status bar
  // and web UI to read. No hardware LED on the Plus 2.
  if (!alertLedEnabled) {
    ledAlertState = false;
    return;
  }

  if (alertZone == 2 || gaitAbnormal) {
    if (millis() - lastLedToggle > 125) {
      ledAlertState = !ledAlertState;
      lastLedToggle = millis();
    }
  } else if (alertZone == 1) {
    if (millis() - lastLedToggle > 250) {
      ledAlertState = !ledAlertState;
      lastLedToggle = millis();
    }
  } else if (isCalibrated && stepCount > 0) {
    ledAlertState = true; // Steady on = OK
  } else {
    ledAlertState = false;
  }
}

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
      // Just became still — reset accumulators
      stillStartTime = millis();
      isStill = true;
      autoCalSumGyroX = autoCalSumGyroY = autoCalSumGyroZ = 0;
      autoCalSamples = 0;
    } else {
      // Accumulate every sample while still
      autoCalSumGyroX += gyroX;
      autoCalSumGyroY += gyroY;
      autoCalSumGyroZ += gyroZ;
      autoCalSamples++;

      if ((millis() - stillStartTime > 3000) && !isCalibrated) {
        // Still for 3 seconds -> auto-calibrate using averaged samples
        gyroBiasX = autoCalSumGyroX / autoCalSamples;
        gyroBiasY = autoCalSumGyroY / autoCalSamples;
        gyroBiasZ = autoCalSumGyroZ / autoCalSamples;

        // Reset navigation state
        pos = {0, 0, 0};
        vel = {0, 0, 0};
        stepCount = 0;
        distanceTotal = 0;
        lastStanceX = 0;
        lastStanceY = 0;
        madgwick.reset();

        isCalibrated = true;
        showToast("Auto-Cal!", 2000);
      }
    }
  } else {
    isStill = false;
    stillStartTime = 0;
  }
}

// =============================================================================
// ABNORMALITY ALERT (PHASE 3)
// =============================================================================
void triggerAbnormalAlert() {
  // LED feedback is now handled by updateAlertLed() in the main loop
  // (zone-based blink rates: 4Hz critical blink when gaitAbnormal=true)

  // Beep using speaker — use distinct sound based on the detected anomaly
  if (alertBeepEnabled) {
    if (abnormalReason == "Irregular rhythm" || abnormalReason == "Shuffling gait" || abnormalReason == "Rushing gait") {
      playAlertBeep("cad_low");
    } else if (abnormalReason == "Inconsistent lift") {
      playAlertBeep("clearance");
    } else if (abnormalReason == "Uneven stride") {
      playAlertBeep("stability");
    } else {
      M5.Speaker.tone(2000, 200);  // Single non-blocking tone
    }
  }

  // Show toast on device screen
  showToast("! " + abnormalReason, 2000);
}

// =============================================================================
// BATTERY MANAGEMENT
// =============================================================================
void updateBattery() {
  if (millis() - lastBatteryCheck < 5000)
    return; // Check every 5s

  // Use M5 Power library for accurate voltage (same source as BatteryApp)
  batteryVoltage = M5.Power.getBatteryVoltage() / 1000.0f;

  // Convert to percentage (LiPo: 4.2V=100%, 3.0V=0%)
  batteryPercent = constrain(((batteryVoltage - 3.0f) / 1.2f) * 100, 0, 100);

  lastBatteryCheck = millis();

  // Low battery warning — trigger once per discharge cycle
  static bool lowBatteryWarned = false;
  if (batteryPercent < 15 && !lowBatteryWarned) {
    showToast("Low Battery!", 3000);
    lowBatteryWarned = true;
  }
  if (batteryPercent >= 20) {
    lowBatteryWarned = false; // Reset when charged back up
  }
}

// =============================================================================
// DATA VALIDATION
// =============================================================================
void validateData() {
  // Only validate if device is calibrated and has actually moved
  if (!isCalibrated || stepCount == 0)
    return;

  // Catch obviously wrong values (only during movement)
  if (abs(vel.x) > 8.0f) { // 8 m/s = 28.8 km/h (increased threshold)
    vel.x = 0;
    showToast("Vel overflow!", 1000);
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
  // Use navigation-frame acceleration magnitude (gravity already removed) —
  // orientation-independent, unlike raw body-frame accZ
  static float maxSwing = 0;
  float accel_nav_mag = sqrt(accel_nav[0] * accel_nav[0] +
                             accel_nav[1] * accel_nav[1] +
                             accel_nav[2] * accel_nav[2]);
  if (!isStance && accel_nav_mag > maxSwing)
    maxSwing = accel_nav_mag;

  if (isStance && maxSwing > MIN_SWING_ACCEL &&
      (millis() - lastStepTime > MIN_STEP_TIME_MS)) {

    // Step detected!
    unsigned long dur = millis() - lastStepTime;
    lastStepTime = millis();
    stepCount++;

    // Stride length from integration — 2D (X+Y) for correct diagonal walking
    float dx = pos.x - lastStanceX;
    float dy = pos.y - lastStanceY;
    float strideLength = sqrt(dx * dx + dy * dy);
    if (strideLength > 0.2f && strideLength < 2.0f) { // Sanity check
      distanceTotal += strideLength;
      recentStrides[strideIdx] = strideLength;
      strideIdx = (strideIdx + 1) % 10;
    }
    lastStanceX = pos.x;
    lastStanceY = pos.y;

    // Cadence calculation
    float instCadence = 60000.0f / dur;
    float prevCadence = currentCadence; // save before EMA update
    currentCadence = (currentCadence * 0.8f) + (instCadence * 0.2f);

    // Stability Index — compare against pre-update cadence so deviation
    // isn't artificially shrunk by the EMA already incorporating instCadence
    float deviation =
        abs(instCadence - prevCadence) / (prevCadence + 1.0f);
    float instStability = constrain(100.0f * (1.0f - deviation), 0.0f, 100.0f);
    stabilityIndex = (stabilityIndex * 0.9f) + (instStability * 0.1f);

    // === PHASE 3: GAIT ABNORMALITY DETECTION ===
    float stepTime = dur / 1000.0f; // Convert to seconds
    float clearance = maxSwing;     // Peak height this step

    // Add step to anomaly detector
    anomalyDetector.addStep(stepTime, clearance, strideLength);

    // Check for abnormalities
    String reason = "";
    bool abnormalDetected =
        anomalyDetector.detectAbnormality(currentCadence, reason);

    if (abnormalDetected &&
        (millis() - lastAbnormalTime > ABNORMAL_ALERT_COOLDOWN)) {
      gaitAbnormal = true;
      abnormalReason = reason;
      lastAbnormalTime = millis();

      // Trigger device alert (LED + beep + toast)
      triggerAbnormalAlert();
    } else if (!abnormalDetected &&
               (millis() - lastAbnormalTime > ABNORMAL_ALERT_COOLDOWN)) {
      // Only clear flag once the alert cooldown has fully elapsed,
      // so the alert doesn't vanish after a single normal step
      gaitAbnormal = false;
      abnormalReason = "";
    }

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
  char buf[896];
  snprintf(buf, sizeof(buf),
    "{\"recording\":%s,\"step_count\":%d,\"dist_m\":%.2f,"
    "\"cad\":%.1f,\"stab\":%.1f,"
    "\"px\":%.3f,\"py\":%.3f,\"pz\":%.3f,"
    "\"phase\":%d,\"pitch\":%.1f,\"roll\":%.1f,\"yaw\":%.1f,"
    "\"is_stat\":%s,\"battery_pct\":%d,\"battery_v\":%.2f,"
    "\"calibrated\":%s,\"abnormal\":%s,"
    "\"abnormal_reason\":\"%s\","
    "\"range_alert\":%s,\"range_reason\":\"%s\","
    "\"zone\":\"%s\","
    "\"beep_on\":%s,\"led_on\":%s,\"range_on\":%s}",
    isRecording ? "true" : "false", (int)stepCount, distanceTotal,
    currentCadence, stabilityIndex,
    pos.x, pos.y, pos.z,
    isStance ? 0 : 1, pitch, roll, yaw,
    isStance ? "true" : "false", batteryPercent, batteryVoltage,
    isCalibrated ? "true" : "false", gaitAbnormal ? "true" : "false",
    abnormalReason.c_str(),
    rangeAlertActive ? "true" : "false", rangeAlertReason.c_str(),
    alertZoneStr.c_str(),
    alertBeepEnabled ? "true" : "false",
    alertLedEnabled ? "true" : "false",
    alertRangeEnabled ? "true" : "false");
  server.send(200, "application/json", buf);
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

// Battery icon helper
void drawBatteryIcon(M5Canvas &c, int x, int y, int percent) {
  c.drawRect(x, y, 20, 10, WHITE);
  c.drawRect(x + 20, y + 3, 2, 4, WHITE); // Terminal

  int fillWidth = (percent * 18) / 100;
  uint16_t color = (percent > 50) ? GREEN : (percent > 20) ? YELLOW : RED;
  if (fillWidth > 0) {
    c.fillRect(x + 1, y + 1, fillWidth, 8, color);
  }
}

// Forward declarations for App classes
class LauncherApp;
extern LauncherApp launcher;
extern App *currentApp;

// 1. LAUNCHER (PHASE 3.5: Redesigned - Now 6 apps)
class LauncherApp : public App {
  int sel = 0;

public:
  void onDraw(M5Canvas &c) override {
    c.fillScreen(UI_BG);

    // Status bar at top
    drawStatusBar(c);

    // App grid (6 apps in 3x2 layout) - starting below status bar
    const char *names[6] = {"Lab", "Scope", "Net", "Files", "Batt", "Cal"};
    const char *desc[6] = {"Gait", "View", "WiFi", "Data", "Power", "Setup"};

    for (int i = 0; i < 6; i++) {
      int col = i % 3;
      int row = i / 3;
      int x = 40 + col * 80;
      int y = 38 + row * 45; // Adjusted for status bar

      bool selected = (sel == i);

      // Card background with enhanced colors
      if (selected) {
        c.fillRoundRect(x - 28, y - 3, 58, 40, 5, UI_CARD);
        c.drawRoundRect(x - 28, y - 3, 58, 40, 5, UI_ACCENT);
      } else {
        c.fillRoundRect(x - 28, y - 3, 58, 40, 5, 0x1082);
      }

      // App name
      c.setTextColor(selected ? UI_ACCENT : UI_TEXT);
      c.setTextSize(1);
      c.drawCenterString(names[i], x, y + 5, 2);

      // Description
      c.setTextColor(UI_MUTED);
      c.setTextSize(1);
      c.drawCenterString(desc[i], x, y + 24, 1);
    }

    // Footer hint
    c.setTextColor(UI_MUTED);
    c.setTextSize(1);
    c.drawCenterString("A: Open  |  B: Next", 120, 125, 1);
  }

  void onBtnB() override {
    sel = (sel + 1) % 6;
    playSound("click");
  }

  void onBtnA() override; // Defined below classes
};

// 1.5 POWER MENU (PHASE 3.5: UI Redesigned)
class PowerMenuApp : public App {
private:
  int sel = 0;
  struct MenuItem {
    const char *label;
    const char *icon;
    uint16_t color;
  };
  MenuItem menuItems[6] = {
      {"Battery Info", "[=]", UI_SUCCESS}, {"Sleep Mode", "zzZ", UI_ACCENT},
      {"Power Off", "(X)", UI_DANGER},     {"Restart", "(!)", UI_WARNING},
      {"Settings", "[*]", UI_MUTED},       {"Back", "<-", UI_TEXT}};

public:
  void onDraw(M5Canvas &c) override {
    c.fillScreen(UI_BG);

    // Header
    c.fillRect(0, 0, 240, 22, UI_HEADER);
    c.drawLine(0, 22, 240, 22, UI_CARD);
    c.setTextColor(UI_ACCENT);
    c.setTextSize(1);
    c.drawCenterString("Power Menu", 120, 6, 2);

    // Menu items
    for (int i = 0; i < 6; i++) {
      int y = 28 + i * 16;
      bool selected = (i == sel);

      if (selected) {
        c.fillRoundRect(15, y - 2, 210, 15, 3, UI_CARD);
        c.drawRoundRect(15, y - 2, 210, 15, 3, menuItems[i].color);
      }

      // Icon
      c.setTextColor(menuItems[i].color);
      c.drawString(menuItems[i].icon, 22, y, 1);

      // Label
      c.setTextColor(selected ? UI_TEXT : UI_MUTED);
      c.drawString(menuItems[i].label, 50, y, 1);

      // Arrow for selected
      if (selected) {
        c.setTextColor(menuItems[i].color);
        c.drawString(">", 210, y, 1);
      }
    }

    // Footer hint
    c.setTextColor(UI_MUTED);
    c.setTextSize(1);
    c.drawCenterString("A: Select  |  B: Next", 120, 125, 1);
  }

  void onBtnB() override {
    sel = (sel + 1) % 6;
    playSound("click");
  }

  void onBtnA() override {
    playSound("select");
    switch (sel) {
    case 0: // Battery Info
      showToast(String(batteryVoltage, 2) + "V / " + String(batteryPercent) +
                "%");
      break;
    case 1: // Sleep Mode — deep sleep, wake on power button (GPIO37)
      showToast("Sleeping...");
      delay(1500);
      prepareForSleep();
      break;
    case 2: // Power Off
      showToast("Shutting down...");
      delay(1500);
      M5.Power.powerOff();
      break;
    case 3: // Restart
      showToast("Restarting...");
      delay(1000);
      ESP.restart();
      break;
    case 4: // Settings
      showToast("Coming soon");
      break;
    case 5: // Back
      currentApp = &launcher;
      currentApp->onOpen();
      break;
    }
  }
};

PowerMenuApp powerMenu;

// 2. GAIT LAB
class GaitLabApp : public App {
public:
  void onDraw(M5Canvas &c) override {
    c.fillScreen(UI_BG);

    // Status bar at top
    drawStatusBar(c);

    // Main content starts below status bar
    int y = STATUS_BAR_H + 4;

    // Large step counter
    c.setTextColor(UI_TEXT);
    c.setTextSize(3);
    c.setCursor(10, y);
    c.printf("%.0f", stepCount);
    c.setTextSize(1);
    c.setTextColor(UI_MUTED);
    c.setCursor(90, y + 10);
    c.print("steps");

    y += 35;

    // Metrics cards
    c.setTextSize(1);

    // Cadence
    c.setTextColor(UI_ACCENT);
    c.setCursor(10, y);
    c.print("Cadence");
    c.setTextColor(UI_TEXT);
    c.setCursor(120, y);
    c.printf("%.0f spm", currentCadence);

    y += 15;

    // Distance
    c.setTextColor(UI_ACCENT);
    c.setCursor(10, y);
    c.print("Distance");
    c.setTextColor(UI_TEXT);
    c.setCursor(120, y);
    c.printf("%.1f m", distanceTotal);

    y += 15;

    // Stability with color-coded bar
    c.setTextColor(UI_ACCENT);
    c.setCursor(10, y);
    c.print("Stability");
    // Draw stability bar
    int barX = 100, barW = 80, barH = 8;
    c.drawRect(barX, y, barW + 2, barH + 2, UI_MUTED);
    int fillW = (int)(barW * stabilityIndex / 100.0);
    uint16_t stabColor = stabilityIndex > 70
                             ? UI_SUCCESS
                             : (stabilityIndex > 40 ? UI_WARNING : UI_DANGER);
    c.fillRect(barX + 1, y + 1, fillW, barH, stabColor);
    c.setTextColor(UI_TEXT);
    c.setCursor(190, y);
    c.printf("%.0f%%", stabilityIndex);

    y += 15;

    // Clearance height
    c.setTextColor(UI_ACCENT);
    c.setCursor(10, y);
    c.print("Height");
    c.setTextColor(UI_TEXT);
    c.setCursor(120, y);
    c.printf("%.2f m", pos.z);

    // Footer hint
    c.setTextColor(UI_MUTED);
    c.drawCenterString("A: Record  |  B: Back", 120, 125, 1);
  }

  void onBtnA() override {
    isRecording = !isRecording;
    if (isRecording) {
      playSound("recstart");
      // Enhanced CSV header with metadata
      logFile = LittleFS.open("/log_" + String(millis()) + ".csv", FILE_WRITE);
      if (!logFile) {
        isRecording = false;
        playSound("error");
        showToast("File open failed!", 2000);
        return;
      }
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
      showToast("Recording...");
    } else {
      playSound("recstop");
      if (logFile)
        logFile.close();
      showToast("Saved!");
    }
  }

  void onBtnB() override {
    currentApp = &launcher;
    playSound("click");
  }
};

// 3. SCOPE (Enhanced with multiple modes)
class ScopeApp : public App {
private:
  int mode = 0; // 0=Trajectory, 1=Angles, 2=Accel, 3=Steps
  const char *modeNames[4] = {"Trajectory", "Angles", "Accel", "Steps"};

  // History buffers for visualization
  float angleHistory[60][3]; // roll, pitch, yaw
  float accelHistory[60][3]; // x, y, z
  int historyIdx = 0;

public:
  void onActivate() {
    // Clear history
    memset(angleHistory, 0, sizeof(angleHistory));
    memset(accelHistory, 0, sizeof(accelHistory));
    historyIdx = 0;
  }

  void updateHistory() {
    // Store current values — only called once per sensor update, not once per draw frame
    angleHistory[historyIdx][0] = roll;
    angleHistory[historyIdx][1] = pitch;
    angleHistory[historyIdx][2] = yaw;
    accelHistory[historyIdx][0] = accX;
    accelHistory[historyIdx][1] = accY;
    accelHistory[historyIdx][2] = accZ;
    historyIdx = (historyIdx + 1) % 60;
  }

  void onDraw(M5Canvas &c) override {
    c.fillScreen(UI_BG);
    // updateHistory() is called by the sensor loop (via scopeNeedsUpdate flag),
    // not here — so the scope doesn't oversample or undersample based on draw rate

    // Header with mode tabs
    c.fillRect(0, 0, 240, 18, UI_HEADER);
    c.drawLine(0, 18, 240, 18, UI_CARD);
    c.setTextSize(1);

    // Mode indicator
    c.setTextColor(UI_MUTED);
    c.drawString("< Back", 5, 5, 1);
    c.setTextColor(UI_ACCENT);
    c.drawCenterString(modeNames[mode], 120, 5, 1);
    c.setTextColor(UI_MUTED);
    c.drawRightString("A: Mode", 235, 5, 1);

    int plotX = 10, plotY = 22, plotW = 220, plotH = 85;

    if (mode == 0) {
      // TRAJECTORY MODE
      if (trajCount < 2) {
        c.setTextColor(UI_MUTED);
        c.drawCenterString("No trajectory data", 120, 55, 1);
        c.drawCenterString("Walk to record path", 120, 70, 1);
      } else {
        // Auto-scale
        float minX = 9999, maxX = -9999, minZ = 9999, maxZ = -9999;
        for (int i = 0; i < trajCount; i++) {
          int idx =
              (trajHead - trajCount + i + TRAJECTORY_LEN) % TRAJECTORY_LEN;
          float x = trajectory[idx].x / 100.0f;
          float z = trajectory[idx].y / 100.0f;
          if (x < minX)
            minX = x;
          if (x > maxX)
            maxX = x;
          if (z < minZ)
            minZ = z;
          if (z > maxZ)
            maxZ = z;
        }
        float rangeX = max(0.1f, maxX - minX);
        float rangeZ = max(0.1f, maxZ - minZ);
        minX -= rangeX * 0.1f;
        maxX += rangeX * 0.1f;
        minZ -= rangeZ * 0.1f;
        maxZ += rangeZ * 0.1f;

        c.drawRect(plotX, plotY, plotW, plotH, UI_MUTED);

        // Plot trajectory
        for (int i = 0; i < trajCount - 1; i++) {
          int idx =
              (trajHead - trajCount + i + TRAJECTORY_LEN) % TRAJECTORY_LEN;
          float x1 = trajectory[idx].x / 100.0f;
          float z1 = trajectory[idx].y / 100.0f;
          float x2 = trajectory[(idx + 1) % TRAJECTORY_LEN].x / 100.0f;
          float z2 = trajectory[(idx + 1) % TRAJECTORY_LEN].y / 100.0f;

          int sx1 = plotX + (x1 - minX) / (maxX - minX) * plotW;
          int sy1 = plotY + plotH - (z1 - minZ) / (maxZ - minZ) * plotH;
          int sx2 = plotX + (x2 - minX) / (maxX - minX) * plotW;
          int sy2 = plotY + plotH - (z2 - minZ) / (maxZ - minZ) * plotH;

          c.drawLine(sx1, sy1, sx2, sy2, UI_ACCENT);
        }

        c.setTextColor(UI_TEXT);
        c.drawString("Pts: " + String(trajCount), plotX, 112, 1);
        c.drawRightString("Range: " + String(rangeX, 1) + "m", plotX + plotW,
                          112, 1);
      }

    } else if (mode == 1) {
      // ANGLES MODE - Roll, Pitch, Yaw
      c.drawRect(plotX, plotY, plotW, plotH, UI_MUTED);

      // Draw zero line
      int zeroY = plotY + plotH / 2;
      c.drawLine(plotX, zeroY, plotX + plotW, zeroY, UI_CARD);

      // Plot angle history
      for (int i = 1; i < 60; i++) {
        int idx1 = (historyIdx - 60 + i - 1 + 60) % 60;
        int idx2 = (historyIdx - 60 + i + 60) % 60;
        int x1 = plotX + (i - 1) * plotW / 60;
        int x2 = plotX + i * plotW / 60;

        // Roll (red)
        int y1 = zeroY - (int)(angleHistory[idx1][0] * plotH / 180);
        int y2 = zeroY - (int)(angleHistory[idx2][0] * plotH / 180);
        c.drawLine(x1, constrain(y1, plotY, plotY + plotH), x2,
                   constrain(y2, plotY, plotY + plotH), UI_DANGER);

        // Pitch (green)
        y1 = zeroY - (int)(angleHistory[idx1][1] * plotH / 180);
        y2 = zeroY - (int)(angleHistory[idx2][1] * plotH / 180);
        c.drawLine(x1, constrain(y1, plotY, plotY + plotH), x2,
                   constrain(y2, plotY, plotY + plotH), UI_SUCCESS);

        // Yaw (cyan)
        y1 = zeroY - (int)(angleHistory[idx1][2] * plotH / 360);
        y2 = zeroY - (int)(angleHistory[idx2][2] * plotH / 360);
        c.drawLine(x1, constrain(y1, plotY, plotY + plotH), x2,
                   constrain(y2, plotY, plotY + plotH), UI_ACCENT);
      }

      // Legend
      c.setTextColor(UI_DANGER);
      c.drawString("R:" + String(roll, 0), plotX, 112, 1);
      c.setTextColor(UI_SUCCESS);
      c.drawString("P:" + String(pitch, 0), plotX + 60, 112, 1);
      c.setTextColor(UI_ACCENT);
      c.drawString("Y:" + String(yaw, 0), plotX + 120, 112, 1);

    } else if (mode == 2) {
      // ACCELEROMETER MODE
      c.drawRect(plotX, plotY, plotW, plotH, UI_MUTED);

      // Draw 1g line
      int zeroY = plotY + plotH / 2;
      c.drawLine(plotX, zeroY, plotX + plotW, zeroY, UI_CARD);

      // Plot accel history
      for (int i = 1; i < 60; i++) {
        int idx1 = (historyIdx - 60 + i - 1 + 60) % 60;
        int idx2 = (historyIdx - 60 + i + 60) % 60;
        int x1 = plotX + (i - 1) * plotW / 60;
        int x2 = plotX + i * plotW / 60;

        // X (red)
        int y1 = zeroY - (int)(accelHistory[idx1][0] * plotH / 4);
        int y2 = zeroY - (int)(accelHistory[idx2][0] * plotH / 4);
        c.drawLine(x1, constrain(y1, plotY, plotY + plotH), x2,
                   constrain(y2, plotY, plotY + plotH), UI_DANGER);

        // Y (green)
        y1 = zeroY - (int)(accelHistory[idx1][1] * plotH / 4);
        y2 = zeroY - (int)(accelHistory[idx2][1] * plotH / 4);
        c.drawLine(x1, constrain(y1, plotY, plotY + plotH), x2,
                   constrain(y2, plotY, plotY + plotH), UI_SUCCESS);

        // Z (cyan)
        y1 = zeroY - (int)(accelHistory[idx1][2] * plotH / 4);
        y2 = zeroY - (int)(accelHistory[idx2][2] * plotH / 4);
        c.drawLine(x1, constrain(y1, plotY, plotY + plotH), x2,
                   constrain(y2, plotY, plotY + plotH), UI_ACCENT);
      }

      // Legend
      c.setTextColor(UI_DANGER);
      c.drawString("X:" + String(accX, 1) + "g", plotX, 112, 1);
      c.setTextColor(UI_SUCCESS);
      c.drawString("Y:" + String(accY, 1) + "g", plotX + 70, 112, 1);
      c.setTextColor(UI_ACCENT);
      c.drawString("Z:" + String(accZ, 1) + "g", plotX + 140, 112, 1);

    } else if (mode == 3) {
      // STEP COUNTER MODE - Large display
      c.setTextColor(UI_TEXT);
      c.setTextSize(4);
      c.drawCenterString(String((int)stepCount), 120, 35, 1);
      c.setTextSize(1);
      c.setTextColor(UI_MUTED);
      c.drawCenterString("steps", 120, 70, 1);

      // Additional metrics
      c.setTextColor(UI_ACCENT);
      c.drawString("Cadence", 20, 85, 1);
      c.setTextColor(UI_TEXT);
      c.drawString(String(currentCadence, 0) + " spm", 20, 98, 1);

      c.setTextColor(UI_ACCENT);
      c.drawString("Distance", 120, 85, 1);
      c.setTextColor(UI_TEXT);
      c.drawString(String(distanceTotal, 1) + " m", 120, 98, 1);

      // Stability bar
      c.setTextColor(UI_ACCENT);
      c.drawString("Stability", 20, 112, 1);
      int barX = 80, barW = 100;
      c.drawRect(barX, 112, barW, 8, UI_MUTED);
      int fillW = (int)(barW * stabilityIndex / 100);
      uint16_t col = stabilityIndex > 70
                         ? UI_SUCCESS
                         : (stabilityIndex > 40 ? UI_WARNING : UI_DANGER);
      c.fillRect(barX + 1, 113, fillW, 6, col);
      c.setTextColor(UI_TEXT);
      c.drawString(String(stabilityIndex, 0) + "%", 185, 112, 1);
    }

    // Footer
    c.setTextColor(UI_MUTED);
    c.drawCenterString("A: Mode  |  B: Back", 120, 125, 1);
  }

  void onBtnA() override {
    mode = (mode + 1) % 4;
    playSound("click");
  }

  void onBtnB() override {
    currentApp = &launcher;
    playSound("click");
  }
};

// 4. NET - Enhanced WiFi Info
class NetApp : public App {
public:
  void onDraw(M5Canvas &c) override {
    c.fillScreen(UI_BG);

    // Header
    c.fillRect(0, 0, 240, 18, UI_HEADER);
    c.drawLine(0, 18, 240, 18, UI_CARD);
    c.setTextColor(UI_TEXT);
    c.setTextSize(1);
    c.drawString("< Back", 5, 5, 1);
    c.drawCenterString("Network Info", 120, 5, 1);

    int y = 24;

    // SSID
    c.setTextColor(UI_ACCENT);
    c.setCursor(10, y);
    c.print("SSID");
    c.setTextColor(UI_TEXT);
    c.setCursor(80, y);
    c.print(WIFI_SSID);
    y += 14;

    // Password
    c.setTextColor(UI_ACCENT);
    c.setCursor(10, y);
    c.print("Password");
    c.setTextColor(UI_TEXT);
    c.setCursor(80, y);
    c.print(WIFI_PASS);
    y += 14;

    // Divider
    c.drawLine(10, y, 230, y, UI_CARD);
    y += 6;

    // Web Server URL
    c.setTextColor(UI_ACCENT);
    c.setCursor(10, y);
    c.print("Dashboard");
    c.setTextColor(UI_SUCCESS);
    c.setCursor(80, y);
    c.print("http://");
    c.print(WiFi.softAPIP().toString());
    y += 14;

    // Connected Clients
    c.setTextColor(UI_ACCENT);
    c.setCursor(10, y);
    c.print("Clients");
    int clientCount = WiFi.softAPgetStationNum();
    c.setTextColor(clientCount > 0 ? UI_SUCCESS : UI_MUTED);
    c.setCursor(80, y);
    c.printf("%d connected", clientCount);
    y += 14;

    // Divider
    c.drawLine(10, y, 230, y, UI_CARD);
    y += 6;

    // Status
    c.setTextColor(UI_ACCENT);
    c.setCursor(10, y);
    c.print("Status");
    c.setTextColor(UI_SUCCESS);
    c.setCursor(80, y);
    c.print("AP Active");
    y += 14;

    // Recording status
    c.setTextColor(UI_ACCENT);
    c.setCursor(10, y);
    c.print("Recording");
    c.setTextColor(isRecording ? UI_DANGER : UI_MUTED);
    c.setCursor(80, y);
    c.print(isRecording ? "IN PROGRESS" : "Idle");

    // Footer
    c.setTextColor(UI_MUTED);
    c.drawCenterString("B: Back to Menu", 120, 125, 1);
  }

  void onBtnB() override {
    currentApp = &launcher;
    playSound("click");
  }
};

// 5. FILES APP (PHASE 3.5)
class FilesApp : public App {
private:
  int sel = 0;
  String fileList[20];
  int fileCount = 0;

public:
  void onActivate() {
    fileCount = 0;
    File root = LittleFS.open("/");
    if (root) {
      File file = root.openNextFile();
      while (file && fileCount < 20) {
        String name = String(file.name());
        // Ensure path has leading slash for LittleFS.remove()
        if (!name.startsWith("/")) {
          name = "/" + name;
        }
        if (name.endsWith(".csv")) {
          fileList[fileCount++] = name;
        }
        file = root.openNextFile();
      }
      root.close();
    }
    sel = 0;
  }

  void onDraw(M5Canvas &c) override {
    c.fillScreen(UI_BG);

    // Header
    c.fillRect(0, 0, 240, 18, UI_HEADER);
    c.drawLine(0, 18, 240, 18, UI_CARD);
    c.setTextColor(UI_TEXT);
    c.setTextSize(1);
    c.drawString("< Back", 5, 5, 1);
    c.drawCenterString("Files (" + String(fileCount) + ")", 120, 5, 1);

    if (fileCount == 0) {
      c.setTextColor(UI_MUTED);
      c.drawCenterString("No CSV files", 120, 55, 1);
      c.drawCenterString("Record a session first", 120, 70, 1);
    } else {
      int startIdx = max(0, sel - 2);
      for (int i = 0; i < min(5, fileCount); i++) {
        int idx = startIdx + i;
        if (idx >= fileCount)
          break;

        int y = 22 + i * 20;
        if (idx == sel) {
          c.fillRect(5, y, 230, 18, UI_CARD);
          c.setTextColor(UI_ACCENT);
          c.drawString(">", 8, y + 3, 1);
        } else {
          c.setTextColor(UI_TEXT);
        }
        c.setTextSize(1);

        String displayName = fileList[idx];
        if (displayName.length() > 25) {
          displayName = displayName.substring(0, 22) + "...";
        }
        c.drawString(displayName, 20, y + 3, 1);
      }

      // Scroll indicator
      if (fileCount > 5) {
        int indicatorY = 22 + (90 * sel / fileCount);
        c.fillRect(232, 22, 3, 100, UI_CARD);
        c.fillRect(232, indicatorY, 3, 20, UI_ACCENT);
      }
    }

    c.setTextColor(UI_MUTED);
    c.setTextSize(1);
    c.drawCenterString("A: Delete  |  B: Next / Back", 120, 125, 1);
  }

  void onBtnB() override {
    if (fileCount == 0 || sel >= fileCount - 1) {
      // At end of list (or empty) — go back to launcher
      currentApp = &launcher;
      playSound("click");
    } else {
      sel++;
      playSound("click");
    }
  }

  void onBtnA() override {
    if (fileCount > 0) {
      String filename = fileList[sel];
      Serial.println("Device delete: " + filename);

      // Try with leading slash
      bool deleted = LittleFS.remove(filename);

      // If failed, try without leading slash
      if (!deleted && filename.startsWith("/")) {
        String altName = filename.substring(1);
        Serial.println("Trying without slash: " + altName);
        deleted = LittleFS.remove(altName);
      }

      if (deleted) {
        playSound("success");
        showToast("Deleted!", 1500);
      } else {
        playSound("error");
        showToast("Delete failed!", 1500);
      }
      onActivate(); // Refresh file list
    } else {
      currentApp = &launcher;
      playSound("click");
    }
  }
};

// 6. BATTERY APP - Detailed battery info
class BatteryApp : public App {
private:
  unsigned long lastUpdate = 0;
  float voltage = 0;
  bool isCharging = false;

public:
  void updateBatteryInfo() {
    if (millis() - lastUpdate > 500) {
      voltage = M5.Power.getBatteryVoltage() / 1000.0f;
      isCharging = M5.Power.isCharging();
      lastUpdate = millis();
    }
  }

  void onDraw(M5Canvas &c) override {
    c.fillScreen(UI_BG);
    updateBatteryInfo();

    // Header
    c.fillRect(0, 0, 240, 18, UI_HEADER);
    c.drawLine(0, 18, 240, 18, UI_CARD);
    c.setTextColor(UI_TEXT);
    c.setTextSize(1);
    c.drawString("< Back", 5, 5, 1);
    c.drawCenterString("Battery Info", 120, 5, 1);

    int y = 26;

    // Large battery percentage
    c.setTextSize(4);
    c.setTextColor(batteryPercent > 20 ? UI_SUCCESS : UI_DANGER);
    c.drawCenterString(String(batteryPercent) + "%", 120, y, 1);
    y += 40;

    // Charging status
    c.setTextSize(1);
    if (isCharging) {
      c.setTextColor(UI_SUCCESS);
      c.drawCenterString("CHARGING", 120, y, 1);
    } else {
      c.setTextColor(UI_MUTED);
      c.drawCenterString("Discharging", 120, y, 1);
    }
    y += 16;

    // Divider
    c.drawLine(20, y, 220, y, UI_CARD);
    y += 8;

    // Voltage
    c.setTextColor(UI_ACCENT);
    c.drawString("Voltage", 20, y, 1);
    c.setTextColor(UI_TEXT);
    c.drawRightString(String(voltage, 2) + " V", 220, y, 1);
    y += 14;

    // Health estimate
    c.setTextColor(UI_ACCENT);
    c.drawString("Health", 20, y, 1);
    String health = "Good";
    uint16_t healthColor = UI_SUCCESS;
    if (voltage < 3.3 && !isCharging) {
      health = "Low";
      healthColor = UI_DANGER;
    } else if (voltage < 3.6 && !isCharging) {
      health = "Fair";
      healthColor = UI_WARNING;
    }
    c.setTextColor(healthColor);
    c.drawRightString(health, 220, y, 1);
    y += 14;

    // Status
    c.setTextColor(UI_ACCENT);
    c.drawString("Status", 20, y, 1);
    c.setTextColor(UI_TEXT);
    c.drawRightString(isCharging ? "Plugged In" : "On Battery", 220, y, 1);

    // Footer
    c.setTextColor(UI_MUTED);
    c.drawCenterString("B: Back to Menu", 120, 125, 1);
  }

  void onBtnB() override {
    currentApp = &launcher;
    playSound("click");
  }
};

// 7. CALIBRATION APP - Manual sensor calibration
class CalibrationApp : public App {
private:
  int state = 0; // 0=ready, 1=calibrating, 2=done
  int progress = 0;
  unsigned long calStart = 0;
  float calAccX = 0, calAccY = 0, calAccZ = 0;
  float calGyrX = 0, calGyrY = 0, calGyrZ = 0;
  int samples = 0;

public:
  void onActivate() {
    state = 0;
    progress = 0;
  }

  void runCalibration() {
    if (state == 1) {
      unsigned long elapsed = millis() - calStart;
      progress = min(100, (int)(elapsed / 30)); // 3 seconds calibration

      // Accumulate samples
      calAccX += accX;
      calAccY += accY;
      calAccZ += (accZ - 1.0f); // Subtract gravity
      calGyrX += gyroX;
      calGyrY += gyroY;
      calGyrZ += gyroZ;
      samples++;

      if (elapsed >= 3000) {
        // Apply calibration offsets
        if (samples > 0) {
          gyroBiasX = calGyrX / samples;
          gyroBiasY = calGyrY / samples;
          gyroBiasZ = calGyrZ / samples;
          accBiasX = calAccX / samples;
          accBiasY = calAccY / samples;
          accBiasZ = calAccZ / samples;
        }
        isCalibrated = true;
        state = 2;
        playSound("success");
      }
    }
  }

  void onDraw(M5Canvas &c) override {
    c.fillScreen(UI_BG);
    runCalibration();

    // Header
    c.fillRect(0, 0, 240, 18, UI_HEADER);
    c.drawLine(0, 18, 240, 18, UI_CARD);
    c.setTextColor(UI_TEXT);
    c.setTextSize(1);
    c.drawString("< Back", 5, 5, 1);
    c.drawCenterString("Calibration", 120, 5, 1);

    if (state == 0) {
      // Ready state
      c.setTextColor(UI_TEXT);
      c.setTextSize(2);
      c.drawCenterString("Ready", 120, 30, 1);

      c.setTextSize(1);
      c.setTextColor(UI_MUTED);
      c.drawCenterString("Place device flat on", 120, 55, 1);
      c.drawCenterString("stable surface", 120, 68, 1);
      c.drawCenterString("Keep still during calibration", 120, 85, 1);

      c.setTextColor(UI_SUCCESS);
      c.drawCenterString("Press A to start", 120, 105, 1);

    } else if (state == 1) {
      // Calibrating
      c.setTextColor(UI_WARNING);
      c.setTextSize(2);
      c.drawCenterString("Calibrating...", 120, 35, 1);

      // Progress bar
      c.setTextSize(1);
      int barX = 30, barY = 60, barW = 180, barH = 16;
      c.drawRect(barX, barY, barW, barH, UI_MUTED);
      c.fillRect(barX + 2, barY + 2, (barW - 4) * progress / 100, barH - 4,
                 UI_ACCENT);

      c.setTextColor(UI_TEXT);
      c.drawCenterString(String(progress) + "%", 120, 82, 1);

      c.setTextColor(UI_DANGER);
      c.drawCenterString("KEEP STILL!", 120, 100, 1);

    } else {
      // Done
      c.setTextColor(UI_SUCCESS);
      c.setTextSize(2);
      c.drawCenterString("Done!", 120, 30, 1);

      c.setTextSize(1);
      c.setTextColor(UI_MUTED);
      c.drawCenterString("Calibration successful", 120, 55, 1);

      // Show offsets
      c.setTextColor(UI_ACCENT);
      c.drawString("Gyro bias", 20, 75, 1);
      c.setTextColor(UI_TEXT);
      c.drawRightString(String(gyroBiasX, 2) + "," + String(gyroBiasY, 2) +
                            "," + String(gyroBiasZ, 2),
                        220, 75, 1);

      c.setTextColor(UI_ACCENT);
      c.drawString("Accel bias", 20, 90, 1);
      c.setTextColor(UI_TEXT);
      c.drawRightString(String(accBiasX, 3) + "," + String(accBiasY, 3) + "," +
                            String(accBiasZ, 3),
                        220, 90, 1);

      c.setTextColor(UI_SUCCESS);
      c.drawCenterString("A: Recalibrate", 120, 110, 1);
    }

    // Footer hint per state
    c.setTextColor(UI_MUTED);
    if (state == 0) {
      c.drawCenterString("B: Cancel", 120, 125, 1);
    } else if (state == 2) {
      c.drawCenterString("B: Back to Menu", 120, 125, 1);
    }
  }

  void onBtnA() override {
    if (state == 0 || state == 2) {
      // Start calibration
      state = 1;
      progress = 0;
      calStart = millis();
      calAccX = calAccY = calAccZ = 0;
      calGyrX = calGyrY = calGyrZ = 0;
      samples = 0;
      playSound("recstart");
    }
  }

  void onBtnB() override {
    if (state != 1) { // Can't cancel during calibration
      currentApp = &launcher;
      playSound("click");
    }
  }
};

LauncherApp launcher;
GaitLabApp gaitLab;
ScopeApp scope;
NetApp netApp;
FilesApp filesApp;
BatteryApp batteryApp;
CalibrationApp calibrationApp;

App *currentApp = &launcher;

void LauncherApp::onBtnA() {
  playSound("select");
  switch (sel) {
  case 0:
    currentApp = &gaitLab;
    break;
  case 1:
    scope.onActivate();
    currentApp = &scope;
    break;
  case 2:
    currentApp = &netApp;
    break;
  case 3:
    filesApp.onActivate();
    currentApp = &filesApp;
    break;
  case 4:
    currentApp = &batteryApp;
    break;
  case 5:
    calibrationApp.onActivate();
    currentApp = &calibrationApp;
    break;
  }
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
    if (idxStep >= 0) {
      int valStart = body.indexOf(":", idxStep) + 1;
      int valEnd = body.indexOf(",", valStart);
      if (valEnd == -1)
        valEnd = body.indexOf("}", valStart);
      MIN_STEP_TIME_MS = body.substring(valStart, valEnd).toFloat();
    }

    int idxZupt = body.indexOf("zupt_acc");
    if (idxZupt >= 0) {
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

// --- JSON HELPER ---
String extractJSON(String json, String key) {
  int idx = json.indexOf("\"" + key + "\":");
  if (idx == -1)
    return "";
  int start = json.indexOf("\"", idx + key.length() + 3);
  if (start == -1)
    return "";
  start++;
  int end = json.indexOf("\"", start);
  if (end == -1)
    return "";
  return json.substring(start, end);
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
      // Ensure consistent format - always include leading slash
      if (!name.startsWith("/")) {
        name = "/" + name;
      }
      if (name.endsWith(".csv")) {
        if (!first)
          output += ",";
        output +=
            "{\"name\":\"" + name + "\",\"size\":" + String(file.size()) + "}";
        first = false;
      }
      file = root.openNextFile();
    }
    root.close();
  }
  output += "]";
  server.send(200, "application/json", output);
}

// --- FILE DOWNLOAD ---
bool handleFileRead(String path) {
  // Normalize path - ensure it starts with /
  if (!path.startsWith("/")) {
    path = "/" + path;
  }

  // URL decode common characters
  path.replace("%20", " ");
  path.replace("%23", "#");
  path.replace("%25", "%");
  path.replace("%2B", "+");
  path.replace("%3D", "=");

  if (LittleFS.exists(path)) {
    File file = LittleFS.open(path, "r");

    // Extract just the filename for Content-Disposition
    String filename = path;
    int lastSlash = filename.lastIndexOf('/');
    if (lastSlash >= 0) {
      filename = filename.substring(lastSlash + 1);
    }

    // Force browser to download instead of displaying inline
    server.sendHeader("Content-Disposition",
                      "attachment; filename=\"" + filename + "\"");
    server.sendHeader("Content-Length", String(file.size()));
    server.sendHeader("Cache-Control", "no-cache");
    server.sendHeader("Connection", "close");
    server.streamFile(file, "application/octet-stream");
    file.close();
    return true;
  }
  return false;
}

// PHASE 4: Filesystem Auto-Cleanup
void checkStorageAndCleanup() {
  size_t total = LittleFS.totalBytes();
  size_t used = LittleFS.usedBytes();
  float usagePercent = (float)used / total;

  if (usagePercent > 0.9f) { // >90% full
    // Find oldest CSV file
    String oldestFile = "";
    time_t oldestTime = 0xFFFFFFFF;
    int fileCount = 0;

    File root = LittleFS.open("/");
    if (root) {
      File file = root.openNextFile();
      while (file) {
        String name = String(file.name());
        if (!name.startsWith("/")) name = "/" + name;
        if (name.endsWith(".csv")) {
          fileCount++;
          time_t modified = file.getLastWrite();
          if (modified < oldestTime) {
            oldestTime = modified;
            oldestFile = name;
          }
        }
        file = root.openNextFile();
      }
      root.close();
    }

    // Only delete if we have more than 5 files
    if (fileCount > 5 && oldestFile.length() > 0) {
      LittleFS.remove(oldestFile);
      showToast("Storage low, deleted: " + oldestFile, 3000);
    } else if (fileCount <= 5) {
      showToast("Storage full! Delete files manually", 3000);
    }
  }
}

// PHASE 4: Deep Sleep tracking
unsigned long lastActivityTime = 0;
float lastStepCount = 0;
const unsigned long SLEEP_TIMEOUT = 300000; // 5 minutes in ms

void prepareForSleep() {
  // stop anything active
  if (logFile) {
    logFile.close();
  }

  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);

  // wait until power button is fully released
  unsigned long t0 = millis();
  while (M5.BtnPWR.isPressed() && (millis() - t0 < 3000)) {
    M5.update();
    delay(10);
  }

  // configure wake only right before sleep
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_37, LOW);

  delay(50);
  esp_deep_sleep_start();
}

// Boot step display — regular function, no lambda, no captures
void bootStep(int n, const char *label) {
  canvas.fillRect(0, 115, 240, 20, UI_BG);
  canvas.setTextColor(0xFD20); // Orange
  canvas.setTextSize(1);
  char buf[32];
  snprintf(buf, sizeof(buf), "Step %d: %s", n, label);
  canvas.drawCenterString(buf, 120, 118, 1);
  canvas.pushSprite(0, 0);
  delay(300);
  Serial.printf("[BOOT] step %d: %s  heap=%d\n", n, label, ESP.getFreeHeap());
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("[BOOT] start");
  Serial.printf("[BOOT] Reset reason: %d\n", (int)esp_reset_reason());
  Serial.printf("[BOOT] Free heap: %d\n", ESP.getFreeHeap());

  // --- Hardware init ---
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(3);
  M5.Display.setBrightness(50);
  Serial.println("[BOOT] M5 ok");

  canvas.createSprite(M5.Display.width(), M5.Display.height());

  // Splash screen
  showSplashScreen();
  Serial.println("[BOOT] splash done");

  // --- Step-by-step init with on-screen progress ---
  bootStep(1, "Storage");
  if (!LittleFS.begin(true)) {
    Serial.println("[BOOT] LittleFS FAIL");
  } else {
    Serial.println("[BOOT] LittleFS ok");
  }

  bootStep(2, "Algorithms");
  madgwick.begin(0.1f);
  zuptDetector.init();
  anomalyDetector.init();

  bootStep(3, "Init");

  bootStep(4, "WiFi");
  WiFi.softAP(WIFI_SSID, WIFI_PASS);
  delay(100); // Let WiFi settle

  bootStep(5, "Routes");
  server.on("/", HTTP_GET,
            []() { server.send_P(200, "text/html", index_html); });

  server.on("/api/status", HTTP_GET, []() { getStatusJSON(); });

  server.on("/api/config", HTTP_POST, handleConfig);

  // Calibration
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
    char json[192];
    snprintf(json, sizeof(json),
      "{\"success\":true,\"calibrated\":true,\"gyroBias\":[%.4f,%.4f,%.4f],\"accBias\":[%.4f,%.4f,%.4f]}",
      gyroBiasX, gyroBiasY, gyroBiasZ, accBiasX, accBiasY, accBiasZ);
    server.send(200, "application/json", json);
  });

  server.on("/api/calibration", HTTP_GET, []() {
    char json[256];
    snprintf(json, sizeof(json),
      "{\"calibrated\":%s,\"gyroBias\":[%.4f,%.4f,%.4f],\"accBias\":[%.4f,%.4f,%.4f],"
      "\"battery\":{\"percent\":%d,\"voltage\":%.2f,\"charging\":%s}}",
      isCalibrated ? "true" : "false",
      gyroBiasX, gyroBiasY, gyroBiasZ, accBiasX, accBiasY, accBiasZ,
      batteryPercent, M5.Power.getBatteryVoltage() / 1000.0f,
      M5.Power.isCharging() ? "true" : "false");
    server.send(200, "application/json", json);
  });

  // Alerts API
  server.on("/api/alerts", HTTP_POST, []() {
    String body = server.arg("plain");
    if (body.indexOf("\"beep\"") >= 0)
      alertBeepEnabled = body.indexOf("\"beep\":true") >= 0;
    if (body.indexOf("\"led\"") >= 0)
      alertLedEnabled = body.indexOf("\"led\":true") >= 0;
    if (body.indexOf("\"range\"") >= 0)
      alertRangeEnabled = body.indexOf("\"range\":true") >= 0;

    int idx;
    idx = body.indexOf("\"cad_min\"");
    if (idx >= 0) {
      int vs = body.indexOf(":", idx) + 1;
      int ve = body.indexOf(",", vs); if (ve == -1) ve = body.indexOf("}", vs);
      alertCadenceMin = body.substring(vs, ve).toFloat();
    }
    idx = body.indexOf("\"cad_max\"");
    if (idx >= 0) {
      int vs = body.indexOf(":", idx) + 1;
      int ve = body.indexOf(",", vs); if (ve == -1) ve = body.indexOf("}", vs);
      alertCadenceMax = body.substring(vs, ve).toFloat();
    }
    idx = body.indexOf("\"stab_min\"");
    if (idx >= 0) {
      int vs = body.indexOf(":", idx) + 1;
      int ve = body.indexOf(",", vs); if (ve == -1) ve = body.indexOf("}", vs);
      alertStabilityMin = body.substring(vs, ve).toFloat();
    }
    idx = body.indexOf("\"clear_min\"");
    if (idx >= 0) {
      int vs = body.indexOf(":", idx) + 1;
      int ve = body.indexOf(",", vs); if (ve == -1) ve = body.indexOf("}", vs);
      alertClearanceMin = body.substring(vs, ve).toFloat();
    }
    showToast("Alerts updated", 1000);
    char resp[256];
    snprintf(resp, sizeof(resp),
      "{\"beep\":%s,\"led\":%s,\"range\":%s,"
      "\"cad_min\":%.0f,\"cad_max\":%.0f,\"stab_min\":%.0f,\"clear_min\":%.3f}",
      alertBeepEnabled ? "true" : "false",
      alertLedEnabled ? "true" : "false",
      alertRangeEnabled ? "true" : "false",
      alertCadenceMin, alertCadenceMax, alertStabilityMin, alertClearanceMin);
    server.send(200, "application/json", resp);
  });

  server.on("/api/alerts", HTTP_GET, []() {
    char resp[256];
    snprintf(resp, sizeof(resp),
      "{\"beep\":%s,\"led\":%s,\"range\":%s,"
      "\"cad_min\":%.0f,\"cad_max\":%.0f,\"stab_min\":%.0f,\"clear_min\":%.3f,"
      "\"alert_active\":%s,\"alert_reason\":\"%s\"}",
      alertBeepEnabled ? "true" : "false",
      alertLedEnabled ? "true" : "false",
      alertRangeEnabled ? "true" : "false",
      alertCadenceMin, alertCadenceMax, alertStabilityMin, alertClearanceMin,
      rangeAlertActive ? "true" : "false", rangeAlertReason.c_str());
    server.send(200, "application/json", resp);
  });

  // Recording API
  server.on("/api/record/start", HTTP_POST, []() {
    String body = server.arg("plain");
    String sessionName = extractJSON(body, "name");
    String patientId = extractJSON(body, "patientId");
    String sessionType = extractJSON(body, "type");
    String notes = extractJSON(body, "notes");

    String filename = "/";
    if (sessionName.length() > 0) {
      filename += sessionName;
    } else {
      filename += "webrec_" + String(millis());
    }
    filename += ".csv";

    checkStorageAndCleanup();

    logFile = LittleFS.open(filename, FILE_WRITE);
    if (!logFile) {
      server.send(500, "text/plain", "Failed to open file");
      return;
    }
    isRecording = true;

    logFile.println("# GaitOS V2.0 - Ankle Mounted");
    if (sessionName.length() > 0) logFile.println("# Session: " + sessionName);
    if (patientId.length() > 0)   logFile.println("# Patient ID: " + patientId);
    if (sessionType.length() > 0) logFile.println("# Type: " + sessionType);
    if (notes.length() > 0)       logFile.println("# Notes: " + notes);
    logFile.println("# Start Time: " + String(millis()) + " ms");
    logFile.println("# Sample Rate: 100Hz");
    logFile.println("#");
    logFile.println("t,ax,ay,az,gx,gy,gz,q0,q1,q2,q3,roll,pitch,yaw,vx,vy,vz,"
                    "px,py,pz,phase,cadence,stability,abnormal");
    server.send(200);
  });

  server.on("/api/record/stop", HTTP_POST, []() {
    isRecording = false;
    if (logFile) logFile.close();
    server.send(200);
  });

  server.on("/api/logs", HTTP_GET, handleLogsList);

  // Delete all files
  server.on("/api/deleteall", HTTP_POST, []() {
    if (isRecording) {
      isRecording = false;
      if (logFile) logFile.close();
    }
    File root = LittleFS.open("/");
    int deleted = 0;
    if (root) {
      File file = root.openNextFile();
      while (file) {
        String name = String(file.name());
        if (!name.startsWith("/")) name = "/" + name;
        file.close();
        file = root.openNextFile();
        if (name.endsWith(".csv")) {
          if (LittleFS.remove(name)) {
            deleted++;
          } else {
            String altName = name.substring(1);
            if (LittleFS.remove(altName)) deleted++;
          }
        }
      }
      root.close();
    }
    showToast("Deleted " + String(deleted) + " files", 2000);
    server.send(200, "text/plain", "Deleted " + String(deleted) + " files");
  });

  // Format storage
  server.on("/api/format", HTTP_POST, []() {
    if (isRecording) {
      isRecording = false;
      if (logFile) logFile.close();
    }
    Serial.println("Formatting LittleFS...");
    LittleFS.end();
    bool success = LittleFS.format();
    LittleFS.begin(true);
    if (success) {
      showToast("Storage formatted!", 2000);
      server.send(200, "text/plain", "Storage formatted successfully");
    } else {
      showToast("Format failed!", 2000);
      server.send(500, "text/plain", "Format failed");
    }
  });

  // Dynamic routing: DELETE + file downloads
  server.onNotFound([]() {
    String uri = server.uri();

    // DELETE /api/delete/{filename}
    if (server.method() == HTTP_DELETE && uri.startsWith("/api/delete/")) {
      String filename = uri.substring(12);
      filename.replace("%20", " ");
      filename.replace("%2F", "/");
      filename.replace("%5C", "\\");
      if (!filename.startsWith("/")) filename = "/" + filename;

      if (LittleFS.exists(filename)) {
        bool success = LittleFS.remove(filename);
        if (success) {
          server.send(200, "text/plain", "Deleted: " + filename);
        } else {
          server.send(500, "text/plain", "Failed to delete");
        }
      } else {
        String alt = filename.substring(1);
        if (LittleFS.exists(alt) && LittleFS.remove(alt)) {
          server.send(200, "text/plain", "Deleted: " + alt);
        } else {
          server.send(404, "text/plain", "File not found: " + filename);
        }
      }
      return;
    }

    // GET — file download fallback
    if (!handleFileRead(uri)) {
      server.send(404, "text/plain", "404: Not Found");
    }
  });

  bootStep(6, "Server");
  server.begin();

  bootStep(7, "Ready");
  // --- Initial state ---
  pos = {0, 0, 0};
  vel = {0, 0, 0};
  lastSampleTime = millis();
  lastActivityTime = millis();

  showToast("GaitOS V2.0", 2000);
  Serial.printf("[BOOT] setup done  heap=%d\n", ESP.getFreeHeap());
}

// Power button long-press tracking
unsigned long btnPwrPressTime = 0;
bool btnPwrLongPress = false;

void loop() {
  M5.update();
  server.handleClient();
  yield(); // let system tasks run

  // Track activity for deep sleep
  if (stepCount > lastStepCount || isRecording) {
    lastActivityTime = millis();
    lastStepCount = stepCount;
  }

  // Deep sleep after 5 min inactivity
  if (millis() - lastActivityTime > SLEEP_TIMEOUT) {
    showToast("Sleeping...", 1500);
    canvas.fillScreen(UI_BG);
    canvas.setTextColor(UI_MUTED);
    canvas.drawCenterString("Sleeping...", 120, 60, 2);
    canvas.pushSprite(0, 0);
    delay(1500);
    prepareForSleep();
  }

  // --- Sensor sampling at 100Hz ---
  unsigned long now = millis();
  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    float dt = (now - lastSampleTime) / 1000.0f;
    lastSampleTime = now;

    // Read IMU
    M5.Imu.getAccel(&accX, &accY, &accZ);
    M5.Imu.getGyro(&gyroX, &gyroY, &gyroZ);

    // Auto-calibration check
    checkAutoCalibration();

    // Apply bias correction
    gyroX -= gyroBiasX;
    gyroY -= gyroBiasY;
    gyroZ -= gyroBiasZ;
    accX -= accBiasX;
    accY -= accBiasY;
    accZ -= accBiasZ;

    // Madgwick orientation filter
    madgwick.update(gyroX * DEG_TO_RAD, gyroY * DEG_TO_RAD, gyroZ * DEG_TO_RAD,
                    accX, accY, accZ, dt);
    madgwick.getQuaternion(&q0, &q1, &q2, &q3);
    madgwick.getEuler(&roll, &pitch, &yaw);

    // ZUPT-INS navigation
    ZUPT_INS_Update(dt);

    // Scope history (only when viewing scope app)
    if (currentApp == &scope) scope.updateHistory();

    // Battery
    updateBattery();

    // Alert system
    checkRangeAlerts();
    updateAlertLed();

    // Data recording
    if (isRecording && logFile) {
      logFile.printf(
          "%lu,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%.4f,%.4f,%.4f,%.4f,%.1f,%.1f,"
          "%.1f,%.3f,%.3f,%.3f,%.4f,%.4f,%.4f,%d,%.1f,%.1f,%d\n",
          now, accX, accY, accZ, gyroX, gyroY, gyroZ, q0, q1, q2, q3, roll,
          pitch, yaw, vel.x, vel.y, vel.z, pos.x, pos.y, pos.z,
          isStance ? 0 : 1, currentCadence, stabilityIndex,
          gaitAbnormal ? 1 : 0);
    }
  }

  // --- Button input ---
  // Power button long-press (2s hold → power menu from anywhere)
  if (M5.BtnPWR.isPressed() && !btnPwrLongPress) {
    if (btnPwrPressTime == 0) {
      btnPwrPressTime = millis();
    } else if (millis() - btnPwrPressTime > 2000) {
      currentApp = &powerMenu;
      btnPwrLongPress = true;
      playSound("select");
      currentApp->onOpen();
    }
  }

  if (M5.BtnPWR.wasReleased()) {
    if (!btnPwrLongPress && btnPwrPressTime > 0) {
      if (currentApp == &launcher) {
        currentApp = &powerMenu;
        playSound("select");
        currentApp->onOpen();
      } else {
        currentApp = &launcher;
        playSound("click");
        currentApp->onOpen();
      }
    }
    btnPwrPressTime = 0;
    btnPwrLongPress = false;
  }

  if (M5.BtnA.wasPressed()) currentApp->onBtnA();
  if (M5.BtnB.wasPressed()) currentApp->onBtnB();

  // --- Draw ---
  currentApp->onDraw(canvas);

  // Toast overlay
  if (millis() < toastEndTime) {
    String displayMsg = toastMsg;
    if (displayMsg.length() > 20) displayMsg = displayMsg.substring(0, 19) + "~";
    canvas.fillRect(60, 100, 120, 30, UI_CARD);
    canvas.setTextColor(UI_TEXT);
    canvas.drawCenterString(displayMsg, 120, 110, 1);
  }

  canvas.pushSprite(0, 0);
}
