import chess
import os
import subprocess


class Engine:
    def __init__(self, stockfish_path=None):
        self.stockfish_path = stockfish_path or os.environ.get('STOCKFISH_PATH')
        self.stockfish_available = self._check_stockfish()

    def _check_stockfish(self):
        if not self.stockfish_path:
            return False
        try:
            result = subprocess.run(
                [self.stockfish_path],
                input='quit\n',
                capture_output=True,
                text=True,
                timeout=2
            )
            return result.returncode == 0
        except (FileNotFoundError, subprocess.TimeoutExpired, OSError):
            return False

    def get_black_move(self, board):
        if self.stockfish_available:
            result = self._get_stockfish_move(board)
            if result and result[0]:
                return result

        return self._get_fallback_move(board)

    def _get_stockfish_move(self, board):
        try:
            proc = subprocess.Popen(
                [self.stockfish_path],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )

            commands = [
                'uci',
                'isready',
                f'position fen {board.fen()}',
                'go movetime 1000',
            ]

            proc.stdin.write('\n'.join(commands) + '\n')
            proc.stdin.flush()

            best_move = None
            score = None

            while True:
                line = proc.stdout.readline()
                if not line:
                    break

                if 'score' in line and 'info' in line and 'depth' in line:
                    if 'cp' in line:
                        parts = line.split()
                        try:
                            cp_idx = parts.index('cp')
                            score = int(parts[cp_idx + 1])
                        except (ValueError, IndexError):
                            pass
                    elif 'mate' in line:
                        parts = line.split()
                        try:
                            mate_idx = parts.index('mate')
                            mate_moves = int(parts[mate_idx + 1])
                            score = f"mate {mate_moves}"
                        except (ValueError, IndexError):
                            pass

                if line.startswith('bestmove'):
                    parts = line.split()
                    if len(parts) >= 2:
                        move_uci = parts[1]
                        try:
                            move = chess.Move.from_uci(move_uci)
                            if move in board.legal_moves:
                                best_move = move_uci
                        except ValueError:
                            pass
                    break

            proc.stdin.write('quit\n')
            proc.stdin.flush()
            proc.wait(timeout=2)

            if best_move:
                return best_move, score

        except (subprocess.TimeoutExpired, OSError, Exception):
            try:
                proc.kill()
            except Exception:
                pass

        return None

    def _get_fallback_move(self, board):
        legal_moves = list(board.legal_moves)
        if not legal_moves:
            return None, None

        captures = [m for m in legal_moves if board.is_capture(m)]
        if captures:
            return captures[0].uci(), None

        checks = []
        for move in legal_moves:
            board_copy = board.copy()
            board_copy.push(move)
            if board_copy.is_check():
                checks.append(move)
        if checks:
            return checks[0].uci(), None

        center_squares = [chess.D4, chess.D5, chess.E4, chess.E5]
        for move in legal_moves:
            if move.to_square in center_squares:
                return move.uci(), None

        return legal_moves[0].uci(), None

    def is_stockfish_available(self):
        return self.stockfish_available
