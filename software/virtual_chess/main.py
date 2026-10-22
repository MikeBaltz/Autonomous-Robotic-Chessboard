#!/usr/bin/env python3

import sys

from game import Game
from move_ranker import MoveRanker
from engine import Engine

YES_RESPONSES = {'y', 'yes'}
NO_RESPONSES = {'n', 'no'}
COMMANDS = {'board', 'fen', 'history', 'restart', 'quit', 'manual'}


def print_header():
    print("=" * 50)
    print("  Autonomous Chessboard — Virtual Chess Prototype")
    print("=" * 50)
    print()
    print("  Human plays White. Computer plays Black.")
    print()


def print_instructions():
    print("Instructions:")
    print("  Think of a legal White move. The program will guess")
    print("  ranked candidates one at a time. Answer y/n.")
    print()
    print("Commands during questioning:")
    print("  board    — show current board")
    print("  fen      — show current FEN")
    print("  history  — show move history")
    print("  restart  — restart guessing for this turn")
    print("  manual   — enter a UCI move directly (e.g. e2e4)")
    print("  quit     — exit the program")
    print()


def print_board(game):
    print()
    print(game.get_board_display())
    print()


def prompt_white_move(game, ranker):
    fen = game.get_fen()
    ranked_moves = ranker.rank_moves(game.board)

    if not ranked_moves:
        print("No legal moves available.")
        return False

    print(f"\n{len(ranked_moves)} candidate moves ranked.")
    print("Answer y/n for each guess.\n")

    for i, move_uci in enumerate(ranked_moves, 1):
        while True:
            response = input(f"  [{i}/{len(ranked_moves)}] Did you play {move_uci}? ").strip().lower()

            if response in YES_RESPONSES:
                if game.is_legal_move(move_uci):
                    success, san = game.apply_move(move_uci)
                    if success:
                        ranker.record_user_choice(fen, move_uci)
                        print(f"\nWhite played: {move_uci} ({san})")
                        return True
                    else:
                        print(f"Error applying move: {san}")
                        return False
                else:
                    print("That move is no longer legal. Trying next candidate.")
                    break

            elif response in NO_RESPONSES:
                break

            elif response in COMMANDS:
                handle_command(response, game)
                continue

            else:
                print("  Please enter y, n, or a command (board/fen/history/restart/manual/quit).")

    print("\nAll candidates rejected.")
    return handle_rejected_candidates(game, ranker, fen)


def handle_rejected_candidates(game, ranker, fen):
    while True:
        choice = input("Restart search (r), enter manual move (m), or quit (q)? ").strip().lower()

        if choice in ('r', 'restart'):
            return prompt_white_move(game, ranker)

        elif choice in ('m', 'manual'):
            return handle_manual_move(game, ranker, fen)

        elif choice in ('q', 'quit'):
            return None

        else:
            print("Please enter r, m, or q.")


def handle_manual_move(game, ranker, fen):
    while True:
        move_uci = input("Enter move in UCI format (e.g. e2e4): ").strip().lower()

        if move_uci in ('quit', 'q'):
            return None

        if not move_uci:
            print("Please enter a move or 'quit'.")
            continue

        if game.is_legal_move(move_uci):
            success, san = game.apply_move(move_uci)
            if success:
                ranker.record_user_choice(fen, move_uci)
                print(f"\nWhite played: {move_uci} ({san})")
                return True
            else:
                print(f"Error: {san}")
                continue
        else:
            print(f"'{move_uci}' is not a legal move. Try again.")


def handle_command(command, game):
    if command == 'board':
        print_board(game)

    elif command == 'fen':
        print(f"\nFEN: {game.get_fen()}\n")

    elif command == 'history':
        print(f"\n{game.get_move_history_display()}\n")

    elif command == 'quit':
        print("\nGoodbye!")
        sys.exit(0)


def play_black_move(game, engine):
    if game.is_game_over():
        return False

    result = engine.get_black_move(game.board)
    if not result or not result[0]:
        print("Black has no legal moves.")
        return False

    move_uci, score = result

    success, san = game.apply_move(move_uci)
    if success:
        print(f"Black played: {move_uci} ({san})")
        if score is not None:
            if isinstance(score, str) and score.startswith('mate'):
                print(f"Evaluation: {score}")
            else:
                print(f"Evaluation: {score:+d} centipawns")
        return True
    else:
        print(f"Error applying Black move: {san}")
        return False


def check_game_over(game):
    result = game.get_game_result()
    if result:
        print(f"\n{'=' * 50}")
        print(f"  {result}")
        print(f"{'=' * 50}")
        print(f"\nFinal FEN: {game.get_fen()}")
        print(f"\n{game.get_move_history_display()}")
        return True
    return False


def main():
    print_header()

    engine = Engine()
    if not engine.is_stockfish_available():
        print("  Stockfish not found. Using fallback move selector for Black.")
        print("  Set STOCKFISH_PATH environment variable for stronger play.\n")
    else:
        print("  Stockfish engine loaded.\n")

    print_instructions()

    game = Game()
    ranker = MoveRanker()

    print_board(game)
    print(f"FEN: {game.get_fen()}\n")

    move_number = 1

    while True:
        print(f"--- Move {move_number}: White's turn ---")

        result = prompt_white_move(game, ranker)

        if result is None:
            print("\nGoodbye!")
            break

        if result is False:
            print("Failed to determine White move. Exiting.")
            break

        print_board(game)

        if check_game_over(game):
            break

        print(f"--- Move {move_number}: Black's turn ---")

        if not play_black_move(game, engine):
            print("Black could not move.")
            break

        print_board(game)
        print(f"FEN: {game.get_fen()}\n")

        if check_game_over(game):
            break

        move_number += 1


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nInterrupted. Goodbye!")
        sys.exit(0)
    except EOFError:
        print("\n\nGoodbye!")
        sys.exit(0)
