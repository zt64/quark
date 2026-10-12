#pragma once

#include <kernel/system.hpp>

extern "C" [[noreturn]] void fault_handler(const regs* r);
