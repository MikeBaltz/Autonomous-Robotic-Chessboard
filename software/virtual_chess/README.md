# Virtual Chess Prototype

## Purpose

This is a command-line chess prototype that simulates the future move-search workflow of the physical autonomous chessboard.

Instead of using Hall sensors to detect which piece moved where, this prototype asks the human player yes/no questions to determine their move. This validates the move-ranking and search logic before the physical carriage is built.

**Human plays White. Computer plays Black.**

---

## Architecture

```
main.py          — CLI interface, game loop, user interaction
game.py          — python-chess Board wrapper, move history, FEN, game-over detection
move_ranker.py   — Deterministic scoring and ranking of legal White moves
engine.py        — Stockfish integration (optional) with fallback move selector for Black
```

### Relationship to the physical board

This prototype replaces physical Hall-sensor checks with yes/no user confirmation.

**Future physical behavior:**

```
rank legal candidate moves
→ carriage moves to candidate source square
→ carriage checks legal destinations
→ board state is updated
```

The Python prototype does **not** communicate with the ESP32. That integration will come later.

---

## Setup

### Requirements

- Python 3.11 or newer
- pip

### Installation

```bash
cd software/virtual_chess
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

### Optional: Stockfish

For stronger Black moves, install Stockfish and set the path:

```bash
export STOCKFISH_PATH=/usr/local/bin/stockfish
```

If Stockfish is not found, the program uses a deterministic fallback move selector. It will still work — just weaker play.

---

## Running

```bash
cd software/virtual_chess
source .venv/bin/activate
python main.py
```

---

## Example Session

```
==================================================
  Autonomous Chessboard — Virtual Chess Prototype
==================================================

  Human plays White. Computer plays Black.

Instructions:
  Think of a legal White move. The program will guess
  ranked candidates one at a time. Answer y/n.

r n b q k b n r
p p p p p p p p
. . . . . . . .
. . . . . . . .
. . . . . . . .
. . . . . . . .
P P P P P P P P
R N B Q K B N R

FEN: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1

--- Move 1: White's turn ---

20 candidate moves ranked.
Answer y/n for each guess.

  [1/20] Did you play e2e4? n
  [2/20] Did you play d2d4? y

White played: d2d4 (d4)

r n b q k b n r
p p p p p p p p
. . . . . . . .
. . . . . . . .
. . . P . . . .
. . . . . . . .
P P P . P P P P
R N B Q K B N R

--- Move 1: Black's turn ---
Black played: d7d5 (d5)
...
```

### Commands during questioning

| Command   | Action |
|-----------|--------|
| `board`   | Show current board |
| `fen`     | Show current FEN string |
| `history` | Show all moves played so far |
| `restart` | Restart guessing for the current turn |
| `manual`  | Enter a UCI move directly (e.g. `e2e4`) |
| `quit`    | Exit the program |

---

## Testing

```bash
cd software/virtual_chess
source .venv/bin/activate
pip install pytest
pytest tests/ -v
```

---

## Move Ranking

The ranker scores legal White moves using a deterministic function with these factors:

| Factor | Weight | Description |
|--------|--------|-------------|
| Promotion | +200 | Pawn promotion moves |
| Capture | +100 | Any capture |
| Castling | +80 | King-side or queen-side castling |
| Check | +50 | Moves that give check |
| Opening bonus | up to +40 | Common first moves (e2e4, d2d4, etc.) scaled by move number |
| Central pawn | +30 | Pawns moving from d/e files |
| Development | +25 | Knights/bishops leaving the back rank |
| Central target | +20 | Pawns moving to d/e files |
| Advancement | +10 | Pawns moving forward |
| User preference | up to +50 | Moves the user has chosen before from the same position |
| Tie-break | <0.1 | Deterministic hash-based tie-breaker |

Results are fully deterministic for the same board state and preference file.

---

## User Preferences

Move choices are saved to `user_move_preferences.json` in the virtual_chess directory.

- Keyed by board position (FEN piece placement)
- Stores how many times each move was chosen from that position
- Used as a ranking signal (more frequent = higher rank)
- The file is created automatically on first use
- Invalid JSON is handled gracefully (preferences reset to empty)
- Preferences never override move legality

---

## Limitations

- No GUI — command-line only
- No machine learning in move ranking (v1)
- No communication with the ESP32 firmware
- Stockfish is optional and not bundled
- Fallback Black move selector is weak (captures > checks > center > first legal)
- No PGN import/export yet
- No time controls

---

## Future Work

- Connect to ESP32 via serial protocol
- Replace yes/no questions with Hall sensor board scans
- Add ML-based move ranking trained on user preferences
- Add PGN export
- Add GUI or web interface
- Support Human vs Human mode
