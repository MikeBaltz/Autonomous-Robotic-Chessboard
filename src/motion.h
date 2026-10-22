#pragma once

#include <Arduino.h>
#include "motor.h"
#include "electromagnet.h"
#include "sensor.h"

class Motion {
public:
    Motion(Motor& xMotor, Motor& yMotor, Electromagnet& electromagnet, Sensor& sensor);

    void begin();

    void moveToSquare(char col, int row);
    void moveToColumn(char col);
    void moveToRow(int row);

    void pickUpPiece();
    void placePiece();

    bool scanSquare(char col, int row);
    String detectWhiteMove();

    void toggleXMotor();
    void toggleYMotor();
    bool isXMotorEnabled() const;
    bool isYMotorEnabled() const;

    float getCurrentColumn() const;
    float getCurrentRow() const;
    bool isHoldingPiece() const;

private:
    void moveXSteps(int steps);
    void moveYSteps(int steps);

    int columnToSteps(char col) const;
    int rowToSteps(int row) const;

    Motor& xMotor;
    Motor& yMotor;
    Electromagnet& electromagnet;
    Sensor& sensor;

    float currentCol;
    float currentRow;
    bool holdingPiece;

    static constexpr int STEPS_PER_COLUMN = 2611;
    static constexpr int STEPS_PER_ROW = 2611;
    static constexpr float START_COLUMN = 3.5;
    static constexpr float START_ROW = 4.5;
    static constexpr uint32_t MOVE_SPEED = 500;
};
