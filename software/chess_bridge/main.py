#!/usr/bin/env python3
"""
Chess Robot Bridge - Connects Python chess engine to Arduino hardware.

This script:
1. Manages chess game state using python-chess
2. Calculates white moves using Stockfish or fallback AI
3. Decomposes special moves (castling, en passant, captures, promotion)
   into firmware primitives (g/o/O/path/lane/scan)
4. Detects black moves via hall-sensor scanning (the sensor only senses
   black pieces; candidates are refuted square by square in ranked order)
5. Tracks captured pieces in a lane ledger (L1-L8 white, R1-R8 black)
"""

import argparse
import json
import re
import sys
import os
import time
from pathlib import Path

import chess
import serial
import serial.tools.list_ports

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'virtual_chess'))

from engine import Engine
from game import Game
from move_ranker import MoveRanker

SQUARE_PITCH = 45.1
HALL_Y_OFFSET = 15.0

LANE_X = {'L': -0.5, 'R': 7.5}
LANE_SLOTS = 8

PIECE_NAMES = {
    chess.QUEEN: 'queen',
    chess.ROOK: 'rook',
    chess.BISHOP: 'bishop',
    chess.KNIGHT: 'knight',
}


class CommandError(Exception):
    pass


class LaneFullError(Exception):
    pass


def fmt(v):
    s = f"{v:.2f}".rstrip('0').rstrip('.')
    return s if s else '0'


def sq_grid(square):
    return (float(chess.square_file(square)), float(chess.square_rank(square)))


def slot_y(slot):
    return float(int(slot[1]) - 1)


def border_route_same_row(src, dst):
    fx, fy = src
    tx, ty = dst
    return [(fx, fy + 0.5), (tx, fy + 0.5), (tx, ty)]


def lane_route(src, edge_x, sy):
    fx, fy = src
    return [(fx, fy + 0.5), (edge_x, fy + 0.5), (edge_x, sy)]


def path_cmd(waypoints):
    parts = ['path']
    for x, y in waypoints:
        parts.append(fmt(x))
        parts.append(fmt(y))
    return ' '.join(parts)


def g_cmd(x, y):
    return f"g {fmt(x)} {fmt(y)}"


def castle_rook_squares(move):
    rank = chess.square_rank(move.from_square)
    if chess.square_file(move.to_square) > chess.square_file(move.from_square):
        return chess.square(7, rank), chess.square(5, rank)
    return chess.square(0, rank), chess.square(3, rank)


def ep_victim_square(move):
    return chess.square(chess.square_file(move.to_square), chess.square_rank(move.from_square))


class ArduinoBridge:
    """Handles serial communication with the Arduino."""

    def __init__(self, port=None, baudrate=115200, timeout=5):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial = None

    def find_arduino(self):
        ports = serial.tools.list_ports.comports()
        for port in ports:
            if any(x in port.description.lower() for x in ['arduino', 'ch340', 'cp210', 'usb serial']):
                return port.device
            if 'esp' in port.description.lower() or 'jtag' in port.description.lower():
                return port.device
        return None

    def connect(self):
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
            time.sleep(2)
            print(f"Connected to Arduino on {self.port}")
            return True
        except serial.SerialException as e:
            print(f"ERROR: Could not connect to {self.port}: {e}")
            return False

    def disconnect(self):
        if self.serial and self.serial.is_open:
            self.serial.close()
            print("Disconnected from Arduino")

    def send_command(self, command):
        if not self.serial or not self.serial.is_open:
            raise CommandError("Not connected to Arduino")

        self.serial.reset_input_buffer()
        self.serial.write(f"{command}\n".encode())
        self.serial.flush()

        response_lines = []
        start_time = time.time()

        while time.time() - start_time < self.timeout * 10:
            if self.serial.in_waiting:
                line = self.serial.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    response_lines.append(line)
                    if line == "DONE":
                        return response_lines
                    if line.startswith("ERROR"):
                        return response_lines
            else:
                time.sleep(0.005)

        raise CommandError(f"Timeout waiting for response to: {command}")

    def execute(self, command):
        lines = self.send_command(command)
        if not any(line == "DONE" for line in lines):
            errors = [line for line in lines if line.startswith("ERROR")]
            raise CommandError(f"Command failed: {command} ({'; '.join(errors) or 'no DONE'})")
        return lines

    def scan_square(self, name):
        lines = self.execute(f"scan {name}")
        for line in lines:
            m = re.match(r"^SCAN \S+ ([EMX])$", line)
            if m:
                return m.group(1)
        raise CommandError(f"No scan result for {name}: {lines}")

    def scan_grid(self, x, y):
        lines = self.execute(f"scanpos {fmt(x)} {fmt(y)}")
        for line in lines:
            m = re.match(r"^SCANPOS \S+ \S+ ([EMX])$", line)
            if m:
                return m.group(1)
        raise CommandError(f"No scan result for {x},{y}: {lines}")


class CaptureLedger:
    """Tracks captured pieces in the side lanes.

    L1-L8 (left, x=-0.5): white pieces (captured by player, promoted pawns)
    R1-R8 (right, x=7.5): black pieces (captured by robot)
    """

    def __init__(self, path=None):
        self.path = Path(path) if path else Path(__file__).parent / 'capture_ledger.json'
        self.entries = []
        self.load()

    def load(self):
        self.entries = []
        if self.path.exists():
            try:
                with open(self.path, 'r') as f:
                    data = json.load(f)
                self.entries = data.get('entries', [])
            except (json.JSONDecodeError, IOError):
                pass

    def save(self):
        try:
            with open(self.path, 'w') as f:
                json.dump({'entries': self.entries}, f, indent=2)
        except IOError as e:
            print(f"WARNING: Could not save ledger: {e}")

    def used_slots(self):
        return {e['slot'] for e in self.entries if e.get('slot')}

    def first_free(self, side):
        used = self.used_slots()
        for i in range(1, LANE_SLOTS + 1):
            slot = f"{side}{i}"
            if slot not in used:
                return slot
        return None

    def record(self, side, slot, piece, origin, move_no, note):
        entry = {
            'side': side,
            'slot': slot,
            'piece': piece,
            'origin': origin,
            'move_no': move_no,
            'note': note,
        }
        self.entries.append(entry)
        self.save()
        return entry

    def report(self):
        if not self.entries:
            return "No pieces captured."
        names = {'p': 'pawn', 'n': 'knight', 'b': 'bishop', 'r': 'rook', 'q': 'queen', 'k': 'king'}
        lines = []
        for e in self.entries:
            piece = names.get(e['piece'].lower(), e['piece'])
            color = 'white' if e['piece'].isupper() else 'black'
            slot = e['slot'] if e['slot'] else 'unlocated'
            note = e['note'].replace('-', ' ')
            lines.append(
                f"  {slot}: {color} {piece} (from {e['origin']}, move {e['move_no']}, {note})"
            )
        return "\n".join(lines)


class MoveExecutor:
    """Decomposes chess moves into firmware command sequences.

    Plans are lists of actions:
      ('cmd', '<firmware command>')
      ('ledger', {side, slot, piece, origin, move_no, note})
      ('prompt', '<text>')
      ('scanpos', x, y, '<expect M|E>')
    """

    def __init__(self, ledger):
        self.ledger = ledger

    def plan(self, board, move, move_no):
        if board.is_castling(move):
            return self._plan_castle(move)
        if move.promotion:
            return self._plan_promotion(board, move, move_no)
        if board.is_en_passant(move):
            return self._plan_en_passant(board, move, move_no)
        if board.is_capture(move):
            return self._plan_capture(board, move, move_no)
        return [('cmd', move.uci())]

    def _carry_to_lane(self, piece, from_sq, origin_sq, side, move_no, note):
        slot = self.ledger.first_free(side)
        if slot is None:
            raise LaneFullError(f"Lane {side} is full")
        fx, fy = sq_grid(from_sq)
        sy = slot_y(slot)
        edge_x = LANE_X[side]
        actions = [
            ('cmd', g_cmd(fx, fy)),
            ('cmd', 'o'),
            ('cmd', path_cmd(lane_route((fx, fy), edge_x, sy))),
            ('cmd', 'O'),
            ('ledger', dict(
                side=side, slot=slot, piece=piece.symbol(),
                origin=chess.square_name(origin_sq), move_no=move_no, note=note,
            )),
        ]
        if piece.color == chess.BLACK:
            # The sensor can see black pieces, so confirm it arrived in the lane.
            actions.append(('scanpos', edge_x, sy, 'M'))
        return actions

    def _victim_side(self, board, victim_sq):
        victim = board.piece_at(victim_sq)
        return 'R' if victim.color == chess.BLACK else 'L'

    def _plan_capture(self, board, move, move_no):
        victim_sq = move.to_square
        side = self._victim_side(board, victim_sq)
        piece = board.piece_at(victim_sq)
        carry = self._carry_to_lane(piece, victim_sq, victim_sq, side, move_no, 'captured')
        return carry + [
            ('cmd', move.uci()),
        ]

    def _plan_en_passant(self, board, move, move_no):
        victim_sq = ep_victim_square(move)
        side = self._victim_side(board, victim_sq)
        piece = board.piece_at(victim_sq)
        carry = self._carry_to_lane(piece, victim_sq, victim_sq, side, move_no, 'captured')
        return carry + [
            ('cmd', move.uci()),
        ]

    def _plan_castle(self, move):
        rook_from, rook_to = castle_rook_squares(move)
        fx, fy = sq_grid(rook_from)
        tx, ty = sq_grid(rook_to)
        route = border_route_same_row((fx, fy), (tx, ty))
        return [
            ('cmd', move.uci()),
            ('cmd', g_cmd(fx, fy)),
            ('cmd', 'o'),
            ('cmd', path_cmd(route)),
            ('cmd', 'O'),
        ]

    def _plan_promotion(self, board, move, move_no):
        actions = []
        if board.is_capture(move):
            victim_sq = move.to_square
            side = self._victim_side(board, victim_sq)
            piece = board.piece_at(victim_sq)
            actions += self._carry_to_lane(
                piece, victim_sq, victim_sq, side, move_no, 'captured')
        actions.append(('cmd', move.uci()))
        pawn = board.piece_at(move.from_square)
        actions += self._carry_to_lane(
            pawn, move.to_square, move.from_square, 'L', move_no, 'promoted-pawn')
        to_name = chess.square_name(move.to_square)
        promo_name = PIECE_NAMES.get(move.promotion, 'piece')
        # The new white piece is invisible to the sensor, so it can't be verified.
        actions.append(('prompt', f"Place the {promo_name} on {to_name} and press Enter: "))
        return actions


def black_squares(board):
    """Squares the hall sensor should read as 'M': it only senses black pieces."""
    return {sq for sq, p in board.piece_map().items() if p.color == chess.BLACK}


def expected_reading(black_set, square):
    return 'M' if square in black_set else 'E'


class ScanEngine:
    """Detects the player's (black) move via hall-sensor scanning.

    The sensor responds to black pieces only; white pieces read 'E'. Each
    legal move therefore predicts an exact set of black-occupied squares,
    including captures (the target square flips from E to M).

    Squares are read one at a time, always from the highest-ranked candidate
    that is still consistent with every reading so far, until every remaining
    candidate has had all of its changed squares observed. Each reading
    usually rules out many candidates at once (e.g. an occupied from-square
    refutes every move of that piece), and a move is only accepted when no
    other legal move fits the same readings.
    """

    def __init__(self, bridge):
        self.bridge = bridge

    def read(self, square):
        reading = self.bridge.scan_square(chess.square_name(square))
        if reading == 'X':
            reading = self.bridge.scan_square(chess.square_name(square))
        return reading

    def detect_move(self, board, ranked_ucis, on_ambiguous=None):
        candidates = []
        seen = set()
        for uci in ranked_ucis:
            try:
                move = chess.Move.from_uci(uci)
            except ValueError:
                continue
            if move in board.legal_moves and move not in seen:
                candidates.append(move)
                seen.add(move)
        candidates += [m for m in board.legal_moves if m not in seen]
        if not candidates:
            return None

        before = black_squares(board)
        predicted = {}
        changed = {}
        for move in candidates:
            after = board.copy(stack=False)
            after.push(move)
            predicted[move] = black_squares(after)
            diff = before ^ predicted[move]
            # Read vacated squares first: they refute the most candidates.
            changed[move] = sorted(diff, key=lambda sq: (sq not in before, sq))

        observed = {}

        def consistent(move):
            for square, reading in observed.items():
                if reading == 'X':
                    continue
                if reading != expected_reading(predicted[move], square):
                    return False
            return True

        while True:
            remaining = [m for m in candidates if consistent(m)]
            pending = None
            for move in remaining:
                unread = [sq for sq in changed[move] if sq not in observed]
                if unread:
                    pending = unread[0]
                    break
            if pending is None:
                break
            observed[pending] = self.read(pending)

        if len(remaining) == 1:
            return remaining[0]
        if len(remaining) > 1 and on_ambiguous:
            return on_ambiguous(remaining, board)
        return None


class ChessRobotBridge:
    """Main bridge class that coordinates chess game with hardware."""

    def __init__(self, serial_port=None, manual=False, verify=True, ledger_path=None):
        self.game = Game()
        self.board = self.game.board
        self.engine = Engine()
        self.ranker = MoveRanker()
        self.bridge = ArduinoBridge(port=serial_port)
        self.ledger = CaptureLedger(path=ledger_path)
        self.ledger.entries = []
        self.ledger.save()
        self.executor = MoveExecutor(self.ledger)
        self.scanner = ScanEngine(self.bridge)
        self.manual = manual
        self.verify = verify
        self.move_no = 0

    def start(self):
        print("=" * 50)
        print("Chess Robot Bridge")
        print("=" * 50)

        if self.engine.is_stockfish_available():
            print("Stockfish: Available")
        else:
            print("Stockfish: Not available (using fallback AI)")

        if self.manual:
            print("Mode: manual move entry (no scanning)")

        if not self.bridge.connect():
            return False

        print()
        print(self.game.get_board_display())
        print()

        return self.game_loop()

    def game_loop(self):
        while not self.game.is_game_over():
            print(f"--- Move {self.move_no + 1}: White (Arduino) ---")
            move = self.get_white_move()
            if move is None:
                print("ERROR: Could not get white move")
                break

            print(f"White plays: {self.board.san(move)}")
            self.move_no += 1

            try:
                plan = self.executor.plan(self.board, move, self.move_no)
                self.execute_plan(plan)
            except (CommandError, LaneFullError) as e:
                print(f"ERROR: Physical move failed: {e}")
                break

            board_before = self.board.copy(stack=False)
            self.game.apply_move(move.uci())

            if self.verify:
                self.verify_robot_move(board_before)

            print()
            print(self.game.get_board_display())
            print()

            if self.game.is_game_over():
                break

            print(f"--- Move {self.move_no}: Black (You) ---")

            if self.manual:
                move = self.get_typed_move()
            else:
                print("Scanning for your move...")
                move = self.scanner.detect_move(
                    self.board,
                    self.ranker.rank_moves(self.board),
                    on_ambiguous=self.prompt_ambiguous,
                )
                if move is None:
                    print("Could not identify your move by scanning.")
                    move = self.get_typed_move()

            if move is None:
                print("Game aborted by user")
                break

            captured = None
            capture_origin = None
            if self.board.is_capture(move):
                victim_sq = ep_victim_square(move) if self.board.is_en_passant(move) else move.to_square
                captured = self.board.piece_at(victim_sq)
                capture_origin = chess.square_name(victim_sq)

            print(f"Black plays: {self.board.san(move)}")
            self.game.apply_move(move.uci())

            if captured is not None:
                self.reconcile_player_capture(captured, capture_origin)

        print()
        print("=" * 50)
        print("Game Over!")
        print(self.game.get_game_result())
        print("=" * 50)
        print()
        print("Move history:")
        print(self.game.get_move_history_display())
        print()
        print("Captured pieces:")
        print(self.ledger.report())

        self.bridge.disconnect()
        return True

    def execute_plan(self, plan):
        for action in plan:
            kind = action[0]
            if kind == 'cmd':
                print(f"  -> {action[1]}")
                self.bridge.execute(action[1])
            elif kind == 'ledger':
                self.ledger.record(**action[1])
                print(f"  Ledger: {action[1]['slot']} = {action[1]['piece']} ({action[1]['note']})")
            elif kind == 'prompt':
                input(action[1])
            elif kind == 'scanpos':
                x, y, expect = action[1], action[2], action[3]
                reading = self.bridge.scan_grid(x, y)
                if reading != expect:
                    print(f"WARNING: scan at {fmt(x)},{fmt(y)} read {reading}, expected {expect}")

    def verify_robot_move(self, board_before):
        """Scan every square the move changed; only black pieces read 'M'."""
        black_after = black_squares(self.board)
        changed = [sq for sq in chess.SQUARES
                   if board_before.piece_at(sq) != self.board.piece_at(sq)]
        for sq in changed:
            expect = expected_reading(black_after, sq)
            reading = self.bridge.scan_square(chess.square_name(sq))
            if reading != expect:
                print(f"WARNING: verify scan {chess.square_name(sq)} read {reading}, expected {expect}")

    def reconcile_player_capture(self, piece, origin):
        # White pieces are invisible to the sensor, so the player is assumed
        # to have put the captured piece on the first free left-lane slot.
        slot = self.ledger.first_free('L')
        name = PIECE_NAMES.get(piece.piece_type, chess.piece_name(piece.piece_type))
        if slot:
            print(f"Captured white {name} recorded on lane slot {slot}")
        else:
            print("WARNING: left lane is full; keep the captured piece off the board.")
        self.ledger.record(
            side='L', slot=slot, piece=piece.symbol(),
            origin=origin,
            move_no=self.move_no, note='captured',
        )

    def get_white_move(self):
        result = self.engine.get_black_move(self.board)
        if result and result[0]:
            try:
                move = chess.Move.from_uci(result[0])
            except ValueError:
                return None
            if move in self.board.legal_moves:
                return move
        return None

    def get_typed_move(self):
        while True:
            try:
                user_input = input("Enter your move (e.g., e7e5) or 'q' to quit: ").strip().lower()
                if user_input == 'q':
                    return None
                if self.game.is_legal_move(user_input):
                    return chess.Move.from_uci(user_input)
                print("Illegal move. Try again.")
            except (EOFError, KeyboardInterrupt):
                print()
                return None

    def prompt_ambiguous(self, moves, board):
        print("Scan result is ambiguous between these moves:")
        for i, move in enumerate(moves, 1):
            print(f"  {i}. {board.san(move)} ({move.uci()})")
        try:
            choice = input("Pick one (1-{}) or 't' to type it: ".format(len(moves))).strip()
        except (EOFError, KeyboardInterrupt):
            return None
        if choice == 't':
            return None
        try:
            idx = int(choice) - 1
            if 0 <= idx < len(moves):
                return moves[idx]
        except ValueError:
            pass
        return None


def main():
    parser = argparse.ArgumentParser(description='Chess robot bridge')
    parser.add_argument('port', nargs='?', default=None, help='serial port')
    parser.add_argument('--manual', action='store_true',
                        help='type black moves instead of scanning')
    parser.add_argument('--no-verify', action='store_true',
                        help='skip post-move verification scans')
    parser.add_argument('--ledger', default=None, help='capture ledger file path')
    args = parser.parse_args()

    bridge = ChessRobotBridge(
        serial_port=args.port,
        manual=args.manual,
        verify=not args.no_verify,
        ledger_path=args.ledger,
    )

    try:
        bridge.start()
    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
        bridge.bridge.disconnect()
        sys.exit(0)


if __name__ == "__main__":
    main()
