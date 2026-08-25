#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"

CONFIG="${1:-debug}"

case "${CONFIG}" in
    debug|release)
        ;;
    *)
        echo "Usage: $0 [debug|release]" >&2
        exit 1
        ;;
esac

BUILD_DIR="${ROOT_DIR}/build/${CONFIG}"
SYSROOT="${HOME}/opt/quark-sysroot"

ISO_DIR="${BUILD_DIR}/iso"
ISO_OUTPUT="${BUILD_DIR}/quark.iso"
QUARK_IMG="${BUILD_DIR}/quark.img"

LIMINE_DIR="${ROOT_DIR}/external/limine"

#
# Required tools
#
command -v xorriso >/dev/null || {
    echo "error: xorriso not found" >&2
    exit 1
}

command -v dd >/dev/null || {
    echo "error: dd not found" >&2
    exit 1
}

command -v mformat >/dev/null || {
    echo "error: mformat not found" >&2
    exit 1
}

command -v mcopy >/dev/null || {
    echo "error: mcopy not found" >&2
    exit 1
}

#
# Install build artifacts
#
echo "Installing ${CONFIG} build..."

cmake \
    --install "${BUILD_DIR}" \
    --config "${CONFIG^}"

#
# Create FAT32 image
#
echo "Creating FAT32 disk image..."

rm -f "${QUARK_IMG}"

dd \
    if=/dev/zero \
    of="${QUARK_IMG}" \
    bs=1M \
    count=64 \
    status=none

mformat \
    -i "${QUARK_IMG}" \
    -F \
    ::

mcopy \
    -i "${QUARK_IMG}" \
    -o \
    -s \
    "${SYSROOT}/boot" \
    ::/

#
# Prepare ISO tree
#
echo "Preparing ISO..."

rm -rf "${ISO_DIR}"

mkdir -p \
    "${ISO_DIR}/boot/limine" \
    "${ISO_DIR}/EFI/BOOT"

cp \
    "${SYSROOT}/boot/quark" \
    "${ISO_DIR}/boot/quark"

cp \
    "${SYSROOT}/boot/init" \
    "${ISO_DIR}/boot/init.elf"

cp \
    "${SYSROOT}/boot/snell" \
    "${ISO_DIR}/boot/snell.elf"

cp \
    "${QUARK_IMG}" \
    "${ISO_DIR}/quark.img"

#
# Limine
#
cp \
    "${ROOT_DIR}/limine.conf" \
    "${ISO_DIR}/boot/limine/limine.conf"

cp \
    "${LIMINE_DIR}/limine-bios.sys" \
    "${LIMINE_DIR}/limine-bios-cd.bin" \
    "${LIMINE_DIR}/limine-uefi-cd.bin" \
    "${ISO_DIR}/boot/limine/"

cp \
    "${LIMINE_DIR}/BOOTX64.EFI" \
    "${ISO_DIR}/EFI/BOOT/"

cp \
    "${LIMINE_DIR}/BOOTIA32.EFI" \
    "${ISO_DIR}/EFI/BOOT/"

#
# Create ISO
#
echo "Creating ISO..."

xorriso \
    -as mkisofs \
    -R \
    -r \
    -J \
    -b boot/limine/limine-bios-cd.bin \
    -no-emul-boot \
    -boot-load-size 4 \
    -boot-info-table \
    -hfsplus \
    -apm-block-size 2048 \
    --efi-boot boot/limine/limine-uefi-cd.bin \
    -efi-boot-part \
    --efi-boot-image \
    --protective-msdos-label \
    "${ISO_DIR}" \
    -o "${ISO_OUTPUT}"

#
# Install Limine BIOS stage
#
"${LIMINE_DIR}/limine" bios-install "${ISO_OUTPUT}"

echo
echo "Build complete:"
echo "  Configuration: ${CONFIG}"
echo "  Build:         ${BUILD_DIR}"
echo "  ISO:            ${ISO_OUTPUT}"
echo "  Disk image:     ${QUARK_IMG}"