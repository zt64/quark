#pragma once
#include "log.hpp"

struct Config {
    LogLevel log_level;
};

void parse_cmdline(char* cmdline);