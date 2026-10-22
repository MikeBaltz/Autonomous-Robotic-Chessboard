# EXP-004 — Motor Control Validationation

## Status

Completed

---

## Objective

Validate TMC2209 stepper motor driver control with NEMA17 motor:

- Verify power wiring and current configuration
- Test bidirectional rotation
- Validate step accuracy and speed control
- Confirm thermal stability under load

---

## Equipment

- **Driver:** TMC2209 StepStick V2
- **Motor:** NEMA17 17HS4401
- **Controller:** ESP32-S3 DevKitC
- **Power supply:** 12V DC
- **Capacitor:** 100µF electrolytic (25V rated)
- **Multimeter:** For Vref measurement

---

## Wiring

| TMC2209 Pin | Connection | Notes |
|---|---|---|
| VM | 12V power supply (+) | Motor power |
| GND | Common ground | Connected to ESP32 GND and 12V GND |
| VDD | ESP32 3.3V | Logic power |
| STEP | GPIO16 | Step pulse |
| DIR | GPIO17 | Direction control |
| EN | GPIO18 | Enable (active low) |
| 1A, 1B, 2A, 2B | Motor coils | Per NEMA17 datasheet |

**Critical:** 100µF capacitor soldered across VM and GND pins to protect against voltage spikes.

---

## Procedure

### 1. Initial Setup

- Connected all wiring per diagram above
- Verified common ground between ESP32, TMC2209, and 12V supply
- Connected capacitor across VM and GND
- Powered 12V supply first, then ESP32

### 2. Vref Adjustment

- Measured voltage between Vref pin and GND
- Adjusted potentiometer to 0.75V
- This provides approximately 0.6-0.8A RMS current to motor

### 3. Motor Test Sequence

Uploaded firmware with 4-test sequence:

1. **Forward movement:** 1000 steps at 500 steps/sec
2. **Pause:** 2 seconds
3. **Backward movement:** 1000 steps at 500 steps/sec
4. **Pause:** 2 seconds
5. **Fast movement:** 2000 steps at 1000 steps/sec
6. **Pause:** 2 seconds
7. **Slow movement:** 500 steps at 200 steps/sec

### 4. Thermal Monitoring

- Monitored motor temperature during 10-minute continuous operation
- Checked for excessive heating or thermal shutdown

---

## Observations

### Serial Output

```
=== Motor Integration Test ===
[MOTOR] Enabled
--- Test 1: Forward movement ---
[MOTOR] Moving 1000 steps at 500 steps/sec
--- Pause 2 seconds ---
--- Test 2: Backward movement ---
[MOTOR] Moving 1000 steps at 500 steps/sec
--- Pause 2 seconds ---
--- Test 3: Fast movement ---
[MOTOR] Moving 2000 steps at 1000 steps/sec
--- Pause 2 seconds ---
--- Test 4: Slow movement ---
[MOTOR] Moving 500 steps at 200 steps/sec
=== Motor test complete ===
[MOTOR] Disabled
```

### Physical Observations

- **Motion quality:** Smooth, quiet operation in both directions
- **Noise level:** Low (StealthChop mode active)
- **Vibration:** Minimal, consistent across all speeds
- **Temperature:** Warm but not hot after 10 minutes (acceptable operating temperature)
- **Step accuracy:** No missed steps observed at any speed
- **Direction control:** Correct reversal when DIR pin toggled

---

## Results

**All validation tests passed:**

✓ Power wiring correct and stable
✓ Current configuration safe (Vref = 0.75V)
✓ Bidirectional rotation working
✓ Step accuracy validated at multiple speeds
✓ Thermal stability confirmed
✓ No missed steps or stalling

---

## Lessons Learned

1. **Capacitor is mandatory** — First TMC2209 driver was damaged due to voltage spikes. Always use 100µF capacitor across VM and GND.

2. **Common ground is critical** — ESP32 GND, TMC2209 GND, and 12V supply GND must all be connected together.

3. **Vref adjustment is important** — Too high causes overheating, too low causes missed steps. 0.75V provided good balance.

4. **Power sequencing** — Apply 12V motor power before or simultaneously with ESP32 logic power to avoid driver confusion.

---

## Next Actions

- Mount motor on carriage mechanism
- Add second motor for XY motion
- Implement CoreXY kinematics
- Test coordinated motion between two motors
- Configure UART mode for advanced features (StallGuard, dynamic current control)
