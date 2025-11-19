#include "kernel/cmdline.hpp"

struct param {
    const char* name;
    char* value;
};

void parse_cmdline(char* cmdline) {
    logger.info("Kernel cmdline=%s", cmdline);

    param* params[32];
}