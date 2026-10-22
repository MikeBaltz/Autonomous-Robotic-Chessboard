# ADR-003 — Virtual Chess Prototype

## Status

Accepted

---

## Context

The physical autonomous chessboard requires a move-search workflow: rank legal candidate moves, move the carriage to each square, check with Hall sensors, and update the board state when a match is found.

This workflow cannot be validated until the carriage, motors, and sensor integration are complete. However, the move-ranking logic, game state management, and search strategy can be prototyped independently.

---

## Decision

Build a Python command-line prototype that simulates the future physical move-search workflow.

Instead of Hall sensor checks, the program asks the human player yes/no questions to confirm their move. This validates the ranking algorithm, game flow, and Stockfish integration before hardware is ready.

**Human plays White. Computer plays Black.**

---

## Considered Options

| Option | Pros | Cons |
|---|---|---|
| **Python CLI prototype** | Fast to build, no hardware needed, easy to iterate | No physical validation |
| **Full firmware simulation** | Tests ESP32 logic directly | Requires mock HAL, slower iteration |
| **Wait for hardware** | No throwaway code | Blocks software development on mechanical progress |
| **GUI prototype** | Visual feedback | Unnecessary complexity for v1, harder to extend later |

---

## Rationale

- Python with `python-chess` provides legal move generation, validation, FEN, SAN, and game-over detection without reimplementing chess rules.
- The yes/no questioning workflow mirrors the future physical search: ranked candidates are checked one at a time until a match is found.
- Stockfish integration can be validated independently of hardware.
- Move-ranking heuristics can be tuned and tested before deployment to the physical system.
- The prototype is disposable — its purpose is to validate the workflow, not to become the final software.

---

## Architecture

```
software/virtual_chess/
├── main.py          — CLI interface, game loop
├── game.py          — python-chess Board wrapper
├── move_ranker.py   — Deterministic move scoring
├── engine.py        — Stockfish wrapper + fallback
├── requirements.txt
└── tests/
```

---

## Consequences

- The prototype does not communicate with the ESP32.
- Move ranking is deterministic and heuristic-based (no ML in v1).
- The physical carriage search will replace yes/no questions with Hall sensor checks.
- User preference data collected by the prototype may inform future ranking models.
- The prototype code is not intended to be reused directly in the final system.

---

## Future Physical Mapping

| Prototype step | Physical equivalent |
|---|---|
| Rank legal candidate moves | Rank legal candidate moves |
| Ask "Did you play e2e4?" | Carriage moves to e2, checks source square |
| User answers yes/no | Hall sensor confirms piece presence/absence |
| Apply move to board state | Carriage tracks piece to destination, board state updated |

---

## Validation Required

- Move ranking produces sensible ordering for common positions.
- Stockfish integration works when available.
- Fallback move selector always returns legal moves.
- Game-over conditions are detected correctly.
- User preference persistence works across sessions.
