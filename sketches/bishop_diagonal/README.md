# Bishop Diagonal Movement Test

Test sketch for bishop diagonal movement with simultaneous X/Y axis motion.

## Purpose

Bishops move diagonally on the chessboard. This sketch demonstrates diagonal movement by interleaving X and Y motor steps, creating smooth diagonal paths between squares.

## Movement Strategy

Instead of moving X then Y (L-shaped path), the bishop moves both axes simultaneously (or nearly so) to create a diagonal trajectory. This is more efficient and mimics the actual bishop movement pattern.

### Diagonal Movement Implementation

The `moveDiagonal()` function:
- Calculates steps needed for both X and Y axes
- Interleaves step pulses to both motors
- Uses acceleration/deceleration for smooth motion
- Moves both axes at the same time when possible

## Test Commands

| Command | Action |
|---------|--------|
| `c1f4` | Bishop diagonal from c1 to f4 (3 squares up-right) |
| `d1a4` | Bishop diagonal from d1 to a4 (3 squares up-left) |
| `e2e4` | UCI move (must be diagonal) |

## Movement Sequence

For a bishop move (e.g., c1 to f4):
1. Move to source square (c1)
2. Pick up piece
3. Move diagonally to target (f4) - both X and Y move simultaneously
4. Place piece

## Commands

| Command | Action |
|---------|--------|
| `c1f4` | Test diagonal c1→f4 |
| `d1a4` | Test diagonal d1→a4 |
| `e2e4` | UCI move (must be diagonal) |
| `s` | Toggle sensor mode |
| `m a3` | Move to square a3 |
| `1-8` | Move to row |
| `a-h` | Move to column |
| `l` | Move to left surround |
| `r` | Move to right surround |
| `t` | Move to top surround |
| `w` | Move to bottom surround |
| `p` | Print current position |
| `x` | Reset to center (4.5, 4.5) |
| `o` | Electromagnet ON |
| `O` | Electromagnet OFF |
| `d` | Diagnostic |

## Usage

```bash
# Upload the sketch
pio run --target upload -d sketches/bishop_diagonal

# Open serial monitor
pio device monitor -d sketches/bishop_diagonal --echo
```

Then type `c1f4` and press Enter to test diagonal movement.

## Future Work

- Obstacle detection: Check if path is clear before moving
- Intermediate waypoints: If path blocked, find alternative route
- Integration with chess bridge: Auto-detect bishop moves and use diagonal navigation

## Notes

- The sketch starts at center position (4.5, 4.5)
- Use `x` command to return to center if needed
- Diagonal moves must have equal X and Y distance (|dx| == |dy|)
- Non-diagonal UCI moves will be rejected
