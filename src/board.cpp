#include "board.h"

#include "config.h"
#include "logger.h"

Board::Board(Sensor& sensor)
    : sensor(sensor),
      heartbeatTimer(Config::HEARTBEAT_PERIOD_MS),
      counter(0) {
}

void Board::begin() {
    Logger::info("Board module started");
    heartbeatTimer.start();
}

void Board::update() {
    if (sensor.stateChanged()) {
        Serial.printf(
            "[BOARD] Magnet: %s\n",
            sensor.isMagnetPresent() ? "DETECTED" : "EMPTY"
        );
    }

    if (heartbeatTimer.expired()) {
        Serial.printf(
            "[INFO] System alive: %lu | Magnet: %s\n",
            counter++,
            sensor.isMagnetPresent() ? "DETECTED" : "EMPTY"
        );
    }
}
