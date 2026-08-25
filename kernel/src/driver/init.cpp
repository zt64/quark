#include "driver/init.hpp"

#include <driver/fs/vfs.hpp>

#include "driver/acpi.hpp"
#include "driver/pci.hpp"
#include "driver/smbios.hpp"
#include "driver/timer.hpp"
#include "driver/ps2/keyboard.hpp"
#include "driver/ps2/ps2.hpp"
#include "kernel/log.hpp"

namespace drivers {
    void init_early() {
        acpi::init();
        logger.info("ACPI tables initialized");

        timer::init(100);
        logger.info("PIC timer initialized");
    }

    void init_late() {
        ps2::init();
        logger.info("PS/2 controller initialized");

        keyboard::init();
        logger.info("PS/2 keyboard initialized");

        logger.info("Starting PCI bus enumeration");
        // pci::enumerate_busses();
        // logger.info("PCI enumeration completed");

        vfs::init();
        logger.info("Virtual file system initialized");
        // smbios::print();
    }
}
