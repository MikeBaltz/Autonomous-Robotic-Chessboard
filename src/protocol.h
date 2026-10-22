#pragma once

#include <Arduino.h>
#include "motion.h"
#include "game.h"
#include "electromagnet.h"

class Protocol {
public:
    Protocol(Motion& motion, Game& game, Electromagnet& electromagnet);

    void begin();
    void update();

    void printHelp() const;

private:
    void handleCommand(const String& cmd);
    void handleMoveCommand(const String& cmd);
    void handleMoveToCommand(const String& cmd);

    Motion& motion;
    Game& game;
    Electromagnet& electromagnet;

    String buffer;
};
