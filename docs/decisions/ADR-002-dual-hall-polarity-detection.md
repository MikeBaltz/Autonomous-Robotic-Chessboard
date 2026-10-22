# ADR-002 — Dual Hall Polarity Detection

## Status

Superseded by ADR-006 (Single Hall Sensor)

---

## Context

The carriage needs to distinguish white pieces from black pieces. A single Hall sensor can only detect the presence of a magnet, not its polarity.

---

## Decision

Use two unipolar Hall sensors mounted on opposite sides of the carriage. White and black pieces use opposite magnet orientations:

- White pieces: magnet north pole facing up
- Black pieces: magnet south pole facing up

Classification:

- Neither sensor active → Empty
- Sensor A active only → White
- Sensor B active only → Black
- Both sensors active → Invalid

---

## Rationale

- Unipolar Hall sensors are inexpensive and widely available.
- Each sensor only responds to one magnetic pole.
- Mounting them on opposite sides means each faces a different pole of the magnet above.
- No additional hardware (camera, RFID) is needed for color classification.

---

## Consequences

- Magnet orientation in each piece must be consistent.
- Captured or manually replaced pieces must preserve the polarity convention.
- External magnetic fields may cause invalid readings.
- Sensor spacing and orientation must be experimentally optimized for the carriage geometry.
- The magnetic actuator (planned) may interfere with sensing.
- Sensor readings should be taken while the actuator is disabled or moved away from the sensing area.

---

## Validation Required

- Both sensors mounted in final carriage geometry.
- Repeatability test across all squares.
- Actuator interference evaluation.
- Behavior under external magnetic fields.
