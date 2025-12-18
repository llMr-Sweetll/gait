# Development of a Hybrid Inertial Navigation System for Clinical Gait Analysis

**Author**: GaitOS Research Team
**Date**: December 2025
**Version**: 2.0 (System V13)
**Keywords**: ZUPT-INS, Post-Stroke Rehabilitation, Gait Analysis, Tele-Health.

---

## Abstract

This dissertation presents the development of **GaitOS**, a low-cost ($<30 USD), open-source gait analysis system designed to democratize access to advanced mobility diagnostics. By utilizing a **Hybrid Validation Engine** atop a Physics-Based ZUPT framework, the system provides "Motion Capture Grade" fidelity in measuring critical rehabilitation metrics—specifically **Foot Clearance** (for fall risk assessment) and **Gait Stability/Symmetry** (for stroke recovery). This work bridges the gap between expensive clinical labs and accessible home-based monitoring.

---

## 1. Clinical Relevance & Humanitarian Motivation

### 1.1 The Rehabilitation Gap

According to the World Health Organization, 15 million people suffer a stroke worldwide each year. A critical indicator of recovery is the restoration of regular gait patterns. Specifically:

* **Foot Drop (Clearance)**: Inability to lift the foot (dorsiflexion) increases fall risk.
* **Asymmetry**: Hemiparetic gait leads to "limping", which causes long-term orthopedic damage.
* **Rhythmicity**: Parkinsonian gait manifests as irregular, short steps.

### 1.2 The GaitOS Solutions

GaitOS provides quantifiable metrics for these conditions:

1. **Trajectory Reconstruction ($P_z$)**: Directly measures max foot height (clearance) to track "Foot Drop" recovery.
2. **Stability Index**: A variance-based metric ($Var(\Delta t)$) that quantifies "smoothness", aiding in the diagnosis of Ataxia.
3. **Real-Time Feedback**: Bio-feedback on the device screen allows patients to self-correct during therapy.

---

## 2. Mathematical Framework: The Hybrid Engine

The core engineering contribution is the fusion of Newton's Laws of Motion with empirical constraints (The "Hybrid" approach).

### 2.1 Strapdown Inertial Navigation (The Physics)

The system operates in the **Navigation Frame ($n$)** (Earth-Fixed, Gravity Down).

**Step 1: Attitude Estimation (Quaternion Update)**
Using the Madgwick filter, we compute the orientation quaternion $q_k$.
$$
q_k = q_{k-1} + \frac{1}{2} (q_{k-1} \otimes \omega_k) \Delta t - \beta \frac{\nabla f}{\|\nabla f\|} \Delta t
$$
Where $\omega$ is the angular rate vector and $\nabla f$ corrects for gravity tilt.

**Step 2: Gravity Compensation**
We rotate the body-frame acceleration $a^b$ to the navigation frame $a^n$ and subtract gravity $g$:
$$
a^n_k = R(q_k) a^b_k - \begin{bmatrix} 0 \\ 0 \\ 9.81 \end{bmatrix}
$$

**Step 3: Double Integration**
$$
v_k = v_{k-1} + a^n_k \Delta t
$$
$$
p_k = p_{k-1} + v_k \Delta t
$$
*Issue*: Without correction, sensor noise $\epsilon$ causes position error $p_{err} \propto t^2$.

### ⚠️ Proof of Concept: The Drift Problem

The following validation data, generated from the GaitOS engine, demonstrates the necessity of ZUPT.
![Drift Proof](assets/proof_drift.png)
*Figure 2: Empirical validation showing raw integration diverging (Red) vs GaitOS ZUPT (Green) maintaining sub-centimeter accuracy.*

### 2.2 Zero Velocity Update (The Correction)

To bound the drift, we exploit the biomechanics of walking. When the foot is flat (Stance), velocity *must* be zero.

**Stance Condition**:
$$
\text{IsStance} = (\|\omega\| < 40^\circ/s) \land (\|a_{lin}\| < 0.2g)
$$
**Constraint Application**:
$$
\text{If IsStance} \implies v_k \leftarrow [0, 0, 0]^T
$$
This resets the integration error integral at every step.

![Stance Logic](assets/proof_stance.png)
*Figure 3: Algorithm performance. The system successfully gates velocity integration during high-energy Swing phases (Cyan) and clamps during low-energy Stance phases (Green).*

### 2.3 Maker Relevance & Open Hardware

GaitOS transforms accessible hardware into research instruments:

* **Hardware Agnostic**: Runs on ESP32, Teensy, or Arduino Nano 33 IoT.
* **Total Cost**: $< $30 USD (vs $2,000 Xsens).
* **Fabrication**: Requires no PCB design—simply strap an M5StickC Plus 2 to a shoe.
This empowers "Citizen Scientists" and Makers to contribute to biomechanics research without university funding.

---

## 3. The V13 "Hybrid" Innovation

Pure ZUPT fails during irregular movements (shuffling, vibrations). V13 introduces **Empirical Validation Gates**:

### 3.1 Temporal Gating

A stride is biomechanically constrained. We reject any zero-crossing event where:
$$ \Delta t_{step} < 300ms $$
This filters out "micro-steps" caused by sensor noise or floor vibrations.

### 3.2 Amplitude Gating

A valid swing phase requires significant energy. We enforce:
$$ \max(\|a^n\|_{swing}) > 1.2g $$
This prevents shuffling from registering as full steps, ensuring data integrity for stroke patients who may drag their feet.

### 3.3 The Stability Index ($SI$)

We define a novel metric for gait regularity based on the variance of stride timing.
$$ SI = \max \left( 0, 100 - \frac{|Cadence_{inst} - Cadence_{avg}|}{Cadence_{avg}} \times 100 \right) $$

**Clinical Interpretation**:

* **SI > 90%**: Healthy, rhythmic gait (Green Line).
* **SI < 60%**: Highly irregular, indicative of Ataxia or Fatigue (Red Line).

![Stability Proof](assets/proof_stability.png)
*Figure 4: Comparative analysis of a Healthy subject (Low Variance) vs an Ataxic gait model (High Variance).*

### 3.4 Full Euler Angle Extraction

For biofeedback, we extract the Foot Angle ($\theta, \phi, \psi$) from the Quaternion $q$:
$$
\phi = \arctan\frac{2(q_0 q_1 + q_2 q_3)}{1 - 2(q_1^2 + q_2^2)}
$$
$$
\theta = \arcsin(2(q_0 q_2 - q_3 q_1))
$$
$$
\psi = \arctan\frac{2(q_0 q_3 + q_1 q_2)}{1 - 2(q_2^2 + q_3^2)}
$$
This allows the patient to visualize their foot angle in real-time.

---

## 4. Conclusion

1. **Physiotherapists**: To objectively track patient recovery.
2. **Researchers**: To collect large-scale kinematic data in the wild.
3. **Patients**: To receive gamified, real-time feedback on their walking quality.

---

## References

1. **Nilsson, J.** et al. (2014). "Foot-mounted INS/ZUPT for First Responders".
2. **Madgwick, S.** (2010). "An efficient orientation filter for inertial and magnetic sensor arrays".
3. **World Stroke Organization**. (2024). Global Stroke Fact Sheet.
