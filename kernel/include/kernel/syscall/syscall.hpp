#pragma once

#include "kernel/system.hpp"
#include <cstddef>
#include <type_traits>
#include <utility>

template <size_t I>
constexpr uint64_t get_arg(const regs* r) {
    if constexpr (I == 0) return r->rdi;
    else if constexpr (I == 1) return r->rsi;
    else if constexpr (I == 2) return r->rdx;
    else if constexpr (I == 3) return r->r10; // syscall ABI: r10, not rcx. syscall clobbers rcx
    else if constexpr (I == 4) return r->r8;
    else if constexpr (I == 5) return r->r9;
    else static_assert(I < 6, "Too many syscall arguments");
}

template <typename T>
constexpr T reg_cast(uint64_t v) {
    if constexpr (std::is_pointer_v<T>) {
        return reinterpret_cast<T>(v);
    } else {
        return static_cast<T>(v);
    }
}

template <typename Ret, typename... Args>
int64_t invoke_syscall(Ret (*fn)(Args...), const regs* r) {
    return [&]<size_t... I>(std::index_sequence<I...>) -> int64_t {
        if constexpr (std::is_void_v<Ret>) {
            fn(reg_cast<Args>(get_arg<I>(r))...);
            return 0;
        } else if constexpr (std::is_pointer_v<Ret>) {
            return reinterpret_cast<int64_t>(fn(reg_cast<Args>(get_arg<I>(r))...));
        } else {
            return static_cast<int64_t>(fn(reg_cast<Args>(get_arg<I>(r))...));
        }
    }(std::index_sequence_for<Args...>{});
}

extern "C" void handle_syscall(regs* r);

#define SYSCALL_HANDLER(name) \
    int64_t handle_##name(const regs* r) { \
        return invoke_syscall(sys_##name, r); \
    }