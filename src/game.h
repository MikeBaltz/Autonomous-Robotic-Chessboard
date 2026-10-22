#pragma once

#include <Arduino.h>

enum class PieceType : uint8_t {
    None,
    Pawn,
    Rook,
    Knight,
    Bishop,
    Queen,
    King
};

enum class PieceColor : uint8_t {
    None,
    White,
    Black
};

struct Piece {
    PieceType type;
    PieceColor color;

    bool isEmpty() const { return type == PieceType::None; }
};

class Game {
public:
    Game();

    void begin();
    void reset();

    Piece getPieceAt(char col, int row) const;
    void setPieceAt(char col, int row, Piece piece);
    void removePieceAt(char col, int row);

    bool movePiece(char fromCol, int fromRow, char toCol, int toRow);
    bool isValidMove(char fromCol, int fromRow, char toCol, int toRow) const;

    void printBoard() const;

private:
    bool isValidSquare(char col, int row) const;
    int colToIndex(char col) const;
    int rowToIndex(int row) const;

    Piece board[8][8];
};
