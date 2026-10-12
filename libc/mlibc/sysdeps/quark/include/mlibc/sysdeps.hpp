#pragma once

#include <mlibc/sysdep-signatures.hpp>

#include "../../../../options/internal/include/mlibc/sysdep-tags.hpp"

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
        ReadEntries,
        Open,
        OpenDir,
        VmMap,
        VmUnmap,
        VmProtect,
        Execve,
        Fork,
        Waitpid,
        Kill,
        ClockGet,
        ClockGetres,
        ClockSet,
        GetPid,
        GetTid,
        Stat,
        GetCwd,
        Chdir,
        Mkdir,
        Sleep {
    };

    template <typename Tag>
    using Sysdeps = SysdepOf<QuarkSysdepTags, Tag>;

    struct SysdepTraits {
        static constexpr bool usesRtNetlink = false;
    };
} // namespace mlibc
