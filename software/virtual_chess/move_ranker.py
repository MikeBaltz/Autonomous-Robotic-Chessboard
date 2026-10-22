import chess
import json
import os
from pathlib import Path


class MoveRanker:
    def __init__(self, preferences_file=None):
        self.preferences_file = Path(preferences_file) if preferences_file else self._default_preferences_path()
        self.preferences = self._load_preferences()

    def _default_preferences_path(self):
        return Path(__file__).parent / "user_move_preferences.json"

    def _load_preferences(self):
        try:
            if self.preferences_file.exists():
                with open(self.preferences_file, 'r') as f:
                    return json.load(f)
        except (json.JSONDecodeError, IOError):
            pass
        return {}

    def _save_preferences(self):
        try:
            with open(self.preferences_file, 'w') as f:
                json.dump(self.preferences, f, indent=2)
        except IOError:
            pass

    def record_user_choice(self, fen, move_uci):
        key = fen.split()[0]
        if key not in self.preferences:
            self.preferences[key] = {}
        if move_uci not in self.preferences[key]:
            self.preferences[key][move_uci] = 0
        self.preferences[key][move_uci] += 1
        self._save_preferences()

    def rank_moves(self, board):
        legal_moves = list(board.legal_moves)
        if not legal_moves:
            return []

        scored_moves = []
        for move in legal_moves:
            score = self._score_move(board, move)
            scored_moves.append((score, move.uci(), move))

        scored_moves.sort(key=lambda x: (-x[0], x[1]))
        return [item[1] for item in scored_moves]

    def _score_move(self, board, move):
        score = 0.0

        if board.is_capture(move):
            score += 100

        board_copy = board.copy()
        board_copy.push(move)
        if board_copy.is_check():
            score += 50

        if move.promotion:
            score += 200

        if board.is_castling(move):
            score += 80

        piece = board.piece_at(move.from_square)
        if piece and piece.piece_type == chess.PAWN:
            from_file = chess.square_file(move.from_square)
            to_file = chess.square_file(move.to_square)
            if from_file in [3, 4]:
                score += 30
            if to_file in [3, 4]:
                score += 20

            from_rank = chess.square_rank(move.from_square)
            to_rank = chess.square_rank(move.to_square)
            if board.turn == chess.WHITE and to_rank > from_rank:
                score += 10

        if piece and piece.piece_type in [chess.KNIGHT, chess.BISHOP]:
            from_rank = chess.square_rank(move.from_square)
            if board.turn == chess.WHITE and from_rank == 0:
                score += 25

        fen = board.fen().split()[0]
        if fen in self.preferences:
            move_uci = move.uci()
            if move_uci in self.preferences[fen]:
                count = self.preferences[fen][move_uci]
                score += min(count * 5, 50)

        opening_moves = {
            'e2e4': 40, 'd2d4': 38, 'c2c4': 35, 'g1f3': 33,
            'e2e3': 30, 'c2c3': 28, 'd2d3': 25, 'b1c3': 23,
            'f2f4': 20, 'g2g3': 18, 'b2b3': 15, 'f1c4': 13
        }
        move_uci = move.uci()
        if move_uci in opening_moves:
            fullmove = board.fullmove_number
            if fullmove <= 10:
                score += opening_moves[move_uci] * (1.0 / fullmove)

        score += hash(move.uci()) % 10 * 0.01

        return score
