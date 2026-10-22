# Knight Offset Test

Test sketch for obstacle-clearance movement when a knight's path is blocked by adjacent pieces.

## Purpose

When a knight tries to leave its square (e.g., g1→f3), adjacent pieces (f2, g2 pawns) can block the exit path. This sketch demonstrates a strategy to temporarily move blocking pieces aside, execute the knight move along grid lines for maximum clearance, then restore all pieces.

## Movement Sequence: g1 → f3

The sketch implements this 11-step sequence:

1. **Go to g1, pick up knight**
   - Position: (7, 1)
   - Action: `moveToSquare('g', 1)` + `pickUpPiece()`

2. **Shift knight left to create exit corridor**
   - Position: (6.75, 1)
   - Action: `moveToPosition(7.0 - CLEARANCE_OFFSET, 1.0)` + `placePiece()`
   - Clearance: 0.25 units (11.3mm) left

3. **Move f2 pawn left**
   - Position: (5.75, 2)
   - Action: Pick up f2 pawn, move left 0.25 units, place
   - Creates gap on left side

4. **Move g2 pawn right**
   - Position: (7.25, 2)
   - Action: Pick up g2 pawn, move right 0.25 units, place
   - Creates gap on right side

5. **Pick up knight again**
   - Position: (6.75, 1)
   - Action: Return to knight, pick up

6. **Move to grid intersection**
   - Position: (6.5, 1.5)
   - Action: Move to border between f/g columns, between rows 1/2

7. **Move along grid line through gap**
   - Position: (6.5, 2.5)
   - Action: Move along f/g column border, passing between displaced pawns
   - This maximizes clearance from both f2 and g2 pieces

8. **Enter f3 center**
   - Position: (6, 3)
   - Action: `moveToSquare('f', 3)` + `placePiece()`

9. **Restore f2 pawn**
   - Action: Pick up from (5.75, 2), return to (6, 2)

10. **Restore g2 pawn**
    - Action: Pick up from (7.25, 2), return to (7, 2)

11. **Return to center**
    - Position: (4.5, 4.5)

## Key Concepts

### Grid Line Navigation
Instead of moving through square centers (where pieces are), the knight travels along grid lines (borders between squares). This maximizes distance from adjacent pieces.

### Temporary Displacement
Blocking pieces are moved `CLEARANCE_OFFSET = 0.25` units (11.3mm) away from the intended path. These are purely physical movements - the logical chess state is unchanged.

### Restoration
After the knight reaches its destination, all temporarily displaced pieces are returned to their exact original positions.

## Commands

| Command | Action |
|---------|--------|
| `k` | Execute knight g1→f3 with obstacle clearance |
| `s` | Toggle sensor mode (hall sensor targeting) |
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
pio run --target upload -d sketches/knight_offset

# Open serial monitor
pio device monitor -d sketches/knight_offset --echo
```

Then type `k` and press Enter to execute the knight move sequence.

## Calibration

- `CLEARANCE_OFFSET = 0.25` units (11.3mm)
- `STEPS_PER_COLUMN = 2220` steps
- `STEPS_PER_ROW = 2220` steps
- `SQUARE_PITCH = 45.1` mm

## Future Work

This is a specific implementation for g1→f3. Future work includes:
- Generalizing to other knight moves with different obstacle configurations
- Automatic obstacle detection using hall sensor
- Dynamic path planning based on board state
- Similar strategies for bishop and other pieces

## Notes

- The sketch starts at center position (4.5, 4.5)
- Use `x` command to return to center if needed
- The movement sequence assumes pieces are present at g1, f2, and g2
- All temporary positions are stored and restored automatically
