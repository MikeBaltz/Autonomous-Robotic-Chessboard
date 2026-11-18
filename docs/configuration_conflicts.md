# Configuration Conflicts

This document records discrepancies between the active implementation and
other source or documentation. Conflicts are intentionally not resolved here.
The active values below are taken from `sketches/movement/src/main.cpp` and
`software/chess_bridge/main.py`.

## 1. Motor axis GPIO labels

**Active value:**

- X STEP/DIR/EN = GPIO 8/9/10.
- Y STEP/DIR/EN = GPIO 16/17/18.

**Conflicting value:** `include/config.h` labels X as GPIO 16/17/18 and Y as
GPIO 8/9/10. `docs/hardware.md` also labels GPIO 8/9/10 as X and
GPIO 16/17/18 as Y, so the documentation agrees with the active movement
sketch but conflicts with the legacy root firmware labels.

**Sources:**

- Active: `sketches/movement/src/main.cpp`
- Legacy: `include/config.h:24-32`
- Documentation: `docs/hardware.md` (pin table)

**Recommended resolution:** Declare the movement sketch mapping authoritative
for the working hardware and rename or clearly mark the root firmware mapping
as legacy.

**Hardware verification required:** Yes. Confirm which physical axis each
driver moves before changing any source.

## 2. Electromagnet GPIO

**Active value:** GPIO39, direct HIGH/LOW control in the movement firmware.

**Conflicting value:** Older ADR and experiment documentation specify GPIO15.

**Sources:**

- Active: `sketches/movement/src/main.cpp`
- Current hardware document: `docs/hardware.md`
- Older design: `docs/decisions/ADR-005-electromagnet-actuator.md:50-61`
- Older experiment: `docs/experiments/EXP-005-electromagnet-control-test.md:20-27`

**Recommended resolution:** Confirm the physical wiring and retain one
authoritative pin map. Do not change firmware until the installed hardware is
identified.

**Hardware verification required:** Yes.

## 3. Steps per square

**Active value:** 2220 steps per column and row.

**Conflicting value:** The legacy root firmware uses 2611 steps per axis.
Several older sketches also use 2611.

**Sources:**

- Active: `sketches/movement/src/main.cpp`
- Legacy: `src/motion.h:49-50`
- Current documentation: `docs/hardware.md` (calibration)
- Older examples: `sketches/engine_simulation/src/main.cpp`,
  `sketches/piece_offset/src/main.cpp`

**Recommended resolution:** Retain 2220 as the active calibration only after
recording the measurement procedure and final mechanical pitch in one source.
Mark 2611-only sketches as historical.

**Hardware verification required:** Yes. Measure repeatability across the full
travel, not just a single square.

## 4. Coordinate origin and square mapping

**Active value:** Square centers are zero-based:

```text
a1 = (0,0), h8 = (7,7), startup = (3.5,3.5)
```

**Conflicting values:**

- Legacy root firmware maps rows to `1..8` and starts at `(3.5,4.5)`.
- Older sketches use one-based coordinates and often start at `(4.5,4.5)`.
- `docs/hardware.md` uses the active `(3.5,3.5)` and notes the older convention.

**Sources:**

- Active: `sketches/movement/src/main.cpp`
- Legacy: `src/motion.h:51-52`, `src/motion.cpp:35-44`
- Documentation: `docs/hardware.md` (board layout)

**Recommended resolution:** Treat the movement sketch's zero-based square
center mapping as authoritative for the bridge and document the physical
meaning of `(3.5,3.5)` explicitly.

**Hardware verification required:** Yes. Establish the physical `a1` center
and verify all four central squares.

## 5. Surround and capture-lane geometry

**Active value:**

- Left lane: X=-0.5.
- Right lane: X=7.5.
- Slot Y: 0 through 7.

**Conflicting value:** The movement sketch still declares an unused
`SURROUND_OFFSET = 0.25`, and EXP-006 documented a 0.25-unit surround
(positions 0.25 and 8.75 in one-based units). Several older sketches also use
0.25 and 8.75.

**Sources:**

- Active firmware: `sketches/movement/src/main.cpp`
- Active bridge: `software/chess_bridge/main.py`
- Documentation: `docs/hardware.md` (board layout), EXP-006
- Older sketches: `sketches/bishop_diagonal/src/main.cpp`,
  `sketches/knight_offset/src/main.cpp`

**Recommended resolution:** Measure the actual board edge and carriage
clearance, then select one lane geometry for both Python and firmware.

**Hardware verification required:** Yes. Lane coordinates can affect piece
collisions and carriage travel limits.

## 6. Hall sensor offset behavior

**Active value:** 15.0 mm Y offset, converted with 45.1 mm pitch. `scan` and
`scanpos` always call `moveToSensorPosition()`, which subtracts the offset.

**Conflicting value:** The legacy root firmware scans directly at the square
coordinate and has no equivalent active offset in `Motion::scanSquare()`.

**Sources:**

- Active: `sketches/movement/src/main.cpp`
- Legacy: `src/motion.cpp:58-68`
- Documentation: `docs/hardware.md` (calibration)

**Recommended resolution:** Keep the offset associated with the active
movement firmware and measure it from the final sensor/electromagnet mount.

**Hardware verification required:** Yes.

## 7. Motor speed and acceleration

**Active value:** 1000 steps/s with up to 100-step linear acceleration and
deceleration in the primary axis movement functions. Diagonal movement uses
fixed 500 us pulse delays.

**Conflicting value:** Legacy root firmware uses 500 steps/s without
acceleration. `include/config.h` defines 500 as a default, but the movement
sketch does not include that configuration.

**Sources:**

- Active: `sketches/movement/src/main.cpp`
- Legacy: `src/motion.h:53`, `src/motor.cpp:53-72`
- Unused/config-only value: `include/config.h:34-36`

**Recommended resolution:** Record speed as a mechanically validated limit,
then centralize it after hardware characterization.

**Hardware verification required:** Yes. Validate missed-step margin under the
actual carriage and payload.

## 8. Serial protocol status

**Active value:** Newline-delimited commands with `DONE` on successful
completion and `ERROR: ...` on rejected input. Scan commands additionally
return `SCAN` or `SCANPOS` records.

**Status: resolved in documentation.** `docs/communication.md` now documents
the implemented text protocol (including `g`, `path`, `scan`, `scanpos`,
`lane`) and labels `PING`, `STATUS`, `HOME`, `SCAN`, `MOVE`, and `STOP` as a
draft future protocol.

**Sources:**

- Active firmware: `sketches/movement/src/main.cpp`
- Active bridge: `software/chess_bridge/main.py`
- Protocol document: `docs/communication.md`

**Recommended resolution:** Mark the current text protocol as the prototype
protocol and separately label the draft protocol as future work.

**Hardware verification required:** No for command syntax; yes for response
timing and long-running motion behavior.

## 9. Python `lane` command usage

**Active value:** Firmware supports `lane L1` through `lane R8`.

**Conflicting assumption:** The bridge README lists `lane`, but
`MoveExecutor` currently generates explicit `g` and `path` commands instead.

**Sources:**

- Firmware: `sketches/movement/src/main.cpp`
- Bridge planner: `software/chess_bridge/main.py`
- Bridge documentation: `software/chess_bridge/README.md`

**Recommended resolution:** Either document `lane` as a manual positioning
primitive only or change the planner later after confirming equivalent safety.

**Hardware verification required:** Yes before relying on lane routing.

## 10. Root firmware versus active bridge

**Active value:** The bridge targets `sketches/movement/` and its `DONE`,
`scan`, `scanpos`, `g`, `path`, and UCI behavior.

**Conflicting value:** The root `src/` firmware has a separate protocol and
does not emit the exact `DONE` responses required by `ArduinoBridge.execute()`.

**Sources:**

- Active build target: `sketches/movement/platformio.ini`
- Bridge: `software/chess_bridge/main.py`
- Root protocol: `src/protocol.cpp:13-91`

**Recommended resolution:** Explicitly designate `sketches/movement/` as the
only active physical firmware and retain `src/` as legacy until it is either
retired or brought into protocol alignment.

**Hardware verification required:** No for source compatibility; yes before
selecting a firmware for physical deployment.

## 11. Final board and homing configuration

**Active value:** No homing routine, TMC2209 UART or StallGuard
configuration, emergency stop, or final travel dimensions exist in the active
movement sketch. TMC2209 UART (GPIO6/GPIO5) and DIAG (GPIO7/GPIO15) are wired
and exercised only by `sketches/tmc_uart_test/`.

**Conflicting value:** Hardware and status documents describe StallGuard
sensorless homing (replacing endstop switches) as in progress. With Y's `MS1`
tied high for UART addressing, the movement sketch runs the Y driver at 1/32
microstepping until it configures UART.

**Sources:**

- Active source: `sketches/movement/src/main.cpp`
- Hardware planning: `docs/hardware.md` (Homing)
- Driver planning: `docs/decisions/ADR-004-motion-driver.md`
- UART bring-up: `sketches/tmc_uart_test/`

**Recommended resolution:** Treat homing and final travel configuration as the
next engineering baseline task, not as current functionality.

**Hardware verification required:** Yes.
