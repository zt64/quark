#pragma once

#include <mlibc/sysdep-signatures.hpp>

namespace mlibc {
    struct QuarkSysdepTags :
        LibcPanic,
        LibcLog,
        Isatty,
        Write,
        TcbSet,
        AnonAllocate,
        AnonFree,
        Seek,
        Exit,
        Close,
        FutexWake,
        FutexWait,
        Read,
        Open,
        OpenDir,
        VmMap,
        VmUnmap,
        Execve,
        Fork,
        Waitpid,
        ClockGet,
        GetPid {
    };

    template <typename Tag>
    using Sysdeps = SysdepOf<QuarkSysdepTags, Tag>;

    struct SysdepTraits {
        static constexpr bool usesRtNetlink = false;
    };
} // namespace mlibc
