import chess


class Game:
    def __init__(self):
        self.board = chess.Board()
        self.move_history = []

    def get_legal_moves(self):
        return list(self.board.legal_moves)

    def is_legal_move(self, move_uci):
        try:
            move = chess.Move.from_uci(move_uci)
            return move in self.board.legal_moves
        except ValueError:
            return False

    def apply_move(self, move_uci):
        move = chess.Move.from_uci(move_uci)
        if move not in self.board.legal_moves:
            return False, "Illegal move"

        san = self.board.san(move)
        self.board.push(move)
        self.move_history.append({
            'uci': move_uci,
            'san': san,
            'turn': 'White' if not self.board.turn else 'Black'
        })
        return True, san

    def get_fen(self):
        return self.board.fen()

    def get_board_display(self):
        lines = []
        board_str = str(self.board)
        rows = board_str.split('\n')
        
        for i, row in enumerate(rows):
            rank = 8 - i
            lines.append(f"  {rank}  {row}")
        
        lines.append("     a b c d e f g h")
        return '\n'.join(lines)

    def is_game_over(self):
        return self.board.is_game_over()

    def get_game_result(self):
        if not self.is_game_over():
            return None

        if self.board.is_checkmate():
            winner = "Black" if self.board.turn else "White"
            return f"Checkmate! {winner} wins."
        elif self.board.is_stalemate():
            return "Stalemate! Draw."
        elif self.board.is_insufficient_material():
            return "Insufficient material! Draw."
        elif self.board.is_seventyfive_moves():
            return "75-move rule! Draw."
        elif self.board.is_fivefold_repetition():
            return "Fivefold repetition! Draw."
        else:
            return "Game over."

    def get_move_history_display(self):
        if not self.move_history:
            return "No moves yet."

        lines = []
        for i, move in enumerate(self.move_history, 1):
            lines.append(f"{i}. {move['turn']}: {move['uci']} ({move['san']})")
        return "\n".join(lines)
