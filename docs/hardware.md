# Hardware

---

## Component Status

| Component | Model | Status | Purpose | Notes |
|---|---|---|---|---|
| Microcontroller | ESP32-S3 DevKitC N16R8 | Tested | Main controller | PlatformIO + Arduino framework |
| Hall sensor | Hall-effect module | Tested | Detects magnet presence | GPIO4, active LOW |
| Neodymium magnets | 2×10 mm (pawn) | Tested | Embedded in chess pieces | For piece detection |
| Carriage magnets | 2×10 mm + 2× 3×3 mm | Received | For carriage assembly | Superseded by electromagnet actuation (ADR-005) |
| Stepper driver | TMC2209 StepStick V2 | Tested | Drives NEMA17 motors | Working |
| Stepper motor | NEMA17 17HS4401 | Tested | XY motion | 2220 steps per square |
| 12 V power supply | TBD | Tested | Powers TMC2209 and motors | Working |
| XY gantry | Custom Cartesian: V-slot extrusions, V-wheel carriage, GT2 belts and pulleys | Built (prototype) | XY motion system | Calibrated, see EXP-006 |
| Carriage | Custom | Tested | Holds sensor and electromagnet | See calibration below |
| Electromagnet | Electromagnet actuator | Tested | Piece pickup and placement | GPIO39, active HIGH; holding-force verification in progress (EXP-005) |
| Endstops | TBD | Planned | Homing reference | Not started |
| Breadboard + jumpers | Generic | Tested | Prototyping | Temporary |

---

## Current Pin Assignments

| Pin | Function | Status |
|---|---|---|
| GPIO4 | Hall sensor | In use |
| GPIO8 | X motor STEP | In use |
| GPIO9 | X motor DIR | In use |
| GPIO10 | X motor EN | In use |
| GPIO16 | Y motor STEP | In use |
| GPIO17 | Y motor DIR | In use |
| GPIO18 | Y motor EN | In use |
| GPIO39 | Electromagnet | In use |
| USB | Serial (115200 baud) | In use |

> **Note (verify before using the main firmware):** `include/config.h` currently lists the X/Y motor pin groups transposed (X = 16/17/18, Y = 8/9/10) relative to the calibrated gantry wiring above. The table reflects the wiring used by the movement-family sketches. See [development-status.md](development-status.md).

> **History:** the electromagnet was originally wired to GPIO15 during [EXP-005](experiments/EXP-005-electromagnet-control-test.md); it is now on GPIO39.

---

## TBD Pin Assignments

The following pins have not been assigned yet:

- Endstop X
- Endstop Y

---

## Chess Pieces

- Each piece contains a neodymium magnet.
- Magnet orientation is consistent across all pieces.
- Pawn: 1× 2×10 mm magnet.

---

## Chessboard

- Thin top surface.
- No electronics inside the board.
- Sensors and carriage move underneath.

---

## Carriage

Contains:

- Single Hall sensor (detects magnet presence)
- Electromagnet (piece pickup)
- Mechanical mount
- Cable management

The carriage moves below the chessboard.

---

## Calibration

| Parameter | Value | Notes |
|---|---|---|
| Square pitch | 45.1 mm | Distance between square centers |
| Steps per square | 2220 | X and Y axes (movement sketch, EXP-006) |
| Hall sensor Y offset | 15.0 mm | Sensor is offset from electromagnet; compensated in software when targeting in sensor mode |
| Surround offset | 0.5 units (≈22.6 mm) | Current movement sketch; EXP-006 documented 0.25 units, later widened |

> **Legacy value:** the main firmware (`src/motion.h`) and older sketches still carry an earlier calibration of 2611 steps/square (65 mm column width era). They must be updated to 2220 when the movement logic is consolidated — see [development-status.md](development-status.md).

### Board Layout (zero-based square units, current movement sketch)

- Playable area: 8×8 squares, coordinates 0–7 (`a1` = (0, 0), `h8` = (7, 7))
- Center/home position: (3.5, 3.5) — between files d/e and ranks 4/5
- Surround parking lanes: X = −0.5 and 7.5, Y = −0.5 and 7.5
- Older sketches (`bishop_diagonal`, `knight_offset`, EXP-006 report) use the earlier one-based convention (`a1` = (1, 1), surrounds 0.25/8.75)
