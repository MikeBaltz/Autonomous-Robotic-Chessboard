# State Machine

## Current behavior (as implemented)

The current firmware does **not** run a formal state machine. Control flow is simple and sequential:

```
power-on
  → setup(): initialize serial, GPIO, subsystems; print command help
  → loop(): poll sensors / process serial input / execute commands
       command execution is blocking (motion runs to completion before
       the next command is read)
```

Position is dead-reckoned in square units; there is no fault state, no homing state, and no interlock preventing motion while the electromagnet is energized. Unknown commands print an error and the prompt continues.

## Planned formal state machine

The aspirational firmware design ([firmware_architecture.md](firmware_architecture.md) §5) defines a full FSM:

| State | Meaning |
|---|---|
| `BOOT` → `INIT` → `SELF_TEST` → `IDLE` | Startup and diagnostics |
| `HOMING` → `HOMED` | Endstop-based position reference (endstops not yet installed) |
| `CALIBRATING` → `READY` | Board geometry and sensor threshold calibration |
| `MOVING` / `SCANNING` | Motion and scan operations, rejecting concurrent commands |
| `ERROR` / `EMERGENCY_STOP` | Fault containment with explicit recovery |

Transition tables, entry/exit actions, and the FreeRTOS task layout are specified there. Implementing this FSM (or a lightweight subset of it) is a Phase 5 roadmap item.
