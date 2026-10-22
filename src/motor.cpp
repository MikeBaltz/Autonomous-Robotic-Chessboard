#include "motor.h"

Motor::Motor(uint8_t stepPin, uint8_t dirPin, uint8_t enPin)
    : stepPin(stepPin), dirPin(dirPin), enPin(enPin), 
      enabled(false), direction(true) {
}

void Motor::begin() {
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    pinMode(enPin, OUTPUT);
    
    digitalWrite(stepPin, LOW);
    digitalWrite(dirPin, HIGH);
    digitalWrite(enPin, HIGH);
    
    Serial.println("[MOTOR] Initialized");
}

void Motor::enable() {
    digitalWrite(enPin, LOW);
    enabled = true;
    Serial.println("[MOTOR] Enabled");
}

void Motor::disable() {
    digitalWrite(enPin, HIGH);
    enabled = false;
    Serial.println("[MOTOR] Disabled");
}

bool Motor::isEnabled() const {
    return enabled;
}

void Motor::setDirection(bool clockwise) {
    direction = clockwise;
    digitalWrite(dirPin, clockwise ? HIGH : LOW);
}

void Motor::step() {
    if (!enabled) {
        Serial.println("[MOTOR] Warning: Cannot step while disabled");
        return;
    }
    
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(1000);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(1000);
}

void Motor::moveSteps(uint32_t steps, uint32_t stepsPerSecond) {
    if (!enabled) {
        Serial.println("[MOTOR] Warning: Cannot move while disabled");
        return;
    }
    
    uint32_t stepDelay = 1000000 / (stepsPerSecond * 2);
    
    Serial.print("[MOTOR] Moving ");
    Serial.print(steps);
    Serial.print(" steps at ");
    Serial.print(stepsPerSecond);
    Serial.println(" steps/sec");
    
    for (uint32_t i = 0; i < steps; i++) {
        digitalWrite(stepPin, HIGH);
        delayMicroseconds(stepDelay);
        digitalWrite(stepPin, LOW);
        delayMicroseconds(stepDelay);
    }
}
