#!/usr/bin/env bash
set -Eeuo pipefail

debug=0
gdb=0
build=1

while (($#)); do
    case "$1" in
        -d|--debug)
            debug=1
            ;;
        --gdb)
            gdb=1
            ;;
        --no-build)
            build=0
            ;;
        -h|--help)
            cat <<EOF
Usage: $(basename "$0") [options] [-- <additional qemu args>]

Options:
  -d, --debug     Enable QEMU debug logging
      --gdb       Wait for GDB on port 1234
      --no-build  Skip rebuilding the ISO
  -h, --help      Show this help

Environment variables:
  PRESET      CMake preset (default: debug)
  BUILD_DIR   Build directory (default: build/\$PRESET)
  MEMORY      Guest memory (default: 4G)

Examples:
  ./run.sh
  ./run.sh --gdb
  MEMORY=8G ./run.sh
  ./run.sh -- -d guest_errors
EOF
            exit 0
            ;;
        --)
            shift
            break
            ;;
        *)
            break
            ;;
    esac
    shift
done

EXTRA_QEMU_ARGS=("$@")

cd "$(dirname "$0")/.."

PRESET="${PRESET:-debug}"
BUILD_DIR="${BUILD_DIR:-build/$PRESET}"
MEMORY="${MEMORY:-4G}"

if ((build)); then
    cmake --preset "$PRESET"
    cmake --build --preset "$PRESET" --target quark_iso
fi

QEMU_ARGS=(
    -cdrom "$BUILD_DIR/quark.iso"

    -machine q35,pcspk-audiodev=speaker
    -cpu max
    -smp 4
    -m "$MEMORY"

    -rtc base=utc,clock=host

    -serial mon:stdio
    -net none

    -boot menu=on,splash-time=0

    -audiodev pa,id=speaker

#    -device isa-debug-exit,iobase=0xf4,iosize=4

    -no-reboot
    -no-shutdown

    -smbios type=0,vendor=hog_vendor,version=1.2.3
    -smbios type=3,version=1.2.3
)

if [[ -e /dev/kvm ]] && ((debug == 0)); then
    QEMU_ARGS+=(-enable-kvm)
fi

if ((debug)); then
    QEMU_ARGS+=(-d int,cpu_reset)
fi

if ((gdb)); then
    QEMU_ARGS+=(-S -gdb tcp:127.0.0.1:1234)
fi

exec qemu-system-x86_64 \
    "${QEMU_ARGS[@]}" \
    "${EXTRA_QEMU_ARGS[@]}"