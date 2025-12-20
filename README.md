# GaitOS V13: The Open Source Clinical Gait Analysis System

![System Demo](assets/demo.webp)

> **"Democratizing Mobility Research, One Step at a Time."**

## 🌟 Overview: What is GaitOS?

**GaitOS** is an open-source firmware and web platform that transforms a $25 consumer microcontroller (M5StickC Plus 2) into a **"Medical Grade" Inertial Navigation System (INS)**.

It is designed for:

* **Physiotherapists**: To monitor post-stroke recovery without expensive lab equipment.
* **Researchers**: To gather high-fidelity kinematic data in "Free Living" environments.
* **Patients**: To receive real-time biofeedback on gait stability and rhythmicity.

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

* ❌ **M5StickC (Original)**: NOT Supported (Screen is smaller, IMU is different).
* ❌ **M5Atom / M5Stack Core**: NOT Supported (Form factor unsuitable for foot).

---

## 👟 Device Mounting & Wearability (CRITICAL)

The quality of data depends entirely on how well the device is mounted. If it wobbles, your data is garbage.

### 1. Positioning the Device

The M5StickC Plus 2 must be mounted on the **Lateral Aspect (Outside)** of the shoe or foot.

* **Why?**: This location minimizes impact shock and provides the clearest signal for Foot Clearance (Z-Height).
* **Location**: Just below the **Lateral Malleolus** (The bony bump on the outside of your ankle).
* **Orientation**:
  * **Screen**: Facing **OUTWARDS** (Away from your other foot).
  * **USB Port**: Facing **BACKWARDS** (Towards your heel).
  * **Button A (Big)**: Facing **FORWARD** (Towards your toes).

```
      ( Leg )
        | |
   [Ankle Bone]
        | |      <--- [ M5StickC Device ]
      __| |__         (Strap goes here)
     /       \
    |  Shoe   |
    \_________/
```

### 2. Strapping Technique

1. **Thread the Strap**: M5StickC has a "Watch Strap" hole on the back case. Thread a 20mm Velcro strap through it.
2. **Tighten**: Wrap the strap around the mid-foot (instep). Pull it **TIGHT**.
3. **Check**: Shake your foot. Does the device wobble independently of the shoe? If yes, tighten again. It must move *rigidly* with the foot.

### 3. Footwear

* **Best**: Sneakers/Trainers with laces (Strap goes over laces).
* **Okay**: Barefoot (Strap directly on skin, might slip).
* **Avoid**: Loose slippers, Flip-flops, or High heels.

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
    * *Troubleshooting*: If "Connecting..." fails, hold the small **Side Button (BtnB)** on the device while plugging it in to force "Bootloader Mode".

---

## 📖 User Manual: Clinical Workflow

### Phase 1: Power & Connection

1. **Turn On**: Hold Power (Side Button) for 2 seconds.
2. **Connect WiFi**: On your Phone/Laptop, connect to the WiFi Network:
    * SSID: `GAIT-LOGGER`
    * Password: `circumduct123`
3. **Open Dashboard**: Scan the QR code on the device screen (Select 'Net' app), or navigate to `http://192.168.4.1` in your browser.

### Phase 2: Sensor Zeroing (Calibration)

**Essential Step**: MEMS sensors drift with temperature. You must zero them before every session.

1. Place the foot **flat on the ground** and hold extremely still.
2. **On Device**: Scroll to `System` (BtnB) $\rightarrow$ Select `Zero Sensors` (BtnA).
3. **On Web**: Click the **"Zero Sensors"** button.
4. Wait 3 seconds. Do not breathe heavily or wiggle toes.
5. Wait for the "Precision Zeroed" message.

---

## 🎛️ Advanced Tuning Guide (The Hybrid Engine)

GaitOS V13 uses a **Hybrid Architecture**: valid defaults for 90% of users, plus manual overrides for specific pathologies.

### 1. Min Step Duration (Speed Filter)

* **What it is**: The minimum time allowed between two steps.
* **Default**: `300ms` (0.3 seconds).
* **When to Adjust**:
  * **Increase (>500ms)**: For **Slow/Ataxic Walkers**. Prevents the system from counting a "wobbly" single step as two steps.
  * **Decrease (<250ms)**: For **Athletes/Runners**. Ensures fast cadence is captured accurately.

### 2. Stance Sensitivity (ZUPT Threshold)

* **What it is**: How "strict" the system is about deciding the foot is on the ground.
* **Default**: `0.2g`.
* **When to Adjust**:
  * **Lower (0.05g - 0.15g)**: **"Sensitive Mode"**. Use for **Frail Patients** who shuffle or walk very softly. The system will detect even faint stops.
  * **Higher (0.25g - 0.50g)**: **"Strict Mode"**. Use for **Heavy Walkers** or uneven terrain. Ignores noise/vibrations.

> **How to Tune**:
> Open the Web Dashboard $\rightarrow$ Expand **"Advanced Tuning"** at the bottom $\rightarrow$ Adjust Sliders $\rightarrow$ Click **Apply Custom Settings**.

---

### Phase 3: Recording

1. **Start**:
    * *Device*: Go to `Lab` app $\rightarrow$ Press BtnA (Red dot appears).
    * *Web*: Click **"Start Recording"**.
2. **Walk**: Perform the clinical test (e.g., 10-Meter Walk Test).
    * Walk naturally.
    * Avoid sudden stops or jumps (this confuses the ZUPT engine).
3. **Monitor**: Watch the dashboard for:
    * **Trajectory**: Is the arc shape consistent?
    * **Stability**: Is it Green (>80%) or Red (<50%)?
4. **Stop**: Press BtnA (Device) or "Stop" (Web).

### Phase 4: Data Export

1. On the Dashboard, scroll down to **"DATA RECORDINGS"**.
2. Click **REFRESH LIST** to see your latest files.
3. Click **DL** next to the file (e.g., `gait_100234.csv`) to download it.

---

## 📊 Data Dictionary (CSV Schema)

The downloaded CSV contains raw 100Hz data.

| Column | Unit | Description |
| :--- | :--- | :--- |
| `t` | ms | Timestamp since boot. |
| `ax, ay, az` | g | Accelerometer data (Body Frame). |
| `gx, gy, gz` | dps | Gyroscope data (Body Frame). |
| `px` | m | Position X (Forward displacement within step). |
| `pz` | m | Position Z (Vertical Clearance). |
| `phase` | 0/1 | Gait Phase (0=Stance, 1=Swing). |
| `cadence` | spm | Instantaneous Steps Per Minute. |
| `stability` | % | Stability Index (0-100). |
| `hfc` | idx | Hip-Foot Coupling Index. |

---

## 🔋 Battery & Safety Info

* **Charging**: Use a standard USB-C cable (5V). Charge from a PC or low-power brick. Do **NOT** use high-wattage (65W+) laptop chargers as the device may not negotiate power correctly.
* **Battery Life**: Approx 20-30 mins on full brightness with WiFi on.
* **Safety**:
  * Do not use in rain/puddles (Not waterproof).
  * Check skin under the strap for irritation if analyzing long sessions.
  * **Medical Disclaimer**: GaitOS is a research tool. It is **NOT** FDA/CE approved for medical diagnosis. Always consult a professional for clinical advice.

---

## 🔬 Scientific Principles (The "Hybrid Engine")

GaitOS uses a custom **Strapdown Inertial Navigation System (SINS)** aided by **Zero Velocity Updates (ZUPT)**.

### 1. The Drift Problem

Integrating accelerometer data twice ($a \rightarrow v \rightarrow p$) creates cubic error drift ($error \propto t^3$). After 5 seconds, a standard IMU would show you moving 100 meters away.

### 2. The ZUPT Solution

We utilize the biomechanical fact that during **Stance Phase** (foot flat), the foot's velocity is exactly zero relative to the ground.

* **Logic**: If Gyro < `40dps` AND Accel variance is low $\rightarrow$ **We are stopped**.
* **Correction**: Force Velocity = `[0,0,0]`. This resets the integration error to zero every single step.

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
