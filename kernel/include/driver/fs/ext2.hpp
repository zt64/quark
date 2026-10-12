#pragma once

#include <driver/fs/fs.hpp>

namespace fs::ext2 {
    class Ext2Filesystem : public Filesystem {
    public:
        void init() override;
    };
}
