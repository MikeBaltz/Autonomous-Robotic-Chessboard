#pragma once

#include <Arduino.h>

#include "sensor.h"
#include "timer.h"

class Board {
public:
    explicit Board(Sensor& sensor);

    void begin();
    void update();

private:
    Sensor& sensor;

    Timer heartbeatTimer;
    unsigned long counter;
};
