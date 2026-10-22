#include <Arduino.h>

#include "logger.h"
#include "config.h"

void Logger::begin() {
    Serial.begin(Config::SERIAL_BAUDRATE);
}

void Logger::info(const char* message) {
    Serial.print("[INFO] ");
    Serial.println(message);
}

void Logger::warning(const char* message) {
    Serial.print("[WARNING] ");
    Serial.println(message);
}

void Logger::error(const char* message) {
    Serial.print("[ERROR] ");
    Serial.println(message);
}