#pragma once

class Logger {
public:
    static void begin();

    static void info(const char* message);
    static void warning(const char* message);
    static void error(const char* message);
};