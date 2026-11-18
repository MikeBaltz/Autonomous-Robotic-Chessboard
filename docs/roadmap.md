# Roadmap

Phase history and forward plan. Statuses reflect the code and experiment reports in this repository.

---

## Phase 1 — Sensor Validation (complete)

- [x] ESP32-S3 PlatformIO setup
- [x] Firmware upload and serial monitor
- [x] Logger module
- [x] Timer module (non-blocking)
- [x] Magnetic coupling test (EXP-001)
- [x] Single Hall sensor test
- [x] Dual Hall sensor firmware
- [x] Empty / White / Black / Invalid classification
- [x] Dual Hall polarity detection (EXP-002) — later simplified to a single sensor (ADR-006)
- [x] Local Git repository
- [x] Initial documentation structure

---

## Phase 2 — Motor Validation (complete)

- [x] Document dual Hall experiment
- [x] Replace burnt TMC2209 driver
- [x] Test TMC2209 power wiring
- [x] Configure TMC2209 current safely
- [x] Rotate NEMA17 clockwise and counter-clockwise
- [x] Build a one-axis motion prototype

---

## Phase 3 — Carriage Prototype (largely complete)

- [x] Mount sensor and electromagnet on a custom carriage
- [x] Select magnetic actuator (electromagnet, ADR-005)
- [x] Add second motor (dual-axis, EXP-006)
- [x] Build and calibrate the XY gantry (V-slot/V-wheel/GT2, EXP-006) — Cartesian XY, one motor per axis
- [x] Movement calibration (square pitch, steps/square, sensor offset, surrounds)
- [ ] Validate scanning at repeatable positions across the full board (scan_test is a skeleton)

---

## Phase 4 — Software Integration (in progress)

- [x] Virtual chess prototype (EXP-003) — 21 passing tests
- [x] Collect user preference data from virtual chess sessions
- [x] Python serial communication with ESP32 (`chess_bridge`)
- [x] Integrate python-chess and Stockfish with the physical board — full robot-vs-human games played
- [x] Replace yes/no questions with Hall sensor board scans (smart scanning, single-polarity sensor; ADR-006 update)
- [x] Special move execution: captures, en passant, castling, promotion
- [x] Capture lane tracking (L1–L8 white, R1–R8 black) with ledger
- [x] Knight grid-line navigation and diagonal moves in the movement sketch
- [x] `chess_bridge` unit tests (23, mocked board)
- [ ] Systematic end-to-end game validation via the bridge
- [ ] Generalized obstacle displacement (`knight_offset` beyond `g1f3`)

---

## Phase 5 — Autonomy and Robustness (planned)

- [x] Wire both TMC2209 drivers to a shared single-wire UART (GPIO6/GPIO5, addresses X = 0, Y = 1); verified with `sketches/tmc_uart_test`
- [ ] Configure TMC2209 UART in the movement sketch (microstepping, current)
- [ ] StallGuard sensorless homing via DIAG (GPIO7/GPIO15), replacing endstop switches — in progress
- [ ] Post-game piece reassembly from the capture-lane ledger
- [ ] General collision-aware path planner with temporary piece displacement
- [ ] Consolidate movement sketch logic into the modular `src/` firmware (fix pin/steps inconsistencies, see [development-status.md](development-status.md))
- [x] Electromagnet holding force: picks up and moves pieces through the board (EXP-005 follow-up)
- [ ] Electromagnet thermal assessment during long games
- [ ] Publish CAD / mechanical design files
- [ ] Binary framed serial protocol (draft in [communication.md](communication.md))
- [ ] GUI or web front end; configurable engine strength
