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

## chess_bridge — hardware bridge (under development)

Host application that connects the chess engine to the physical board ([`main.py`](../software/chess_bridge/)). Currently used with the **movement sketch** flashed on the ESP32-S3.

- **Game side** — reuses `virtual_chess`'s `Game`/`Engine`; the robot plays **White** (Stockfish or fallback), the human enters **Black** moves in UCI at the terminal
- **Hardware side** — `ArduinoBridge` auto-detects the serial port (matches `arduino`, `ch340`, `cp210`, `esp`, `jtag` descriptors) or accepts an explicit port argument; sends UCI moves line-terminated and waits for `DONE` (or `ERROR`) with a 20 s move timeout
- **Loop** — engine move → serial dispatch → apply to game state → human move → repeat until game over; prints move history and result

Dependencies: `chess>=1.7.0`, `pyserial>=3.5`.

```bash
python3 -m venv .venv && source .venv/bin/activate
pip install -r software/chess_bridge/requirements.txt
python software/chess_bridge/main.py [serial_port]
```

Convenience targets exist in the repository `Makefile`: `make bridge` (flash movement sketch + start bridge) and `make test-knight`.

**Status:** initial integration sessions completed (serial connection, engine move dispatch). Systematic full-game validation is ongoing — see [development-status.md](development-status.md).

---

## Relationship between the two

`virtual_chess` validated the *decision* logic (ranking, game state, engine integration) before hardware existed. `chess_bridge` is the same architecture with the yes/no questioning replaced by serial commands to the real board. The next step (roadmap) is replacing human-confirmation entirely with Hall-sensor board scans verified by the firmware.
