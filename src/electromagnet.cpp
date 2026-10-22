#include "electromagnet.h"
#include "logger.h"

Electromagnet::Electromagnet(uint8_t pin, uint8_t duty)
    : pin(pin), duty(duty), enabled(false) {
}

void Electromagnet::begin() {
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(pin, PWM_CHANNEL);
    ledcWrite(PWM_CHANNEL, 0);
    Logger::info("Electromagnet initialized");
}

void Electromagnet::enable() {
    ledcWrite(PWM_CHANNEL, duty);
    enabled = true;
    Logger::info("Electromagnet ON");
}

void Electromagnet::disable() {
    ledcWrite(PWM_CHANNEL, 0);
    enabled = false;
    Logger::info("Electromagnet OFF");
}

void Electromagnet::toggle() {
    if (enabled) {
        disable();
    } else {
        enable();
    }
}

void Electromagnet::setStrength(uint8_t newDuty) {
    duty = newDuty;
    if (enabled) {
        ledcWrite(PWM_CHANNEL, duty);
    }
}

uint8_t Electromagnet::getStrength() const {
    return duty;
}

bool Electromagnet::isEnabled() const {
    return enabled;
}
