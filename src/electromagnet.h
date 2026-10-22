#pragma once

#include <Arduino.h>

class Electromagnet {
public:
    Electromagnet(uint8_t pin, uint8_t duty = 255);

    void begin();
    void enable();
    void disable();
    void toggle();
    void setStrength(uint8_t duty);
    uint8_t getStrength() const;
    bool isEnabled() const;

private:
    uint8_t pin;
    uint8_t duty;
    bool enabled;
    static constexpr uint8_t PWM_CHANNEL = 0;
    static constexpr uint32_t PWM_FREQ = 1000;
    static constexpr uint8_t PWM_RESOLUTION = 8;
};
