#pragma once

#include <Arduino.h>

class Sensor {
public:
    Sensor(uint8_t sensorPin);

    void begin();
    void update();

    bool isMagnetPresent() const;
    bool stateChanged();

private:
    bool readRawState() const;
    bool isPinActive(uint8_t p) const;

    uint8_t sensorPin;

    bool stableState;
    bool candidateState;
    uint8_t candidateCount;
    bool changed;
    unsigned long lastSampleTime;
};