#pragma once

#include <Arduino.h>

namespace Config {

constexpr uint32_t SERIAL_BAUDRATE = 115200;

// Existing setting
constexpr unsigned long HEARTBEAT_PERIOD_MS = 10000;

// Single Hall sensor for magnet presence detection
constexpr uint8_t HALL_SENSOR_PIN = 4;

constexpr bool HALL_ACTIVE_LOW = true;

// Number of identical consecutive readings required
// before accepting a state change.
constexpr uint8_t HALL_DEBOUNCE_SAMPLES = 5;

// Time between sensor samples.
constexpr unsigned long HALL_SAMPLE_PERIOD_MS = 10;

// X-axis motor control pins
constexpr uint8_t MOTOR_X_STEP_PIN = 16;
constexpr uint8_t MOTOR_X_DIR_PIN = 17;
constexpr uint8_t MOTOR_X_EN_PIN = 18;

// Y-axis motor control pins (to be wired)
constexpr uint8_t MOTOR_Y_STEP_PIN = 8;
constexpr uint8_t MOTOR_Y_DIR_PIN = 9;
constexpr uint8_t MOTOR_Y_EN_PIN = 10;

// Motor settings
constexpr uint32_t MOTOR_DEFAULT_SPEED = 500;
constexpr uint8_t MOTOR_ENABLE_ACTIVE_LOW = 1;

// Electromagnet
constexpr uint8_t ELECTROMAGNET_PIN = 39;
constexpr uint8_t ELECTROMAGNET_STRENGTH = 255;  // PWM duty: 0-255 (128 = ~50%)

}  // namespace Config
