# ADR-001 — Moving Sensor Carriage

## Status

Accepted — the moving-carriage concept stands; the two-sensor detail was later revised to a single sensor by [ADR-006](ADR-006-single-hall-sensor.md)

---

## Context

The board needs to detect which squares are occupied and the color of the piece on each square. This information is required for game state tracking and move validation.

---

## Considered Options

- **64 fixed Hall sensors** — one sensor per square, always-on, instant full-board read
- **Camera-based detection** — overhead camera with computer vision
- **RFID** — RFID tag in each piece, reader in each square or on carriage
- **One moving Hall sensor** — single sensor on carriage, scans squares sequentially
- **Two moving Hall sensors** — two sensors on carriage, can distinguish polarity

---

## Decision

Use two moving Hall sensors mounted on the carriage.

---

## Rationale

| Factor | 64 fixed | Camera | RFID | 1 Hall | 2 Hall |
|---|---|---|---|---|---|
| Cost | High | Medium | High | Low | Low |
| Wiring complexity | Very high | Low | High | Low | Low |
| Color detection | No | Yes | Yes | No | Yes |
| Piece type detection | No | Possible | Yes | No | No |
| Maintenance | High | Medium | Medium | Low | Low |

Two moving Hall sensors provide occupancy and color classification at low cost and low wiring complexity.

---

## Consequences

- Board scans take time (carriage must visit each square).
- Carriage positioning must be accurate and repeatable.
- Sensing cannot occur reliably while the magnetic actuator is active.
- Calibration is required to map carriage position to board squares.
- Only color and occupancy are directly measured — not the exact piece type.
- Piece type must be inferred from game state (tracked by Python/Stockfish).

---

## Validation Required

- Scanning repeatability across all 64 squares.
- Sensor interference from magnetic actuator.
- Scan time for full board.
