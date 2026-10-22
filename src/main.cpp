#include <Arduino.h>
#include "logger.h"
#include "sensor.h"
#include "board.h"
#include "motor.h"
#include "electromagnet.h"
#include "motion.h"
#include "game.h"
#include "protocol.h"
#include "config.h"

Sensor sensor(Config::HALL_SENSOR_PIN);
Board board(sensor);

Motor xMotor(Config::MOTOR_X_STEP_PIN, Config::MOTOR_X_DIR_PIN, Config::MOTOR_X_EN_PIN);
Motor yMotor(Config::MOTOR_Y_STEP_PIN, Config::MOTOR_Y_DIR_PIN, Config::MOTOR_Y_EN_PIN);
Electromagnet electromagnet(Config::ELECTROMAGNET_PIN, Config::ELECTROMAGNET_STRENGTH);

Motion motion(xMotor, yMotor, electromagnet, sensor);
Game game;
Protocol protocol(motion, game, electromagnet);

void setup() {
    Logger::begin();
    delay(2000);

    Logger::info("=== Chess Robot Initialization ===");

    sensor.begin();
    board.begin();
    motion.begin();
    game.begin();
    protocol.begin();

    Logger::info("System ready");
}

void loop() {
    sensor.update();
    board.update();
    protocol.update();

    delay(10);
}
