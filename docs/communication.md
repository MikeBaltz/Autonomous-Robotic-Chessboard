# Communication Protocol

The host ↔ firmware link is USB CDC serial at 115200 baud, line-based text.

---

## Implemented protocol (movement sketch ↔ chess_bridge)

This is the protocol currently running on the physical board. The Python `chess_bridge` is the reference client ([software.md](software.md)).

### PC → ESP32

| Command | Description | Status |
|---|---|---|
| `e2e4` / `e7e8q` (UCI, optional promotion letter) | Execute a physical chess move: navigate to source, energize electromagnet (pickup), transport (knight grid-line, diagonal, otherwise X-then-Y), place, reply `DONE`. The promotion letter is logged; the replacement piece is placed by hand | Implemented |
| `g x y` | Move to absolute grid coordinates (bounds −1.5 … 8.5) | Implemented |
| `path x1 y1 x2 y2 …` | Move through waypoints in order | Implemented |
| `scan e4` | Move the Hall sensor over the square and read it; replies `SCAN e4 M\|E\|X` (magnet / empty / unstable) | Implemented |
| `scanpos x y` | Same as `scan`, at grid coordinates (e.g. capture-lane slots) | Implemented |
| `lane L1`…`R8` | Move to a capture-lane slot (manual positioning; the bridge routes lanes with `g`/`path`) | Implemented |
| `m a3` / `m a` | Move carriage to square / column | Implemented |
| `a`–`h`, `1`–`8` | Move to column / row | Implemented |
| `l` `r` `t` `w` | Park in left/right/top/bottom surround | Implemented |
| `s` | Toggle sensor mode (targets compensate the 15 mm Hall-sensor offset) | Implemented |
| `o` / `O` | Electromagnet ON / OFF | Implemented |
| `x` | Return to home/center | Implemented |
| `p` | Print current position | Implemented |
| `h` | Help | Implemented |
| `d` / `E` | Pin diagnostics / force-enable motors | Implemented |

### ESP32 → PC

| Response | Description | Status |
|---|---|---|
| `DONE` | Command completed (bridge treats this as success; every successful command ends with it) | Implemented |
| `SCAN <sq> M\|E\|X` / `SCANPOS <x> <y> M\|E\|X` | Hall reading: `M` black piece, `E` empty or white piece (single-polarity sensor), `X` unstable | Implemented |
| `ERROR: …` | Invalid move format or square | Implemented |
| Human-readable logs | Move progress, positions, diagnostics | Implemented |

### Main firmware CLI (`src/`)

The modular main firmware implements a similar line-based CLI (see `protocol.cpp`): `e2e4`, `m a`/`m a3`, `p` (board), `r` (reset), `s` (status), `scan`, `1/0/t` (electromagnet), `x/y` (motor toggles), `h` (help).

---

## Draft protocol (future)

Draft command/response vocabulary for the integrated system. **Status: unimplemented** — retained as a design reference for the planned framed binary protocol.

### Commands

| Command | Direction | Description | Status |
|---|---|---|---|
| `PING` | PC → ESP32 | Check if firmware is alive | Unimplemented |
| `STATUS` | PC → ESP32 | Request current state, position, sensor readings | Unimplemented |
| `HOME` | PC → ESP32 | Start homing sequence | Unimplemented |
| `SCAN` | PC → ESP32 | Scan all 64 squares, return board state | Unimplemented |
| `STOP` | PC → ESP32 | Emergency stop, disable all motors | Unimplemented |

### Responses

| Response | Direction | Description | Status |
|---|---|---|---|
| `OK` | ESP32 → PC | Command accepted | Unimplemented |
| `ERROR` | ESP32 → PC | Command rejected with error code | Unimplemented |
| `BOARD` | ESP32 → PC | 64-square board state array | Unimplemented |

---

## Notes

- Binary framing, CRC, and sequence numbers are planned (see [firmware_architecture.md](firmware_architecture.md) §6).
- The bridge currently relies on `DONE`/`ERROR`/`SCAN` lines plus human-readable logs; timeouts are enforced client-side.
- A stop/emergency-stop command does not exist yet in the running protocol — motors can be disabled via `E`-family diagnostics or power removal. A real e-stop is a roadmap item.
