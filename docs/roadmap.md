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
- [x] Build and calibrate the XY gantry (V-slot/V-wheel/GT2, EXP-006) — Cartesian layout rather than the originally considered CoreXY
- [x] Movement calibration (square pitch, steps/square, sensor offset, surrounds)
- [ ] Validate scanning at repeatable positions across the full board (scan_test is a skeleton)

---

## Phase 4 — Software Integration (in progress)

- [x] Virtual chess prototype (EXP-003) — 21 passing tests
- [x] Collect user preference data from virtual chess sessions
- [x] Python serial communication with ESP32 (`chess_bridge`)
- [x] Integrate python-chess and Stockfish with the physical board (initial sessions)
- [ ] Systematic end-to-end game validation via the bridge
- [ ] Replace yes/no questions with Hall sensor board scans
- [ ] Piece-specific path planning: knight grid-line navigation and bishop diagonals implemented experimentally (`knight_offset`, `bishop_diagonal`); generalization pending

---

## Phase 5 — Autonomy and Robustness (planned)

- [ ] Endstop switches and homing (replace assumed start position)
- [ ] Full 64-square scan integrated into the game loop (sensor-verified moves)
- [ ] Automated captures, castling, promotion, captured-piece removal
- [ ] General collision-aware path planner with temporary piece displacement
- [ ] Consolidate movement sketch logic into the modular `src/` firmware (fix pin/steps inconsistencies, see [development-status.md](development-status.md))
- [ ] Electromagnet holding-force verification and thermal assessment (EXP-005 follow-up)
- [ ] Publish CAD / mechanical design files
- [ ] Binary framed serial protocol (draft in [communication.md](communication.md))
- [ ] GUI or web front end; configurable engine strength
