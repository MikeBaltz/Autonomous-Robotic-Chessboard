# ADR-004 — Motion Driver

## Status

Accepted — physical motor validation completed

---

## Context

The carriage requires precise, quiet, and reliable stepper motor control for XY motion. The driver must support current regulation and microstepping.

---

## Decision

Selected driver: **TMC2209 StepStick V2**
Selected motor: **NEMA17 17HS4401**
Initial operating mode: **STEP / DIR**

UART configuration is planned after basic motor validation.

---

## Considered Options

| Driver | Current regulation | Microstepping | Noise | UART | Stepper-specific |
|---|---|---|---|---|---|
| L298N | No | No | High | No | No (DC motor driver) |
| A4988 | Yes | Yes (1/16) | Medium | No | Yes |
| TMC2209 | Yes | Yes (1/256) | Low | Yes | Yes |

---

## Rationale

The TMC2209 was selected over the L298N and A4988 because:

- **Current regulation** — protects the motor and provides consistent torque.
- **Stepper-specific control** — designed for stepper motors, unlike the L298N.
- **Microstepping** — up to 1/256 for smooth, quiet motion.
- **Lower noise** — StealthChop mode for quiet operation.
- **UART support** — allows runtime configuration of current, microstepping, and StallGuard thresholds.

---

## Initial Configuration

- STEP / DIR mode for basic motion validation.
- UART mode planned for advanced features (StallGuard, dynamic current control).

---

## Wiring

| TMC2209 Pin | Connection | Notes |
|---|---|---|
| VM | 12V power supply | Motor power |
| GND | Common ground | Must connect to ESP32 GND |
| VDD | ESP32 3.3V | Logic power |
| STEP | GPIO16 | Step pulse |
| DIR | GPIO17 | Direction control |
| EN | GPIO18 | Enable (active low) |
| 1A, 1B, 2A, 2B | Motor coils | NEMA17 wiring |

**Critical:** 100µF capacitor required across VM and GND to protect against voltage spikes.

---

## Validation Results

Physical motor validation completed successfully:

- **Vref setting:** 0.75V (adjusted via potentiometer)
- **Operation:** Smooth, quiet motion in both directions
- **Temperature:** Stable after 10 minutes of continuous operation
- **Test sequence:**
  - Forward movement: 1000 steps at 500 steps/sec
  - Backward movement: 1000 steps at 500 steps/sec
  - Fast movement: 2000 steps at 1000 steps/sec
  - Slow movement: 500 steps at 200 steps/sec
- **Result:** All tests passed, motor control validated

See [EXP-004](../experiments/EXP-004-motor-control-validation.md) for detailed experiment report.

---

## Consequences

- TMC2209 requires a 12 V power supply (ordered / received).
- UART wiring adds complexity but enables runtime tuning.
- STEP / DIR mode is sufficient for initial testing.
- Capacitor is mandatory to prevent driver damage.

---

## Next Steps

- Integrate motor control with carriage mechanism
- Add second motor for XY motion
- Implement CoreXY kinematics
- Configure UART mode for advanced features (StallGuard, dynamic current control)
