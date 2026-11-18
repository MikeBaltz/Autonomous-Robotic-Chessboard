# ADR-006 — Single Hall Sensor for Magnet Detection

## Status

Accepted, amended (see [Update — single-polarity sensing](#update--single-polarity-sensing))

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

---

## Update — single-polarity sensing

The sensor was modified to respond to one magnet polarity only. It now
detects **black pieces only**; white pieces read as empty.

- The bridge predicts, for each legal black move, exactly which squares
  hold black pieces afterwards, so captures are detected unambiguously.
- Robot (white) moves are verified by checking the changed squares read
  empty; black pieces carried to the right lane are confirmed by a scan.
- White pieces (promotions, pieces captured by the player) cannot be
  verified by hardware. Captured white pieces are assumed to be on the
  first free left-lane slot.

