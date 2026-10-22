# EXP-001 — Magnetic Coupling

## Status

Completed

---

## Objective

Verify that a neodymium magnet mounted below a thin wooden surface can couple with a magnet above the surface, enabling a carriage underneath the board to move chess pieces on top.

---

## Hypothesis

Two neodymium magnets separated by a thin wooden board will maintain sufficient magnetic coupling for the lower magnet to drag the upper magnet during manual movement.

---

## Equipment

- ESP32-S3 DevKitC
- Neodymium magnets (2×10 mm)
- Thin wooden surface (chessboard)

---

## Setup

One magnet placed on top of the wooden surface. A second magnet held below the surface, aligned with the upper magnet.

---

## Procedure

1. Place a magnet on top of the board surface.
2. Hold a second magnet below the surface, directly under the first.
3. Move the lower magnet manually in various directions.
4. Observe whether the upper magnet follows.

---

## Observations

- The upper magnet followed the lower magnet during manual movement.
- Coupling was maintained through the board thickness.
- The upper magnet tracked smoothly at moderate manual speeds.

---

## Result

Magnetic coupling through the thin wooden surface is viable for moving chess pieces.

---

## Limitations

- Maximum board thickness for reliable coupling was not formally measured.
- Maximum speed and payload were not tested.
- Repeatability across all 64 squares was not evaluated.
- No automated carriage was used — movement was manual.

---

## Conclusion

The magnetic coupling concept is validated. A carriage moving below the board can drag a piece on the surface above it.

---

## Next Actions

- Build a carriage prototype to test automated coupling.
- Measure maximum reliable board thickness.
- Evaluate coupling strength at different carriage speeds.
- Proceed to Hall sensor characterization (EXP-002).
