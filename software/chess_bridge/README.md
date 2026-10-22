# Chess Robot Bridge

Python script that connects the chess engine (Stockfish) to the Arduino hardware.

## Overview

This bridge script:
- Manages chess game state using `python-chess`
- Calculates white moves using Stockfish (or fallback AI)
- Sends UCI move commands to Arduino via serial
- Receives black moves from user input
- Coordinates the physical chess robot movements

## Requirements

```bash
pip install -r requirements.txt
```

- `chess` - Chess logic library
- `pyserial` - Serial communication with Arduino
- Stockfish (optional) - Strong chess AI

## Usage

### 1. Flash Arduino

First, flash the movement sketch to your Arduino:

```bash
pio run --target upload -d sketches/movement
```

### 2. Run the Bridge

```bash
python software/chess_bridge/main.py
```

Or specify a serial port:

```bash
python software/chess_bridge/main.py /dev/cu.usbmodem1234
```

### 3. Play Chess

The script will:
1. Connect to Arduino
2. Display the board
3. Calculate white's move using Stockfish
4. Execute the move on Arduino (pick up piece, move, place down)
5. Wait for your black move input
6. Repeat until game over

## Commands

- Enter moves in UCI format: `e7e5` (from-to squares)
- `q` - Quit the game

## Arduino Commands

The Arduino sketch accepts:
- UCI moves: `e2e4` (pick up, move, place)
- Manual commands: `a-h`, `1-8`, `m a3`, `l`, `r`, `t`, `w`, `p`, `x`, `o`, `O`, `s`, `d`

## Architecture

```
┌─────────────────┐     Serial      ┌─────────────────┐
│   Python Bridge │ ◄─────────────► │    Arduino      │
│                 │                 │                 │
│ - python-chess  │                 │ - Motor control │
│ - Stockfish     │                 │ - Electromagnet │
│ - Game state    │                 │ - Positioning   │
└─────────────────┘                 └─────────────────┘
```

## Notes

- White pieces are played by Arduino (controlled by Stockfish)
- Black pieces are played by you (user input)
- Captured pieces must be removed manually
- The Arduino waits at the destination square after placing a piece
