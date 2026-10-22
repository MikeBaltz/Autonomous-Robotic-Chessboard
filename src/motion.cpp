#include "motion.h"
#include "logger.h"

Motion::Motion(Motor& xMotor, Motor& yMotor, Electromagnet& electromagnet, Sensor& sensor)
    : xMotor(xMotor), yMotor(yMotor), electromagnet(electromagnet), sensor(sensor),
      currentCol(START_COLUMN), currentRow(START_ROW), holdingPiece(false) {
}

void Motion::begin() {
    xMotor.begin();
    yMotor.begin();
    electromagnet.begin();
    
    xMotor.enable();
    yMotor.enable();
}

void Motion::moveToSquare(char col, int row) {
    moveToColumn(col);
    moveToRow(row);
}

void Motion::moveToColumn(char col) {
    if (col < 'a' || col > 'h') return;

    float targetCol = col - 'a';
    float distance = targetCol - currentCol;
    int steps = abs(distance) * STEPS_PER_COLUMN;
    bool direction = (distance > 0);

    moveXSteps(direction ? steps : -steps);
    currentCol = targetCol;
}

void Motion::moveToRow(int row) {
    if (row < 1 || row > 8) return;

    float distance = row - currentRow;
    int steps = abs(distance) * STEPS_PER_ROW;
    bool direction = (distance > 0);

    moveYSteps(direction ? steps : -steps);
    currentRow = row;
}

void Motion::pickUpPiece() {
    electromagnet.enable();
    holdingPiece = true;
    delay(100);
}

void Motion::placePiece() {
    electromagnet.disable();
    holdingPiece = false;
    delay(100);
}

bool Motion::scanSquare(char col, int row) {
    moveToSquare(col, row);
    delay(50);
    
    for (int i = 0; i < 5; i++) {
        sensor.update();
        delay(10);
    }
    
    return sensor.isMagnetPresent();
}

String Motion::detectWhiteMove() {
    Logger::info("Scanning for white move...");
    
    for (int row = 1; row <= 2; row++) {
        if (row == 1) {
            for (int colIdx = 0; colIdx < 8; colIdx++) {
                char col = 'a' + colIdx;
                if (!scanSquare(col, row)) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "Missing piece at %c%d", col, row);
                    Logger::info(buf);
                    
                    String move = "";
                    move += col;
                    move += String(row);
                    move += "?";
                    return move;
                }
            }
        } else {
            for (int colIdx = 7; colIdx >= 0; colIdx--) {
                char col = 'a' + colIdx;
                if (!scanSquare(col, row)) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "Missing piece at %c%d", col, row);
                    Logger::info(buf);
                    
                    String move = "";
                    move += col;
                    move += String(row);
                    move += "?";
                    return move;
                }
            }
        }
    }
    
    Logger::info("No missing white piece found");
    return "ERROR";
}

void Motion::toggleXMotor() {
    if (xMotor.isEnabled()) {
        xMotor.disable();
    } else {
        xMotor.enable();
    }
}

void Motion::toggleYMotor() {
    if (yMotor.isEnabled()) {
        yMotor.disable();
    } else {
        yMotor.enable();
    }
}

bool Motion::isXMotorEnabled() const {
    return xMotor.isEnabled();
}

bool Motion::isYMotorEnabled() const {
    return yMotor.isEnabled();
}

float Motion::getCurrentColumn() const {
    return currentCol;
}

float Motion::getCurrentRow() const {
    return currentRow;
}

bool Motion::isHoldingPiece() const {
    return holdingPiece;
}

void Motion::moveXSteps(int steps) {
    if (steps == 0) return;
    xMotor.enable();
    xMotor.setDirection(steps > 0);
    xMotor.moveSteps(abs(steps), MOVE_SPEED);
}

void Motion::moveYSteps(int steps) {
    if (steps == 0) return;
    yMotor.enable();
    yMotor.setDirection(steps > 0);
    yMotor.moveSteps(abs(steps), MOVE_SPEED);
}

int Motion::columnToSteps(char col) const {
    if (col < 'a' || col > 'h') return 0;
    float targetCol = col - 'a';
    return (targetCol - currentCol) * STEPS_PER_COLUMN;
}

int Motion::rowToSteps(int row) const {
    if (row < 1 || row > 8) return 0;
    return (row - currentRow) * STEPS_PER_ROW;
}
