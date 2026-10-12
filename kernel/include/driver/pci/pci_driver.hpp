#pragma once

/** Base class for all PCI drivers */
class PciDriver {
    /**
     * Initialize the PCI driver. This function is called by the PCI subsystem when the driver is registered.
     * It should perform any necessary initialization, such as allocating resources or registering device handlers.
     */
    void init();
};