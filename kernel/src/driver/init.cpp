#include "driver/init.hpp"

#include <kernel/console.hpp>
#include <kernel/fs/vfs.hpp>
#include "../../include/driver/pci/pci.hpp"
#include "driver/acpi/acpi.hpp"
#include "driver/ps2/keyboard.hpp"
#include "driver/ps2/ps2.hpp"
#include "kernel/log.hpp"

namespace drivers {
    void init_early() {
        acpi::init();
        logger.info("ACPI tables initialized");
    }

    void init_late() {
        ps2::init();
        logger.info("PS/2 controller initialized");

        logger.info("Starting PCI bus enumeration");
        pci::enumerate_busses();
        logger.info("PCI enumeration completed");

        vfs::init();
        logger.info("Virtual file system initialized");

        console::init();
        logger.info("Console initialized");
    }
}
