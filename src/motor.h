#pragma once

#include <Arduino.h>

class Motor {
public:
    Motor(uint8_t stepPin, uint8_t dirPin, uint8_t enPin);
    
    void begin();
    void enable();
    void disable();
    bool isEnabled() const;
    
    void setDirection(bool clockwise);
    void step();
    void moveSteps(uint32_t steps, uint32_t stepsPerSecond = 500);
    
private:
    uint8_t stepPin;
    uint8_t dirPin;
    uint8_t enPin;
    bool enabled;
    bool direction;
};
