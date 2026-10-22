# ADR-006 — Single Hall Sensor for Magnet Detection

## Status

Accepted

---

## Context

The original design (ADR-002) used two Hall sensors to distinguish white pieces from black pieces based on magnet polarity. This approach required:

- Two sensors mounted on opposite sides of the carriage
- Consistent magnet orientation in all pieces
- Complex logic to interpret dual sensor readings

During testing, we found that distinguishing piece color by polarity was unreliable and added unnecessary complexity. The chess robot can track piece colors in software instead.

---

## Decision

Use a single Hall sensor to detect magnet presence only. Piece color tracking is handled by the game state in software.

---

## Rationale

- Simpler hardware: one sensor instead of two
- More reliable: no polarity discrimination issues
- Easier calibration: single sensor position to optimize
- Software can track piece colors based on game state
- Reduces carriage complexity

---

## Consequences

- Cannot independently verify piece color by hardware
- Must rely on software game state for color tracking
- Manual piece placement errors won't be detected by hardware
- Simpler code and easier maintenance

---

## Implementation

- Single Hall sensor on GPIO4
- Detects magnet presence (true/false)
- Debounced reading with 5 samples
- Game state tracks which pieces are where
