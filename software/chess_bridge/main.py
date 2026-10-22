#!/usr/bin/env python3
"""
Chess Robot Bridge - Connects Python chess engine to Arduino hardware.

This script:
1. Manages chess game state using python-chess
2. Calculates white moves using Stockfish or fallback AI
3. Sends UCI move commands to Arduino via serial
4. Receives black moves from user input
5. Coordinates the physical chess robot
"""

import sys
import os
import time
import serial
import serial.tools.list_ports

# Add virtual_chess to path for engine and game modules
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'virtual_chess'))

from engine import Engine
from game import Game


class ArduinoBridge:
    """Handles serial communication with Arduino."""
    
    def __init__(self, port=None, baudrate=115200, timeout=5):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial = None
        
    def find_arduino(self):
        """Auto-detect Arduino serial port."""
        ports = serial.tools.list_ports.comports()
        for port in ports:
            # Common Arduino identifiers
            if any(x in port.description.lower() for x in ['arduino', 'ch340', 'cp210', 'usb serial']):
                return port.device
            # ESP32-S3 often shows as USB JTAG
            if 'esp' in port.description.lower() or 'jtag' in port.description.lower():
                return port.device
        return None
    
    def connect(self):
        """Connect to Arduino via serial."""
        if self.port is None:
            self.port = self.find_arduino()
            if self.port is None:
                print("ERROR: Could not auto-detect Arduino.")
                print("Available ports:")
                for port in serial.tools.list_ports.comports():
                    print(f"  {port.device}: {port.description}")
                return False
        
        try:
            self.serial = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=self.timeout
            )
            time.sleep(2)  # Wait for Arduino to reset
            print(f"Connected to Arduino on {self.port}")
            return True
        except serial.SerialException as e:
            print(f"ERROR: Could not connect to {self.port}: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from Arduino."""
        if self.serial and self.serial.is_open:
            self.serial.close()
            print("Disconnected from Arduino")
    
    def send_command(self, command):
        """Send a command to Arduino and wait for response."""
        if not self.serial or not self.serial.is_open:
            print("ERROR: Not connected to Arduino")
            return None
        
        # Clear any pending input
        self.serial.reset_input_buffer()
        
        # Send command
        self.serial.write(f"{command}\n".encode())
        self.serial.flush()
        
        # Wait for response
        response_lines = []
        start_time = time.time()
        
        while time.time() - start_time < self.timeout * 10:  # 20 second timeout for moves
            if self.serial.in_waiting:
                line = self.serial.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    response_lines.append(line)
                    if line == "DONE":
                        return response_lines
                    if line.startswith("ERROR"):
                        return response_lines
        
        return response_lines if response_lines else None
    
    def execute_move(self, uci_move):
        """Execute a UCI move on the Arduino."""
        print(f"Sending move to Arduino: {uci_move}")
        
        # Parse the move to show what we expect
        from_col = uci_move[0]
        from_row = uci_move[1]
        to_col = uci_move[2]
        to_row = uci_move[3]
        print(f"  Expected: {from_col}{from_row} -> {to_col}{to_row}")
        print(f"  Grid coords: ({ord(from_col) - ord('a')}, {int(from_row) - 1}) -> ({ord(to_col) - ord('a')}, {int(to_row) - 1})")
        
        response = self.send_command(uci_move)
        
        if response is None:
            print("ERROR: No response from Arduino (timeout)")
            return False
        
        for line in response:
            print(f"  Arduino: {line}")
        
        if "DONE" not in response:
            print("ERROR: Arduino did not send DONE signal")
            return False
        
        return True


class ChessRobotBridge:
    """Main bridge class that coordinates chess game with hardware."""
    
    def __init__(self, serial_port=None):
        self.game = Game()
        self.engine = Engine()
        self.bridge = ArduinoBridge(port=serial_port)
        
    def start(self):
        """Start the chess robot bridge."""
        print("=" * 50)
        print("Chess Robot Bridge")
        print("=" * 50)
        
        # Check Stockfish
        if self.engine.is_stockfish_available():
            print("Stockfish: Available")
        else:
            print("Stockfish: Not available (using fallback AI)")
        
        # Connect to Arduino
        if not self.bridge.connect():
            return False
        
        print()
        print(self.game.get_board_display())
        print()
        
        # White starts first
        return self.game_loop()
    
    def game_loop(self):
        """Main game loop."""
        move_number = 1
        
        while not self.game.is_game_over():
            # White's turn (Arduino plays)
            print(f"--- Move {move_number}: White (Arduino) ---")
            white_move = self.get_white_move()
            
            if white_move is None:
                print("ERROR: Could not get white move")
                break
            
            print(f"White plays: {white_move}")
            
            # Execute on Arduino
            if not self.bridge.execute_move(white_move):
                print("ERROR: Failed to execute move on Arduino")
                break
            
            # Apply to game state
            success, result = self.game.apply_move(white_move)
            if not success:
                print(f"ERROR: {result}")
                break
            
            print(f"Applied: {result}")
            print()
            
            # Check if game over
            if self.game.is_game_over():
                break
            
            # Black's turn (user input)
            print(f"--- Move {move_number}: Black (You) ---")
            print(self.game.get_board_display())
            print()
            
            black_move = self.get_black_move()
            if black_move is None:
                print("Game aborted by user")
                break
            
            # Apply to game state
            success, result = self.game.apply_move(black_move)
            if not success:
                print(f"Invalid move: {result}")
                print("Try again.")
                continue
            
            print(f"Black plays: {black_move} ({result})")
            print()
            
            move_number += 1
        
        # Game over
        print()
        print("=" * 50)
        print("Game Over!")
        print(self.game.get_game_result())
        print("=" * 50)
        print()
        print("Move history:")
        print(self.game.get_move_history_display())
        
        self.bridge.disconnect()
        return True
    
    def get_white_move(self):
        """Get white's move from the engine."""
        result = self.engine.get_black_move(self.game.board)
        if result and result[0]:
            return result[0]
        return None
    
    def get_black_move(self):
        """Get black's move from user input."""
        while True:
            try:
                user_input = input("Enter your move (e.g., e7e5) or 'q' to quit: ").strip().lower()
                
                if user_input == 'q':
                    return None
                
                if len(user_input) == 4:
                    # Validate format
                    if (user_input[0] >= 'a' and user_input[0] <= 'h' and
                        user_input[1] >= '1' and user_input[1] <= '8' and
                        user_input[2] >= 'a' and user_input[2] <= 'h' and
                        user_input[3] >= '1' and user_input[3] <= '8'):
                        
                        # Check if legal
                        if self.game.is_legal_move(user_input):
                            return user_input
                        else:
                            print("Illegal move. Try again.")
                    else:
                        print("Invalid format. Use: e7e5")
                else:
                    print("Invalid format. Use: e7e5")
                    
            except (EOFError, KeyboardInterrupt):
                print()
                return None


def main():
    """Main entry point."""
    # Parse optional serial port argument
    serial_port = None
    if len(sys.argv) > 1:
        serial_port = sys.argv[1]
    
    # Create and start bridge
    bridge = ChessRobotBridge(serial_port=serial_port)
    
    try:
        bridge.start()
    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
        bridge.bridge.disconnect()
        sys.exit(0)


if __name__ == "__main__":
    main()
