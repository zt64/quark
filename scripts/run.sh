#!/bin/bash
set -e

debug=0
gdb=0

while test $# != 0
do
    case "$1" in
    -d|--debug) debug=1 ;;
    --gdb) gdb=1 ;;
    --) shift; break;;
    *)  break ;;
    esac
    shift
done

cd "$(dirname "$0")/.."

PRESET="${PRESET:-debug}"
BUILD_DIR="${BUILD_DIR:-build/$PRESET}"

if [ "$debug" -eq 1 ]; then
    QEMU_ARGS="-d int,cpu_reset $QEMU_ARGS"
fi

if [ "$gdb" -eq 1 ]; then
    QEMU_ARGS="-S -gdb tcp:127.0.0.1:1234 $QEMU_ARGS"
fi

cmake --preset "$PRESET"
cmake --build --preset "$PRESET" --target quark_iso

exec qemu-system-x86_64 \
    -cdrom "$BUILD_DIR/quark.iso" \
    -net none \
    -serial mon:stdio \
    -m 4G \
    -audiodev pa,id=speaker \
    -machine pcspk-audiodev=speaker \
    -no-reboot \
    -no-shutdown \
    -smbios type=0,vendor=hog_vendor,version=1.2.3 \
    -smbios type=3,version=te \
    $QEMU_ARGS
