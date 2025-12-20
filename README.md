# GaitOS V13: Advanced Clinical Gait Analysis System

![System Demo](assets/demo.webp)
*Figure 1: Real-time V13 Dashboard showing Cadence, Stability, and Trajectory metrics.*

## 🌟 Mission Statement: Democratizing Mobility Research

**GaitOS** was developed with a single purpose: **To make clinical-grade gait analysis accessible to everyone.**

Traditional gait labs cost upwards of **$50,000**, making them inaccessible to smaller physiotherapy clinics and patients in developing regions. GaitOS bridges this gap by transforming a **$25 M5StickC Plus 2** into a precision instrument capable of tracking:

* **Post-Stroke Recovery**: Monitoring foot clearance (to prevent falls) and swing symmetry.
* **Hip-Foot Coupling**: Estimates hip functionality using the Chen et al. kinematic chain model ($HFC$), detecting compensatory hiking.
* **Rehabilitation Progress**: Quantifying improvements in cadence and stability over time.
* **Neurological Disorders**: Detecting irregular gait patterns (Parkinsonian shuffle) via the Stability Index.

This is **Open Science**. This code is for researchers, doctors, and patients who believe that mobility is a human right.

## 🏆 Competitive Analysis: Why GaitOS?

GaitOS fills the critical gap between "Consumer Toys" and "Clinical Labs".

| Feature | **GaitOS V13** | Optical Mocap (Vicon) | Consumer (Apple/Fitbit) | Research IMU (Xsens) |
| :--- | :--- | :--- | :--- | :--- |
| **Cost** | **$25 USD** | $50,000+ | $400+ | $2,000+ |
| **Metric Fidelity** | **Trajectory (ZUPT)** | Perfect (Gold Standard) | Step Count Only | Trajectory |
| **Foot Clearance** | **✅ Yes (<1cm error)** | ✅ Yes | ❌ No | ✅ Yes |
| **Environment** | **Anywhere** (Indoors/Out) | Lab Only (Line of Sight) | Anywhere | Anywhere |
| **Data Access** | **Open Source (Raw CSV)** | Proprietary | Closed Garden (Aggregated) | Proprietary SDK |
| **Real-Time Valid.**| **✅ Biofeedback Screen** | ❌ Post-Processing | ⚠️ Minimal (Ring close) | ❌ Post-Processing |

### Power of the Hybrid Engine

Unlike standard pedometers (Fitbit) or simple integrations, GaitOS uses **Physics + Logic**:

* **VS Consumer**: Consumer bands barely detect "steps" from wrist/pocket vibrations. GaitOS detects *foot trajectory* to the millimeter.
* **VS Optical**: Optical systems fail if the camera view is blocked. GaitOS works under a blanket, outside, or in a crowded hallway.

---

## 🛠️ Installation & Development Guide

This section details how to set up the Arduino IDE to compile and flash the GaitOS firmware.

### 1. Install Arduino IDE

Download and install the latest **Arduino IDE (2.0+)** from [arduino.cc](https://www.arduino.cc/en/software).

### 2. Install ESP32 Board Support (Critical)

The M5StickC Plus 2 is based on the ESP32 chip. You must install the ESP32 board definitions to access features like **WiFi**, **WebServer**, and **LittleFS**.

1. Open Arduino IDE $\rightarrow$ **File** $\rightarrow$ **Preferences**.
2. In "Additional Boards Manager URLs", add:
    `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. Go to **Tools** $\rightarrow$ **Board** $\rightarrow$ **Boards Manager**.
4. Search for **"esp32"** (by Espressif Systems) and install the latest version (3.0+ recommended).

### 3. Install Required Libraries

These libraries are **not** installed by default and must be added via the **Library Manager**.

1. Go to **Tools** $\rightarrow$ **Manage Libraries...** (Ctrl+Shift+I).
2. Search for and install the following **exact** libraries:

| Library Name | Author | Purpose |
| :--- | :--- | :--- |
| **M5Unified** | M5Stack | Core hardware abstraction (Screen, IMU, Power). |
| **M5StickCPlus2** | M5Stack | Specific drivers for the Plus 2 model. |

*Note: Libraries like `WiFi`, `WebServer`, and `LittleFS` are automatically installed with the ESP32 Board Support package (Step 2).*

### 4. Flashing the Firmware

use the `gait.ino` file for the Codebase.

1. **Select Board**: Go to **Tools** $\rightarrow$ **Board** $\rightarrow$ **ESP32 Arduino** $\rightarrow$ **M5Stick-C-Plus2**.
2. **Settings**:
    * **Upload Speed**: 1500000 (1.5Mbps) for speed, or 115200 for reliability.
    * **Partition Scheme**: Default (usually "Default 4MB with spiffs").
3. **Connect**: Plug in your M5StickC Plus 2 via USB-C. Select the correct **Port**.
4. **Upload**: Click the arrow button (→) to compile and flash.

*Troubleshooting*: If upload fails, hold the small **BtnB** (Side button) while plugging in USB to enter Bootloader mode.

---

## 📚 Evidence & Documentation Roadmap

We provide full transparency into the math, code, and validation.

### 🔬 Validation Gallery

| **Drift Cancellation** | **Stance Logic** | **Gait Stability** |
| :---: | :---: | :---: |
| ![Drift](assets/proof_drift.png) | ![Logic](assets/proof_stance.png) | ![Stability](assets/proof_stability.png) |
| *ZUPT vs Raw Integration* | *Gyroscope Thresholds* | *Healthy vs Ataxic Gait* |

---

## 🔬 Scientific Principles (The "Hybrid Engine")

GaitOS V13 uses a **Strapdown Inertial Navigation System (SINS)** aided by **Zero Velocity Updates (ZUPT)**.

### 1. Orientation Estimate (Madgwick Filter)

We fuse Accelerometer ($a$) and Gyroscope ($\omega$) data to compute the orientation Quaternion ($q$):

$$
\dot{q}_{est} = \dot{q}_{\omega} - \beta \frac{\nabla f}{\| \nabla f \|}
$$

Where $\beta$ is the gain (tuned to 0.5) and $\nabla f$ is the gradient descent direction ensuring the gravity vector points DOWN ($[0,0,1]$).

### 2. Gravity Compensation

Linear acceleration ($a_{lin}$) is isolated by subtracting gravity ($g$):

$$
a_{lin}^n = R(q) \cdot a_{measured}^b - [0, 0, 1]^T \cdot 9.81
$$

### 3. The ZUPT Hypothesis

We assume that during the **Stance Phase** (foot flat), velocity is zero.

* **Condition**: $\|\omega\| < 40^\circ/s$ AND $\|a_{lin}\| < 0.2g$.
* **Action**: If Stance detected, $v_k = [0,0,0]$. This "clamps" the integration drift.

### 4. Hybrid Validation (V13 Novelty)

To assist the physics engine, V13 applies empirical gates:

* **Time Gate**: $\Delta t_{step} > 300ms$ (Rejects micro-vibrations).
* **Amplitude Gate**: $a_{peak} > 1.2g$ (Requires physical lift).
* **Stability Index ($SI$)**:

$$
SI = 100 - |Cadence_{instant} - Cadence_{avg}|
$$

(High SI = Rhythmic, Healthy Gait. Low SI = Ataxic/Irregular Gait).

---

## 👥 Authors & Acknowledgements

**Author**: Chandrashekhar Hegde
**License**: MIT (Open Source)

*Special thanks to the Open Source Biomechanics community for the foundational ZUPT research.*
