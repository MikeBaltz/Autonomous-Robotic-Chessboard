# Development Status

Honest, evidence-based status of every subsystem. Claims are tied to code in this repository or to experiment reports under [experiments/](experiments/). Last reviewed: 2026-10-22.

---

## Implemented (code exists and has been exercised on hardware)

| Subsystem | Evidence |
|---|---|
| ESP32-S3 PlatformIO + Arduino toolchain | `platformio.ini`, all sketches |
| Modular main firmware (sensor/motor/electromagnet/motion/game/protocol/logger/timer) | `src/`, flashed per experiment logs |
| NEMA17 + TMC2209 motor control, both axes, bidirectional | [EXP-004](experiments/EXP-004-motor-control-validation.md), [ADR-004](decisions/ADR-004-motion-driver.md) |
| Two-axis XY gantry motion with square-level targeting | [EXP-006](experiments/EXP-006-movement-calibration.md), `sketches/movement/` |
| Movement calibration (45.1 mm pitch, 2220 steps/square, 15 mm hall offset, surrounds) | [EXP-006](experiments/EXP-006-movement-calibration.md) |
| Hall-effect sensing with debounce (single sensor, GPIO4) | [ADR-006](decisions/ADR-006-single-hall-sensor.md), `src/sensor.*` |
| Electromagnet control via MOSFET (GPIO on/off and PWM) | [EXP-005](experiments/EXP-005-electromagnet-control-test.md), `src/electromagnet.*` |
| Serial command CLIs (positioning, motors, magnet, diagnostics) | `src/protocol.cpp`, `sketches/*/src/main.cpp` |
| UCI move execution (pickup → transport → place → `DONE`) in movement sketch | `sketches/movement/src/main.cpp` |
| Acceleration/deceleration ramps for step generation | `sketches/movement/src/main.cpp` (`moveXSteps`/`moveYSteps`) |
| Virtual chess prototype (move ranking, Stockfish optional, preferences) | [EXP-003](experiments/EXP-003-virtual-chess.md), `software/virtual_chess/` |
| Python unit test suite (21 tests) | `software/virtual_chess/tests/` |
| Chess bridge application (python-chess + pyserial host loop) | `software/chess_bridge/` |

## Experimentally validated (formal reports with measured results)

- Magnetic coupling through the board surface — [EXP-001](experiments/EXP-001-magnetic-coupling.md) (completed; limits not formally characterized)
- Dual-Hall polarity classification — [EXP-002](experiments/EXP-002-dual-hall-sensor.md) (completed; approach later simplified, see ADR-006)
- Motor control validation — [EXP-004](experiments/EXP-004-motor-control-validation.md) (all tests passed; lessons on capacitors, grounding, Vref)
- Movement calibration — [EXP-006](experiments/EXP-006-movement-calibration.md) (movement accurate and repeatable)
- Electromagnet control electronics — [EXP-005](experiments/EXP-005-electromagnet-control-test.md) (control PASSED; physical force FAILED at time of report — see below)

## Under development

- **Full-game integration via `chess_bridge`** — the bridge connects to the board, sends UCI moves and waits for `DONE`. Initial serial sessions have been performed; systematic end-to-end game validation is ongoing.
- **Electromagnet physical holding force** — EXP-005 documents an open power-delivery issue (MOSFET switches correctly; no measurable pull at the time). The movement sketch's pickup/place sequence assumes a working magnet; sustained holding force still requires verification.
- **Bishop diagonal motion strategy** — `sketches/bishop_diagonal/` (interleaved dual-axis stepping with ramps). Experimental; no formal report yet.
- **Knight obstacle-clearance strategy** — `sketches/knight_offset/` (grid-line navigation, optional temporary piece displacement). Experimental; no formal report yet.
- **Board scanning** — `sketches/scan_test/` is a skeleton; the main firmware has `scanSquare`/`detectWhiteMove` (rows 1–2 of the white side) but scanning is not integrated into the game loop.
- **Main-firmware convergence** — `src/` is the modular architectural target, but its motion constants predate the latest calibration (see known issues below). The actively flashed firmware is `sketches/movement/`.

## Planned (not started)

- Endstop switches and homing sequences (open-loop dead-reckoning today)
- Full 64-square scan integrated into the bridge game loop (sensor-verified moves)
- General collision-aware path planning for all piece types
- Automated captures, castling, promotion, and captured-piece removal
- Binary framed serial protocol (CRC, sequence numbers — draft in [communication.md](communication.md))
- CAD / mechanical design file publication
- GUI or web front end; configurable Stockfish strength

---

## Known Issues / Inconsistencies

These are real, tracked discrepancies found in the codebase. They are preserved as-is (not silently "fixed") because resolving them requires physical re-verification against the hardware:

1. **X/Y pin groups transposed in `include/config.h`** — the main firmware lists X = GPIO16/17/18 and Y = GPIO8/9/10, while the calibrated gantry wiring (all movement-family sketches and [hardware.md](hardware.md)) uses X = GPIO8/9/10 and Y = GPIO16/17/18. The main firmware's pin groups must be swapped and re-verified before it drives the gantry.
2. **Stale steps-per-square in the main firmware** — `src/motion.h` uses 2611 steps/square (an earlier calibration era, also present in older sketches), while the current calibrated value is 2220 (EXP-006, movement sketch). Same discrepancy applies to older sketches (`chess_positioner`, `sensor_test`, `dual_motor_test`, `engine_simulation`, `piece_offset`).
3. **Coordinate origin drift across sketches** — `movement` targets squares in zero-based units (`a1` → (0, 0)) after recent refactoring, while `bishop_diagonal`/`knight_offset` still use one-based units (`a1` → (1, 1)). Both compile and run, but the two conventions must not be mixed when porting logic between sketches.
4. **Electromagnet pin history** — EXP-005 and ADR-005 were written when the electromagnet was on GPIO15; the current pin is GPIO39 (see [hardware.md](hardware.md)). Editorial notes added to those documents.
5. **Open-loop start position** — the movement sketch assumes the carriage starts at (3.5, 3.5). If the carriage is moved by hand while powered off, position tracking is wrong until reset.
