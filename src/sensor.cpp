#include "sensor.h"

#include "config.h"
#include "logger.h"

Sensor::Sensor(uint8_t sensorPin)
    : sensorPin(sensorPin),
      stableState(false),
      candidateState(false),
      candidateCount(0),
      changed(false),
      lastSampleTime(0) {
}

void Sensor::begin() {
    pinMode(sensorPin, INPUT_PULLUP);

    stableState = readRawState();
    candidateState = stableState;

    Logger::info("Hall sensor started");

    Serial.printf(
        "[SENSOR] Initial state: %s (pin: %d)\n",
        stableState ? "MAGNET" : "EMPTY",
        isPinActive(sensorPin)
    );
}

void Sensor::update() {
    changed = false;

    const unsigned long now = millis();

    if (now - lastSampleTime < Config::HALL_SAMPLE_PERIOD_MS) {
        return;
    }

    lastSampleTime = now;

    const bool rawState = readRawState();

    if (rawState != candidateState) {
        candidateState = rawState;
        candidateCount = 1;
        return;
    }

    if (candidateCount < Config::HALL_DEBOUNCE_SAMPLES) {
        candidateCount++;
    }

    if (
        candidateCount >= Config::HALL_DEBOUNCE_SAMPLES &&
        candidateState != stableState
    ) {
        stableState = candidateState;
        changed = true;

        Serial.printf(
            "[SENSOR] State changed to: %s (pin: %d)\n",
            stableState ? "MAGNET" : "EMPTY",
            isPinActive(sensorPin)
        );
    }
}

bool Sensor::isMagnetPresent() const {
    return stableState;
}

bool Sensor::stateChanged() {
    const bool result = changed;
    changed = false;
    return result;
}

bool Sensor::readRawState() const {
    return isPinActive(sensorPin);
}

bool Sensor::isPinActive(uint8_t p) const {
    const int value = digitalRead(p);

    if (Config::HALL_ACTIVE_LOW) {
        return value == LOW;
    }

    return value == HIGH;
}
