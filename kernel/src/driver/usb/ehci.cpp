#include <cstdint>

// Extended Host Controller Interface (EHCI) driver

namespace usb::ehci {
    constexpr uint8_t CAPLENGTH = 0x00;
    constexpr uint8_t RESERVED01 = 0x01;
    constexpr uint8_t HCI_VERSION = 0x02;
    constexpr uint8_t HCS_PARAMS = 0x04;
    constexpr uint8_t HCC_PARAMS = 0x08;
    constexpr uint8_t HCSP_PORTROUTE = 0x0C;

    constexpr uint8_t USBCMD = 0x00;
    constexpr uint8_t USBSTS = 0x04;
    constexpr uint8_t USBINTR = 0x08;
    constexpr uint8_t FRINDEX = 0x0C;
    constexpr uint8_t CTRLDSSEGMENT = 0x10;
    constexpr uint8_t PERIODICLISTBASE = 0x14;
    constexpr uint8_t ASYNCLISTADDR = 0x18;
    constexpr uint8_t CONFIGFLAG = 0x40;
    constexpr uint8_t PORTSC = 0x44;

    struct usbcmdreg {
        uint8_t reserved1 : 8;
        uint8_t interrupt_threshold : 8;
        uint8_t reserved0 : 4;
        bool async_schedule_park_mode_enable : 1;
        bool reserved : 1;
        uint8_t async_schedule_park_mode_count : 2;
        bool light_host_controller_reset : 1;
        bool interrupt_on_async_advance : 1;
        bool async_schedule_enable : 1;
        bool periodic_schedule_enable : 1;
        uint8_t frame_list_size : 2;
        bool host_controller_reset : 1;
        bool run : 1;
    };

    struct usb_statusreg {
        uint16_t usb_transfer_interrupt : 1;
        uint16_t usb_error_interrupt : 1;
        uint16_t port_change_detect : 1;
        uint16_t frame_list_rollover : 1;
        uint16_t host_system_error : 1;
        uint16_t doorbell_interrupt : 1;
        uint16_t reserved0 : 6;
        bool halted : 1;
        bool reclamation : 1;
        bool periodic_schedule_status : 1;
        bool async_schedule_status : 1;
        uint16_t reserved1 : 16;
    };

    struct endpoint_capability {
        uint8_t interrupt_sched_mask : 8;
        uint8_t split_completion_mask : 8;
        uint8_t hub_addr: 7;
        uint8_t port_num: 7;
        uint8_t hb_pipe_mul : 2;
    } __attribute__((packed));

    constexpr uint8_t CONTROLLER_CLASS_ID = 0x0C;
    constexpr uint8_t CONTROLLER_SUBCLASS_ID = 0x03;
    constexpr uint8_t CONTROLLER_PROG_IF_ID = 0x20;

    void init() {
        // TODO: Find EHCI controller and initialize it
    }
}
