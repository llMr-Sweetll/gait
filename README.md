# GaitOS V2.0: The Open Source Clinical Gait Analysis System

![System Demo](assets/demo.webp)

> **"Democratizing Mobility Research, One Step at a Time."**

> [!WARNING]
> **V2.0 Breaking Changes from V1.3**:
>
> - **Ankle mounting ONLY** (foot mounting no longer supported)
> - **New CSV format** with quaternion data (incompatible with old analysis scripts)
> - **Removed HFC metric** (hip-foot coupling - pending clinical validation)
> - **Improved algorithms**: Madgwick quaternion filter + adaptive ZUPT detection
> - **Auto-calibration**: Automatically zeros sensors after 3 seconds of stillness

## 🌟 Overview: What is GaitOS?

**GaitOS** is an open-source firmware and web platform that transforms a $25 consumer microcontroller (M5StickC Plus 2) into a **"Medical Grade" Inertial Navigation System (INS)** optimized for **ankle-mounted** gait analysis.

It is designed for:

- **Physiotherapists**: To monitor post-stroke recovery without expensive lab equipment.
- **Researchers**: To gather high-fidelity kinematic data in "Free Living" environments.
- **Patients**: To receive real-time biofeedback on gait stability and rhythmicity.

Unlike standard fitness trackers that only count steps, GaitOS tracks **3D Foot Trajectory**, **Clearance Height**, and **pathological Compensations** (like Hip Hiking) using advanced aerospace navigation algorithms (ZUPT).

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

### Footwear Compatibility

- ✅ **Best**: Sneakers/athletic shoes with laces (strap goes over shoe tongue)
- ✅ **Good**: Ankle-high boots, hiking shoes
- ✅ **Okay**: Barefoot (ensure strap is tight on skin)
- ❌ **Avoid**: Loose slippers, flip-flops, high heels (unstable gait pattern)

### Left vs Right Foot

- **Default**: Mount on **dominant leg** (right foot for most people)
- **Bilateral Analysis**: Requires two devices (future feature)
- **Affected Leg**: For stroke patients, mount on **affected side** to track recovery

---

## 💻 Installation Guide (Developer Manual)

Follow these steps precisely to build and load the firmware onto your device.

### Step 1: Install Arduino IDE

Download and install the latest **Arduino IDE (2.0+)** from [arduino.cc](https://www.arduino.cc/en/software).

### Step 2: Install ESP32 Board Support

The device uses an ESP32 chip. You must tell Arduino IDE how to talk to it.

1. Open Arduino IDE.
2. Go to **File** $\rightarrow$ **Preferences**.
3. In the "Additional Boards Manager URLs" field, paste this:
    `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
4. Click **OK**.
5. Go to **Tools** $\rightarrow$ **Board** $\rightarrow$ **Boards Manager...**
6. Type `esp32` in the search bar.
7. Install **"esp32 by Espressif Systems"** (Version 3.0.0 or later).

### Step 3: Install Required Libraries

The code relies on the `M5Unified` library to handle the screen, IMU, and power management.

1. Go to **Tools** $\rightarrow$ **Manage Libraries...** (Ctrl+Shift+I).
2. Type `M5Unified` in the search bar.
3. Click **INSTALL** on **M5Unified by M5Stack**.

> **⚠️ IMPORTANT**: Do **NOT** install the `M5StickCPlus2` library. Our code uses the modern `M5Unified` system which handles everything more efficiently. Installing the legacy library causes conflict errors.

### Step 4: Configure & Flash

1. Connect your device to your computer via USB-C.
2. **Select Board**: Go to **Tools** $\rightarrow$ **Board** $\rightarrow$ **ESP32 Arduino** $\rightarrow$ **M5Stick-C-Plus2**.
3. **Upload Speed**: Select **1500000** (Fast) or **115200** (Reliable).
4. **Open Code**: Open the `gait.ino` file from this repository.
5. **Upload**: Click the **Arrow Icon (→)** in the top left.
    - *Troubleshooting*: If "Connecting..." fails, hold the small **Side Button (BtnB)** on the device while plugging it in to force "Bootloader Mode".

---

## 📖 User Manual: Clinical Workflow

### Phase 1: Power & Connection

1. **Turn On**: Hold Power (Side Button) for 2 seconds.
2. **Connect WiFi**: On your Phone/Laptop, connect to the WiFi Network:
    - SSID: `GAIT-LOGGER`
    - Password: `circumduct123`
3. **Open Dashboard**: Scan the QR code on the device screen (Select 'Net' app), or navigate to `http://192.168.4.1` in your browser.

### Phase 2: Sensor Calibration

**V2.0 Feature**: **Auto-Calibration** is now enabled by default!

#### Option 1: Automatic (Recommended)

1. After mounting the device, **stand perfectly still** for 3 seconds
2. Device will automatically detect stillness and zero the sensors
3. You'll see "Auto-Cal!" toast message on screen
4. Done! You're ready to walk

#### Option 2: Manual (If Auto-Cal Doesn't Trigger)

1. Place the foot **flat on the ground** and hold extremely still
2. **On Device**: Scroll to `System` (BtnB) → Select `Zero Sensors` (BtnA)
3. **On Web**: Click the **"Zero Sensors"** button
4. Wait 3 seconds without moving
5. Wait for "Zeroed!" confirmation message

> [!TIP]
> **Pro Tip**: The auto-calibration runs continuously. If you stand still for 3+ seconds at any point, it will automatically re-zero. This is useful if you notice drift during a session.

---

## 🎛️ Advanced Tuning Guide (The Hybrid Engine)

GaitOS V13 uses a **Hybrid Architecture**: valid defaults for 90% of users, plus manual overrides for specific pathologies.

### 1. Min Step Duration (Speed Filter)

- **What it is**: The minimum time allowed between two steps.
- **Default**: `300ms` (0.3 seconds).
- **When to Adjust**:
  - **Increase (>500ms)**: For **Slow/Ataxic Walkers**. Prevents the system from counting a "wobbly" single step as two steps.
  - **Decrease (<250ms)**: For **Athletes/Runners**. Ensures fast cadence is captured accurately.

### 2. Stance Sensitivity (ZUPT Threshold)

- **What it is**: How "strict" the system is about deciding the foot is on the ground.
- **Default**: `0.2g`.
- **When to Adjust**:
  - **Lower (0.05g - 0.15g)**: **"Sensitive Mode"**. Use for **Frail Patients** who shuffle or walk very softly. The system will detect even faint stops.
  - **Higher (0.25g - 0.50g)**: **"Strict Mode"**. Use for **Heavy Walkers** or uneven terrain. Ignores noise/vibrations.

> **How to Tune**:
> Open the Web Dashboard $\rightarrow$ Expand **"Advanced Tuning"** at the bottom $\rightarrow$ Adjust Sliders $\rightarrow$ Click **Apply Custom Settings**.

---

### Phase 3: Recording

1. **Start**:
    - *Device*: Go to `Lab` app $\rightarrow$ Press BtnA (Red dot appears).
    - *Web*: Click **"Start Recording"**.
2. **Walk**: Perform the clinical test (e.g., 10-Meter Walk Test).
    - Walk naturally.
    - Avoid sudden stops or jumps (this confuses the ZUPT engine).
3. **Monitor**: Watch the dashboard for:
    - **Trajectory**: Is the arc shape consistent?
    - **Stability**: Is it Green (>80%) or Red (<50%)?
4. **Stop**: Press BtnA (Device) or "Stop" (Web).

### Phase 4: Data Export

1. On the Dashboard, scroll down to **"DATA RECORDINGS"**.
2. Click **REFRESH LIST** to see your latest files.
3. Click **DL** next to the file (e.g., `gait_100234.csv`) to download it.

---

## 📊 Data Dictionary (CSV Schema)

The downloaded CSV contains raw 100Hz data with enhanced metadata.

### CSV Header (V2.0 Format)

```csv
# GaitOS V2.0 - Ankle Mounted
# Sample Rate: 100Hz
# ZUPT Threshold: 35.0 deg/s
# Calibration: Auto
#
# Columns:
# t(ms), ax(g), ay(g), az(g), gx(dps), gy(dps), gz(dps),
# q0, q1, q2, q3 (quaternion),
# roll(deg), pitch(deg), yaw(deg),
# vx(m/s), vy(m/s), vz(m/s),
# px(m), py(m), pz(m),
# phase(0=stance,1=swing), cadence(spm), stability(%)
```

### Column Descriptions

| Column | Unit | Description |
| :--- | :--- | :--- |
| `t` | ms | Timestamp since device boot |
| `ax, ay, az` | g | Accelerometer data (Body Frame) |
| `gx, gy, gz` | deg/s | Gyroscope data (Body Frame) |
| `q0, q1, q2, q3` | - | **NEW**: Quaternion orientation (q0=w, q1=x, q2=y, q3=z) |
| `roll, pitch, yaw` | deg | **NEW**: Euler angles derived from quaternion |
| `vx, vy, vz` | m/s | **NEW**: Velocity in navigation frame |
| `px` | m | Position X (Forward displacement) |
| `py` | m | **NEW**: Position Y (Lateral displacement) |
| `pz` | m | Position Z (Vertical clearance height) |
| `phase` | 0/1 | Gait Phase (0=Stance, 1=Swing) |
| `cadence` | steps/min | Instantaneous cadence (smoothed) |
| `stability` | % | Stability Index (0-100, higher = more rhythmic) |

> [!NOTE]
> **V1.3 → V2.0 Changes**:
>
> - ❌ **Removed**: `hfc` (Hip-Foot Coupling - pending validation)
> - ✅ **Added**: Quaternion (q0-q3), Euler angles (roll, pitch, yaw), full velocity vector (vx, vy, vz), lateral position (py)
> - ✅ **Improved**: Distance is now integrated from actual stride length (not hardcoded 0.7m)

---

## 🔋 Battery & Safety Info

- **Charging**: Use a standard USB-C cable (5V). Charge from a PC or low-power brick. Do **NOT** use high-wattage (65W+) laptop chargers as the device may not negotiate power correctly.
- **Battery Life**: Approx 20-30 mins on full brightness with WiFi on.
- **Safety**:
  - Do not use in rain/puddles (Not waterproof).
  - Check skin under the strap for irritation if analyzing long sessions.
  - **Medical Disclaimer**: GaitOS is a research tool. It is **NOT** FDA/CE approved for medical diagnosis. Always consult a professional for clinical advice.

---

## 🔬 Scientific Principles (The "Hybrid Engine")

GaitOS uses a custom **Strapdown Inertial Navigation System (SINS)** aided by **Zero Velocity Updates (ZUPT)**.

### 1. The Drift Problem

Integrating accelerometer data twice ($a \rightarrow v \rightarrow p$) creates cubic error drift ($error \propto t^3$). After 5 seconds, a standard IMU would show you moving 100 meters away.

### 2. The ZUPT Solution

We utilize the biomechanical fact that during **Stance Phase** (foot flat), the foot's velocity is exactly zero relative to the ground.

- **Logic**: If Gyro < `40dps` AND Accel variance is low $\rightarrow$ **We are stopped**.
- **Correction**: Force Velocity = `[0,0,0]`. This resets the integration error to zero every single step.

![Drift Proof](assets/proof_drift.png)
*Figure: Raw Integration (Red) vs GaitOS ZUPT (Green). Note the drift clamping.*

### 3. Stability Index ($SI$)

Derived from Hausdorff's "Fractal Dynamics of Gait Rhythm":
$$ SI = 100 - \frac{|Cadence_{inst} - Cadence_{avg}|}{Cadence_{avg}} \times 100 $$
This quantifies "Arrhythmia" in walking, a key predictor of fall risk in Parkinson's and Stroke.

---

## ❓ Troubleshooting

| Issue | Possible Cause | Solution |
| :--- | :--- | :--- |
| **"M5StickCPlus2.h not found"** | Legacy library conflict. | Remove `M5StickCPlus2` lib. Use only `M5Unified`. |
| **"Upload Failed"** | Device is sleeping or busy. | Hold Side Button (BtnB) while plugging in USB. |
| **Trajectory Looks Crazy** | Sensors not zeroed. | Perform **Phase 2: Calibration** while perfectly still. |
| **No "Steps" Counting** | Walking too soft/slow. | Stomp slightly harder. Threshold is `1.2g`. |
| **WiFi Disconnects** | Power saving mode. | Keep the device screen ON (do not let it sleep). |

---

## 👥 Use License & Credits

**License**: MIT (Open Source). Free for academic, clinical, and personal use.
**Citation**: If used in research, please cite:
> Hegde, C. et al. "GaitOS: A Low-Cost Hybrid Inertial Navigation System for Democratized Clinical Gait Analysis." (2025).

**Repository**: [github.com/llMr-Sweetll/gait.git](https://github.com/llMr-Sweetll/gait.git)
