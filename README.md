# Autonomous Robotic Chessboard

**An open-source chessboard that moves its own pieces** — an ESP32-S3 drives a custom XY gantry beneath a thin wooden board, dragging magnetically-coupled chess pieces with an electromagnet while a single-polarity Hall-effect sensor detects the black pieces to work out the human's move. Python chess software (python-chess + optional Stockfish) plans the game and commands the firmware over USB serial.

> Status: **active development.** Motion, electromagnet pickup, sensing, and full robot-vs-human games via the bridge are working; sensorless homing and broader end-to-end validation are in progress. See [Development Status](docs/development-status.md) for an honest breakdown of what works, what is experimental, and what is planned.

---

## Overview

Building a machine that plays physical chess raises a distinct set of embedded-systems and robotics problems that this project tackles end-to-end:

- **Coordinate mapping** — translating algebraic chess squares (`e2`, `f3`) into stepper-motor step counts on a physical gantry
- **Open-loop motion accuracy** — positioning repeatability without encoders, using calibration, acceleration ramps, and dead-reckoned position tracking
- **Magnetic actuation through a surface** — moving pieces on top of the board with an electromagnet moving underneath it
- **Piece detection** — sensing magnets through the board with a single-polarity Hall-effect sensor that sees black pieces only (white pieces read as empty), and inferring the human's move from as few scans as possible
- **Physical path planning** — knights jump over pieces, bishops travel diagonals, and blocking pieces may need temporary displacement
- **System decomposition** — the MCU executes *how* to move; the host software decides *what* to move

The project is documented as an engineering log: six Architecture Decision Records (ADRs) and six formal experiment reports (EXPs) capture the reasoning and measured results behind every major subsystem.

---

## Key Features (Implemented)

- **Modular ESP32-S3 firmware** (Arduino framework, built with PlatformIO): `Sensor`, `Motor`, `Electromagnet`, `Motion`, `Board`, `Game`, `Protocol`, `Logger`, `Timer` modules wired via constructor injection
- **Calibrated XY gantry motion** — 45.1 mm square pitch, 2220 steps/square, software acceleration/deceleration ramps, per-axis direction inversion, hall-sensor offset compensation
- **UCI move execution on hardware** — send `e2e4` over serial; the carriage picks up the piece, transports it, and places it, replying `DONE`
- **Piece-specific path strategies** — knights navigate along grid lines (between squares) for clearance; bishops move via simultaneous dual-axis diagonal stepping
- **Debounced Hall-effect piece detection** — 5 consecutive samples at 10 ms before a state change is accepted
- **Working electromagnet actuation** via MOSFET driver — picks up pieces and moves them through the board ([EXP-005 follow-up](docs/experiments/EXP-005-electromagnet-control-test.md)); PWM-adjustable strength in the main firmware (LEDC, 1 kHz, 8-bit)
- **Single-polarity Hall sensing** — the sensor detects black pieces only; white pieces read as empty, and white pieces are tracked in software ([ADR-006](docs/decisions/ADR-006-single-hall-sensor.md))
- **Interactive serial command CLIs** for every subsystem (motor enable, magnet toggle, square targeting, surrounds, diagnostics)
- **Python chess stack** — `python-chess` game state, deterministic heuristic move ranking with persistent user preferences, optional Stockfish integration with graceful fallback
- **`chess_bridge` host application** — connects Stockfish/python-chess to the firmware over pyserial, auto-detects the board's serial port, and orchestrates robot-vs-human games (robot plays White)
- **Smart scanning** — the bridge identifies the human's move by scanning for the highest-ranked candidate move that still fits every reading, usually in 2–10 scans, and accepts a move only if it is unambiguous
- **Full move handling** — captures (with a capture-lane ledger: L1–L8 white pieces, R1–R8 black pieces), en passant, castling, and promotion are decomposed by the bridge into firmware moves
- **44 Python unit tests** (21 in `virtual_chess`, 23 in `chess_bridge` against a mocked board) covering game logic, move ranking, preferences, engine fallback, move plans, scan detection, and the capture ledger
- **Engineering documentation** — ADR-001…006, EXP-001…006, architecture, hardware, firmware, and software docs

---

## System Architecture

```mermaid
graph TB
    subgraph Host["Host PC — Python"]
        SF["Stockfish (optional)"]
        PC["python-chess<br/>game state"]
        BR["chess_bridge<br/>pyserial"]
        SF --> PC
        PC --> BR
    end

    subgraph ESP32["ESP32-S3 firmware (Arduino)"]
        PR["Protocol<br/>serial CLI"]
        MO["Motion<br/>square → steps"]
        GM["Game<br/>board state"]
        EM["Electromagnet<br/>PWM"]
        SE["Sensor<br/>debounced Hall"]
    end

    subgraph Mechanics["XY gantry (Cartesian)"]
        MX["TMC2209 → NEMA17 X<br/>GT2 belt"]
        MY["TMC2209 → NEMA17 Y<br/>GT2 belt"]
        CA["Carriage<br/>V-wheels on V-slot"]
    end

    subgraph Board["Chessboard"]
        SURF["Thin wooden surface"]
        PIECE["Magnetic piece"]
    end

    BR <-->|"USB serial 115200<br/>UCI moves / DONE / logs"| PR
    PR --> MO
    PR --> GM
    MO -->|"STEP/DIR/EN<br/>+ single-wire UART"| MX
    MO --> MY
    MO --> EM
    MX --> CA
    MY --> CA
    CA --> SE
    EM -.->|"magnetic coupling<br/>through surface"| PIECE
    SE -.->|"detects black pieces"| PIECE
    PIECE --- SURF
```

**Responsibility split:** the ESP32-S3 owns real-time behavior — step generation, electromagnet PWM, sensor debouncing, and safe motion. The host owns chess logic — legality, engine search, game state, and high-level commands. The boundary is a simple line-based serial protocol (UCI moves and status commands).

---

## Hardware

| Component | Model / Detail | Status |
|---|---|---|
| Microcontroller | ESP32-S3 DevKitC N16R8 (USB CDC serial) | Tested |
| Stepper motors | 2× NEMA17 17HS4401 | Tested |
| Stepper drivers | 2× BigTreeTech TMC2209 V1.3 (STEP/DIR/EN, Vref 0.75 V) on a shared single-wire UART (addresses X = 0, Y = 1) | STEP/DIR tested; UART verified with `tmc_uart_test`, movement-sketch integration in progress |
| Power | 12 V DC supply (motors + electromagnet), 100 µF cap across VM/GND | Tested |
| XY gantry | Custom Cartesian XY frame (one motor per axis): V-slot aluminium extrusions, V-wheel carriage, GT2 belts and pulleys | Prototype, calibrated |
| Piece detection | Hall-effect sensor module, single polarity (black pieces only; white reads empty), GPIO4, active-low | Tested |
| Piece actuation | 12 V electromagnet + MOSFET driver, GPIO39 | Working: picks up and moves pieces through the board |
| Chess pieces | Neodymium magnet in each base (2×10 mm in pawns) | Tested |
| Homing | No endstop switches: StallGuard sensorless homing via TMC2209 DIAG | In progress (open-loop start position assumed for now) |

**Mechanical concept:** the chessboard is a thin passive surface. All electronics ride on a carriage *below* the board: the electromagnet drags pieces above it via magnetic coupling through the wood, and the Hall sensor verifies occupancy as the carriage visits squares.

**Key calibration values** (see [docs/hardware.md](docs/hardware.md) and [EXP-006](docs/experiments/EXP-006-movement-calibration.md)):

| Parameter | Value |
|---|---|
| Square pitch | 45.1 mm |
| Steps per square | 2220 (X and Y) |
| Hall sensor Y-offset from electromagnet | 15.0 mm (compensated in software) |
| Home/center position | (3.5, 3.5) in zero-based square units |

**Pin map** (movement sketch / gantry wiring):

| Pin | Function |
|---|---|
| GPIO4 | Hall sensor (active-low) |
| GPIO8 / 9 / 10 | X motor STEP / DIR / EN |
| GPIO16 / 17 / 18 | Y motor STEP / DIR / EN |
| GPIO39 | Electromagnet (MOSFET) |
| GPIO6 / GPIO5 | TMC2209 shared single-wire UART: TX (open-drain) / RX, joined to both drivers' `RX` pins |
| GPIO7 / GPIO15 | X / Y driver DIAG (StallGuard stall output, being wired) |
| USB | Serial console, 115200 baud |

> Note: the `movement` sketch does not configure the TMC2209 UART yet. With Y's `MS1` tied high for addressing, the Y driver runs at 1/32 microstepping until UART sets 1/8, so use `tmc_uart_test` for bring-up until that is integrated. Also, `include/config.h` of the main firmware currently lists the X/Y pin groups transposed relative to the calibrated gantry wiring above — tracked as a known issue in [docs/development-status.md](docs/development-status.md).

---

## Software and Firmware

### Firmware (`src/` — main modular firmware)

| Module | Responsibility |
|---|---|
| `logger` / `timer` | Serial logging; non-blocking periodic timer |
| `sensor` | Debounced Hall-effect reading (5 samples @ 10 ms) |
| `board` | Sensor state change reporting + heartbeat |
| `motor` | STEP/DIR generation with blocking move profiles |
| `electromagnet` | PWM control (LEDC channel, adjustable duty) |
| `motion` | Chess-square kinematics: square ⇄ steps, pickup/place, scan-square |
| `game` | 8×8 piece board model, move application, ASCII rendering |
| `protocol` | Line-based serial command interpreter |
| `main` | Composition root wiring all modules |

### Active development firmware (`sketches/movement/`)

A self-contained sketch that currently runs the physical board: calibrated positioning with acceleration ramps, **UCI move execution** (`e2e4`, `e7e8q`) with knight grid-line navigation and diagonal motion, waypoint moves (`g`, `path`), single-square scans (`scan e4`, `scanpos x y`), capture-lane positioning, and surround parking. Every command ends with `DONE` (consumed by `chess_bridge`). `sketches/tmc_uart_test/` brings up the TMC2209 UART and tunes StallGuard thresholds for sensorless homing. The remaining sketches under `sketches/` are preserved hardware experiments — motor bring-up, calibration tools, sensor tests, and path-planning prototypes (see each directory).

### Python software (`software/`)

- **`virtual_chess/`** — validated CLI prototype of the move-search workflow: ranks legal moves with deterministic heuristics (+ user-preference learning from played sessions), optional Stockfish for the engine side, 21 pytest tests
- **`chess_bridge/`** — the host application that closes the loop with the physical board: auto-detects the serial port, plays the robot's White moves via Stockfish/fallback (including captures, en passant, castling and promotion, with a capture-lane ledger), and detects the human's Black moves by smart scanning (typing is available as a fallback); 23 pytest tests against a mocked board

---

## Repository Structure

```
Autonomous-Robotic-Chessboard/
├── README.md                       # this file
├── LICENSE                         # MIT
├── Makefile                        # flash / bridge / test-knight targets
├── platformio.ini                  # ESP32-S3 build config (main firmware)
├── run                             # build + flash + monitor helper script
├── include/
│   └── config.h                    # firmware pin assignments & constants
├── src/                            # main firmware (modular architecture)
├── lib/                            # PlatformIO local libraries (reserved)
├── sketches/                       # hardware experiment sketches (PIO projects)
│   ├── motor_calibration/          #   manual step/direction validation
│   ├── chess_positioner/           #   single-axis square targeting
│   ├── sensor_test/                #   sensing during motion
│   ├── dual_motor_test/            #   two-axis bring-up
│   ├── engine_simulation/          #   move sequence with sensor offset
│   ├── piece_offset/               #   obstacle-displacement sequencing
│   ├── movement/                   #   ACTIVE: calibrated motion + UCI moves
│   ├── scan_test/                  #   board-scanning skeleton
│   ├── bishop_diagonal/            #   simultaneous-axis diagonal experiment
│   ├── knight_offset/              #   knight obstacle-clearance experiment
│   └── tmc_uart_test/              #   TMC2209 UART + StallGuard bring-up
├── software/
│   ├── virtual_chess/              # validated prototype + tests
│   └── chess_bridge/               # serial bridge to hardware + tests
└── docs/
    ├── README.md                   # documentation index
    ├── architecture.md             # system & physical architecture
    ├── hardware.md                 # components, pins, calibration
    ├── firmware.md                 # firmware modules as implemented
    ├── software.md                 # Python components
    ├── communication.md            # implemented protocol + draft spec
    ├── development-status.md       # implemented / experimental / in-progress
    ├── active_system_configuration.md # source-level reference for the active system
    ├── configuration_conflicts.md  # audit of discrepancies between code and docs
    ├── roadmap.md                  # project phases
    ├── firmware_architecture.md    # aspirational next-gen firmware design
    ├── state_machine.md            # current control flow (formal FSM planned)
    ├── decisions/                  # ADR-001 … ADR-006
    └── experiments/                # EXP-001 … EXP-006
```

---

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) CLI (`pip install platformio` or the VS Code extension) with the `espressif32` platform
- Python 3.11+
- Stockfish (optional — set `STOCKFISH_PATH` if installed)
- ESP32-S3 board connected via USB

### Build & flash firmware

```bash
# Main modular firmware (repo root project)
pio run                      # build only
pio run -t upload            # build + flash
pio device monitor           # serial monitor (115200)

# Active movement firmware (recommended for the current hardware)
pio run -d sketches/movement -t upload
pio device monitor -d sketches/movement
```

The `run` script at the repo root performs build + upload + monitor for the main firmware. The `Makefile` provides `make flash` (movement sketch), `make bridge`, and `make test-knight`.

### Play via the bridge (robot plays White)

```bash
python3 -m venv .venv && source .venv/bin/activate
pip install -r software/chess_bridge/requirements.txt
python software/chess_bridge/main.py              # auto-detects the serial port
# or: python software/chess_bridge/main.py /dev/cu.usbmodem101
```

Make your Black move on the board; the bridge scans to detect it. Use `--manual` to type moves in UCI instead (e.g. `e7e5`, `q` quits), `--no-verify` to skip post-move verification scans, or `--ledger FILE` for a custom capture-ledger path. Requires the movement sketch flashed and the board powered.

### Virtual chess prototype (no hardware needed)

```bash
cd software/virtual_chess
python3 -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
python main.py
```

### Run the Python tests

```bash
pip install pytest chess pyserial
python3 -m pytest software/virtual_chess/tests -v   # 21 tests
python3 -m pytest software/chess_bridge/tests -v    # 23 tests (mocked board, no hardware)
```

---

## Development Status

An honest snapshot — details and evidence in [docs/development-status.md](docs/development-status.md).

**Implemented and exercised on hardware**
- XY gantry motion with square-level accuracy (calibrated, [EXP-006](docs/experiments/EXP-006-movement-calibration.md))
- Hall-effect sensing with debounce ([EXP-002](docs/experiments/EXP-002-dual-hall-sensor.md), [ADR-006](docs/decisions/ADR-006-single-hall-sensor.md))
- TMC2209/NEMA17 motor control ([EXP-004](docs/experiments/EXP-004-motor-control-validation.md), [ADR-004](docs/decisions/ADR-004-motion-driver.md))
- Electromagnet pickup and transport of pieces through the board ([EXP-005](docs/experiments/EXP-005-electromagnet-control-test.md), follow-up 2026-11-18)
- Single-polarity Hall sensing of black pieces ([ADR-006](docs/decisions/ADR-006-single-hall-sensor.md))
- Serial command CLIs and UCI move execution in the movement sketch
- Robot-vs-human games via `chess_bridge`: smart scanning, captures with capture-lane ledger, en passant, castling, promotion
- Virtual chess prototype ([EXP-003](docs/experiments/EXP-003-virtual-chess.md)); 44 Python unit tests passing

**Under development**
- TMC2209 UART configuration in the movement sketch (wiring verified with `sketches/tmc_uart_test/`)
- StallGuard sensorless homing (DIAG on GPIO7/GPIO15), replacing endstop switches
- Systematic end-to-end game validation and recovery from partially executed moves
- Generalized knight obstacle displacement (`sketches/knight_offset/` is specific to `g1f3`)

**Planned**
- Post-game piece reassembly from the capture-lane ledger
- General collision-aware path planner (temporary piece displacement for blocked paths)
- Convergence of the movement sketch back into the modular `src/` firmware
- CAD/mechanical design files publication
- GUI / web interface; binary framed protocol (draft in [docs/communication.md](docs/communication.md))

---

## Engineering Challenges

- **Coordinate mapping & calibration** — square pitch (45.1 mm) and steps/square (2220) were measured empirically ([EXP-006]); the Hall sensor sits 15 mm off the electromagnet axis, so "sensor mode" targets compensate by a Y offset. Surrounds (parking lanes outside the playing area) were adjusted from the ideal 0.25-unit margin to match the physical frame.
- **Open-loop positioning** — no encoders or endstop switches; accuracy relies on calibrated step counts, acceleration ramps to avoid missed steps, and consistent dead-reckoned position tracking. StallGuard sensorless homing (the TMC2209 detects the stall at each mechanical end) is in progress and is the top roadmap priority.
- **Magnetic coupling through the board** — validated early ([EXP-001](docs/experiments/EXP-001-magnetic-coupling.md)) that a magnet below the surface drags one above it; coupling limits (board thickness, speed, payload) are not yet formally characterized.
- **Sensing next to an actuator** — the electromagnet can disturb Hall readings; ADR-005 requires readings to be taken with the actuator off.
- **Detecting moves with a one-colour sensor** — the sensor sees black pieces only, so every legal black move predicts exactly which squares hold black pieces afterwards. The bridge scans the square that best tests the highest-ranked move still consistent with all readings, typically finishing in 2–10 scans instead of 64, and only accepts the move when no other legal move fits.
- **Obstacle avoidance** — a knight leaving g1 must clear the f2/g7 pawns. Implemented strategy: travel along grid lines (borders between squares) maximizing clearance; experiments also explore temporarily displacing blocking pieces and restoring them afterward (`sketches/piece_offset/`, `sketches/knight_offset/`).
- **Diagonal motion** — bishops require synchronized X/Y stepping; implemented via interleaved step pulses with shared acceleration ramps (`moveDiagonal` in the movement sketch and `sketches/bishop_diagonal/`).
- **Host–device protocol** — currently a simple line-based text protocol (UCI in, `DONE`/logs out). A binary framed protocol with CRC and sequence numbers is drafted in [docs/communication.md](docs/communication.md) for later.

---

## Roadmap

1. **Sensorless homing** — TMC2209 UART in the movement sketch + StallGuard homing, eliminating assumed start positions
2. **Robustness** — systematic end-to-end game validation, recovery from partial moves, emergency stop
3. **Firmware consolidation** — port calibrated movement logic into the modular `src/` firmware
4. **General path planner** — piece-aware routing for all piece types, including temporary displacement
5. **Captured-piece management** — post-game reassembly from the capture-lane ledger
6. **Documentation** — experiment reports for the bishop/knight strategies; publish CAD files
7. **Polish** — GUI/web front end, configurable engine strength, framed binary protocol

Full phase history in [docs/roadmap.md](docs/roadmap.md).

---

## Documentation

- [Documentation index](docs/README.md)
- [Architecture](docs/architecture.md) · [Hardware](docs/hardware.md) · [Firmware](docs/firmware.md) · [Software](docs/software.md)
- [Communication protocol](docs/communication.md) · [Development status](docs/development-status.md) · [Roadmap](docs/roadmap.md)
- [Architecture Decision Records (ADR-001…006)](docs/decisions/) · [Experiment Reports (EXP-001…006)](docs/experiments/)

---

## Author

**Michail Effraim Baltzis**

GitHub: [https://github.com/MikeBaltz](https://github.com/MikeBaltz)

## License

Released under the [MIT License](LICENSE).
