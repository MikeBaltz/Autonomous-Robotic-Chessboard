#include <Arduino.h>

#include "timer.h"

Timer::Timer(uint32_t periodMs)
    : periodMs(periodMs),
      previousTime(0) {
}

void Timer::start() {
    previousTime = millis();
}

void Timer::reset() {
    previousTime = millis();
}

bool Timer::expired() {
    const uint32_t currentTime = millis();

    if (currentTime - previousTime >= periodMs) {
        previousTime = currentTime;
        return true;
    }

    return false;
}