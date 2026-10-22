#include "protocol.h"
#include "logger.h"

Protocol::Protocol(Motion& motion, Game& game, Electromagnet& electromagnet)
    : motion(motion), game(game), electromagnet(electromagnet) {
}

void Protocol::begin() {
    buffer = "";
    printHelp();
}

void Protocol::update() {
    while (Serial.available()) {
        char c = Serial.read();

        if (c == '\n' || c == '\r') {
            if (buffer.length() > 0) {
                handleCommand(buffer);
                buffer = "";
            }
        } else {
            buffer += c;
        }
    }
}

void Protocol::printHelp() const {
    Logger::info("=== Chess Robot Commands ===");
    Logger::info("  e2e4      - Move piece from e2 to e4");
    Logger::info("  m a       - Move carriage to column a");
    Logger::info("  m a3      - Move carriage to square a3");
    Logger::info("  p         - Print board state");
    Logger::info("  r         - Reset game");
    Logger::info("  s         - Status");
    Logger::info("  scan      - Scan for white move");
    Logger::info("  1         - Electromagnet ON");
    Logger::info("  0         - Electromagnet OFF");
    Logger::info("  t         - Toggle electromagnet");
    Logger::info("  x         - Enable/disable X motor");
    Logger::info("  y         - Enable/disable Y motor");
    Logger::info("  h         - Show this help");
    Logger::info("============================");
}

void Protocol::handleCommand(const String& cmd) {
    if (cmd.length() == 0) return;

    if (cmd.length() == 4 && cmd[0] >= 'a' && cmd[0] <= 'h' &&
        cmd[1] >= '1' && cmd[1] <= '8' &&
        cmd[2] >= 'a' && cmd[2] <= 'h' &&
        cmd[3] >= '1' && cmd[3] <= '8') {
        handleMoveCommand(cmd);
    } else if (cmd.startsWith("m ")) {
        handleMoveToCommand(cmd);
    } else if (cmd == "p") {
        game.printBoard();
    } else if (cmd == "r") {
        game.reset();
        Logger::info("Game reset");
    } else if (cmd == "s") {
        char buf[64];
        snprintf(buf, sizeof(buf), "Position: %.1f,%.1f, Holding: %s",
                 motion.getCurrentColumn(), motion.getCurrentRow(),
                 motion.isHoldingPiece() ? "yes" : "no");
        Logger::info(buf);
    } else if (cmd == "1") {
        electromagnet.enable();
        Logger::info("Electromagnet ON");
    } else if (cmd == "0") {
        electromagnet.disable();
        Logger::info("Electromagnet OFF");
    } else if (cmd == "t") {
        electromagnet.toggle();
        Logger::info(electromagnet.isEnabled() ? "Electromagnet ON" : "Electromagnet OFF");
    } else if (cmd == "x") {
        motion.toggleXMotor();
        Logger::info(motion.isXMotorEnabled() ? "X motor ENABLED" : "X motor DISABLED");
    } else if (cmd == "y") {
        motion.toggleYMotor();
        Logger::info(motion.isYMotorEnabled() ? "Y motor ENABLED" : "Y motor DISABLED");
    } else if (cmd == "scan") {
        String result = motion.detectWhiteMove();
        char buf[64];
        snprintf(buf, sizeof(buf), "Scan result: %s", result.c_str());
        Logger::info(buf);
    } else if (cmd == "h") {
        printHelp();
    } else {
        Logger::info("Unknown command. Type 'h' for help.");
    }
}

void Protocol::handleMoveCommand(const String& cmd) {
    char fromCol = cmd[0];
    int fromRow = cmd[1] - '0';
    char toCol = cmd[2];
    int toRow = cmd[3] - '0';

    if (!game.isValidMove(fromCol, fromRow, toCol, toRow)) {
        Logger::info("Invalid move");
        return;
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "Moving %c%d to %c%d", fromCol, fromRow, toCol, toRow);
    Logger::info(buf);

    motion.moveToSquare(fromCol, fromRow);
    motion.pickUpPiece();
    motion.moveToSquare(toCol, toRow);
    motion.placePiece();

    game.movePiece(fromCol, fromRow, toCol, toRow);
    Logger::info("Move complete");
}

void Protocol::handleMoveToCommand(const String& cmd) {
    if (cmd.length() < 3) {
        Logger::info("Usage: m <col> or m <col><row>");
        return;
    }

    char col = cmd[2];
    if (col < 'a' || col > 'h') {
        Logger::info("Invalid column");
        return;
    }

    if (cmd.length() == 3) {
        motion.moveToColumn(col);
        char buf[32];
        snprintf(buf, sizeof(buf), "Moved to column %c", col);
        Logger::info(buf);
    } else if (cmd.length() == 4) {
        int row = cmd[3] - '0';
        if (row < 1 || row > 8) {
            Logger::info("Invalid row");
            return;
        }
        motion.moveToSquare(col, row);
        char buf[32];
        snprintf(buf, sizeof(buf), "Moved to %c%d", col, row);
        Logger::info(buf);
    }
}
