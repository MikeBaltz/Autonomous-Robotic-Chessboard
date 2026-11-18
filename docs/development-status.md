# Development Status

Honest, evidence-based status of every subsystem. Claims are tied to code in this repository or to experiment reports under [experiments/](experiments/). Last reviewed: 2026-11-18.

---

## Implemented (code exists and has been exercised on hardware)

| Subsystem | Evidence |
|---|---|
| ESP32-S3 PlatformIO + Arduino toolchain | `platformio.ini`, all sketches |
| Modular main firmware (sensor/motor/electromagnet/motion/game/protocol/logger/timer) | `src/`, flashed per experiment logs |
| NEMA17 + BigTreeTech TMC2209 V1.3 motor control, both axes, bidirectional (STEP/DIR/EN) | [EXP-004](experiments/EXP-004-motor-control-validation.md), [ADR-004](decisions/ADR-004-motion-driver.md) |
| Cartesian XY gantry (one motor per axis) with square-level targeting | [EXP-006](experiments/EXP-006-movement-calibration.md), `sketches/movement/` |
| Movement calibration (45.1 mm pitch, 2220 steps/square, 15 mm hall offset, surrounds) | [EXP-006](experiments/EXP-006-movement-calibration.md) |
| Single-polarity Hall sensing (GPIO4): black pieces read as occupied, white pieces read as empty | [ADR-006](decisions/ADR-006-single-hall-sensor.md) (update), `scan` / `scanpos` in `sketches/movement/` |
| Electromagnet control via MOSFET (GPIO39) | [EXP-005](experiments/EXP-005-electromagnet-control-test.md), `src/electromagnet.*`, `sketches/movement/` |
| Electromagnet holding force: picks up and moves pieces through the board | [EXP-005 follow-up (2026-11-18)](experiments/EXP-005-electromagnet-control-test.md), exercised in `chess_bridge` games |
| Serial command CLIs (positioning, waypoints, scans, motors, magnet, diagnostics) | `src/protocol.cpp`, `sketches/*/src/main.cpp` |
| UCI move execution (pickup → transport → place → `DONE`) with knight grid-line routes and diagonal moves | `sketches/movement/src/main.cpp` |
| Acceleration/deceleration ramps for step generation | `sketches/movement/src/main.cpp` (`moveXSteps`/`moveYSteps`) |
| Virtual chess prototype (move ranking, Stockfish optional, preferences) | [EXP-003](experiments/EXP-003-virtual-chess.md), `software/virtual_chess/` |
| Chess bridge: robot (White) vs human (Black) games over serial | `software/chess_bridge/` |
| Smart scanning: the bridge identifies the human's move by scanning for the highest-ranked candidate that still fits every reading (typically 2–10 scans), accepting a move only when it is unambiguous | `ScanEngine` in `software/chess_bridge/main.py` |
| Full move handling in the bridge: captures, en passant, castling, promotion | `MoveExecutor` in `software/chess_bridge/main.py` |
| Capture-lane ledger (L1–L8 white pieces, R1–R8 black pieces) | `CaptureLedger` in `software/chess_bridge/main.py` |
| Python unit tests: 44 passing (21 `virtual_chess`, 23 `chess_bridge`, mocked hardware) | `software/virtual_chess/tests/`, `software/chess_bridge/tests/` |

## Experimentally validated (formal reports with measured results)

- Magnetic coupling through the board surface — [EXP-001](experiments/EXP-001-magnetic-coupling.md) (completed; limits not formally characterized)
- Dual-Hall polarity classification — [EXP-002](experiments/EXP-002-dual-hall-sensor.md) (completed; approach later simplified, see ADR-006)
- Motor control validation — [EXP-004](experiments/EXP-004-motor-control-validation.md) (all tests passed; lessons on capacitors, grounding, Vref)
- Movement calibration — [EXP-006](experiments/EXP-006-movement-calibration.md) (movement accurate and repeatable)
- Electromagnet — [EXP-005](experiments/EXP-005-electromagnet-control-test.md) (control PASSED; physical operation PASSED after a wiring fix, see the 2026-11-18 follow-up; thermal behaviour not yet characterized)

## Under development

- **TMC2209 UART configuration** — both drivers share a single-wire UART (GPIO6 TX open-drain + GPIO5 RX; addresses X = 0, Y = 1). Wiring and communication are verified with `sketches/tmc_uart_test/`, which also sets microstepping to 1/8. The `movement` sketch does not configure UART yet (see known issue 1).
- **StallGuard sensorless homing** — DIAG outputs on GPIO7 (X) and GPIO15 (Y) are being wired; `tmc_uart_test` is used to choose StallGuard thresholds and homing speed. Replaces the previously planned endstop switches. No homing routine exists yet.
- **End-to-end game validation** — full bridge games have been played on the board; systematic validation across many games, and recovery from a partially executed move, is ongoing.
- **Knight obstacle displacement** — the movement sketch routes knights along grid lines for every knight move; temporarily displacing blocking pieces (`sketches/knight_offset/`) is still specific to `g1f3` and not generalized.
- **Main-firmware convergence** — `src/` is the modular architectural target, but its motion constants predate the latest calibration and its protocol does not emit the `DONE` replies the bridge requires. The actively flashed firmware is `sketches/movement/`.

## Planned (not started)

- Post-game piece reassembly from the capture-lane ledger
- General collision-aware path planning for all piece types (ordinary moves can still pass over occupied squares)
- Thermal characterization of the electromagnet during long games
- Binary framed serial protocol (CRC, sequence numbers — draft in [communication.md](communication.md)) and an emergency-stop command
- CAD / mechanical design file publication
- GUI or web front end; configurable Stockfish strength

---

## Known Issues / Inconsistencies

These are real, tracked discrepancies found in the codebase (re-checked 2026-11-18). They are preserved as-is (not silently "fixed") because resolving them requires physical re-verification against the hardware:

1. **Y-axis microstepping under the movement sketch** — with Y's `MS1` tied to 3.3 V for UART address 1, the Y driver defaults to 1/32 microstepping until firmware sets it over UART. `tmc_uart_test` sets 1/8 (the setting the 2220 steps/square calibration was measured with), but the `movement` sketch does not configure UART yet, so it must not drive the Y axis with the new wiring until UART setup is added.
2. **X/Y pin groups transposed in `include/config.h`** — the main firmware lists X = GPIO16/17/18 and Y = GPIO8/9/10, while the calibrated gantry wiring (all movement-family sketches and [hardware.md](hardware.md)) uses X = GPIO8/9/10 and Y = GPIO16/17/18. The main firmware's pin groups must be swapped and re-verified before it drives the gantry.
3. **Stale steps-per-square in the main firmware** — `src/motion.h` uses 2611 steps/square (an earlier calibration era), while the current calibrated value is 2220 (EXP-006, movement sketch). The same applies to older sketches (`chess_positioner`, `sensor_test`, `dual_motor_test`, `engine_simulation`, `piece_offset`, `motor_calibration`).
4. **Coordinate origin drift across sketches** — `movement` targets squares in zero-based units (`a1` → (0, 0)), while `bishop_diagonal`/`knight_offset` still use one-based units (`a1` → (1, 1), start (4.5, 4.5)). Both conventions must not be mixed when porting logic between sketches.
5. **Open-loop start position** — until sensorless homing is finished, the movement sketch assumes the carriage starts at (3.5, 3.5). If the carriage is moved by hand while powered off, position tracking is wrong until reset.
6. **White pieces are invisible to the sensor** — a consequence of single-polarity sensing. Promotions require the player to place the new piece by hand, and white pieces the player captures are assumed to be on the first free left-lane slot; neither can be verified by hardware.
7. **No game resume** — the bridge always starts a new game from the initial position and clears the capture ledger at startup, so an interrupted game (and the record of pieces already in the lanes) cannot be resumed.

See also [configuration_conflicts.md](configuration_conflicts.md) for a source-level audit of these and related conflicts.
