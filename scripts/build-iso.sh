#!/bin/sh
set -e
while [ $# -gt 0 ]; do
    case "$1" in
        --iso-dir) ISO_DIR=$2; shift 2 ;;
        --sysroot) QUARK_SYSROOT=$2; shift 2 ;;
        --img) QUARK_IMG=$2; shift 2 ;;
        --limine-src) LIMINE_SRC_DIR=$2; shift 2 ;;
        --out) ISO_OUTPUT=$2; shift 2 ;;
        --limine-conf) LIMINE_CONF=$2; shift 2 ;;
        *) echo "unknown arg: $1" >&2; exit 1 ;;
    esac
done
LIMINE_DEPLOY="$LIMINE_SRC_DIR/limine"
rm -rf "$ISO_DIR"
mkdir -p "$ISO_DIR/boot/limine" "$ISO_DIR/EFI/BOOT"
cp "$QUARK_SYSROOT/boot/quark" "$ISO_DIR/boot/quark"
for d in "$QUARK_SYSROOT"/*; do
    mcopy -i "$QUARK_IMG" -o -s "$d" ::/
done
cp "$QUARK_IMG" "$ISO_DIR/quark.img"
cp "$LIMINE_CONF" "$ISO_DIR/boot/limine/limine.conf"
cp "$LIMINE_SRC_DIR/limine-bios.sys" "$ISO_DIR/boot/limine/limine-bios.sys"
cp "$LIMINE_SRC_DIR/limine-bios-cd.bin" "$ISO_DIR/boot/limine/limine-bios-cd.bin"
cp "$LIMINE_SRC_DIR/limine-uefi-cd.bin" "$ISO_DIR/boot/limine/limine-uefi-cd.bin"
cp "$LIMINE_SRC_DIR/BOOTX64.EFI" "$ISO_DIR/EFI/BOOT/BOOTX64.EFI"
cp "$LIMINE_SRC_DIR/BOOTIA32.EFI" "$ISO_DIR/EFI/BOOT/BOOTIA32.EFI"
xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
    -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
    -apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
    -efi-boot-part --efi-boot-image --protective-msdos-label \
    "$ISO_DIR" -o "$ISO_OUTPUT"
"$LIMINE_DEPLOY" bios-install "$ISO_OUTPUT"