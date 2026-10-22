# EXP-006 — Movement Calibration

## Status

Completed

---

## Objective

Calibrate the carriage movement system to accurately position the electromagnet and Hall sensor at any chess square.

---

## Setup

- ESP32-S3 controller
- Two NEMA17 stepper motors (X and Y axes)
- TMC2209 stepper drivers
- Hall sensor for magnet detection
- Electromagnet for piece pickup
- 10×10 board layout (8×8 playable + surrounds)

---

## Calibration Process

### Steps per Square

Measured and calibrated to **2220 steps per square** (45.1 mm pitch).

### Hall Sensor Offset

The Hall sensor is mounted 15 mm above the electromagnet on the Y axis. This offset is compensated in software:

- When targeting a square with the electromagnet: position directly at square center
- When targeting a square with the Hall sensor (sensor mode): offset Y by -15mm

### Surround Positions

Surrounds are positioned 0.25 units (11.3 mm) from the playable area edge:

| Surround | Position |
|----------|----------|
| Left | X = 0.25 |
| Right | X = 8.75 |
| Bottom | Y = 0.25 |
| Top | Y = 8.75 |

Note: Top and right surrounds required adjustment to 8.75 to account for the physical board layout.

---

## Movement Commands

| Command | Action |
|---------|--------|
| `a`-`h` | Move to column |
| `1`-`8` | Move to row |
| `m a3` | Move to square |
| `l` | Left surround |
| `r` | Right surround |
| `t` | Top surround |
| `w` | Bottom surround |
| `x` | Reset to center (4.5, 4.5) |
| `s` | Toggle sensor mode |

---

## Results

- Movement is accurate and repeatable
- Sensor mode correctly compensates for Hall sensor offset
- All surround positions verified

---

## Files

- `sketches/movement/` - Movement control sketch
- `sketches/scan_test/` - Skeleton for scanning logic
