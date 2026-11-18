# Software

Python components under `software/`. Both target Python 3.11+.

---

## virtual_chess — move-search prototype

Validated CLI prototype of the move-search workflow that the physical board will eventually perform ([ADR-003](decisions/ADR-003-virtual-chess-prototype.md), [EXP-003](experiments/EXP-003-virtual-chess.md)). The human plays White; the computer plays Black.

Instead of moving a carriage and reading Hall sensors, the prototype asks yes/no questions about ranked candidate moves — same search structure, no hardware.

| Module | Responsibility |
|---|---|
| `main.py` | CLI game loop, candidate guessing, commands (`board`, `fen`, `history`, `restart`, `manual`, `quit`) |
| `game.py` | `python-chess` `Board` wrapper: legality, SAN, FEN, move history, game-over detection |
| `move_ranker.py` | Deterministic heuristic scoring: promotion +200, capture +100, castling +80, check +50, opening book, center/development/advancement bonuses, user-preference weight (up to +50), hash tie-break |
| `engine.py` | Optional Stockfish via `STOCKFISH_PATH` (UCI, 1 s movetime); fallback selector: captures → checks → center → first legal |

**User preferences:** choices are persisted per board position (FEN piece-placement key) into `user_move_preferences.json` (auto-created on first use; git-ignored as runtime data).

**Tests:** `tests/test_virtual_chess.py` — 21 pytest tests covering game logic, ranker determinism/legality, preference persistence, and engine fallback. Run:

```bash
cd software/virtual_chess
python3 -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
pip install pytest
pytest tests/ -v
```

Dependencies: `python-chess>=1.9.0`.

---

## chess_bridge — hardware bridge

Host application that connects the chess engine to the physical board ([`main.py`](../software/chess_bridge/)). Currently used with the **movement sketch** flashed on the ESP32-S3.

- **Game side** — reuses `virtual_chess`'s `Game`/`Engine`/`MoveRanker`; the robot plays **White** (Stockfish or fallback), the human plays **Black** by moving pieces on the board
- **Hardware side** — `ArduinoBridge` auto-detects the serial port (matches `arduino`, `ch340`, `cp210`, `esp`, `jtag` descriptors) or accepts an explicit port argument; sends line-terminated commands and waits for `DONE` (or `ERROR`)
- **`MoveExecutor`** — plans each robot move as firmware primitives: plain UCI for normal moves; for captures and en passant the victim is first carried along grid lines to a capture-lane slot; castling moves the king, then routes the rook along the row border; promotion moves the pawn, carries it to the left lane and asks the player to place the new piece
- **`CaptureLedger`** — records which captured piece is in which lane slot (L1–L8 white pieces, R1–R8 black pieces) in `capture_ledger.json` (git-ignored, reset at startup)
- **`ScanEngine` (smart scanning)** — the Hall sensor detects black pieces only, so each legal black move predicts exactly which squares hold black pieces afterwards. The engine scans one square at a time for the highest-ranked candidate that still fits every reading, usually 2–10 scans per move, and accepts a move only when no other legal move fits. Remaining ambiguity (e.g. the promotion piece) is resolved by asking the player; if nothing fits, the player types the move
- **Loop** — engine move → planned primitives → verification scans (unless `--no-verify`) → scan for the human's move (or `--manual` typing) → repeat until game over; prints move history, result, and the capture ledger

Dependencies: `chess>=1.7.0`, `pyserial>=3.5`.

```bash
python3 -m venv .venv && source .venv/bin/activate
pip install -r software/chess_bridge/requirements.txt
python software/chess_bridge/main.py [serial_port] [--manual] [--no-verify] [--ledger FILE]
```

**Tests:** `tests/test_bridge.py` — 23 tests against a mocked board (no hardware) covering move plans (captures, en passant, castling, promotion), scan detection, the capture ledger, and lane scanning. Run `python3 -m pytest software/chess_bridge/tests`.

Convenience targets exist in the repository `Makefile`: `make bridge` (flash movement sketch + start bridge) and `make test-knight`.

**Status:** full robot-vs-human games have been played on the physical board. Systematic validation across many games is ongoing — see [development-status.md](development-status.md).

---

## Relationship between the two

`virtual_chess` validated the *decision* logic (ranking, game state, engine integration) before hardware existed. `chess_bridge` is the same architecture with the yes/no questions replaced by Hall-sensor scans of the real board: the same ranked candidate list decides which square to scan next.
