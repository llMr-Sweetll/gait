# GaitOS V10 (Medical Grade)

**The Professional Gait Analysis System for M5StickC Plus2.**

GaitOS V10 is a research-grade firmware that transforms the M5StickC Plus2 into a precision inertial measurement tool. Unlike previous versions that relied on empirical estimates, V10 utilizes a **Physics-Based Engine (ZUPT-INS)** to mathematically reconstruct the 3D trajectory of the foot in real-time.

---

## 🏥 Medical Grade Features

### 1. 3D Trajectory Reconstruction

Visualizes the actual side-profile arc of your footstep (Z vs X position) in real-time.

- **Why it matters**: Allows detection of "foot drop" (low clearance) or shuffling gait dynamics not visible in simple angle plots.

### 2. ZUPT-Aided Inertial Navigation

Uses **Zero Velocity Update (ZUPT)** algorithms to eliminate sensor drift.

- **Mechanism**: The system detects the precise millisecond your foot impacts the ground (Stance Phase) and resets velocity errors to zero, enabling high-accuracy tracking.

### 3. Clinical Metrics

- **Clearance**: Maximum foot height during swing (cm).
- **Step Length**: Physical distance traveled per step (m).
- **Stance/Swing Ratio**: Precise timing of gait phases.

---

## ⚠️ CRITICAL: How to Wear

**THIS SYSTEM REQUIRS FOOT ATTACHMENT.**

# GaitOS V12.0 (The Wozniak Edition)

**"It Just Works" (Jobs) + "It Works Perfectly" (Woz)**

GaitOS is a medical-grade gait analysis system for the M5StickC Plus2. It combines physics-based ZUPT-INS algorithms with a frictionless, high-performance "Jobsian" interface.

## 🚀 Key Features (V12)

* **Medical Grade Accuracy**: ZUPT-INS Engine with Circular Buffer Optimization.
- **Jobsian Interface**: Minimalist Geometric Icons, Organic "Breathing" Animations, and "Hero" Trajectory Visualization.
- **Frictionless Sync**: Built-in **QR Code** for instant WiFi connection.
- **Wozniak Engineering**: Zero-allocation Loop, Fast Math, and Robust Drift Cancellation.

## 🛠️ Calibration Protocol (The "Woz" Zero)

To achieve medical-grade data without "horrible drift", you MUST follow this protocol:

1. **Boot**: Place device FLAT on a table. Turn on. Wait 5 seconds.
2. **Zeroing**: If you see drift or "weird numbers":
    - Go to **System -> Zero Sensors**.
    - **CRITICAL**: Keep the device ABSOLUTELY STILL for 2 seconds while it samples the Gyro Bias.
    - Once "Zeroed" toast appears, you are ready.

## 📱 How to Use

1. **Attach**: Strap firmly to your foot/shoe (Screen facing OUT/SIDE).
2. **Connect**:
    - Open **Connect** App on device.
    - Scan QR Code with phone.
    - Or connect to WiFi `GAIT-LOGGER` (Pass: `circumduct123`) and browse to `192.168.4.1`.
3. **Walk**:
    - Open **Lab** App to see live steps.
    - Use Web UI to visualize Trajectory.
    - Press **Record** (Button A) or use Web UI to log data.

## 📂 Downloads

Files are saved as `.csv` in internal storage.
- **Method A**: Click filename in Web Dashboard.
- **Method B**: Access `/api/logs` for JSON list.

## 🔧 Build & Flash

1. Install VS Code + PlatformIO.
2. Open Project.
3. Upload `firmware/main.cpp`.
4. Upload `data` (LittleFS Image).
cting the "Stance" phase.

- **Fix**: Ensure the device is **firmly attached to the foot** and you are walking with a distinct "flat foot" phase. The ZUPT threshold might need tuning for very soft sneakers or running.

**Issue: Screen is upside down.**

- **Fix**: Go to **Settings -> Flip Screen** (Manual toggle). Or restart the device (V10 static default is Landscape).

---

**Developed for Advanced Gait Research.**
*Based on 2023-2024 ZUPT-INS methodologies.*
