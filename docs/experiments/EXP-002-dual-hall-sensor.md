# EXP-002 — Dual Hall Sensor

## Status

Completed

---

## Objective

Evaluate whether two Hall sensors mounted on the carriage can distinguish:

- Empty square
- White piece
- Black piece
- Invalid state

using opposite magnet polarities.

---

## Hypothesis

Two unipolar Hall sensors mounted on opposite sides of the carriage can detect which pole of a magnet is facing them. White pieces (north up) trigger Sensor A. Black pieces (south up) trigger Sensor B.

---

## Equipment

- ESP32-S3 DevKitC
- 2× Hall-effect sensor modules
- Neodymium magnets (2×10 mm)
- Breadboard and jumper wires

---

## Configuration

| Parameter | Value |
|---|---|
| Sensor A pin | GPIO4 |
| Sensor B pin | GPIO5 |
| Pin mode | INPUT_PULLDOWN |
| Active level | LOW |
| Debounce | 5 consecutive identical samples at 10 ms intervals |

---

## State Mapping

| Sensor A active | Sensor B active | Square state |
|---|---|---|
| false | false | Empty |
| true | false | White |
| false | true | Black |
| true | true | Invalid |

---

## Analog Exploratory Measurements

Analog output readings taken during initial sensor characterization:

| Condition | Approximate reading |
|---|---|
| No magnet nearby | ≈4095 |
| Magnet very close | ≈330 |
| Practical detection distance | ≈10 mm |

These measurements were exploratory. The current classification firmware uses digital outputs only.

---

## Procedure

1. Connect Sensor A to GPIO4 and Sensor B to GPIO5.
2. Flash firmware and open serial monitor.
3. Present a magnet with north pole facing Sensor A.
4. Flip the magnet so south pole faces Sensor A.
5. Remove the magnet entirely.
6. Observe serial output for state transitions.

---

## Observations

- Sensor A triggered when the north pole faced it.
- Sensor B triggered when the south pole faced it (opposite orientation).
- With no magnet present, both sensors reported inactive → Empty.
- Both sensors triggering simultaneously was not observed during normal testing.

---

## Result

The two Hall sensors successfully distinguished magnet polarity. The firmware correctly classifies Empty, White, Black, and Invalid states.

---

## Limitations

- Sensors were tested individually, not simultaneously mounted in the intended carriage geometry.
- Calibration and repeatability across all 64 squares have not been evaluated.
- The magnetic actuator (planned) may interfere with sensor readings.
- External magnetic fields were not tested.
- 100% reliability has not been formally measured.

---

## Conclusion

Dual Hall sensors with opposite magnet polarity can classify square occupancy and piece color. No camera or RFID system is required.

---

## Next Actions

- Mount both sensors on a carriage in the intended geometry.
- Validate scanning at repeatable positions across the full board.
- Evaluate sensor interference from the magnetic actuator.
- Structured repeatability test with both sensors in final mounting position.
