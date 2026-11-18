# Active System Configuration

This document describes the currently active prototype implementation as it
exists in source code (last updated 2026-11-18). Source code is authoritative;
older architecture and hardware documents are not treated as configuration.
For project-level status see [development-status.md](development-status.md).

## A. Active System Architecture

### Active components

```text
software/chess_bridge/main.py
    |
    | 115200 baud, newline-delimited text commands
    v
sketches/movement/src/main.cpp
    |
    +--> X STEP/DIR/EN outputs -> TMC2209 -> X motor
    +--> Y STEP/DIR/EN outputs -> TMC2209 -> Y motor
    +--> GPIO 39 -> electromagnet driver
    +--> GPIO 4 <- active-low Hall sensor
```

### Python bridge

Entry point: `software/chess_bridge/main.py`

The bridge owns the authoritative chess state through `python-chess` via the
`Game` wrapper. `ChessRobotBridge` coordinates the engine, move ranking,
serial transport, physical move planning, scan-based human move detection,
verification, and the capture ledger.

Python responsibilities:

- Generate or select legal chess moves.
- Use Stockfish when configured, otherwise use the fallback selector.
- Decompose captures, en passant, castling, and promotion.
- Send movement primitives to the ESP32.
- Scan the board to detect the human player's move.
- Track captured pieces in the side-lane ledger.

### ESP32 movement firmware

Entry point: `sketches/movement/src/main.cpp`

The firmware owns:

- Blocking open-loop X/Y step generation.
- Software carriage position tracking.
- Electromagnet GPIO control.
- Single digital Hall sensor reads.
- UCI geometry dispatch for knight and diagonal paths.
- Waypoint and capture-lane positioning.
- Newline-delimited command parsing and responses.

The firmware does not own a chess board model or validate legal chess moves.

### Complete normal-move flow

```text
ChessRobotBridge.get_white_move()
    -> MoveExecutor.plan()
    -> ArduinoBridge.execute("e2e4")
    -> firmware handleCommand("e2e4")
    -> moveToSquareCenter(e2)
    -> electromagnet HIGH, 200 ms
    -> moveToSquareCenter(e4)
    -> electromagnet LOW, 200 ms
    -> firmware prints DONE
    -> Python updates python-chess state
    -> optional source/destination verification scans
```

The human move is either typed with `--manual` or inferred by Python's
`ScanEngine`; the ESP32 only supplies individual occupancy readings.

## B. Hardware Pin Map

The following values are from `sketches/movement/src/main.cpp`.

| Function | GPIO | Active behavior | Status |
|---|---:|---|---|
| X STEP | 8 | Step pulse | Configured |
| X DIR | 9 | Direction level | Configured |
| X ENABLE | 10 | LOW enables driver | Configured |
| Y STEP | 16 | Step pulse | Configured |
| Y DIR | 17 | Direction level | Configured |
| Y ENABLE | 18 | LOW enables driver | Configured |
| Hall sensor | 4 | `INPUT_PULLUP`, LOW means magnet (single polarity: black pieces only) | Configured and installed |
| Electromagnet | 39 | HIGH on, LOW off | Configured |
| USB serial | ESP32 USB CDC | 115200 baud | Configured |

Wired but not used by the movement sketch (used by `sketches/tmc_uart_test/`):

| Function | GPIO | Notes |
|---|---:|---|
| TMC2209 UART TX | 6 | Open-drain, joined with GPIO5 to both drivers' `RX` pins |
| TMC2209 UART RX | 5 | Shared single-wire bus; addresses X = 0, Y = 1 |
| X DIAG (StallGuard) | 7 | Being wired |
| Y DIAG (StallGuard) | 15 | Being wired |

Unconfigured:

- No endstop switches (replaced by planned StallGuard sensorless homing).
- No emergency-stop input.
- No motor fault input.
- No electromagnet current-sense input.

### Motor driver configuration

The firmware uses only external STEP, DIR, and active-low ENABLE signals. It
does not configure TMC2209 UART, current, microstepping, StallGuard, or
StealthChop in software. Driver current, microstepping, and power wiring are
therefore external hardware settings or unknown from the active source.
UART configuration (1/8 microstepping, StallGuard) currently exists only in
`sketches/tmc_uart_test/`. With Y's `MS1` tied high for UART addressing, the
Y driver defaults to 1/32 microstepping under the movement sketch.

## C. Motion Configuration

| Parameter | Active value | Source |
|---|---:|---|
| Steps per column | 2220 | `sketches/movement/src/main.cpp` |
| Steps per row | 2220 | `sketches/movement/src/main.cpp` |
| Square pitch | 45.1 mm | `sketches/movement/src/main.cpp` |
| Derived steps/mm | approximately 49.224 | `2220 / 45.1` |
| Nominal axis speed | 1000 steps/s | `MOVE_SPEED` |
| Minimum pulse half-period | 500 us | `1,000,000 / (1000 * 2)` |
| Acceleration length | up to 100 steps | `accelSteps` |
| Deceleration length | up to 100 steps | `decelSteps` |
| Acceleration start delay | 4x minimum delay | `startDelay` |
| Hall offset | 15.0 mm in Y | `HALL_Y_OFFSET` |
| Hall offset in grid units | approximately 0.3326 | `15.0 / 45.1` |
| Coordinate minimum | -1.5 | `COORD_MIN` |
| Coordinate maximum | 8.5 | `COORD_MAX` |
| Startup X | 3.5 | `START_COLUMN` |
| Startup Y | 3.5 | `START_ROW` |

### Coordinate system

The active firmware uses zero-based square-center coordinates:

```text
a1 = (0, 0)
...
h8 = (7, 7)
```

The software origin is the center of `a1`, not a board corner coordinate.
The startup assumption `(3.5, 3.5)` is the geometric center between the four
central square centers. The firmware does not home to this position; it simply
initializes its software estimate there.

### Axis directions

Logical positive X is passed directly to the X DIR output because
`xDirectionInverted` is `false`.

Logical positive Y is inverted at the output because `yDirectionInverted` is
`true`.

The actual physical direction of positive motion still depends on motor and
belt wiring and has not been established by this source audit.

### Movement behavior

`moveToPosition()` moves X completely, then Y completely. It is not
simultaneous Cartesian interpolation except in the separate diagonal helper.
After the commanded step loops finish, the software position is assigned the
requested target regardless of missed steps or mechanical error.

`moveDiagonal()` interleaves X and Y pulses for equal or unequal diagonal
geometry. It uses fixed 500 us pulse delays rather than the normal acceleration
profile.

There is no homing, encoder, limit switch, position feedback, or runtime
travel enforcement in the movement sketch. StallGuard sensorless homing is in
progress (see `sketches/tmc_uart_test/`). The coordinate bounds are checked for `g`,
`path`, and `scanpos` inputs, but normal square movement relies on validated
square syntax.

## D. Serial Protocol

Transport:

- ESP32 USB serial.
- 115200 baud.
- Commands terminate with newline.
- Carriage return is ignored.
- Input is trimmed before dispatch.
- Maximum buffered command length is 64 characters.

For a recognized successful command, the firmware prints command-specific
diagnostic output followed by:

```text
DONE
```

For invalid input, it prints an `ERROR: ...` line and does not print `DONE`.
The Python bridge treats any response without an exact `DONE` line as a
`CommandError`.

### Commands implemented by the active firmware

| Syntax | Parameters | Response | Side effects / errors |
|---|---|---|---|
| `e2e4` | Four-square UCI move | Diagnostics, then `DONE` | Picks up source, routes to target, releases. Invalid squares are rejected. |
| `e7e8q` | UCI move plus `q`, `r`, `b`, or `n` | Diagnostics, then `DONE` | Promotion letter is logged but does not select a replacement piece. |
| `scan e4` | Algebraic square | `SCAN e4 M/E/X`, then `DONE` | Moves to sensor offset position and polls Hall input. Invalid square returns `ERROR`. |
| `scanpos x y` | Two grid coordinates | `SCANPOS x y M/E/X`, then `DONE` | Applies Hall Y offset before polling. Out-of-range or malformed values return `ERROR`. |
| `g x y` | Two grid coordinates | Position diagnostics, then `DONE` | Moves to an absolute coordinate. Bounds are `-1.5` through `8.5`. |
| `path x1 y1 ...` | Waypoint coordinate pairs | Waypoint diagnostics, then `DONE` | Moves through each pair sequentially. Requires at least one pair and even argument count. |
| `lane L1` / `lane R8` | Lane side and slot 1-8 | Lane diagnostics, then `DONE` | Moves only; does not energize the magnet or update the ledger. Uses X=-0.5 or X=7.5 and Y=slot-1. |
| `m a` | Column `a`-`h` | Column diagnostics, then `DONE` | Moves to the column at current Y. |
| `m a3` | Column and row | Square diagnostics, then `DONE` | Moves to the requested square. |
| `a`-`h` | One column | Column diagnostics, then `DONE` | Same column positioning as `m a`. |
| `1`-`8` | One row | Row diagnostics, then `DONE` | Moves to the row at current X. |
| `s` | None | Mode diagnostics, then `DONE` | Toggles sensor mode. Sensor mode changes square, row, and surround targeting by the Hall offset; column-only moves are not offset. `scan` always applies its own offset. |
| `p` | None | Current position, then `DONE` | Reports software position. |
| `l` | None | Left-surround diagnostics, then `DONE` | Moves to X=-0.5. |
| `r` | None | Right-surround diagnostics, then `DONE` | Moves to X=7.5. |
| `t` | None | Top-surround diagnostics, then `DONE` | Moves to Y=7.5. |
| `w` | None | Bottom-surround diagnostics, then `DONE` | Moves to Y=-0.5. |
| `x` | None | Reset diagnostics, then `DONE` | Returns to software startup coordinate `(3.5, 3.5)`; it does not home physically. |
| `o` | None | Magnet diagnostics, then `DONE` | Drives GPIO39 HIGH. |
| `O` | None | Magnet diagnostics, then `DONE` | Drives GPIO39 LOW. |
| `d` | None | Diagnostic block, then `DONE` | Reports GPIO levels and Hall state. |
| `E` | None | Enable diagnostics, then `DONE` | Forces both enable pins LOW. |
| `h` | None | Help block, then `DONE` | Prints command help. |

### Commands used by Python

The Python bridge generates or consumes:

- UCI moves such as `e2e4` and `e7e8q`.
- `g`.
- `path`.
- `o` and `O`.
- `scan`.
- `scanpos`.

The firmware also supports `lane`, but the current Python move planner uses
explicit `g` and `path` commands for lane routing rather than sending `lane`.

### Unsupported commands

No command used by the current Python bridge is unsupported by the active
movement firmware source.

The older draft commands in `docs/communication.md` are not implemented:
`PING`, `STATUS`, `HOME`, `SCAN` as a full-board command, `MOVE`, and `STOP`.

## E. Chess Movement Architecture

### Normal moves

`MoveExecutor.plan()` returns one UCI command. The firmware picks up at the
source square, moves to the destination, and releases. The path is sequential
X then Y and has no occupancy model.

### Knight moves

The firmware recognizes knight geometry using `(dx, dy) = (2,1)` or `(1,2)`.
It routes the held piece's long (two-square) leg along the grid line between
the files or ranks it crosses, so it never passes over a square center. The
strategy is generalized by knight geometry but does not inspect actual
obstacles.

### Bishop-style diagonal moves

Any nonzero move with equal absolute X and Y differences uses interleaved
diagonal motion. Dispatch is geometric, not piece-type aware; diagonal queen,
king, or pawn moves can also take this path.

### Captures

Python first carries the victim to a side lane using:

```text
g source
o
path source-border, lane-border, lane-slot
O
```

It records the victim in the ledger, then sends the original UCI move.

### Castling

Python sends the king UCI move first. It then separately picks up the rook and
routes the rook along a same-row border path to its destination.

### En passant

Python computes the removed pawn's square, carries that pawn to the appropriate
lane, then sends the en-passant UCI move.

### Promotion

Python optionally removes the captured victim, sends the promotion UCI move,
relocates the pawn to the left lane, prompts the human to place the replacement
piece, and scans the promotion square for `M`.

### Capture lanes

```text
L1-L8: X=-0.5, Y=0 through 7
R1-R8: X= 7.5, Y=0 through 7
```

The bridge uses the left lane for captured white pieces and promoted pawns,
and the right lane for captured black pieces. The ledger is stored in
`software/chess_bridge/capture_ledger.json` by default and is reset at bridge
startup.

## F. Known Limitations

- No physical homing sequence.
- Open-loop position tracking only.
- No encoder, limit switch, StallGuard, or other physical position feedback.
- Hall readings are single polarity: black pieces read `M`, white pieces and
  empty squares read `E`. Piece type is not measured.
- No generalized collision avoidance or occupancy-aware path planner.
- Ordinary paths can pass through locations occupied by other pieces.
- Knight obstacle displacement is not generalized; the dedicated offset sketch
  is specific to `g1 -> f3` and its displacement code is disabled.
- No transactional command protocol or recovery after a partial plan.
- No emergency stop command.
- No pickup, release, or lane-placement confirmation.
- Promotion requires manual placement of the replacement piece.
- The bridge resets the capture ledger at startup.
- The active firmware does not maintain a legal chess board model.
- The active firmware has no TMC2209 UART, current, microstep, or StallGuard
  configuration (only `sketches/tmc_uart_test/` does).
- Final board dimensions and mechanical travel are not authoritative in source.

## G. Legacy Code

The following code remains in the repository but is not the active bridge path:

- `src/`: original modular Arduino firmware with its own protocol, motion
  model, Hall class, PWM electromagnet, and simplified `Game` class.
- `software/virtual_chess/`: standalone interactive virtual chess prototype and
  preference ranker.
- `sketches/sensor_test/`: earlier dual-Hall/single-axis validation sketch.
- `sketches/motor_calibration/`: single-motor calibration sketch.
- `sketches/engine_simulation/`: early engine/movement prototype.
- `sketches/chess_positioner/`: early one-axis positioner.
- `sketches/dual_motor_test/`: early independent two-axis test.
- `sketches/piece_offset/`: earlier hardcoded obstacle-displacement experiment.
- `sketches/scan_test/`: scanning skeleton, not the active scanner.

These files are retained for history and experiments. They must not be treated
as the active pin, coordinate, or protocol reference.

## Build and Upload

Build the active firmware without hardware interaction:

```bash
pio run -d sketches/movement
```

Upload, when intentionally authorized, is documented as:

```bash
pio run --target upload -d sketches/movement
```

The repository `Makefile` also provides `make flash`.
