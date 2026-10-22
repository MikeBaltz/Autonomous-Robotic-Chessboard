#include "game.h"
#include "logger.h"

Game::Game() {
    reset();
}

void Game::begin() {
    reset();
}

void Game::reset() {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            board[i][j] = {PieceType::None, PieceColor::None};
        }
    }

    for (int i = 0; i < 8; i++) {
        board[i][1] = {PieceType::Pawn, PieceColor::White};
        board[i][6] = {PieceType::Pawn, PieceColor::Black};
    }

    board[0][0] = board[7][0] = {PieceType::Rook, PieceColor::White};
    board[0][7] = board[7][7] = {PieceType::Rook, PieceColor::Black};

    board[1][0] = board[6][0] = {PieceType::Knight, PieceColor::White};
    board[1][7] = board[6][7] = {PieceType::Knight, PieceColor::Black};

    board[2][0] = board[5][0] = {PieceType::Bishop, PieceColor::White};
    board[2][7] = board[5][7] = {PieceType::Bishop, PieceColor::Black};

    board[3][0] = {PieceType::Queen, PieceColor::White};
    board[3][7] = {PieceType::Queen, PieceColor::Black};

    board[4][0] = {PieceType::King, PieceColor::White};
    board[4][7] = {PieceType::King, PieceColor::Black};
}

Piece Game::getPieceAt(char col, int row) const {
    if (!isValidSquare(col, row)) return {PieceType::None, PieceColor::None};
    return board[colToIndex(col)][rowToIndex(row)];
}

void Game::setPieceAt(char col, int row, Piece piece) {
    if (!isValidSquare(col, row)) return;
    board[colToIndex(col)][rowToIndex(row)] = piece;
}

void Game::removePieceAt(char col, int row) {
    setPieceAt(col, row, {PieceType::None, PieceColor::None});
}

bool Game::movePiece(char fromCol, int fromRow, char toCol, int toRow) {
    if (!isValidMove(fromCol, fromRow, toCol, toRow)) return false;

    Piece piece = getPieceAt(fromCol, fromRow);
    removePieceAt(fromCol, fromRow);
    setPieceAt(toCol, toRow, piece);
    return true;
}

bool Game::isValidMove(char fromCol, int fromRow, char toCol, int toRow) const {
    if (!isValidSquare(fromCol, fromRow) || !isValidSquare(toCol, toRow)) return false;

    Piece from = getPieceAt(fromCol, fromRow);
    Piece to = getPieceAt(toCol, toRow);

    if (from.isEmpty()) return false;
    if (!to.isEmpty() && from.color == to.color) return false;

    return true;
}

void Game::printBoard() const {
    Logger::info("  a b c d e f g h");
    for (int row = 7; row >= 0; row--) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d ", row + 1);
        String line = buf;

        for (int col = 0; col < 8; col++) {
            Piece p = board[col][row];
            char c = '.';
            if (p.type == PieceType::Pawn) c = (p.color == PieceColor::White) ? 'P' : 'p';
            else if (p.type == PieceType::Rook) c = (p.color == PieceColor::White) ? 'R' : 'r';
            else if (p.type == PieceType::Knight) c = (p.color == PieceColor::White) ? 'N' : 'n';
            else if (p.type == PieceType::Bishop) c = (p.color == PieceColor::White) ? 'B' : 'b';
            else if (p.type == PieceType::Queen) c = (p.color == PieceColor::White) ? 'Q' : 'q';
            else if (p.type == PieceType::King) c = (p.color == PieceColor::White) ? 'K' : 'k';

            line += c;
            line += ' ';
        }

        snprintf(buf, sizeof(buf), "%d", row + 1);
        line += buf;
        Logger::info(line.c_str());
    }
    Logger::info("  a b c d e f g h");
}

bool Game::isValidSquare(char col, int row) const {
    return col >= 'a' && col <= 'h' && row >= 1 && row <= 8;
}

int Game::colToIndex(char col) const {
    return col - 'a';
}

int Game::rowToIndex(int row) const {
    return row - 1;
}
