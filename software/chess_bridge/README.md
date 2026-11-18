# Chess Robot Bridge

Python host application that connects the chess engine (Stockfish or the fallback ranker) to the ESP32-S3 movement firmware (`sketches/movement/`).

## Overview

This bridge script:
- Manages chess game state using `python-chess`
- Calculates white moves using Stockfish (or fallback AI)
- Decomposes special moves (castling, en passant, captures, promotions) into firmware primitives
- Detects black moves via smart hall-sensor scanning (no typing needed)
- Tracks all captured pieces in a side-lane ledger

## Requirements

```bash
pip install -r requirements.txt
```

- `chess` - Chess logic library
- `pyserial` - Serial communication with Arduino
- Stockfish (optional) - Strong chess AI

## Usage

### 1. Flash Arduino

```bash
pio run --target upload -d sketches/movement
```

### 2. Run the Bridge

```bash
python software/chess_bridge/main.py
```

Options:

```bash
python software/chess_bridge/main.py /dev/cu.usbmodem1234   # explicit port
python software/chess_bridge/main.py --manual               # type black moves instead of scanning
python software/chess_bridge/main.py --no-verify           # skip post-move verification scans
python software/chess_bridge/main.py --ledger mygame.json  # custom capture ledger file
```

### 3. Play Chess

1. Connect to Arduino
2. Arduino (white) calculates its move, physically executes it, verifies with scans
3. You (black) move your piece by hand
4. The bridge scans the board to detect your move (see Smart Scanning below)
5. Repeat until game over, then the captured-piece ledger is printed

## Move Decomposition

The bridge sends plain UCI for normal moves. Special moves are decomposed
into firmware primitives (`g`, `o`, `O`, `path`):

| Move type | Sequence |
|-----------|----------|
| Normal | UCI move (knight/bishop geometry handled by firmware) |
| Capture | Pick up victim, carry to lane slot via grid-line route, then UCI move |
| En passant | Same, but victim square is the passed pawn's square |
| Castling | King moves directly (path is clear), rook routes along the row border via grid line |
| Promotion | Pawn move, pawn carried to left lane, you place the new piece (white pieces can't be scanned) |

Carries use grid lines (borders between squares) so dragged pieces never
pass over other pieces. Lane routes exit via the nearest row border and
travel along the board edge.

## Capture Lanes

- **Left lane (L1-L8, x=-0.5): white pieces** - you place pieces you capture here
- **Right lane (R1-R8, x=7.5): black pieces** - the robot places pieces it captures here

The ledger (`capture_ledger.json`, auto-reset per game) records what piece
is in which slot and where it was captured from, enabling post-game
reassembly. Black pieces the robot carries to the right lane are
confirmed by a scan. White pieces you capture are invisible to the sensor,
so the bridge assumes you put each one on the first free left-lane slot.

## Smart Scanning

The hall sensor only senses **black** pieces (single polarity); white
pieces read empty. Every legal move therefore predicts exactly which
squares should hold a black piece afterwards, captures included (the
target square changes from empty to magnet).

Squares are scanned one at a time, always for the highest-ranked
candidate (from `move_ranker.py`) that still fits every reading so far,
until only moves whose changed squares have all been checked remain.
Reading a from-square rules out every move of that piece at once, so a
typical move needs 2-10 scans. A move is accepted only if no other legal
move fits the readings. Promotions (which piece?) prompt you to choose.
Typing the move is always available as a fallback.

## Arduino Commands

| Command | Action |
|---------|--------|
| `e2e4` / `e7e8q` | UCI move (with optional promotion letter) |
| `g 3.5 2.0` | Move to grid coordinates |
| `path 7 0.5 5 0.5 5 0` | Move through waypoints |
| `lane R3` | Move to capture lane slot (L1-L8, R1-R8); manual positioning only, the bridge routes lanes with `g`/`path` |
| `scan e4` | Scan square, replies `SCAN e4 M\|E\|X` |
| `scanpos -0.5 3` | Scan grid coordinates |
| `m a3`, `a-h`, `1-8` | Manual positioning |
| `l` / `r` / `t` / `w` | Surround lanes |
| `o` / `O` | Electromagnet ON / OFF |
| `s` | Toggle sensor mode |
| `p`, `x`, `d`, `h` | Position, reset, diagnostic, help |

Every command replies `DONE` on success or `ERROR: ...` on invalid input.

## Architecture

```
┌─────────────────────┐     Serial      ┌──────────────────┐
│   Python Bridge     │ ◄─────────────► │     ESP32-S3     │
│                     │                │                  │
│ - python-chess      │  g/o/O/path ──► │ - Motor control  │
│ - Stockfish         │  uci           │ - Electromagnet  │
│ - MoveExecutor      │  scan ◄─────── │ - Hall sensor    │
│ - ScanEngine        │                │ - Path geometry  │
│ - CaptureLedger     │                │   (knight/diag)  │
└─────────────────────┘                └──────────────────┘
```

Python decides *what* to move. The ESP32 decides *how* to move.

## Tests

```bash
python3 -m pytest software/chess_bridge/tests   # 23 tests
# or: python3 -m unittest discover -s software/chess_bridge/tests
```

Tests run against a mocked Arduino (no hardware needed) and cover move
plans (captures, en passant, castling, promotions), scan detection
(probe, diff, refutation), the capture ledger, and lane scanning.
