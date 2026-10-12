#!/bin/sh
set -e

while [ $# -gt 0 ]; do
    case "$1" in
        --img) QUARK_IMG=$2; shift 2 ;;
        *) echo "unknown arg: $1" >&2; exit 1 ;;
    esac
done

dd if=/dev/zero of="$QUARK_IMG" bs=1M count=512 status=none
mformat -i "$QUARK_IMG" -F ::