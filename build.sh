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

fail() { printf '%s\n' "WHP-Drivers: $*" >&2; exit 1; }

validate() {
    case "$profile" in abi|vxd|radeon6410d|rtl8111e|all) ;; *) fail "invalid profile: $profile" ;; esac
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
    printf '  Workflow:        %s\n' "$profile"
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

menuconfig() {
    while :; do
        printf '\n%s\n' '=== WHP-Drivers menuconfig ==='
        show_config
        printf '%s\n' '  1) Cycle workflow (abi -> vxd -> radeon6410d -> rtl8111e -> all)'
        printf '%s\n' '  2) Toggle VxD backend (llvm / openwatcom)'
        printf '%s\n' '  3) Cycle toolchain bootstrap (off / llvm / openwatcom / both)'
        printf '%s\n' '  4) Set LLVM build jobs (1-16)'
        printf '%s\n' '  s) Save and exit  q) Exit without saving'
        printf '> '
        IFS= read -r choice || { printf '\n' >&2; fail 'menu closed without saving'; }
        case "$choice" in
            1) case "$profile" in abi) profile=vxd ;; vxd) profile=radeon6410d ;;
                  radeon6410d) profile=rtl8111e ;; rtl8111e) profile=all ;; all) profile=abi ;; esac ;;
            2) if [ "$vxd_backend" = llvm ]; then vxd_backend=openwatcom; else vxd_backend=llvm; fi ;;
            3) case "$bootstrap" in off) bootstrap=llvm ;; llvm) bootstrap=openwatcom ;;
                  openwatcom) bootstrap=both ;; both) bootstrap=off ;; esac ;;
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

run_vxd() {
    if [ "$vxd_backend" = openwatcom ]; then "$backend" watcom-smoke-vxd
    else "$backend" smoke-vxd; fi
}
run_bootstrap() {
    case "$bootstrap" in
        off) ;;
        llvm) "$backend" bootstrap ;;
        openwatcom) "$backend" bootstrap-watcom ;;
        both) "$backend" bootstrap
              "$backend" bootstrap-watcom ;;
    esac
}
run_profile() {
    run_bootstrap
    case "$profile" in
        abi) "$backend" smoke ;;
        vxd) run_vxd ;;
        radeon6410d) "$backend" smoke-radeon6410d ;;
        rtl8111e) "$backend" smoke-rtl8111e ;;
        all) "$backend" smoke
             "$backend" test-format
             "$backend" test-le
             run_vxd
             "$backend" smoke-radeon6410d
             "$backend" smoke-rtl8111e ;;
    esac
}
usage() {
    cat <<'HELP'
Usage: ./build.sh [menuconfig|config|build|test|bootstrap|bootstrap-watcom|bootstrap-all|<legacy target>]
  menuconfig      Configure build profile using portable interactive menu
  config          Show active settings (environment overrides .whpconfig)
  build           Run selected compile/smoke workflow; no driver installation
  test            Run format/LE tests, then selected workflow
  bootstrap       Explicit LLVM bootstrap (existing backend)
  bootstrap-watcom Explicit native OpenWatcom host + release toolchain bootstrap
  bootstrap-all   Build both pinned LLVM and OpenWatcom toolchains
  test-menuconfig Run isolated build.sh configuration regression checks
  <legacy target> Pass through to ./build (smoke, smoke-vxd, verify-vxd, ...)
Profiles: abi (default), vxd, radeon6410d, rtl8111e, all
VxD backends: llvm (default), openwatcom (explicit and LE-verified only)
Bootstrap choices: off, llvm, openwatcom, both (legacy on = llvm)
Defaults: bootstrap OFF; LLVM build jobs 2 (max configured 16)
Config: .whpconfig (data only, ignored by Git)
Overrides: WHP_DRIVER_PROFILE, WHP_DRIVER_VXD_BACKEND,
           WHP_DRIVER_BOOTSTRAP, WHP_LLVM_JOBS, WHP_WATCOM_CLANG
HELP
}

case "${1:-build}" in
    -h|--help|help) usage; exit 0 ;;
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
    build) [ "$#" -eq 1 ] || fail 'build takes no arguments'; run_profile ;;
    test) [ "$#" -eq 1 ] || fail 'test takes no arguments';
          "$backend" inf-check
          if [ "$profile" != all ]; then
              "$backend" test-format
              "$backend" test-le
          fi
          run_profile ;;
    *) [ -x "$backend" ] || fail 'legacy ./build backend missing'
       exec "$backend" "$@" ;;
esac
