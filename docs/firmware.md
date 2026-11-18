# Firmware

Describes the firmware **as implemented** in this repository. For the future (aspirational) architecture with HAL abstraction, FreeRTOS task split, and a formal state machine, see [firmware_architecture.md](firmware_architecture.md).

There are two firmware lines:

1. **Main modular firmware** (`src/` + `include/config.h` + root `platformio.ini`) — the architectural target: clean class-per-module design, constructor-injected dependencies, serial CLI. Its motion constants predate the latest calibration (see [development-status.md](development-status.md) known issues).
2. **Movement sketch** (`sketches/movement/`) — the actively deployed, calibrated firmware that currently drives the physical board and executes UCI moves from `chess_bridge`.

---

## Main firmware (`src/`)

| Module | Files | Responsibility |
|---|---|---|
| Logger | `logger.{h,cpp}` | `[INFO]/[WARNING]/[ERROR]` serial logging, 115200 baud |
| Timer | `timer.{h,cpp}` | Non-blocking periodic `expired()` check based on `millis()` |
| Sensor | `sensor.{h,cpp}` | Debounced Hall-effect input: 5 identical consecutive samples at 10 ms before a state change is accepted; reports transitions |
| Board | `board.{h,cpp}` | Heartbeat + sensor state change reporting |
| Motor | `motor.{h,cpp}` | One stepper: STEP/DIR/EN pins, enable/disable (active-low), `moveSteps(steps, stepsPerSecond)` blocking profile |
| Electromagnet | `electromagnet.{h,cpp}` | PWM via LEDC (channel 0, 1 kHz, 8-bit), adjustable duty (default 255), enable/disable/toggle |
| Motion | `motion.{h,cpp}` | Chess kinematics: square → steps mapping, `pickUpPiece`/`placePiece` (magnet on/off), `scanSquare`, `detectWhiteMove` (scans white's back two ranks for a vacated square) |
| Game | `game.{h,cpp}` | 8×8 piece model (type/color), initial setup, basic move validation (source occupied, no friendly capture), ASCII board rendering |
| Protocol | `protocol.{h,cpp}` | Line-based serial CLI: `e2e4` moves, `m a3` positioning, `p` board, `r` reset, `s` status, `1/0/t` electromagnet, `x/y` motor toggles, `scan`, `h` help |
| main | `main.cpp` | Composition root — constructs and `begin()`s all modules; loop polls sensor/board/protocol |

**Dependencies:** injected via constructors; no globals besides the top-level objects in `main.cpp`.

**Serial commands (main firmware):** see the help text in `protocol.cpp` — `e2e4`, `m a` / `m a3`, `p`, `r`, `s`, `scan`, `1`, `0`, `t`, `x`, `y`, `h`.

## Movement sketch (`sketches/movement/`)

Single-file firmware with the calibrated constants (45.1 mm pitch, 2220 steps/square, zero-based coordinates):

- **Motion core** — `moveXSteps`/`moveYSteps` with linear acceleration/deceleration ramps (up to 100 ramp steps, 4× start delay), per-axis direction inversion, dead-reckoned position in square units
- **Targeting** — `moveToSquare(col,row)`, `moveToColumn`, `moveToRow`, surround parking (`l/r/t/w`), sensor-mode Y offset (−15 mm / pitch) so the Hall sensor — not the electromagnet — lands on the square center
- **UCI move execution** — `executeUCIMove` parses `e2e4`-style input (optional promotion letter, e.g. `e7e8q`) and dispatches by geometry. Piece moves always target the electromagnet at the square center (`moveToSquareCenter`), regardless of the manual sensor-mode toggle:
  - **Knight moves** → grid-line navigation: the long (two-square) leg travels along the border between files or ranks, so the knight never passes over the center of an occupied square
  - **Diagonal moves** → `moveDiagonal` with interleaved X/Y step pulses for a straight-line diagonal trajectory (dispatch is geometric, so diagonal queen/king/pawn moves use it too)
  - **Other moves** → pickup at source, straight transport (X then Y), place at destination
- **Primitives used by `chess_bridge`** — `g x y` (absolute grid move), `path x1 y1 x2 y2 …` (waypoints), `o`/`O` (magnet on/off), `scan e4` / `scanpos x y` (single Hall reading at the sensor-offset position, reply `SCAN e4 M|E|X`), `lane L1`…`R8` (capture-lane positioning)
  - Replies `DONE` when finished (consumed by `chess_bridge`); `ERROR: …` on invalid input
- **Diagnostics** — pin state readback, test pulse, force-enable command

## TMC2209 UART / StallGuard test (`sketches/tmc_uart_test/`)

Bring-up sketch for the BigTreeTech TMC2209 V1.3 drivers on a shared single-wire UART (`Serial1`, GPIO6 TX open-drain + GPIO5 RX, addresses X = 0 and Y = 1, using `TMCStepper`). It checks communication with both drivers, sets 1/8 microstepping, jogs each axis while printing `SG_RESULT`, and helps pick the StallGuard threshold and speed for sensorless homing (DIAG on GPIO7 / GPIO15). See its [README](../sketches/tmc_uart_test/README.md). The `movement` sketch does not configure UART yet.

## Other sketches

Each sketch under `sketches/` is an independent PlatformIO project (same board config) documenting a hardware bring-up or experiment step — see the table in the repository README. `sketches/scan_test/` is a skeleton for board scanning; `sketches/bishop_diagonal/` and `sketches/knight_offset/` are current path-planning experiments.

## Configuration

All pin assignments and tunables for the main firmware live in [`include/config.h`](../include/config.h). The movement sketch keeps its constants at the top of `src/main.cpp` (self-contained by design, as a calibrated hardware experiment).
