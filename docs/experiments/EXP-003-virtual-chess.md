# EXP-003 — Virtual Chess Prototype

## Status

Completed

---

## Objective

Validate the move-search workflow planned for the physical autonomous chessboard:

1. Generate all legal moves from the current board position.
2. Rank candidates from most likely to least likely.
3. Check candidates one at a time until a match is found.
4. Apply the confirmed move and update the board state.

In the physical system, "checking" means moving the carriage and reading Hall sensors. In this prototype, "checking" means asking the human player a yes/no question.

---

## Hypothesis

A deterministic heuristic-based move ranker can order legal White moves such that the correct move is found within the first few candidates for typical positions.

---

## Equipment

- Python 3.11+
- `python-chess` library
- Stockfish engine (optional)

---

## Setup

```bash
cd software/virtual_chess
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

Optional: set `STOCKFISH_PATH` environment variable for stronger Black moves.

---

## Move Ranking Heuristics

| Factor | Weight | Description |
|---|---|---|
| Promotion | +200 | Pawn promotion moves |
| Capture | +100 | Any capture |
| Castling | +80 | King-side or queen-side castling |
| Check | +50 | Moves that give check |
| Opening bonus | up to +40 | Common first moves scaled by move number |
| Central pawn | +30 | Pawns moving from d/e files |
| Development | +25 | Knights/bishops leaving the back rank |
| Central target | +20 | Pawns moving to d/e files |
| Advancement | +10 | Pawns moving forward |
| User preference | up to +50 | Moves previously chosen from the same position |

---

## Procedure

1. Run `python3 main.py`.
2. Think of a legal White move.
3. Answer yes/no to ranked candidate guesses.
4. If all candidates are rejected, enter the move manually in UCI format.
5. Observe Black's response (Stockfish or fallback selector).
6. Repeat until game over or quit.

---

## Observations

- At the initial position, e2e4, d2d4, g1f3, and c2c4 rank in the top candidates.
- The ranker produces deterministic output for the same board state.
- Fallback Black move selector prefers captures, then checks, then center moves.
- User preferences persist across sessions and influence future ranking.
- All 21 automated tests pass.

---

## Result

The move-search workflow is viable. The yes/no questioning model correctly simulates the future physical carriage search. Move ranking produces sensible ordering for common positions.

---

## Limitations

- No machine learning — ranking is purely heuristic.
- No communication with the ESP32 firmware.
- Stockfish is optional and not bundled.
- Fallback Black move selector is weak.
- No GUI — command-line only.
- No PGN import/export.
- No time controls.

---

## Conclusion

The virtual chess prototype validates the core move-search workflow before hardware is available. The ranking heuristics provide a reasonable starting point for the physical system.

---

## Next Actions

- Collect user preference data during play sessions.
- Evaluate ranking accuracy (how many guesses before the correct move is found).
- Add ML-based ranking when sufficient preference data is available.
- Connect to ESP32 via serial protocol when hardware is ready.
- Replace yes/no questions with Hall sensor board scans.
