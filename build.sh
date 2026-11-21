#!/bin/sh
# WHP-Drivers configuration front end. Legacy ./build remains the backend.
set -eu

root=$(CDPATH= cd "$(dirname "$0")" && pwd)
backend="$root/build"
config="$root/.whpconfig"
profile=abi
vxd_backend=llvm
bootstrap=off
jobs=2
plan_only=0

fail() { printf '%s\n' "WHP-Drivers: $*" >&2; exit 1; }

# Share accepted profile names between configuration and direct selection.
valid_profile() {
    case "$1" in
        abi|vxd|radeon6410d|rtl8111e|hda|brother-mfc290c|dos-probe|dos-pci|all)
            return 0 ;;
        *) return 1 ;;
    esac
}

show_profiles() {
    cat <<'PROFILES'
WHP-Drivers profiles:
  abi              Windows 95 ABI object smoke (default)
  vxd              Experimental VxD format/link probe
  radeon6410d      Radeon HD 6410D smoke
  rtl8111e         RTL8111E network smoke
  hda              Read-only HDA PCI/ABI smoke; not a sound driver
  brother-mfc290c  Offline USB tests; macOS native read-only observer
  dos-probe        16-bit WHPPROBE.EXE build (OpenWatcom required)
  all              Core ABI/VxD/Radeon/network/HDA smoke; excludes DOS and Brother
  dos-pci          Legacy name for DOS probe (existing configurations only)
PROFILES
}

validate() {
    valid_profile "$profile" || fail "invalid profile: $profile"
    case "$vxd_backend" in llvm|openwatcom) ;; *) fail "invalid VxD backend: $vxd_backend" ;; esac
    case "$bootstrap" in
        on) bootstrap=llvm ;; # backwards-compatible .whpconfig migration
        off|llvm|openwatcom|both) ;;
        *) fail "invalid bootstrap setting: $bootstrap" ;;
    esac
    case "$jobs" in ''|0|*[!0-9]*) fail "LLVM jobs must be 1-16" ;; esac
    [ "$jobs" -ge 1 ] && [ "$jobs" -le 16 ] || fail "LLVM jobs must be 1-16"
}

load_config() {
    [ -f "$config" ] || return 0
    seen=' '
    while IFS= read -r line || [ -n "$line" ]; do
        case "$line" in ''|'#'*) continue ;; *=*) ;; *) fail "malformed .whpconfig line" ;; esac
        key=${line%%=*}
        value=${line#*=}
        case "$seen" in *" $key "*) fail "duplicate .whpconfig key: $key" ;; esac
        case "$key" in
            WHP_DRIVER_PROFILE) profile=$value ;;
            WHP_DRIVER_VXD_BACKEND) vxd_backend=$value ;;
            WHP_DRIVER_BOOTSTRAP) bootstrap=$value ;;
            WHP_DRIVER_JOBS) jobs=$value ;;
            *) fail "unknown .whpconfig key: $key" ;;
        esac
        seen="$seen$key "
    done < "$config"
    validate
}

apply_env() {
    if [ "${WHP_DRIVER_PROFILE+x}" ]; then profile=$WHP_DRIVER_PROFILE; fi
    if [ "${WHP_DRIVER_VXD_BACKEND+x}" ]; then vxd_backend=$WHP_DRIVER_VXD_BACKEND; fi
    if [ "${WHP_DRIVER_BOOTSTRAP+x}" ]; then bootstrap=$WHP_DRIVER_BOOTSTRAP; fi
    if [ "${WHP_LLVM_JOBS+x}" ]; then jobs=$WHP_LLVM_JOBS; fi
    validate
    WHP_LLVM_JOBS=$jobs
    export WHP_LLVM_JOBS
}

show_config() {
    printf '%s\n' 'WHP-Drivers configuration'
    printf '  Profile:         %s\n' "$profile"
    printf '  VxD backend:     %s\n' "$vxd_backend"
    printf '  Tool bootstrap:  %s\n' "$bootstrap"
    printf '  LLVM jobs:       %s\n' "$jobs"
    printf '%s\n' '  Windows 95 RTM hardware loading: NOT ENABLED'
}

save_config() {
    tmp=$(mktemp "$config.tmp.XXXXXX") || fail 'cannot create temporary config'
    trap 'rm -f "$tmp"' 0
    trap 'exit 1' 1 2 3 15
    {
        printf '# WHP-Drivers menuconfig v1 (data only; never shell-source)\n'
        printf 'WHP_DRIVER_PROFILE=%s\n' "$profile"
        printf 'WHP_DRIVER_VXD_BACKEND=%s\n' "$vxd_backend"
        printf 'WHP_DRIVER_BOOTSTRAP=%s\n' "$bootstrap"
        printf 'WHP_DRIVER_JOBS=%s\n' "$jobs"
    } > "$tmp" || fail 'cannot write config'
    mv -f "$tmp" "$config" || fail 'cannot save config'
    trap - 0 1 2 3 15
    printf 'WHP-Drivers: saved %s\n' "$config"
}

choose_profile() {
    show_profiles
    printf 'Profile name: '
    IFS= read -r candidate || fail 'menu closed without saving'
    if valid_profile "$candidate"; then
        profile=$candidate
    else
        printf 'Unknown profile; unchanged: %s\n' "$candidate" >&2
    fi
}

choose_bootstrap() {
    printf '%s\n' 'Bootstrap choices: off / llvm / openwatcom / both'
    printf 'Bootstrap choice: '
    IFS= read -r candidate || fail 'menu closed without saving'
    case "$candidate" in
        off|llvm|openwatcom|both) bootstrap=$candidate ;;
        *) printf 'Unknown bootstrap choice; unchanged: %s\n' "$candidate" >&2 ;;
    esac
}

menuconfig() {
    while :; do
        printf '\n%s\n' '=== WHP-Drivers menuconfig ==='
        show_config
        printf '%s\n' '  Workflow:'
        printf '%s\n' '    1) Cycle profile (legacy control)    p) Choose profile by name'
        printf '%s\n' '  Toolchains:'
        printf '%s\n' '    2) Toggle VxD backend (llvm / openwatcom)'
        printf '%s\n' '    3) Cycle bootstrap (off / llvm / openwatcom / both)'
        printf '%s\n' '    b) Choose bootstrap by name'
        printf '%s\n' '    4) Set LLVM build jobs (1-16)'
        printf '%s\n' '  Actions:'
        printf '%s\n' '    s) Save and exit    q) Exit without saving'
        printf '> '
        IFS= read -r choice || { printf '\n' >&2; fail 'menu closed without saving'; }
        case "$choice" in
            1) case "$profile" in abi) profile=vxd ;; vxd) profile=radeon6410d ;;
                  radeon6410d) profile=rtl8111e ;; rtl8111e) profile=hda ;;
                  hda) profile=brother-mfc290c ;;
                  brother-mfc290c) profile=dos-probe ;;
                  dos-probe|dos-pci) profile=all ;; all) profile=abi ;; esac ;;
            p|P) choose_profile ;;
            2) if [ "$vxd_backend" = llvm ]; then vxd_backend=openwatcom; else vxd_backend=llvm; fi ;;
            3) case "$bootstrap" in off) bootstrap=llvm ;; llvm) bootstrap=openwatcom ;;
                  openwatcom) bootstrap=both ;; both) bootstrap=off ;; esac ;;
            b|B) choose_bootstrap ;;
            4) printf 'LLVM jobs [1-16]: '
               IFS= read -r candidate || fail 'menu closed without saving'
               old=$jobs; jobs=$candidate
               if ! (validate) 2>/dev/null; then
                   jobs=$old
                   printf '%s\n' 'Invalid job count; unchanged.' >&2
               fi ;;
            s|S) validate; save_config; return 0 ;;
            q|Q) printf '%s\n' 'WHP-Drivers: configuration unchanged'; return 0 ;;
            *) printf '%s\n' 'Unknown selection.' >&2 ;;
        esac
    done
}

# The printed plan and real execution share one dependency order.
# Never use eval or shell-source .whpconfig; each step has fixed argv.
step() {
    if [ "$plan_only" -eq 1 ]; then
        printf '  ./build'
        for arg do printf ' %s' "$arg"; done
        printf '\n'
    else
        "$backend" "$@"
    fi
}
run_vxd() {
    if [ "$vxd_backend" = openwatcom ]; then step watcom-smoke-vxd
    else step smoke-vxd; fi
}
run_bootstrap() {
    case "$bootstrap" in
        off) ;;
        llvm) step bootstrap ;;
        openwatcom) step bootstrap-watcom ;;
        both) step bootstrap
              step bootstrap-watcom ;;
    esac
}
run_profile() {
    run_bootstrap
    case "$profile" in
        abi) step smoke ;;
        vxd) run_vxd ;;
        radeon6410d) step smoke-radeon6410d ;;
        rtl8111e) step smoke-rtl8111e ;;
        hda) step smoke-hda ;;
        brother-mfc290c) step test-brother-mfc290c
                        if [ "$(uname -s)" = Darwin ]; then
                            step build-brother-mfc290c-macos
                        fi ;;
        dos-probe) step dos-probe-build ;;
        dos-pci) step dos-pci-build ;; # legacy saved workflow
        all) step smoke
             step test-format
             step test-le
             run_vxd
             step smoke-radeon6410d
             step smoke-rtl8111e
             step smoke-hda ;;
    esac
}

# Host-only DOS checks: no 16-bit compiler or real hardware access.
run_dos_host_tests() {
    step test-dos-version
    step test-dos-legacy-bios
    step test-fch-sata-identity
    step test-usb-pci-hci
    step test-hda-probe
    step test-pci95-bridge
    step test-amd12-pcie-topology
    step test-pci-dos-memory
    step test-pci95-bios-read
    step test-pci-config-registers
    step test-probe-menu
    step test-dos-memory-survey
    step test-rtl8111e-pci-snapshot
    step test-dos-mz
    step test-dos-packages
}

# Both fast and full network test plans use this NDIS regression group.
run_ndis31_tests() {
    step test-ndis31-audit
    step test-ndis31-libs
    step test-ndis31-adapter
    step test-ndis31-llvm
}

run_fast_tests() {
    if [ "$profile" = brother-mfc290c ]; then
        step test-brother-mfc290c
        return
    fi
    if [ "$profile" = hda ]; then
        step test-hda-probe
        return
    fi
    if [ "$profile" = dos-pci ] || [ "$profile" = dos-probe ]; then
        run_dos_host_tests
        return
    fi
    step test-format
    case "$profile" in radeon6410d|rtl8111e|all) step inf-check ;; esac
    case "$profile" in
        all) step test-hda-probe ;;
    esac
    case "$profile" in
        rtl8111e|all)
            step test-pci95-bridge
            step test-pci95-bios-read
            step test-rtl8111e-pci-snapshot
            step test-dos-mz
            run_ndis31_tests ;;
    esac
}
run_tests() {
    if [ "$profile" = brother-mfc290c ]; then
        run_profile
        return
    fi
    if [ "$profile" = hda ]; then
        run_profile
        return
    fi
    if [ "$profile" = dos-pci ] || [ "$profile" = dos-probe ]; then
        run_fast_tests
        run_profile
        return
    fi
    step inf-check
    if [ "$profile" != all ]; then
        step test-format
        step test-le
    fi
    case "$profile" in
        rtl8111e|all)
            step test-pci95-bios-read
            step test-dos-mz
            run_ndis31_tests ;;
    esac
    run_profile
}

# DOS outputs live separately; never remove the source, toolchain or arbitrary paths.
clean_dos_probe() {
    out="$root/.build/dos-pci"
    [ ! -L "$root/.build" ] && [ ! -L "$out" ] ||
        fail 'clean-dos-probe refuses symlinked output path'
    if [ -e "$out" ]; then
        [ -d "$out" ] || fail 'DOS output path is not a directory'
        rm -rf "$out" || fail 'cannot remove DOS build output'
    fi
    printf 'WHP-Drivers: removed generated DOS probe artifacts only\n'
}

# Deliberately do not delete LLVM/OpenWatcom toolchains or other artifacts.
# Reject custom LLVM build tree paths to avoid removing external data.
clean_tests() {
    [ -z "${WHP_LLVM_BUILD_DIR:-}" ] ||
        fail 'clean-tests refuses WHP_LLVM_BUILD_DIR override (manual cleanup required)'
    out="$root/.build/llvm-native/whp-tests"
    [ ! -L "$root/.build" ] && [ ! -L "$root/.build/llvm-native" ] &&
        [ ! -L "$out" ] || fail 'clean-tests refuses symlinked output path'
    if [ -e "$out" ]; then
        [ -d "$out" ] || fail 'test output path is not a directory'
        rm -rf "$out" || fail 'cannot remove test output'
    fi
    printf 'WHP-Drivers: removed generated test artifacts only\n'
}
usage() {
    cat <<'HELP'
Usage: ./build.sh [command]
       ./build.sh <existing ./build target> [target arguments...]

Discover and configure:
  help, -h, --help Show this help
  profiles         List build profiles and their boundaries
  menuconfig       Configure profile, VxD backend, bootstrap and LLVM jobs
  config           Show effective settings
  plan             Print configured build steps without running them

Build and test:
  build            Build selected profile; no driver installation
  incremental      Build selected profile without toolchain bootstrapping
  test-fast        Host-only profile regressions; no compiler bootstrap
  test             Full profile tests and build; may bootstrap if configured
  test-menuconfig  Isolated configuration/dispatch regression tests

Explicit toolchain bootstrap:
  bootstrap        Build pinned LLVM toolchain
  bootstrap-watcom Build pinned OpenWatcom toolchain
  bootstrap-all    Build LLVM, then OpenWatcom

Cleanup (generated outputs only):
  clean-tests      Remove default LLVM test outputs
  clean-dos-probe  Remove DOS probe outputs (legacy: clean-dos-pci)

Other existing backend targets:
  ./build.sh <target> [arguments] passes through to ./build unchanged.
  Examples: smoke-vxd, verify-vxd, build-brother-mfc290c-macos,
            probe-brother-mfc290c-macos, probe-system-macos.
  Native macOS Brother build slices: WHP_MFC290C_MACOS_ARCH=
    native / arm64 / x86_64 / universal2.

Configuration:
  .whpconfig is validated data, not sourced as shell code.
  WHP_DRIVER_PROFILE, WHP_DRIVER_VXD_BACKEND, WHP_DRIVER_BOOTSTRAP,
  WHP_LLVM_JOBS override saved settings. WHP_WATCOM_CLANG chooses
  a native compiler for explicit OpenWatcom bootstrap.
  Bootstrap defaults to off; LLVM jobs default to 2 (range 1-16).
  Legacy bootstrap=on maps to llvm. VxD backends: llvm / openwatcom.

Profiles are listed with './build.sh profiles'.
  'all' excludes the opt-in DOS and Brother device workflows.
  DOS builds are 16-bit OpenWatcom MZ; no DOS runtime/hardware tests.
  VxD LE structure checks do not establish Windows 95 loading.
HELP
}

case "${1:-build}" in
    -h|--help|help) usage; exit 0 ;;
    profiles) [ "$#" -eq 1 ] || fail 'profiles takes no arguments'
              show_profiles; exit 0 ;;
    test-menuconfig) [ "$#" -eq 1 ] || fail "test-menuconfig takes no arguments"
                     exec sh "$root/tests/build-menuconfig.sh" ;;
esac
load_config
apply_env
case "${1:-build}" in
    bootstrap-all) [ "$#" -eq 1 ] || fail 'bootstrap-all takes no arguments';
                   "$backend" bootstrap
                   "$backend" bootstrap-watcom ;;
    menuconfig) [ "$#" -eq 1 ] || fail 'menuconfig takes no arguments'; menuconfig ;;
    config|showconfig) [ "$#" -eq 1 ] || fail 'config takes no arguments'; show_config ;;
    plan) [ "$#" -eq 1 ] || fail 'plan takes no arguments'
          plan_only=1; printf 'WHP-Drivers: selected build plan (%s)\n' "$profile"
          run_profile ;;
    build) [ "$#" -eq 1 ] || fail 'build takes no arguments'; run_profile ;;
    incremental) [ "$#" -eq 1 ] || fail 'incremental takes no arguments'
                 bootstrap=off; run_profile ;;
    test-fast) [ "$#" -eq 1 ] || fail 'test-fast takes no arguments'
               run_fast_tests ;;
    test) [ "$#" -eq 1 ] || fail 'test takes no arguments'; run_tests ;;
    clean-tests) [ "$#" -eq 1 ] || fail 'clean-tests takes no arguments'
                 clean_tests ;;
    clean-dos-probe|clean-dos-pci) [ "$#" -eq 1 ] || fail 'DOS probe cleanup takes no arguments'
                   clean_dos_probe ;;
    *) [ -x "$backend" ] || fail 'legacy ./build backend missing'
       exec "$backend" "$@" ;;
esac
