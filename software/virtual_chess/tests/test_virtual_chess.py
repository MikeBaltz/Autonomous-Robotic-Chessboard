import sys
import os
import json
import tempfile

import pytest
import chess

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from game import Game
from move_ranker import MoveRanker
from engine import Engine


class TestGame:
    def test_initial_position_has_20_legal_moves(self):
        game = Game()
        moves = game.get_legal_moves()
        assert len(moves) == 20

    def test_e2e4_is_legal_initially(self):
        game = Game()
        assert game.is_legal_move('e2e4')

    def test_illegal_move_does_not_change_board(self):
        game = Game()
        fen_before = game.get_fen()
        result = game.is_legal_move('e2e5')
        assert result is False
        assert game.get_fen() == fen_before

    def test_apply_legal_move_updates_fen(self):
        game = Game()
        fen_before = game.get_fen()
        success, san = game.apply_move('e2e4')
        assert success is True
        assert san == 'e4'
        assert game.get_fen() != fen_before

    def test_apply_illegal_move_returns_false(self):
        game = Game()
        fen_before = game.get_fen()
        success, msg = game.apply_move('e2e5')
        assert success is False
        assert game.get_fen() == fen_before

    def test_move_history_records_moves(self):
        game = Game()
        game.apply_move('e2e4')
        history = game.get_move_history_display()
        assert 'e2e4' in history
        assert 'e4' in history

    def test_game_over_checkmate(self):
        game = Game()
        game.board = chess.Board('rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3')
        assert game.is_game_over()
        result = game.get_game_result()
        assert 'Checkmate' in result

    def test_game_over_stalemate(self):
        game = Game()
        game.board = chess.Board('k7/2Q5/1K6/8/8/8/8/8 b - - 0 1')
        assert game.is_game_over()
        result = game.get_game_result()
        assert 'Stalemate' in result

    def test_game_not_over_initially(self):
        game = Game()
        assert not game.is_game_over()
        assert game.get_game_result() is None


class TestMoveRanker:
    def test_ranking_is_deterministic(self):
        game = Game()
        ranker = MoveRanker(preferences_file='/dev/null')
        ranked1 = ranker.rank_moves(game.board)
        ranked2 = ranker.rank_moves(game.board)
        assert ranked1 == ranked2

    def test_only_legal_moves_in_ranked_candidates(self):
        game = Game()
        ranker = MoveRanker(preferences_file='/dev/null')
        ranked = ranker.rank_moves(game.board)
        legal_ucis = [m.uci() for m in game.get_legal_moves()]
        for move_uci in ranked:
            assert move_uci in legal_ucis

    def test_ranked_count_matches_legal_count(self):
        game = Game()
        ranker = MoveRanker(preferences_file='/dev/null')
        ranked = ranker.rank_moves(game.board)
        assert len(ranked) == len(game.get_legal_moves())

    def test_e2e4_ranks_high_initially(self):
        game = Game()
        ranker = MoveRanker(preferences_file='/dev/null')
        ranked = ranker.rank_moves(game.board)
        top_5 = ranked[:5]
        assert 'e2e4' in top_5

    def test_empty_position_returns_empty(self):
        ranker = MoveRanker(preferences_file='/dev/null')
        board = chess.Board()
        board.clear()
        ranked = ranker.rank_moves(board)
        assert ranked == []

    def test_rejecting_candidates_does_not_mutate_board(self):
        game = Game()
        fen_before = game.get_fen()
        ranker = MoveRanker(preferences_file='/dev/null')
        ranker.rank_moves(game.board)
        assert game.get_fen() == fen_before

    def test_user_preferences_persist(self):
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            prefs_path = f.name

        try:
            ranker = MoveRanker(preferences_file=prefs_path)
            fen = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -'
            ranker.record_user_choice(fen, 'e2e4')

            ranker2 = MoveRanker(preferences_file=prefs_path)
            key = fen.split()[0]
            assert key in ranker2.preferences
            assert 'e2e4' in ranker2.preferences[key]
            assert ranker2.preferences[key]['e2e4'] == 1
        finally:
            os.unlink(prefs_path)

    def test_invalid_json_preferences_handled(self):
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            f.write('{ invalid json }}}')
            prefs_path = f.name

        try:
            ranker = MoveRanker(preferences_file=prefs_path)
            assert ranker.preferences == {}
        finally:
            os.unlink(prefs_path)


class TestEngine:
    def test_fallback_returns_legal_move(self):
        game = Game()
        engine = Engine(stockfish_path='/nonexistent/path')
        result = engine.get_black_move(game.board)
        assert result is not None
        move_uci, score = result
        assert move_uci is not None
        legal_ucis = [m.uci() for m in game.get_legal_moves()]
        assert move_uci in legal_ucis

    def test_fallback_after_white_move(self):
        game = Game()
        game.apply_move('e2e4')
        engine = Engine(stockfish_path='/nonexistent/path')
        result = engine.get_black_move(game.board)
        assert result is not None
        move_uci, score = result
        assert move_uci is not None
        legal_ucis = [m.uci() for m in game.board.legal_moves]
        assert move_uci in legal_ucis

    def test_stockfish_not_available_with_bad_path(self):
        engine = Engine(stockfish_path='/nonexistent/stockfish')
        assert not engine.is_stockfish_available()

    def test_fallback_no_legal_moves(self):
        engine = Engine(stockfish_path='/nonexistent/path')
        board = chess.Board()
        board.clear()
        result = engine.get_black_move(board)
        move_uci, score = result
        assert move_uci is None
