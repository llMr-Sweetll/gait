# GaitOS V2.0: The Open Source Clinical Gait Analysis System

![System Demo](assets/demo.webp)

> **"Democratizing Mobility Research, One Step at a Time."**

> [!WARNING]
> **V2.0 Breaking Changes from V1.3**:
>
> - **Ankle mounting ONLY** (foot mounting no longer supported)
> - **New CSV format** with quaternion data + abnormality flags (incompatible with old analysis scripts)
> - **Removed HFC metric** (hip-foot coupling - pending clinical validation)
> - **Improved algorithms**: Madgwick quaternion filter + adaptive ZUPT detection
> - **Auto-calibration**: Automatically zeros sensors after 3 seconds of stillness
> - **NEW**: Gait abnormality detection with device alerts
> - **NEW**: Deep sleep mode for extended battery life
> - **NEW**: Session comparison for progress tracking

---

## 🌟 Overview: What is GaitOS?

**GaitOS** is an open-source firmware and web platform that transforms a $25 consumer microcontroller (M5StickC Plus 2) into a **"Medical Grade" Inertial Navigation System (INS)** optimized for **ankle-mounted** gait analysis.

### Key Features

✅ **Real-time 3D Trajectory Tracking** - See foot path during walking  
✅ **Abnormality Detection** - Automatic alerts for irregular gait patterns  
✅ **Session Comparison** - Visual before/after progress tracking  
✅ **Production-Ready** - Deep sleep, auto-storage cleanup, watchdog recovery  
✅ **Web Dashboard** - No app required, works on any phone/tablet  
✅ **Clinical Metrics** - Cadence, stability, clearance, stride length  

### Designed For

- **Physiotherapists**: Monitor post-stroke recovery without expensive lab equipment
- **Researchers**: Gather high-fidelity kinematic data in "Free Living" environments
- **Patients**: Receive real-time biofeedback on gait stability and rhythmicity

Unlike standard fitness trackers that only count steps, GaitOS tracks **3D Foot Trajectory**, **Clearance Height**, and **pathological Compensations** (like Hip Hiking) using advanced aerospace navigation algorithms (ZUPT).

> [!NOTE]
> **📖 For Medical Practitioners**: If you're a physiotherapist, occupational therapist, or rehabilitation specialist, see the **[Clinical Practitioner Guide](CLINICAL_GUIDE.md)** for plain-language instructions and clinical interpretation guidelines.

---

## 🛠️ Hardware Requirements

| Component | Description | Est. Cost |
| :--- | :--- | :--- |
| **M5StickC Plus 2** | The core device (ESP32-PICO-D4 + MPU6886 IMU + Screen/Battery). | ~$25 USD |
| **Velcro Strap** | 20mm width hook-and-loop strap (Watch strap style). | ~$2 USD |
| **PC/Mac** | To flash the firmware initially. | N/A |
| **Smartphone** | To access the Web Dashboard (No App required). | N/A |

**Critical Compatibility Warning**:
This software is optimized *specifically* for the **M5StickC Plus 2** (Yellow or Blue PCB).

- ❌ **M5StickC (Original)**: NOT Supported (Screen is smaller, IMU is different).
- ❌ **M5Atom / M5Stack Core**: NOT Supported (Form factor unsuitable for foot).

---

## 👟 Device Mounting: Ankle Position (REQUIRED)

### ⚠️ CRITICAL: Mounting Position

The M5StickC Plus 2 **MUST** be mounted on the **ANKLE**, not the foot. This is non-negotiable for accurate data.

### Why Ankle Mounting?

- ✅ **More stable**: Less soft tissue movement compared to foot
- ✅ **Easier to wear**: Simple velcro around ankle bone
- ✅ **Better signal**: Clearer swing phase detection, higher accelerometer peaks
- ✅ **Research standard**: Most IMU gait studies use ankle or shank mounting
- ✅ **Reduced artifacts**: Minimal impact from shoe flex or ground contact

### Exact Position

**Location**: **Lateral malleolus** (outer ankle bone)

- **Height**: 2-3cm **ABOVE** the ankle bone prominence
- **Side**: Outer (lateral) side of leg

**Orientation** (CRITICAL):

- **Screen**: Facing **OUTWARD** (away from your other leg)
- **USB Port**: Facing **DOWN** (toward ground)
- **Button A (Large)**: Facing **FORWARD** (toward toes)

```text
      ( Leg )
        | |
        | |  <--- [Ankle Bone]
   [M5StickC]  <--- Device positioned 2-3cm above
        | |           (Velcro strap around ankle)
      __| |__
     /   |   \
    |  Shoe  |
    \_________/
```

### Strapping Technique

1. **Strap Selection**: Use included velcro strap or a watch band (20mm width)
2. **Threading**: Thread strap through the device's built-in strap hole
3. **Positioning**: Place device on outer ankle, 2-3cm above bone
4. **Tightening**: Wrap strap around ankle, pull **TIGHT** - device must not rotate independently
5. **Verification**: Shake your leg vigorously - device should feel "glued" to ankle, no wobbling

> [!IMPORTANT]
> **Tightness Test**: If you can rotate the device with your finger while wearing it, it's TOO LOOSE. Re-tighten until it doesn't move.

---

## 💻 Installation Guide (Developer Manual)

### Step 1: Install Arduino IDE

Download and install the latest **Arduino IDE (2.0+)** from [arduino.cc](https://www.arduino.cc/en/software).

### Step 2: Install ESP32 Board Support

1. Open Arduino IDE.
2. Go to **File** → **Preferences**.
3. In the "Additional Boards Manager URLs" field, paste:
    `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
4. Click **OK**.
5. Go to **Tools** → **Board** → **Boards Manager...**
6. Type `esp32` in the search bar.
7. Install **"esp32 by Espressif Systems"** (Version 3.0.0 or later).

### Step 3: Install Required Libraries

1. Go to **Tools** → **Manage Libraries...** (Ctrl+Shift+I).
2. Type `M5Unified` in the search bar.
3. Click **INSTALL** on **M5Unified by M5Stack**.

> **⚠️ IMPORTANT**: Do **NOT** install the `M5StickCPlus2` library. Our code uses the modern `M5Unified` system which handles everything more efficiently.

### Step 4: Configure & Flash

1. Connect your device to your computer via USB-C.
2. **Select Board**: **Tools** → **Board** → **ESP32 Arduino** → **M5Stick-C-Plus2**.
3. **Upload Speed**: Select **1500000** (Fast) or **115200** (Reliable).
4. **Open Code**: Open the `gait.ino` file from this repository.
5. **Upload**: Click the **Arrow Icon (→)** in the top left.
    - *Troubleshooting*: If "Connecting..." fails, hold the small **Side Button (BtnB)** on the device while plugging it in to force "Bootloader Mode".

---

## 📖 User Manual: Device Interface

### Device Apps (Navigate with Button B)

GaitOS has 4 apps accessible from the launcher:

1. **📊 Gait Lab** - Start/stop recording, view live metrics
2. **📈 Scope** - View real-time 2D trajectory path
3. **📡 Net** - WiFi connection info + QR code
4. **📁 Files** - Browse and delete CSV files on device

### Power Menu (NEW in Phase 3.5)

**Access**: Hold Power Button for **2 seconds**

Options:

- **Battery Info**: Shows voltage + percentage
- **Sleep Mode**: Enter deep sleep manually
- **Restart**: Reboot device
- **Settings**: Reserved for future features
- **Cancel**: Return to launcher

---

## 📱 Clinical Workflow

### Phase 1: Power & Connection

1. **Turn On**: Hold Power (Side Button) for 2 seconds.
2. **Connect WiFi**: On your Phone/Laptop, connect to:
    - SSID: `GAIT-LOGGER`
    - Password: `circumduct123`
3. **Open Dashboard**: Scan QR code on device screen (Net app), or navigate to `http://192.168.4.1` in your browser.

### Phase 2: Sensor Calibration

**Auto-Calibration** is enabled by default!

#### Option 1: Automatic (Recommended)

1. After mounting the device, **stand perfectly still** for 3 seconds
2. Device will automatically detect stillness and zero the sensors
3. You'll see "Auto-Cal!" toast message on screen
4. Done! You're ready to walk

#### Option 2: Manual (If Auto-Cal Doesn't Trigger)

1. Place the foot **flat on the ground** and hold extremely still
2. **On Device**: Scroll to `System` (BtnB) → Select `Zero Sensors` (BtnA)
3. **On Web**: Click the **"Zero Sensors"** button
4. Wait for "Zeroed!" confirmation message

> [!TIP]
> **Pro Tip**: The auto-calibration runs continuously. If you stand still for 3+ seconds at any point, it will automatically re-zero. This is useful if you notice drift during a session.

### Phase 3: Recording

1. **Start**:
    - *Device*: Go to `Lab` app → Press BtnA (Red dot appears).
    - *Web*: Click **"Start Recording"**.
2. **Walk**: Perform the clinical test (e.g., 10-Meter Walk Test).
    - Walk naturally.
    - Avoid sudden stops or jumps (this confuses the ZUPT engine).
3. **Monitor**: Watch the dashboard for:
    - **Trajectory**: Is the arc shape consistent?
    - **Stability**: Is it Green (>80%) or Red (<50%)?
    - **Abnormality**: ⚠️ warning if irregular gait detected
4. **Stop**: Press BtnA (Device) or "Stop" (Web).

### Phase 4: Data Export & Analysis

1. On the Dashboard, scroll down to **"Session History"**.
2. Click **REFRESH** to see your latest files.
3. Click **Download** next to the file (e.g., `session_baseline.csv`).
4. For progress tracking, use **Session Comparison** (see below).

---

## 🔬 Gait Abnormality Detection (NEW in Phase 3)

### What It Detects

GaitOS continuously analyzes your gait and alerts you to:

- **Shuffling** - Low clearance height (<3cm)
- **Irregular cadence** - Steps too slow/fast compared to baseline
- **Inconsistent stride** - Stride length varies >30%

### How It Works

1. **Baseline Learning**: First 10 steps establish your normal pattern
2. **Real-time Analysis**: Every step is compared to baseline
3. **Alert Trigger**: If abnormal pattern detected:
   - **Device**: Red LED flashes 3x + dual beep (2kHz)
   - **Device Screen**: Toast shows warning + reason
   - **Web UI**: ⚠️ Abnormal flag displayed
   - **CSV Data**: `abnormal` column = 1

### Alert Cooldown

- **3 seconds** between alerts to avoid notification spam
- Resets automatically when gait returns to normal

### Clinical Use Cases

- **Post-stroke monitoring**: Detect compensation patterns
- **Fall risk assessment**: Identify shuffle or instability
- **Fatigue detection**: Catch deterioration during long walks

---

## 📊 Session Comparison (NEW in Phase 4)

### Purpose

Visually compare two gait sessions to quantify patient progress.

### How to Use

1. **Record Sessions**: Capture baseline and post-therapy walks
2. **Open Web Dashboard** → Scroll to **"Session Comparison"**
3. **Select Sessions**: Choose 2 different sessions from dropdowns
4. **Click "Compare Trajectories"**

### What You Get

**Trajectory Overlay**:

- Cyan line = Session 1
- Orange line = Session 2
- Auto-scaled axes

**Metrics Comparison Table**:

| Metric | Session 1 | Session 2 | Δ |
|--------|-----------|-----------|---|
| Avg Cadence (spm) | 95.2 | 108.3 | +13.1 🟢 |
| Avg Stability (%) | 72.4 | 81.9 | +9.5 🟢 |
| Distance (m) | 8.5 | 10.2 | +1.7 |

- Green = Improvement
- Red = Decline

### Clinical Workflow

```
Baseline → Therapy → Follow-up → Compare Sessions → Quantified Progress
```

---

## 🎛️ Advanced Tuning Guide

### 1. Min Step Duration (Speed Filter)

- **What it is**: Minimum time allowed between two steps
- **Default**: `280ms`
- **When to Adjust**:
  - **Increase (>400ms)**: For slow/ataxic walkers
  - **Decrease (<250ms)**: For athletes/runners

### 2. Stance Sensitivity (ZUPT Threshold)

- **What it is**: How strict the system is about detecting foot-on-ground
- **Default**: `0.25g`
- **When to Adjust**:
  - **Lower (0.05g-0.15g)**: Sensitive mode for frail patients who shuffle
  - **Higher (0.30g-0.50g)**: Strict mode for heavy walkers or uneven terrain

> **How to Tune**:
> Open Web Dashboard → Expand **"Advanced Tuning"** → Adjust Sliders → Click **Apply Custom Settings**.

---

## 📊 Data Dictionary (CSV Schema)

### CSV Header (V2.0 Format)

```csv
# GaitOS V2.0 - Ankle Mounted
# Sample Rate: 100Hz
# ZUPT Threshold: 35.0 deg/s
# Calibration: Auto
# Session: baseline_walk
# Patient ID: P-12345
#
# Columns:
# t(ms), ax(g), ay(g), az(g), gx(dps), gy(dps), gz(dps),
# q0, q1, q2, q3 (quaternion),
# roll(deg), pitch(deg), yaw(deg),
# vx(m/s), vy(m/s), vz(m/s),
# px(m), py(m), pz(m),
# phase(0=stance,1=swing), cadence(spm), stability(%), abnormal(0/1)
```

### Column Descriptions

| Column | Unit | Description |
| :--- | :--- | :--- |
| `t` | ms | Timestamp since device boot |
| `ax, ay, az` | g | Accelerometer data (Body Frame) |
| `gx, gy, gz` | deg/s | Gyroscope data (Body Frame) |
| `q0, q1, q2, q3` | - | Quaternion orientation (q0=w, q1=x, q2=y, q3=z) |
| `roll, pitch, yaw` | deg | Euler angles derived from quaternion |
| `vx, vy, vz` | m/s | Velocity in navigation frame |
| `px` | m | Position X (Forward displacement) |
| `py` | m | Position Y (Lateral displacement) |
| `pz` | m | Position Z (Vertical clearance height) |
| `phase` | 0/1 | Gait Phase (0=Stance, 1=Swing) |
| `cadence` | steps/min | Instantaneous cadence (smoothed) |
| `stability` | % | Stability Index (0-100, higher = more rhythmic) |
| `abnormal` | 0/1 | **NEW**: Abnormality flag (1 = irregular gait detected) |

---

## ✅ Data Validation Procedures

### 1. Visual Inspection

**Trajectory Check**:

- ✅ **Good**: Smooth arc, consistent peaks, no sudden jumps
- ❌ **Bad**: Jagged lines, spikes >2m, negative clearance

**Cadence Check**:

- ✅ **Good**: Stays within 80-130 spm for normal walking
- ❌ **Bad**: Drops to 0 or spikes >200 (indicates missed steps)

### 2. Automated Quality Checks

Run these validation scripts (Python required):

```python
import pandas as pd

# Load CSV
df = pd.read_csv('session_001.csv', comment='#')

# Check 1: No missing values
assert df.isnull().sum().sum() == 0, "Missing values detected!"

# Check 2: Reasonable ranges
assert df['px'].max() < 100, "Forward position exceeds 100m - likely drift!"
assert df['pz'].min() >= -0.1, "Negative clearance detected - calibration issue!"

# Check 3: Step count validation
num_steps = df['phase'].diff().clip(lower=0).sum()
print(f"Total steps: {num_steps}")

# Check 4: Abnormality rate
abnormal_rate = df['abnormal'].mean() * 100
print(f"Abnormality rate: {abnormal_rate:.1f}%")
if abnormal_rate > 50:
    print("⚠️ WARNING: >50% abnormal - check device mounting!")
```

### 3. Common Data Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| Trajectory drifts upward | Not calibrated | Re-zero sensors while standing still |
| No steps detected | Threshold too high | Lower ZUPT threshold to 0.15g |
| Excessive abnormal flags | Baseline contaminated | Ensure first 10 steps are clean |
| Clearance too low (<2cm) | Shuffling gait | Expected for certain pathologies |
| Stability <30% | Very irregular gait | May indicate neurological issue |

---

## 🔍 Analysis & Interpretation

### Key Metrics Interpretation

**Cadence (steps/min)**:

- **Normal**: 100-120 spm
- **Slow**: <90 spm (elderly, cautious gait)
- **Fast**: >130 spm (athletes, hurried walking)

**Stability (%)**:

- **Excellent**: >85% (highly rhythmic)
- **Good**: 70-85% (minor variability)
- **Fair**: 50-70% (moderate irregularity)
- **Poor**: <50% (ataxic, high fall risk)

**Clearance Height (pz)**:

- **Normal**: 5-15cm during swing
- **Shuffling**: <3cm (trip hazard)
- **High-stepping**: >20cm (compensation)

**Abnormality Rate**:

- **Normal**: <10% of steps
- **Mild pathology**: 10-30%
- **Severe pathology**: >30%

---

## 🛠️ Troubleshooting Guide

### Device Issues

| Issue | Possible Cause | Solution |
| :--- | :--- | :--- |
| **Device won't turn on** | Battery depleted | Charge via USB-C for 30 min |
| **"M5StickCPlus2.h not found"** | Wrong library | Remove `M5StickCPlus2`, use only `M5Unified` |
| **Upload fails** | Device sleeping | Hold BtnB while plugging in USB |
| **Power menu doesn't appear** | Need to update firmware | Reflash latest version (fixed in Phase 3.5) |

### Data Quality Issues

| Issue | Possible Cause | Solution |
| :--- | :--- | :--- |
| **Trajectory looks crazy** | Sensors not zeroed | Stand still, wait for "Auto-Cal!" toast |
| **No steps counting** | Walking too soft | Stomp slightly harder or lower threshold |
| **Drift in position** | Calibration during movement | Re-zero while **completely** still |
| **Excessive abnormal flags** | Device loose on ankle | Tighten strap until device can't rotate |
| **Clearance always <1cm** | Mounted on foot instead of ankle | Remount 2-3cm ABOVE ankle bone |

### WiFi Issues

| Issue | Possible Cause | Solution |
| :--- | :--- | :--- |
| **Can't connect to WiFi** | Wrong password | Use `circumduct123` (all lowercase) |
| **WiFi disconnects** | Deep sleep activated | Keep device screen on during session |
| **Can't access 192.168.4.1** | Not connected to device WiFi | Check WiFi: must be `GAIT-LOGGER` |

### Storage Issues (NEW in Phase 4)

| Issue | Solution |
|-------|----------|
| **"Storage low" toast** | Auto-cleanup deleted oldest file - normal behavior |
| **Can't start recording** | Delete files manually from Files app or web UI |
| **Files not appearing** | Click REFRESH in web UI Session History |

---

## 🔋 Battery & Power Management (NEW in Phase 4)

### Deep Sleep Mode

**What it does**: After 5 minutes of inactivity (no steps), device automatically enters deep sleep to save battery.

**How to wake**: Press power button

**Battery life improvement**:

- **Without deep sleep**: ~4 hours  
- **With deep sleep**: >24 hours (for intermittent use)

### Manual Power Control

**Sleep**: Power Menu → Sleep Mode  
**Restart**: Power Menu → Restart  
**Power Off**: Hold power button 6 seconds (hard shutdown)

### Charging

- Use standard USB-C cable (5V)
- Charge from PC or low-power brick
- **Avoid**: High-wattage (65W+) laptop chargers

---

## 🔬 Scientific Principles

### The Hybrid Engine

GaitOS uses a custom **Strapdown Inertial Navigation System (SINS)** aided by **Zero Velocity Updates (ZUPT)**.

**The Drift Problem**: Integrating accelerometer data twice creates cubic error drift. After 5 seconds, a standard IMU would show you moving 100 meters away.

**The ZUPT Solution**: We utilize the biomechanical fact that during **Stance Phase** (foot flat), the foot's velocity is exactly zero.

- **Logic**: If Gyro < 40dps AND Accel variance is low → We are stopped
- **Correction**: Force Velocity = [0,0,0]. This resets the integration error every single step.

### Stability Index

Derived from Hausdorff's "Fractal Dynamics of Gait Rhythm":

$$ SI = 100 - \frac{|Cadence_{inst} - Cadence_{avg}|}{Cadence_{avg}} \times 100 $$

This quantifies "Arrhythmia" in walking, a key predictor of fall risk in Parkinson's and Stroke.

---

## 📋 Safety & Medical Disclaimer

- **Not waterproof**: Do not use in rain/puddles
- **Skin irritation**: Check skin under strap for long sessions
- **Medical device**: GaitOS is a **research tool**, NOT FDA/CE approved for medical diagnosis
- **Clinical advice**: Always consult a professional for medical decisions

---

## 👥 License & Citation

**License**: MIT (Open Source). Free for academic, clinical, and personal use.

**Citation**: If used in research, please cite:
> Hegde, C. et al. "GaitOS: A Low-Cost Hybrid Inertial Navigation System for Democratized Clinical Gait Analysis." (2025).

**Repository**: [github.com/llMr-Sweetll/gait](https://github.com/llMr-Sweetll/gait)

---

## 📞 Support & Contributing

For issues, feature requests, or contributions:

- Open an issue on GitHub
- Submit a pull request
- Email: [your-email]

**Version**: 2.0.0 (Phase 4 Complete)  
**Last Updated**: February 2026
