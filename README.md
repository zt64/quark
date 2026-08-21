[//]: # (Logo here soon:tm:)

# quark

### What is quark?

Quark is a unix inspired x86_64 bit hobby operating system. I originally began this whole project in interest of creating 
Tetris but in the form of an operating system. For archival purposes, I've moved quark into its own repository. My tetris 
OS is at [zt64/tetros](https://github.com/zt64/tetros) I've massively expanded from that, turning it into a more general 
purpose OS. The ultimate goal is to have an operating system capable of running on real hardware with a full desktop environment.

### Features

* 64 bit support
* libc implementation via [mlibc](https://github.com/managarm/mlibc)

### Building

Prerequisites:
* git
* CMake (>=v3.20)
* texinfo (>=v7.2)
* mtools (>=v4.0.x) for building the filesystem
* xz
* xorriso
* Ninja
* Meson (>=v1.11.x) for building mlibc

---

##### Build the toolchain:

These steps will change in the future to make it less prone to user error (me)

1. Open a shell in the [toolchain](toolchain) directory
2. Run `cmake -S . -B build`
3. Run `cmake --build build --target binutils_build` 

   This needs to be run twice for some reason, I will fix this eventually.
4. `cmake --build build --target mlibc_headers`
5. `cmake --build build --target gcc_bootstrap`
6. `cmake --build build --target mlibc_build`
7. `cmake --build build --target gcc_final`

##### Build the filesystem:

`cmake --build build --target quark_img`

##### Build the ISO:

iso depends on quark_img target, so it will be built if necessary

`cmake --build build --target iso`

### Running

#### Qemu

`./scripts/run.sh`

#### Bare metal

Currently I'm unable to test on my laptop until I get IO APIC implemented.

### Roadmap

#### Kernel

- [X] Physical memory manager
- [X] Virtual memory
- [X] Kernel heap
- [X] GDT/IDT
- [X] Exception handling
- [ ] APIC
- [ ] IOAPIC
- [ ] SMP
- [ ] Scheduler
- [ ] Processes
- [ ] Threads
- [ ] Syscalls

#### Drivers

- [ ] PCI
- [X] PS/2 keyboard
- [X] PS/2 mouse
- [X] Serial
- [ ] AHCI
- [ ] XHCI
- [ ] Networking
- [ ] Sound

#### Userspace

- [X] ELF loader
- [X] init
- [ ] Shell
- [ ] Core utilities
- [X] mlibc integration
- [ ] Dynamic linking

#### Filesystems

- [ ] VFS
- [ ] Initramfs
- [X] FAT32

#### Graphics

- [X] Framebuffer
- [ ] Input system
- [ ] Window manager
- [ ] Compositor
- [ ] Desktop environment

### Acknowledgements

* [OSDev.wiki](https://wiki.osdev.org) - Great resource for learning and how to get started
* [Limine](https://github.com/limine-bootloader/limine) - Minimal and easy to use 
* [mlibc](https://github.com/managarm/mlibc) - Minimal C library for hobby operating systems