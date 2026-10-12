#include "kernel/cmdline.hpp"
#include <cstdint>
#include "lib/string.hpp"

namespace {
    struct param {
        const char* name;
        const char* value;
    };
}

void parse_cmdline(const char* cmdline) {
    logger.info("Kernel cmdline=%s", cmdline);

    param* params[32];
    uint16_t param_count = 0;

    const uint16_t cmdline_length = strlen(cmdline);
    uint16_t i = 0;

    while (i < cmdline_length) {
        const char* name = cmdline + i;
        const char* value = nullptr;

        while (i < cmdline_length && cmdline[i] != '=' && cmdline[i] != ' ') {
            ++i;
        }

        if (i < cmdline_length && cmdline[i] == '=') {
            ++i;
            value = cmdline + i;
            while (i < cmdline_length && cmdline[i] != ' ') {
                ++i;
            }
        }

        if (i < cmdline_length && cmdline[i] == ' ') {
            ++i;
        }

        params[param_count++] = new param{.name = name, .value = value};
    }

    for (int i = 0; i < 32; ++i) {
        if (params[i] == nullptr) break;
        logger.info("Cmdline param: %s=%s", params[i]->name, params[i]->value);
    }
}