# ADR-005 — Electromagnet Actuator

## Status

Accepted — control validated, power debugging in progress

> **Update (2026-10):** the control pin has since moved from GPIO15 (documented below) to **GPIO39**; see [hardware.md](../hardware.md) for the current pin map.

---

## Context

The carriage needs a mechanism to pick up and release chess pieces. The system must:

- Securely hold pieces during movement
- Release pieces precisely at target squares
- Be controllable via software
- Not interfere with Hall sensor readings

---

## Decision

Use an **electromagnet** controlled via **MOSFET driver** for piece pickup and release.

---

## Considered Options

| Option | Pros | Cons |
|---|---|---|
| **Electromagnet** | Simple, fast, strong holding force, software controllable | Requires power, generates heat |
| **Servo + gripper** | Precise, no power when holding | Complex mechanism, slow, bulky |
| **Solenoid** | Fast, simple | Limited stroke, weak holding force |
| **Permanent magnet + servo** | No power to hold | Complex release mechanism, slower |

---

## Rationale

The electromagnet was selected because:

- **Simplicity** — single component, no moving parts
- **Speed** — instant on/off response
- **Strength** — sufficient holding force for chess pieces
- **Control** — simple GPIO control via MOSFET
- **Compact** — fits within carriage constraints
- **Cost** — inexpensive and readily available

---

## Wiring

| Component | Connection | Notes |
|---|---|---|
| Electromagnet (+) | 12V power supply | Direct connection |
| Electromagnet (-) | MOSFET drain | Switched ground |
| MOSFET source | Common ground | Connected to ESP32 GND |
| MOSFET gate | GPIO15 | Control signal |

**Control logic:**
- GPIO15 HIGH → MOSFET ON → electromagnet energized → piece held
- GPIO15 LOW → MOSFET OFF → electromagnet off → piece released

---

## Implementation

### Electromagnet Class

```cpp
class Electromagnet {
public:
    Electromagnet(uint8_t pin);
    void begin();
    void enable();   // Turn ON (pick up piece)
    void disable();  // Turn OFF (release piece)
    void toggle();
    bool isEnabled() const;
};
```

### Serial Commands

For testing and debugging:

- `1` — Turn electromagnet ON
- `0` — Turn electromagnet OFF
- `t` — Toggle state
- `s` — Show current status

---

## Validation Results

Control signal validation completed:

✓ GPIO15 control working
✓ MOSFET switching correctly (LED indicator confirms)
✓ Serial command interface functional
✓ Integration with motor and sensor systems successful

**Current issue:** Electromagnet not physically pulling pieces. MOSFET LED responds to commands, but no magnetic force observed.

**Suspected causes:**
- Power supply issue (insufficient current)
- Wiring problem (electromagnet not receiving power)
- Electromagnet defect

See [EXP-005](../experiments/EXP-005-electromagnet-control-test.md) for detailed experiment report.

---

## Consequences

- Electromagnet requires 12V power supply (shared with motor)
- Generates heat during extended use — may need thermal management
- May interfere with Hall sensors if not properly shielded
- Must be disabled during sensor readings to avoid false positives

---

## Next Steps

- Debug electromagnet power issue
- Test holding force with actual chess pieces
- Measure heat generation during extended operation
- Add thermal protection if needed
- Integrate with piece pickup/release sequences
- Test interference with Hall sensors
