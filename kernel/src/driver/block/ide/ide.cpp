#include <cstdint>
#include <driver/timer.hpp>
#include <driver/pci/pci.hpp>
#include <kernel/log.hpp>
#include <kernel/system.hpp>

namespace ide {
    constexpr uint8_t ATA_SR_BSY = 0x80;
    constexpr uint8_t ATA_SR_DRDY = 0x40;
    constexpr uint8_t ATA_SR_DF = 0x20;
    constexpr uint8_t ATA_SR_DSC = 0x10;
    constexpr uint8_t ATA_SR_DRQ = 0x08;
    constexpr uint8_t ATA_SR_CORR = 0x04;
    constexpr uint8_t ATA_SR_IDX = 0x02;
    constexpr uint8_t ATA_SR_ERR = 0x01;

    constexpr uint8_t ATA_ER_BBK = 0x80;
    constexpr uint8_t ATA_ER_UNC = 0x40;
    constexpr uint8_t ATA_ER_MC = 0x20;
    constexpr uint8_t ATA_ER_IDNF = 0x10;
    constexpr uint8_t ATA_ER_MCR = 0x08;
    constexpr uint8_t ATA_ER_ABRT = 0x04;
    constexpr uint8_t ATA_ER_TK0NF = 0x02;
    constexpr uint8_t ATA_ER_AMNF = 0x01;

    constexpr uint8_t ATA_CMD_READ_PIO = 0x20;
    constexpr uint8_t ATA_CMD_READ_PIO_EXT = 0x24;
    constexpr uint8_t ATA_CMD_READ_DMA = 0xC8;
    constexpr uint8_t ATA_CMD_READ_DMA_EXT = 0x25;
    constexpr uint8_t ATA_CMD_WRITE_PIO = 0x30;
    constexpr uint8_t ATA_CMD_WRITE_PIO_EXT = 0x34;
    constexpr uint8_t ATA_CMD_WRITE_DMA = 0xCA;
    constexpr uint8_t ATA_CMD_WRITE_DMA_EXT = 0x35;
    constexpr uint8_t ATA_CMD_CACHE_FLUSH = 0xE7;
    constexpr uint8_t ATA_CMD_CACHE_FLUSH_EXT = 0xEA;
    constexpr uint8_t ATA_CMD_PACKET = 0xA0;
    constexpr uint8_t ATA_CMD_IDENTIFY_PACKET = 0xA1;
    constexpr uint8_t ATA_CMD_IDENTIFY = 0xEC;

    constexpr uint8_t ATAPI_CMD_READ = 0xA8;
    constexpr uint8_t ATAPI_CMD_EJECT = 0x1B;

    constexpr uint8_t ATA_IDENT_DEVICETYPE = 0;
    constexpr uint8_t ATA_IDENT_CYLINDERS = 2;
    constexpr uint8_t ATA_IDENT_HEADS = 6;
    constexpr uint8_t ATA_IDENT_SECTORS = 12;
    constexpr uint8_t ATA_IDENT_SERIAL = 20;
    constexpr uint8_t ATA_IDENT_MODEL = 54;
    constexpr uint8_t ATA_IDENT_CAPABILITIES = 98;
    constexpr uint8_t ATA_IDENT_FIELDVALID = 106;
    constexpr uint8_t ATA_IDENT_MAX_LBA = 120;
    constexpr uint8_t ATA_IDENT_COMMANDSETS = 164;
    constexpr uint8_t ATA_IDENT_MAX_LBA_EXT = 200;

    constexpr uint8_t IDE_ATA = 0x00;
    constexpr uint8_t IDE_ATAPI = 0x01;

    constexpr uint8_t ATA_MASTER = 0x00;
    constexpr uint8_t ATA_SLAVE = 0x01;

    constexpr uint8_t ATA_REG_DATA = 0x00;
    constexpr uint8_t ATA_REG_ERROR = 0x01;
    constexpr uint8_t ATA_REG_FEATURES = 0x01;
    constexpr uint8_t ATA_REG_SECCOUNT0 = 0x02;
    constexpr uint8_t ATA_REG_LBA0 = 0x03;
    constexpr uint8_t ATA_REG_LBA1 = 0x04;
    constexpr uint8_t ATA_REG_LBA2 = 0x05;
    constexpr uint8_t ATA_REG_HDDEVSEL = 0x06;
    constexpr uint8_t ATA_REG_COMMAND = 0x07;
    constexpr uint8_t ATA_REG_STATUS = 0x07;
    constexpr uint8_t ATA_REG_SECCOUNT1 = 0x08;
    constexpr uint8_t ATA_REG_LBA3 = 0x09;
    constexpr uint8_t ATA_REG_LBA4 = 0x0A;
    constexpr uint8_t ATA_REG_LBA5 = 0x0B;
    constexpr uint8_t ATA_REG_CONTROL = 0x0C;
    constexpr uint8_t ATA_REG_ALTSTATUS = 0x0C;
    constexpr uint8_t ATA_REG_DEVADDRESS = 0x0D;

    constexpr uint8_t ATA_PRIMARY = 0x00;
    constexpr uint8_t ATA_SECONDARY = 0x01;

    constexpr uint8_t ATA_READ = 0x00;
    constexpr uint8_t ATA_WRITE = 0x01;

    struct IDEChannelRegisters {
        uint16_t base; // I/O Base.
        uint16_t ctrl; // Control Base
        uint16_t bmide; // Bus Master IDE
        uint8_t nIEN; // nIEN (No Interrupt);
    } channels[2];

    uint32_t ide_buf[2048] = {0};
    volatile uint8_t ide_irq_invoked = 0;
    uint8_t atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    struct ide_device {
        uint8_t reserved; // 0 (Empty) or 1 (This Drive really exists).
        uint8_t channel; // 0 (Primary Channel) or 1 (Secondary Channel).
        uint8_t drive; // 0 (Master Drive) or 1 (Slave Drive).
        uint16_t type; // 0: ATA, 1:ATAPI.
        uint16_t signature; // Drive Signature
        uint16_t capabilities; // Features.
        uint32_t command_sets; // Command Sets Supported.
        uint32_t size; // Size in Sectors.
        uint8_t model[41]; // Model in string.
    } ide_devices[4];

    void ide_write(const uint8_t channel, const uint8_t reg, const uint8_t data) {
        if (reg > 0x07 && reg < 0x0C) {
            ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
        }
        if (reg < 0x08) {
            outb(channels[channel].base + reg - 0x00, data);
        } else if (reg < 0x0C) {
            outb(channels[channel].base + reg - 0x06, data);
        } else if (reg < 0x0E) {
            outb(channels[channel].ctrl + reg - 0x0A, data);
        } else if (reg < 0x16) {
            outb(channels[channel].bmide + reg - 0x0E, data);
        }
        if (reg > 0x07 && reg < 0x0C) {
            ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
        }
    }

    uint8_t ide_read(const uint8_t channel, const uint8_t reg) {
        uint8_t result;
        if (reg > 0x07 && reg < 0x0C) {
            ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
        }
        if (reg < 0x08) {
            result = inb(channels[channel].base + reg - 0x00);
        } else if (reg < 0x0C) {
            result = inb(channels[channel].base + reg - 0x06);
        } else if (reg < 0x0E) {
            result = inb(channels[channel].ctrl + reg - 0x0A);
        } else if (reg < 0x16) {
            result = inb(channels[channel].bmide + reg - 0x0E);
        }
        if (reg > 0x07 && reg < 0x0C) {
            ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
        }
        return result;
    }

    void insl(unsigned reg, uint32_t* buffer, const int quads) {
        int index;
        for (index = 0; index < quads; index++) {
            buffer[index] = inl(reg);
        }
    }

    void ide_read_buffer(uint8_t channel, uint8_t reg, uint32_t* buffer, uint32_t quads) {
        /* WARNING: This code contains a serious bug. The inline assembly trashes ES and
         *           ESP for all of the code the compiler generates between the inline
         *           assembly blocks.
         */
        if (reg > 0x07 && reg < 0x0C) {
            ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
        }

        // asm("pushw %es; movw %ds, %ax; movw %ax, %es");

        if (reg < 0x08) {
            insl(channels[channel].base + reg - 0x00, buffer, quads);
        } else if (reg < 0x0C) {
            insl(channels[channel].base + reg - 0x06, buffer, quads);
        } else if (reg < 0x0E) {
            insl(channels[channel].ctrl + reg - 0x0A, buffer, quads);
        } else if (reg < 0x16) {
            insl(channels[channel].bmide + reg - 0x0E, buffer, quads);
        }

        // asm("popw %es;"); // restore

        if (reg > 0x07 && reg < 0x0C) {
            ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
        }
    }

    void ide_initialize(const uint32_t bar0, const uint32_t bar1, const uint32_t bar2, const uint32_t bar3,
                        const uint32_t bar4) {
        uint32_t j, k, count = 0;

        constexpr uint32_t BAR_MASK = 0xFFFFFFFC;
        channels[ATA_PRIMARY].base = (bar0 & BAR_MASK) + 0x1F0 * (!bar0);
        channels[ATA_PRIMARY].ctrl = (bar1 & BAR_MASK) + 0x3F6 * (!bar1);
        channels[ATA_SECONDARY].base = (bar2 & BAR_MASK) + 0x170 * (!bar2);
        channels[ATA_SECONDARY].ctrl = (bar3 & BAR_MASK) + 0x376 * (!bar3);
        channels[ATA_PRIMARY].bmide = (bar4 & BAR_MASK) + 0; // Bus Master IDE
        channels[ATA_SECONDARY].bmide = (bar4 & BAR_MASK) + 8; // Bus Master IDE

        // Disable IRQs
        ide_write(ATA_PRIMARY, ATA_REG_CONTROL, 2);
        ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);

        for (uint8_t i = 0; i < 2; i++) {
            for (j = 0; j < 2; j++) {
                uint8_t err = 0, type = IDE_ATA, status;

                ide_devices[count].reserved = 0;

                // Select drive
                ide_write(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4)); // Select drive
                timer::sleep_ms(1);

                // Send ATA identify command
                ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
                timer::sleep_ms(1);

                if (ide_read(i, ATA_REG_STATUS) == 0) continue;

                while (true) {
                    status = ide_read(i, ATA_REG_STATUS);

                    // If error, device is not ATA
                    if (status & ATA_SR_ERR) {
                        err = 1;
                        break;
                    }

                    if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break; // No error
                }

                if (err != 0) {
                    const uint8_t cl = ide_read(i, ATA_REG_LBA1);
                    const uint8_t ch = ide_read(i, ATA_REG_LBA2);

                    if (cl == 0x14 && ch == 0xEB) {
                        type = IDE_ATAPI;
                    } else if (cl == 0x69 && ch == 0x96) {
                        type = IDE_ATAPI;
                    } else {
                        continue; // Unknown Type (may not be a device).
                    }

                    ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
                    timer::sleep(1);
                }

                ide_read_buffer(i, ATA_REG_DATA, ide_buf, 128);

                ide_devices[count].reserved = 1;
                ide_devices[count].type = type;
                ide_devices[count].channel = i;
                ide_devices[count].drive = j;
                ide_devices[count].signature = *reinterpret_cast<uint16_t*>(ide_buf + ATA_IDENT_DEVICETYPE);
                ide_devices[count].capabilities = *reinterpret_cast<uint16_t*>(ide_buf + ATA_IDENT_CAPABILITIES);
                ide_devices[count].command_sets = *reinterpret_cast<uint32_t*>(ide_buf + ATA_IDENT_COMMANDSETS);

                // (VII) Get Size:
                if (ide_devices[count].command_sets & (1 << 26)) {
                    // Device uses 48-Bit Addressing:
                    ide_devices[count].size = *reinterpret_cast<uint32_t*>(ide_buf + ATA_IDENT_MAX_LBA_EXT);
                } else {
                    // Device uses CHS or 28-bit Addressing:
                    ide_devices[count].size = *reinterpret_cast<uint32_t*>(ide_buf + ATA_IDENT_MAX_LBA);
                }

                // (VIII) String indicates model of device (like Western Digital HDD and SONY DVD-RW...):
                for (k = 0; k < 40; k += 2) {
                    ide_devices[count].model[k] = ide_buf[ATA_IDENT_MODEL + k + 1];
                    ide_devices[count].model[k + 1] = ide_buf[ATA_IDENT_MODEL + k];
                }
                ide_devices[count].model[40] = 0; // Terminate String.

                count++;
            }

            for (uint32_t i = 0; i < 4; i++) {
                if (ide_devices[i].reserved == 1) {
                    logger.debug(" Found %s Drive %dGB - %p\n",
                                 (const char*[]){"ATA", "ATAPI"}[ide_devices[i].type], /* Type */
                                 ide_devices[i].size / 1024 / 1024 / 2, /* Size */
                                 ide_devices[i].model);
                }
            }
        }
    }

    void init() {
        for (uint8_t i = 0; i < pci::pci_device_count; ++i) {
            const pci::PciDevice* device = &pci::pci_devices[i];

            if (device->class_code.class_code == 0x01 && device->subclass_code.subclass_code == 0x01) {
                logger.debug("Found IDE controller: %s, %s, device ID: %04X", device->vendor.name,
                             device->class_code.name, device->device_id);

                const uint32_t bar0 = pci::read_conf32(device->bus, device->slot, device->function, 0x10);
                const uint32_t bar1 = pci::read_conf32(device->bus, device->slot, device->function, 0x14);
                const uint32_t bar2 = pci::read_conf32(device->bus, device->slot, device->function, 0x18);
                const uint32_t bar3 = pci::read_conf32(device->bus, device->slot, device->function, 0x1C);
                const uint32_t bar4 = pci::read_conf32(device->bus, device->slot, device->function, 0x20);

                ide_initialize(bar0, bar1, bar2, bar3, bar4);
            }
        }
    }

    void ide_wait_irq() {
        while (!ide_irq_invoked) {
        }
        ide_irq_invoked = 0;
    }

    void irq() {
        ide_irq_invoked = 1;
    }
}
