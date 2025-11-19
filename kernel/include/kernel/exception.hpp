#pragma once

struct regs;

extern "C" [[noreturn]] void fault_handler(const regs* r);
