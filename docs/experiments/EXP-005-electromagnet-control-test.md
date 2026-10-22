# EXP-005 — Electromagnet Control Test

## Status

In Progress — control validated, power debugging needed

> **Note (2026-10):** this experiment was performed with the electromagnet on GPIO15. The pin has since moved to **GPIO39** (see [hardware.md](../hardware.md)); the findings below describe the original wiring.

---

## Objective

Validate electromagnet control system:

- Verify GPIO control via MOSFET driver
- Test serial command interface
- Confirm integration with motor and sensor systems
- Debug power delivery issue

---

## Equipment

- **Electromagnet:** 12V DC electromagnet
- **Driver:** MOSFET module with indicator LED
- **Controller:** ESP32-S3 DevKitC
- **Power supply:** 12V DC (shared with motor)
- **Control pin:** GPIO15

---

## Wiring

| Component | Connection | Notes |
|---|---|---|
| Electromagnet (+) | 12V power supply | Direct connection |
| Electromagnet (-) | MOSFET drain | Switched ground |
| MOSFET source | Common ground | Connected to ESP32 GND |
| MOSFET gate | GPIO15 | Control signal |

---

## Procedure

### 1. Initial Setup

- Connected electromagnet to 12V power supply
- Connected MOSFET between electromagnet and ground
- Connected MOSFET gate to GPIO15
- Verified common ground between all components

### 2. Firmware Upload

Uploaded firmware with:
- Electromagnet class implementation
- Serial command interface (1/0/t/s)
- Integration with existing motor test sequence

### 3. Control Testing

Opened serial monitor and tested commands:

1. Sent `1` — turn electromagnet ON
2. Sent `0` — turn electromagnet OFF
3. Sent `t` — toggle state
4. Sent `s` — check status

### 4. Physical Testing

Attempted to pick up chess pieces with electromagnet energized.

---

## Observations

### Serial Output

```
=== System Initialization ===
[SENSOR] Initialized
[BOARD] Initialized
[MOTOR] Initialized
[ELECTROMAGNET] Initialized
=== Motor Integration Test ===
[MOTOR] Enabled
--- Test 1: Forward movement ---
[MOTOR] Moving 1000 steps at 500 steps/sec
--- Pause 2 seconds ---
--- Test 2: Backward movement ---
[MOTOR] Moving 1000 steps at 500 steps/sec
--- Pause 2 seconds ---
--- Test 3: Fast movement ---
[MOTOR] Moving 2000 steps at 1000 steps/sec
--- Pause 2 seconds ---
--- Test 4: Slow movement ---
[MOTOR] Moving 500 steps at 200 steps/sec
=== Motor test complete ===
[MOTOR] Disabled
=== Electromagnet Control ===
Commands:
  1 = Turn ON
  0 = Turn OFF
  t = Toggle
  s = Status
```

### Command Testing

| Command | Expected | Observed | Result |
|---|---|---|---|
| `1` | Electromagnet ON | MOSFET LED ON, serial: "[ELECTROMAGNET] ON" | ✓ Pass |
| `0` | Electromagnet OFF | MOSFET LED OFF, serial: "[ELECTROMAGNET] OFF" | ✓ Pass |
| `t` | Toggle state | LED toggles, serial confirms state | ✓ Pass |
| `s` | Show status | Serial shows current state | ✓ Pass |

### Physical Testing

**Issue observed:** MOSFET LED responds correctly to commands, but electromagnet does not generate magnetic force. Pieces are not attracted when electromagnet is energized.

---

## Results

**Control system validation: PASSED**

✓ GPIO15 control working correctly
✓ MOSFET switching confirmed (LED indicator)
✓ Serial command interface functional
✓ Integration with motor and sensor systems successful
✓ No interference with Hall sensor readings observed

**Physical operation: FAILED**

✗ Electromagnet not generating magnetic force
✗ Pieces not attracted when energized

---

## Troubleshooting

### Suspected Causes

1. **Power supply issue**
   - Insufficient current capacity
   - Voltage drop under load
   - Power supply not connected to electromagnet

2. **Wiring problem**
   - Loose connection at electromagnet terminals
   - Broken wire
   - Incorrect polarity

3. **Electromagnet defect**
   - Internal coil break
   - Manufacturing defect

### Diagnostic Steps

1. **Measure voltage at electromagnet terminals**
   - Expected: 12V when energized
   - If 0V: wiring or power supply issue
   - If 12V but no force: electromagnet defect

2. **Measure current draw**
   - Expected: 0.5-2A depending on electromagnet
   - If 0A: open circuit (broken wire or coil)
   - If very high: short circuit

3. **Check continuity**
   - Measure resistance across electromagnet coil
   - Expected: few ohms to tens of ohms
   - If infinite: open circuit (broken coil)

4. **Test with known good power supply**
   - Connect electromagnet directly to 12V battery
   - Should energize immediately
   - If not: electromagnet is defective

---

## Lessons Learned

1. **Control validation is separate from power validation** — The control system can work perfectly while the power delivery fails.

2. **LED indicators are helpful** — The MOSFET LED confirmed the control signal was working, narrowing the problem to power delivery.

3. **Integration testing is important** — Testing electromagnet alongside motor and sensors revealed no interference issues early.

---

## Next Actions

1. **Immediate:**
   - Measure voltage at electromagnet terminals when energized
   - Check continuity of electromagnet coil
   - Verify power supply current capacity

2. **If power issue found:**
   - Repair wiring or upgrade power supply
   - Re-test physical operation

3. **If electromagnet defective:**
   - Replace electromagnet
   - Re-test with new unit

4. **After power issue resolved:**
   - Test holding force with actual chess pieces
   - Measure heat generation during extended operation
   - Test pickup/release sequences
   - Integrate with carriage movement
