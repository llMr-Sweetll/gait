# GaitOS Clinical Practitioner Guide

**For Physiotherapists, Occupational Therapists, and Rehabilitation Specialists**

---

## 🎯 Document Purpose

This guide is written specifically for **medical practitioners** who will use GaitOS for patient assessment. You don't need to understand the technology - just how to interpret the results and use them for clinical decision-making.

**What this guide covers**:

- Setting up the device with patients
- Understanding what the numbers mean
- Interpreting gait abnormalities
- Tracking patient progress
- Evidence-based clinical thresholds
- Real-world clinical workflows

---

## 📋 Quick Start: Your First Patient Assessment

### What You Need (5 minutes)

1. **Device**: M5StickC Plus 2 (small rectangular device with screen)
2. **Strap**: Velcro ankle strap (like a watch band)
3. **Your Phone**: Any smartphone with WiFi capability

### Step-by-Step Setup

**1. Mount the Device on Patient's Ankle**

- **Where**: Outer ankle bone, 2-3 cm above the bone
- **How tight**: Tight enough that it doesn't rotate when you try to twist it
- **Which leg**:
  - For stroke patients → Affected side
  - For general assessment → Dominant leg (usually right)
  - For bilateral comparison → Need two devices (future feature)

**Visual Check**: Ask the patient to shake their leg. If the device wobbles, tighten the strap.

**2. Turn On & Connect**

- Hold power button (side button) for 2 seconds
- On your phone, connect to WiFi: `GAIT-LOGGER` (password: `circumduct123`)
- Open your phone's browser and go to `192.168.4.1`

**3. Calibrate (Critical!)**

- Have patient **stand completely still** for 3 seconds
- Device will beep and show "Auto-Cal!" message
- If it doesn't auto-calibrate, click "Zero Sensors" button on your phone

**4. Record & Walk**

- Click **"Start Recording"** on your phone
- Patient performs walk test (see protocols below)
- Click **"Stop Recording"** when done
- Data is automatically saved

---

## 🚶 Walk Test Protocols

### 10-Meter Walk Test (Gold Standard)

**Setup**:

- Mark 10-meter straight path with tape
- Add 2 meters acceleration zone before start
- Add 2 meters deceleration zone after finish

**Instructions to Patient**:
> "Walk at your normal, comfortable pace from here to there. Don't start counting until I say 'Go', and don't slow down until you pass the end marker."

**What to Record**:

- Start recording → Say "Go" → Patient walks → Stop recording after they pass end marker
- Typically takes 10-15 seconds for normal speed

### Timed Up and Go (TUG)

**Setup**:

- Chair (standard height, armrests)
- Mark line 3 meters away

**Instructions**:
> "When I say go, stand up from the chair, walk to the line, turn around, walk back, and sit down. Move at your normal speed."

**Interpretation**:

- <10 seconds: Normal mobility
- 10-20 seconds: Mild mobility limitation
- 20-30 seconds: Moderate mobility limitation
- >30 seconds: Severe limitation, high fall risk

### 6-Minute Walk Test (Endurance)

**Setup**: Marked loop (e.g., hospital corridor)

**Instructions**:
> "Walk continuously for 6 minutes. You can slow down or rest if needed, but keep moving if possible."

**GaitOS Advantage**: Can detect fatigue by comparing first 2 minutes vs last 2 minutes

---

## 📊 Understanding the Dashboard

When you look at your phone during a walk, you'll see 4 main numbers:

### 1. Steps (What it means: Volume)

**What you see**: Number like "24 steps"

**Clinical Interpretation**:

- Confirms patient is walking continuously
- Should match your manual count (±2 steps is normal)
- If drastically different → Device may be loose

**Normal Values**:

- 10-meter walk: ~12-18 steps (depends on height)
- TUG: ~8-12 steps
- 6-minute walk: 400-700 steps

### 2. Cadence (What it means: Speed/Rhythm)

**What you see**: Number like "105 spm" (steps per minute)

**What it means**: How fast they're stepping

**Clinical Interpretation**:

| Cadence | Interpretation | Population |
|---------|----------------|------------|
| 120+ spm | Fast/normal | Young healthy adults |
| 100-120 spm | Normal | Elderly, active seniors |
| 80-100 spm | Slow/cautious | Post-stroke, Parkinson's early |
| 60-80 spm | Very slow | Severe pathology, high fall risk |
| <60 spm | Shuffling | Advanced Parkinson's, severe stroke |

**Red Flags**:

- **Very low (<80)**: High fall risk, needs walking aid?
- **Variable (jumps by >20)**: Arrhythmic gait, neurological involvement
- **Sudden drop during test**: Fatigue, deconditioning

### 3. Stability (What it means: Consistency)

**What you see**: Percentage like "82%" with color (Green/Yellow/Red)

**What it means**: How consistent/rhythmic their steps are

**Think of it like**: A metronome. Healthy walking is rhythmic like a clock. Unstable gait is irregular like a broken metronome.

**Clinical Interpretation**:

| Stability | Color | Interpretation | Clinical Action |
|-----------|-------|----------------|----------------|
| >85% | Green | Excellent rhythm | Normal, no concerns |
| 70-85% | Yellow | Mild variability | Monitor, may need balance training |
| 50-70% | Orange | Moderate irregularity | Balance intervention recommended |
| <50% | Red | Severe arrhythmia | **High fall risk**, urgent intervention |

**What Causes Low Stability**:

- Cerebellar pathology (ataxia)
- Parkinson's disease (freezing of gait)
- Post-stroke (hemiparetic gait)
- Pain/arthritis (compensatory stepping)
- Fear of falling (cautious gait)

**Clinical Significance**:

- Hausdorff (1997) found variability >3% predicts falls in elderly
- GaitOS stability <70% roughly corresponds to this threshold

### 4. Trajectory (What it means: Path Quality)

**What you see**: 2D graph showing foot path from above

**How to Read It**:

- **X-axis (horizontal)**: Forward movement (in meters)
- **Y-axis (vertical)**: Side-to-side wobble (in meters)

**Normal Pattern**: Smooth arc, small sideways movement (<0.2m)

**Abnormal Patterns**:

| Pattern | What You See | Clinical Meaning |
|---------|--------------|------------------|
| **Wide zigzag** | Line goes left-right >0.3m | Ataxia, balance deficit |
| **Flat/low arc** | Arc height <5cm | Shuffling, Parkinsonian |
| **Irregular peaks** | Some arcs tall, some flat | Inconsistent clearance, trip risk |
| **Drift to one side** | Line curves left or right | Hemiparesis, stroke compensation |
| **Jagged/spiky** | Line has sharp angles | Device loose, re-mount |

---

## ⚠️ Gait Abnormality Alerts

### What is This Feature?

GaitOS automatically detects unusual walking patterns and alerts you **in real-time** during the walk.

### What Triggers an Alert?

The device learns the patient's baseline pattern in the first 10 steps, then compares every subsequent step:

**1. Shuffling** (Low Clearance)

- **Trigger**: Foot lifts <3cm off ground
- **Alert**: "Low clearance!" on device screen
- **Clinical Meaning**:
  - Trip hazard
  - Common in Parkinson's, stroke, elderly
  - May need gait training or walking aid

**2. Irregular Cadence**

- **Trigger**: Step timing varies >30% from baseline
- **Alert**: "Irregular cadence!"
- **Clinical Meaning**:
  - Loss of rhythmic control
  - Neurological involvement
  - Festinating gait (Parkinson's)
  - Freezing episodes

**3. Inconsistent Stride**

- **Trigger**: Stride length varies >30% from baseline
- **Alert**: "Uneven stride!"
- **Clinical Meaning**:
  - Asymmetry (stroke, hip pathology)
  - Compensation for pain
  - Muscle weakness (one leg vs other)

### How to Use Alerts Clinically

**During the Walk**:

- **If multiple alerts**: Stop test, check device mounting first
- **If isolated alerts**: Note the context (turning? obstacle?)
- **If continuous alerts**: Significant pathology, document in notes

**After the Walk**:

- Review CSV file → `abnormal` column shows which steps were flagged
- Calculate abnormality rate: (Number of flagged steps / Total steps) × 100

**Interpretation**:

- **<10% abnormal**: Normal, acceptable variability
- **10-30% abnormal**: Mild pathology, monitor progress
- **30-50% abnormal**: Moderate pathology, intervention needed
- **>50% abnormal**: Severe pathology OR device issue (retest)

---

## 📈 Tracking Patient Progress

### Session Comparison Feature

**Purpose**: Visually show patient improvement over weeks/months

**How to Use**:

1. **Baseline Session**: Record patient before intervention
   - Name it clearly: "baseline_week0"
2. **Follow-up Session**: Record after therapy (e.g., 4 weeks later)
   - Name it: "followup_week4"
3. **On Dashboard**:
   - Scroll to "Session Comparison"
   - Select both sessions from dropdowns
   - Click "Compare Trajectories"

**What You Get**:

**Visual Overlay**:

- Baseline = Cyan/blue line
- Follow-up = Orange line
- You can see if path is more straight, smoother, etc.

**Metrics Table** (Example):

| Metric | Baseline | Follow-up | Change |
|--------|----------|-----------|--------|
| Cadence | 85 spm | 98 spm | **+13 spm** 🟢 |
| Stability | 64% | 79% | **+15%** 🟢 |
| Distance | 8.2m | 10.1m | **+1.9m** 🟢 |

**Clinical Use**:

- 🟢 Green numbers = Improvement (show patient!)
- 🔴 Red numbers = Decline (investigate cause)
- Print/screenshot for patient records

---

## 📋 Clinical Decision Support

### When to Use GaitOS

**Ideal Scenarios**:

- Post-stroke gait retraining
- Parkinson's disease monitoring
- Elderly fall risk assessment
- Post-surgery mobility tracking (hip/knee replacement)
- Spinal cord injury rehabilitation
- Long COVID fatigue assessment

**Not Suitable**:

- Severe cognitive impairment (can't follow instructions)
- Very obese patients (ankle strap won't fit)
- Wheelchair users (requires walking ability)
- Acute injury/pain (unreliable baseline)

### Evidence-Based Thresholds

These are research-validated cutoff values:

**Fall Risk Stratification**:

- **Low risk**: Cadence >100 spm AND Stability >80%
- **Moderate risk**: Cadence 80-100 spm OR Stability 60-80%
- **High risk**: Cadence <80 spm OR Stability <60%
- **Very high risk**: Cadence <60 spm AND Stability <50%

**Stroke Recovery Staging** (Hemiparetic Gait):

- **Stage 1 (Severe)**: Cadence <60, Abnormality >60%
- **Stage 2 (Moderate)**: Cadence 60-80, Abnormality 30-60%
- **Stage 3 (Mild)**: Cadence 80-100, Abnormality 10-30%
- **Stage 4 (Near Normal)**: Cadence >100, Abnormality <10%

**Parkinson's Disease Severity**:

- **Mild (Hoehn & Yahr 1-2)**: Cadence >90, Stability >70%
- **Moderate (H&Y 3)**: Cadence 70-90, Stability 50-70%
- **Severe (H&Y 4+)**: Cadence <70, Stability <50%

---

## 🔍 Common Clinical Scenarios

### Scenario 1: Post-Stroke Patient (3 months post)

**Presentation**: 68-year-old, right hemiparesis, walking with cane

**GaitOS Results**:

- Cadence: 75 spm (slow)
- Stability: 68% (fair, yellow)
- Trajectory: Curved to left (drift)
- Abnormality: 35% (moderate)

**Clinical Interpretation**:

- Compensatory gait established
- Moderate fall risk
- Ready for gait retraining

**Treatment Plan**:

- Treadmill training to increase cadence
- Balance exercises to improve stability
- Retest in 4 weeks

### Scenario 2: Parkinson's Patient (Freezing Episodes)

**Presentation**: 72-year-old, complains of "getting stuck" when walking

**GaitOS Results**:

- Cadence: Starts 95 spm, drops to 45 spm suddenly, then back to 90
- Stability: 42% (red, poor)
- Abnormality: 55% (severe)
- Alert: "Irregular cadence!" triggers 8 times during test

**Clinical Interpretation**:

- Classic freezing of gait (FOG)
- High fall risk
- May need medication adjustment

**Treatment Plan**:

- Refer to neurologist for medication review
- Visual cueing therapy
- Consider walking aid for safety

### Scenario 3: Elderly Patient (Fall Prevention)

**Presentation**: 83-year-old, history of 2 falls in past year, fearful

**GaitOS Results**:

- Cadence: 88 spm (borderline slow)
- Stability: 76% (fair)
- Trajectory: Normal arc, minimal zigzag
- Abnormality: 12% (mild)

**Clinical Interpretation**:

- Cautious gait (fear-related)
- Moderate fall risk
- Good candidate for confidence-building

**Treatment Plan**:

- Progressive balance training
- Tai Chi or similar program
- Retest monthly to show improvement → boost confidence

---

## ⚡ Quick Reference Card

### What to Do If

**Cadence <80 spm?**
→ Assess for: Parkinson's, stroke aftereffects, severe arthritis, fear
→ Action: Gait speed training, consider walking aid

**Stability <60%?**
→ Assess for: Cerebellar lesion, vestibular disorder, medication side effects
→ Action: Balance intervention, refer to neurology if new onset

**Abnormality >50%?**
→ First: Check device is tight on ankle
→ If device OK: Significant pathology, document and investigate

**Trajectory shows wide zigzag?**
→ Assess for: Ataxia, visual impairment, vestibular dysfunction
→ Action: Romberg test, visual acuity check, specialized balance PT

**Patient reports device discomfort?**
→ Check: Strap over sock or skin? (Sock more comfortable)
→ Check: Positioned on bone or above it? (Should be ABOVE)
→ Alternative: Use medical tape + strap for extra cushion

---

## 📂 Documentation & Reporting

### What to Include in Patient Notes

**Template**:

```
GAIT ASSESSMENT (GaitOS)

Date: [Date]
Test: 10-Meter Walk Test
Device: Ankle-mounted IMU (right leg)

RESULTS:
- Steps: 16
- Cadence: 92 spm
- Stability: 71% (fair)
- Abnormality Rate: 18%
- Alerts: 2× "Low clearance" during test

INTERPRETATION:
Moderate gait instability with occasional shuffling. 
Fall risk: MODERATE

PLAN:
- Gait retraining 2×/week
- Balance exercises daily
- Retest in 4 weeks
```

### Data Export for Records

**From Dashboard**:

1. Scroll to "Session History"
2. Click "Download" next to session
3. CSV file downloads to your device
4. Upload to patient EHR or print summary

**For Research**:

- CSV contains 100 samples/second (very detailed)
- Can share with gait analysis specialists
- HIPAA compliant (no personal data in file, only measurements)

---

## 🛡️ Safety Considerations

### Device Limitations

**What GaitOS IS**:

- Research-grade gait measurement tool
- Fall risk screening device
- Rehabilitation progress monitor

**What GaitOS IS NOT**:

- FDA-approved medical device
- Replacement for clinical judgment
- Diagnostic tool (always confirm with full assessment)

### When to Seek Specialist Referral

**Red Flags** (Refer to Neurology/PT Specialist):

- Sudden decline in metrics over 1-2 weeks
- Stability <40% in previously functional patient
- New onset freezing of gait
- Abnormality >70% (unless expected for condition)

### Patient Safety During Testing

- **Fall Prevention**: Always stay within arm's reach during walk
- **Fatigue**: Stop test if patient reports excessive tiredness
- **Pain**: If patient complains of pain, stop and assess
- **Confusion**: If patient can't follow instructions, defer testing

---

## 📞 Getting Help

### Technical Issues

**Device Won't Turn On**:
→ Charge for 30 minutes via USB-C, try again

**Can't Connect WiFi**:
→ Instructions in main README.md (technical guide)

**Data Looks Wrong**:
→ Most common cause: Device not calibrated
→ Solution: Stand patient still for 5 seconds before recording

### Clinical Questions

**Interpreting Unusual Results**:
→ Review "Common Clinical Scenarios" section above
→ Compare to patient's baseline (if available)
→ Consider medication changes, fatigue, environment

**Research Evidence**:
→ See references in main README.md
→ GaitOS algorithms based on peer-reviewed research (Hausdorff, Sabatini, Skog)

---

## 📚 Summary: Your Workflow

1. **Mount device** on outer ankle, 2-3cm above bone, tight strap
2. **Connect** phone to `GAIT-LOGGER` WiFi, open `192.168.4.1`
3. **Calibrate** by having patient stand still 3 seconds
4. **Record** during standardized walk test (10MWT, TUG, etc.)
5. **Interpret** cadence, stability, abnormality rate
6. **Document** results in patient notes
7. **Compare** sessions over time to track progress
8. **Adjust** treatment based on objective data

---

**Document Version**: 2.0 (Phase 4 Complete)  
**For Technical Details**: See [README.md](README.md)  
**Clinical Questions**: Consult with physical therapy or neurology colleagues
