#pragma once

#include <Arduino.h>

class Timer {
public:
    explicit Timer(uint32_t periodMs);

    void start();
    void reset();

    bool expired();

private:
    uint32_t periodMs;
    uint32_t previousTime;
};