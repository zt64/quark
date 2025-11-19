#!/bin/bash
set -euo pipefail

readonly project_dir="$(cd "$(dirname "$0")/.." && pwd)"

resolve_build_dir() {
    local candidate

    if [[ -n "${BUILD_DIR:-}" ]]; then
        if [[ "${BUILD_DIR}" == /* ]]; then
            candidate="${BUILD_DIR}"
        else
            candidate="${project_dir}/${BUILD_DIR}"
        fi

        if [[ -f "${candidate}/quark.iso" ]]; then
            printf '%s\n' "${candidate}"
            return 0
        fi
    fi

    local config_name="${PRESET:-${CONFIG_NAME:-${CMAKE_BUILD_TYPE:-}}}"
    if [[ -n "${config_name}" ]]; then
        config_name="${config_name,,}"

        for candidate in \
            "${project_dir}/build/${config_name}" \
            "${project_dir}/cmake-build-${config_name}"
        do
            if [[ -f "${candidate}/quark.iso" ]]; then
                printf '%s\n' "${candidate}"
                return 0
            fi
        done
    fi

    local candidates=(
        "${project_dir}/build/release"
        "${project_dir}/cmake-build-release"
        "${project_dir}/build/debug"
        "${project_dir}/cmake-build-debug"
        "${project_dir}/build"
    )

    local newest_candidate=""
    local newest_mtime=0
    local mtime
    for candidate in "${candidates[@]}"; do
        if [[ -f "${candidate}/quark.iso" ]]; then
            mtime="$(stat -c '%Y' "${candidate}/quark.iso")"
            if (( mtime > newest_mtime )); then
                newest_mtime="${mtime}"
                newest_candidate="${candidate}"
            fi
        fi
    done

    if [[ -n "${newest_candidate}" ]]; then
        printf '%s\n' "${newest_candidate}"
        return 0
    fi

    return 1
}

if ! build_dir="$(resolve_build_dir)"; then
    echo "Boot ISO not found. Build the quark target first." >&2
    exit 1
fi

exec qemu-system-x86_64 \
    -cdrom "${build_dir}/quark.iso" \
    -net none \
    -serial mon:stdio \
    -m 4G \
    -audiodev pa,id=speaker \
    -machine pcspk-audiodev=speaker \
    -no-reboot \
    -no-shutdown \
    -smbios type=0,vendor=hog_vendor,version=1.2.3 \
    -smbios type=3,version=te
