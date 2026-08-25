#!/bin/bash
set -e

cd "$(dirname "$0")/.."

SYSROOT="${SYSROOT:-${HOME}/opt/quark-sysroot}"
PRESET="${PRESET:-debug}"
BUILD_DIR="${BUILD_DIR:-build/$PRESET}"

cmake --preset "$PRESET"
cmake --build --preset "$PRESET" --target quark_iso
exec gdb "$SYSROOT/boot/quark" -ex "target remote localhost:1234"
