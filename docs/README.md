# Documentation

---

## Overview

- [Architecture](architecture.md) — system architecture and physical stack
- [Hardware](hardware.md) — component list, status, pin assignments, calibration
- [Firmware](firmware.md) — firmware modules as implemented
- [Software](software.md) — Python components (virtual chess prototype, chess bridge)
- [Communication](communication.md) — implemented serial protocol + draft future spec
- [Development Status](development-status.md) — implemented / experimental / in-progress / planned, known issues
- [Roadmap](roadmap.md) — project phases and progress

---

## Experiments

- [EXP-001 — Magnetic Coupling](experiments/EXP-001-magnetic-coupling.md)
- [EXP-002 — Dual Hall Sensor](experiments/EXP-002-dual-hall-sensor.md)
- [EXP-003 — Virtual Chess Prototype](experiments/EXP-003-virtual-chess.md)
- [EXP-004 — Motor Control Validation](experiments/EXP-004-motor-control-validation.md)
- [EXP-005 — Electromagnet Control Test](experiments/EXP-005-electromagnet-control-test.md)
- [EXP-006 — Movement Calibration](experiments/EXP-006-movement-calibration.md)

---

## Decisions

- [ADR-001 — Moving Sensor Carriage](decisions/ADR-001-moving-sensor-carriage.md)
- [ADR-002 — Dual Hall Polarity Detection](decisions/ADR-002-dual-hall-polarity-detection.md) (superseded)
- [ADR-003 — Virtual Chess Prototype](decisions/ADR-003-virtual-chess-prototype.md)
- [ADR-004 — Motion Driver](decisions/ADR-004-motion-driver.md)
- [ADR-005 — Electromagnet Actuator](decisions/ADR-005-electromagnet-actuator.md)
- [ADR-006 — Single Hall Sensor](decisions/ADR-006-single-hall-sensor.md)

---

## Design References

- [Firmware Architecture](firmware_architecture.md) — aspirational next-generation firmware design (not yet implemented)
- [State Machine](state_machine.md) — current firmware control flow; formal FSM planned
- [`run`](../run) — build, flash, and monitor script (repository root)
