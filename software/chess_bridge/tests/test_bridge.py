"""Bridge tests against a mocked Arduino.

The MockBridge simulates the firmware: position tracking, magnet pick/drop
with occupancy, and hall-scan responses derived from the simulated board.
No hardware or serial connection is needed.
"""

import os
import sys
import json
import tempfile
import unittest

import chess

BRIDGE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
sys.path.insert(0, BRIDGE_DIR)

import main as bm


class MockBridge:
    def __init__(self, board=None):
        self.pos = (3.5, 3.5)
        self.magnet = False
        self.holding = None
        self.commands = []
        self.scans = []
        self.occupancy = {}
        if board is not None:
            for sq, piece in board.piece_map().items():
                x, y = bm.sq_grid(sq)
                self.occupancy[(x, y)] = piece.symbol()

    def execute(self, cmd):
        self.commands.append(cmd)
        parts = cmd.split()
        if not parts:
            raise bm.CommandError('empty command')
        head = parts[0]

        if head == 'g':
            self.pos = (float(parts[1]), float(parts[2]))
        elif head == 'path':
            vals = [float(v) for v in parts[1:]]
            for i in range(0, len(vals), 2):
                self.pos = (vals[i], vals[i + 1])
        elif head == 'lane':
            side, n = parts[1][0], int(parts[1][1])
            self.pos = (bm.LANE_X[side], float(n - 1))
        elif head == 'o':
            if (float(self.pos[0]), float(self.pos[1])) not in self.occupancy:
                raise bm.CommandError(f'no piece at {self.pos} to pick up')
            self.magnet = True
            self.holding = self.occupancy.pop((float(self.pos[0]), float(self.pos[1])))
        elif head == 'O':
            if not self.magnet or self.holding is None:
                raise bm.CommandError('nothing held')
            self.occupancy[(float(self.pos[0]), float(self.pos[1]))] = self.holding
            self.holding = None
            self.magnet = False
        elif head[0] in 'abcdefgh' and len(head) in (4, 5) and \
                head[1] in '12345678' and head[2] in 'abcdefgh' and head[3] in '12345678':
            fx = float(ord(head[0]) - ord('a'))
            fy = float(int(head[1]) - 1)
            tx = float(ord(head[2]) - ord('a'))
            ty = float(int(head[3]) - 1)
            self.pos = (fx, fy)
            self.magnet = True
            self.holding = self.occupancy.pop((fx, fy), None)
            self.pos = (tx, ty)
            if self.holding is not None:
                self.occupancy[(tx, ty)] = self.holding
                self.holding = None
            self.magnet = False
        else:
            raise bm.CommandError(f'unhandled command: {cmd}')
        return ['DONE']

    def _sense(self, key):
        # The real sensor only reacts to black pieces (lowercase symbols).
        piece = self.occupancy.get(key)
        return 'M' if piece is not None and piece.islower() else 'E'

    def scan_square(self, name):
        self.scans.append(name)
        sq = chess.parse_square(name)
        return self._sense(bm.sq_grid(sq))

    def scan_grid(self, x, y):
        return self._sense((float(x), float(y)))


def make_ledger(tmpdir):
    path = os.path.join(tmpdir, 'ledger.json')
    return bm.CaptureLedger(path=path)


def run_plan(mock, ledger, plan, on_prompt=None):
    for action in plan:
        kind = action[0]
        if kind == 'cmd':
            mock.execute(action[1])
        elif kind == 'ledger':
            ledger.record(**action[1])
        elif kind == 'prompt':
            if on_prompt:
                on_prompt()
        elif kind == 'scanpos':
            reading = mock.scan_grid(action[1], action[2])
            assert reading == action[3], f"scanpos {action[1]},{action[2]}: {reading} != {action[3]}"


class TestFmt(unittest.TestCase):
    def test_fmt(self):
        self.assertEqual(bm.fmt(4.0), '4')
        self.assertEqual(bm.fmt(4.5), '4.5')
        self.assertEqual(bm.fmt(-0.5), '-0.5')
        self.assertEqual(bm.fmt(0.0), '0')
        self.assertEqual(bm.fmt(7.5), '7.5')


class TestLedger(unittest.TestCase):
    def test_first_free_and_record(self):
        with tempfile.TemporaryDirectory() as tmp:
            ledger = make_ledger(tmp)
            self.assertEqual(ledger.first_free('R'), 'R1')
            ledger.record(side='R', slot='R1', piece='p', origin='e5', move_no=3, note='captured')
            self.assertEqual(ledger.first_free('R'), 'R2')
            self.assertEqual(ledger.first_free('L'), 'L1')

    def test_persistence_roundtrip(self):
        with tempfile.TemporaryDirectory() as tmp:
            ledger = make_ledger(tmp)
            ledger.record(side='R', slot='R1', piece='n', origin='g8', move_no=5, note='captured')
            loaded = bm.CaptureLedger(path=ledger.path)
            self.assertEqual(len(loaded.entries), 1)
            self.assertEqual(loaded.entries[0]['piece'], 'n')
            self.assertEqual(loaded.entries[0]['slot'], 'R1')


class TestMovePlans(unittest.TestCase):
    def plan_and_run(self, fen, uci):
        board = chess.Board(fen)
        move = chess.Move.from_uci(uci)
        self.assertIn(move, board.legal_moves)
        with tempfile.TemporaryDirectory() as tmp:
            ledger = make_ledger(tmp)
            executor = bm.MoveExecutor(ledger)
            plan = executor.plan(board, move, 1)
            mock = MockBridge(board)
            run_plan(mock, ledger, plan)
            return board, move, plan, mock, ledger

    def test_normal_move(self):
        board, move, plan, mock, ledger = self.plan_and_run(
            chess.STARTING_FEN, 'e2e4')
        self.assertEqual(plan, [('cmd', 'e2e4')])
        self.assertIn((4.0, 3.0), mock.occupancy)
        self.assertNotIn((4.0, 1.0), mock.occupancy)

    def test_capture(self):
        fen = 'rnbqkbnr/pppp1ppp/8/4p2Q/8/8/PPPPPPPP/RNBQKBNR w KQkq - 2 3'
        board, move, plan, mock, ledger = self.plan_and_run(fen, 'h5e5')
        self.assertEqual(plan, [
            ('cmd', 'g 4 4'),
            ('cmd', 'o'),
            ('cmd', 'path 4 4.5 7.5 4.5 7.5 0'),
            ('cmd', 'O'),
            ('ledger', dict(side='R', slot='R1', piece='p', origin='e5', move_no=1, note='captured')),
            ('scanpos', 7.5, 0.0, 'M'),
            ('cmd', 'h5e5'),
        ])
        self.assertEqual(mock.occupancy[(7.5, 0.0)], 'p')
        self.assertEqual(mock.occupancy[(4.0, 4.0)], 'Q')
        self.assertNotIn((4.0, 4.0) and None, mock.occupancy)
        self.assertEqual(ledger.entries[0]['slot'], 'R1')

    def test_en_passant(self):
        fen = 'rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3'
        board, move, plan, mock, ledger = self.plan_and_run(fen, 'e5d6')
        self.assertEqual(plan, [
            ('cmd', 'g 3 4'),
            ('cmd', 'o'),
            ('cmd', 'path 3 4.5 7.5 4.5 7.5 0'),
            ('cmd', 'O'),
            ('ledger', dict(side='R', slot='R1', piece='p', origin='d5', move_no=1, note='captured')),
            ('scanpos', 7.5, 0.0, 'M'),
            ('cmd', 'e5d6'),
        ])
        self.assertEqual(mock.occupancy[(7.5, 0.0)], 'p')
        self.assertEqual(mock.occupancy[(3.0, 5.0)], 'P')
        self.assertNotIn((3.0, 4.0), mock.occupancy)

    def test_castle_kingside(self):
        fen = 'r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1'
        board, move, plan, mock, ledger = self.plan_and_run(fen, 'e1g1')
        self.assertEqual(plan, [
            ('cmd', 'e1g1'),
            ('cmd', 'g 7 0'),
            ('cmd', 'o'),
            ('cmd', 'path 7 0.5 5 0.5 5 0'),
            ('cmd', 'O'),
        ])
        self.assertEqual(mock.occupancy[(6.0, 0.0)], 'K')
        self.assertEqual(mock.occupancy[(5.0, 0.0)], 'R')
        self.assertNotIn((4.0, 0.0), mock.occupancy)
        self.assertNotIn((7.0, 0.0), mock.occupancy)

    def test_castle_queenside(self):
        fen = 'r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1'
        board, move, plan, mock, ledger = self.plan_and_run(fen, 'e1c1')
        self.assertEqual(plan, [
            ('cmd', 'e1c1'),
            ('cmd', 'g 0 0'),
            ('cmd', 'o'),
            ('cmd', 'path 0 0.5 3 0.5 3 0'),
            ('cmd', 'O'),
        ])
        self.assertEqual(mock.occupancy[(2.0, 0.0)], 'K')
        self.assertEqual(mock.occupancy[(3.0, 0.0)], 'R')

    def test_promotion(self):
        fen = '3k4/4P3/8/8/8/8/8/4K3 w - - 0 1'

        def place_queen(mock):
            mock.occupancy[(4.0, 7.0)] = 'Q'

        board = chess.Board(fen)
        move = chess.Move.from_uci('e7e8q')
        self.assertIn(move, board.legal_moves)
        with tempfile.TemporaryDirectory() as tmp:
            ledger = make_ledger(tmp)
            executor = bm.MoveExecutor(ledger)
            plan = executor.plan(board, move, 1)
            mock = MockBridge(board)
            run_plan(mock, ledger, plan,
                     on_prompt=lambda: place_queen(mock))
        self.assertEqual(plan, [
            ('cmd', 'e7e8q'),
            ('cmd', 'g 4 7'),
            ('cmd', 'o'),
            ('cmd', 'path 4 7.5 -0.5 7.5 -0.5 0'),
            ('cmd', 'O'),
            ('ledger', dict(side='L', slot='L1', piece='P', origin='e7', move_no=1, note='promoted-pawn')),
            ('prompt', 'Place the queen on e8 and press Enter: '),
        ])
        self.assertEqual(mock.occupancy[(-0.5, 0.0)], 'P')
        self.assertEqual(mock.occupancy[(4.0, 7.0)], 'Q')
        self.assertEqual(ledger.entries[0]['note'], 'promoted-pawn')


class TestScanEngine(unittest.TestCase):
    def detect(self, fen_before, mirror_moves, ranked):
        board = chess.Board(fen_before)
        for m in mirror_moves:
            board.push_uci(m)
        mirror = MockBridge(board)
        scanner = bm.ScanEngine(mirror)

        detect_board = chess.Board(fen_before)
        for m in mirror_moves[:-1]:
            detect_board.push_uci(m)
        return scanner.detect_move(detect_board, ranked), mirror

    def test_probe_quiet_move(self):
        move, _ = self.detect(
            chess.STARTING_FEN,
            ['e2e4', 'e7e5'],
            ['a7a6', 'a7a5', 'e7e5', 'g8f6'],
        )
        self.assertEqual(move.uci(), 'e7e5')

    def test_diff_fallback(self):
        ranked = ['a7a6', 'a7a5', 'b7b6', 'b7b5', 'c7c6', 'c7c5', 'd7d6', 'g8f6', 'e7e5']
        move, _ = self.detect(
            chess.STARTING_FEN,
            ['e2e4', 'e7e5'],
            ranked,
        )
        self.assertEqual(move.uci(), 'e7e5')

    def test_probe_capture(self):
        fen = 'rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2'
        move, mirror = self.detect(
            fen,
            ['d5e4'],
            ['d5e4', 'd5d4', 'a7a6'],
        )
        self.assertEqual(move.uci(), 'd5e4')
        self.assertEqual(mirror.occupancy[(4.0, 3.0)], 'p')
        self.assertNotIn((3.0, 4.0), mirror.occupancy)

    def test_probe_black_castling(self):
        fen = 'r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1'
        move, _ = self.detect(
            fen,
            ['e8g8'],
            ['e8g8', 'e8e7', 'h8h7'],
        )
        self.assertEqual(move.uci(), 'e8g8')

    def test_probe_en_passant(self):
        fen = 'rnbqkbnr/p1pppppp/8/8/Pp6/8/1PPPPPPP/RNBQKBNR b KQkq a3 0 3'
        move, mirror = self.detect(
            fen,
            ['b4a3'],
            ['b4a3', 'b4b3', 'a7a6'],
        )
        self.assertEqual(move.uci(), 'b4a3')
        self.assertNotIn((0.0, 3.0), mirror.occupancy)
        self.assertIn((0.0, 2.0), mirror.occupancy)

    def test_diff_castling(self):
        fen = 'r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1'
        ranked = [
            'e8e7', 'e8d7', 'e8f7', 'e8f8', 'e8d8',
            'h8h7', 'h8g8', 'a8a7', 'a8b8', 'e8g8',
        ]
        move, mirror = self.detect(fen, ['e8g8'], ranked)
        self.assertEqual(move.uci(), 'e8g8')
        self.assertEqual(mirror.occupancy[(6.0, 7.0)], 'k')
        self.assertEqual(mirror.occupancy[(5.0, 7.0)], 'r')

    def test_probe_castling_refutes_quiet_candidates(self):
        fen = 'r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1'
        ranked = ['e8f8', 'h8g8', 'e8g8', 'e8e7', 'a8a7']
        move, mirror = self.detect(fen, ['e8g8'], ranked)
        self.assertEqual(move.uci(), 'e8g8')

    def test_unmoved_board_returns_none(self):
        move, _ = self.detect(
            chess.STARTING_FEN,
            ['e2e4'],
            ['a7a6', 'a7a5', 'g8f6'],
        )
        self.assertIsNone(move)

    def test_capture_ranked_first_but_quiet_move_played(self):
        # 1.e4 Nf6 2.d3: black could play Nxe4 but plays Nd5 instead.
        fen = 'rnbqkb1r/pppppppp/5n2/8/4P3/3P4/PPP2PPP/RNBQKBNR b KQkq - 0 2'
        move, _ = self.detect(fen, ['f6d5'], ['f6e4', 'f6g4', 'f6d5'])
        self.assertEqual(move.uci(), 'f6d5')

    def test_capture_detected(self):
        fen = 'rnbqkb1r/pppppppp/5n2/8/4P3/3P4/PPP2PPP/RNBQKBNR b KQkq - 0 2'
        move, mirror = self.detect(fen, ['f6e4'], ['f6d5', 'f6e4'])
        self.assertEqual(move.uci(), 'f6e4')

    def test_castling_ranked_below_king_step(self):
        fen = 'r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1'
        move, _ = self.detect(fen, ['e8g8'], ['e8f8', 'e8f7'])
        self.assertEqual(move.uci(), 'e8g8')

    def test_promotion_asks_which_piece(self):
        fen = '4k3/8/8/8/8/8/p7/4K3 b - - 0 1'
        seen = []

        def on_ambiguous(moves, board):
            seen.extend(m.uci() for m in moves)
            return chess.Move.from_uci('a2a1n')

        board = chess.Board(fen)
        board.push_uci('a2a1q')
        scanner = bm.ScanEngine(MockBridge(board))
        move = scanner.detect_move(chess.Board(fen), [], on_ambiguous=on_ambiguous)
        self.assertEqual(sorted(seen), ['a2a1b', 'a2a1n', 'a2a1q', 'a2a1r'])
        self.assertEqual(move.uci(), 'a2a1n')

    def test_few_scans_for_ranked_move(self):
        move, mirror = self.detect(
            chess.STARTING_FEN,
            ['e2e4', 'e7e5'],
            ['e7e5', 'd7d5', 'g8f6'],
        )
        self.assertEqual(move.uci(), 'e7e5')
        self.assertLessEqual(len(mirror.scans), 25)


class TestRobotBridgeSmoke(unittest.TestCase):
    def test_execute_and_verify_white_move(self):
        mock = MockBridge(chess.Board())
        with tempfile.TemporaryDirectory() as tmp:
            robot = bm.ChessRobotBridge.__new__(bm.ChessRobotBridge)
            robot.game = bm.Game()
            robot.board = robot.game.board
            robot.ledger = make_ledger(tmp)
            robot.executor = bm.MoveExecutor(robot.ledger)
            robot.scanner = bm.ScanEngine(mock)
            robot.verify = True
            robot.move_no = 0
            robot.bridge = mock

            move = chess.Move.from_uci('e2e4')
            plan = robot.executor.plan(robot.board, move, 1)
            robot.execute_plan(plan)
            before = robot.board.copy(stack=False)
            robot.game.apply_move('e2e4')
            robot.verify_robot_move(before)

            self.assertIn((4.0, 3.0), mock.occupancy)
            self.assertNotIn((4.0, 1.0), mock.occupancy)

            mock.execute('e7e5')
            detected = robot.scanner.detect_move(
                robot.board,
                ['a7a6', 'e7e5', 'g8f6', 'd7d5'],
            )
            self.assertEqual(detected.uci(), 'e7e5')


if __name__ == '__main__':
    unittest.main()
