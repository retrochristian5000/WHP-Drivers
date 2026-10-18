#!/bin/sh
# Isolated build.sh frontend test: no LLVM bootstrap, network, or VxD build.
set -eu
root=$(CDPATH= cd "$(dirname "$0")/.." && pwd)
tmp=$(mktemp -d "${TMPDIR:-/tmp}/whp-drivers-menu.XXXXXX") || exit 1
trap 'rm -rf "$tmp"' 0
trap 'exit 1' 1 2 3 15
cp "$root/build.sh" "$tmp/build.sh"
cat > "$tmp/build" <<'MOCK'
#!/bin/sh
printf '%s\n' "$*" >> "$WHP_TEST_LOG"
MOCK
chmod +x "$tmp/build" "$tmp/build.sh"
export WHP_TEST_LOG="$tmp/log"
: > "$WHP_TEST_LOG"

"$tmp/build.sh" build
printf 'smoke\n' > "$tmp/expected"
cmp "$tmp/expected" "$WHP_TEST_LOG" >/dev/null || exit 1
printf '1\n2\ns\n' | "$tmp/build.sh" menuconfig >/dev/null
grep -qx 'WHP_DRIVER_PROFILE=vxd' "$tmp/.whpconfig"
grep -qx 'WHP_DRIVER_VXD_BACKEND=openwatcom' "$tmp/.whpconfig"
"$tmp/build.sh" build
printf 'smoke\nwatcom-smoke-vxd\n' > "$tmp/expected"
cmp "$tmp/expected" "$WHP_TEST_LOG" >/dev/null || exit 1
"$tmp/build.sh" test
printf 'smoke\nwatcom-smoke-vxd\ntest-format\ntest-le\nwatcom-smoke-vxd\n' > "$tmp/expected"
cmp "$tmp/expected" "$WHP_TEST_LOG" >/dev/null || exit 1
WHP_DRIVER_PROFILE=radeon6410d "$tmp/build.sh" build
printf 'smoke\nwatcom-smoke-vxd\ntest-format\ntest-le\nwatcom-smoke-vxd\nsmoke-radeon6410d\n' > "$tmp/expected"
cmp "$tmp/expected" "$WHP_TEST_LOG" >/dev/null || exit 1

cp "$tmp/.whpconfig" "$tmp/expected-config"
printf '1\nq\n' | "$tmp/build.sh" menuconfig >/dev/null
cmp "$tmp/.whpconfig" "$tmp/expected-config" >/dev/null || exit 1
printf '4\n0\ns\n' | "$tmp/build.sh" menuconfig >/dev/null 2>&1
grep -qx 'WHP_DRIVER_JOBS=2' "$tmp/.whpconfig"

printf 'WHP_DRIVER_PROFILE=abi\nWHP_DRIVER_PROFILE=vxd\n' > "$tmp/.whpconfig"
if "$tmp/build.sh" config > /dev/null 2>&1; then
    printf '%s\n' 'duplicate config was accepted' >&2
    exit 1
fi
printf 'WHP_DRIVER_PROFILE=$(touch %s/injected)\n' "$tmp" > "$tmp/.whpconfig"
if "$tmp/build.sh" config > /dev/null 2>&1; then
    printf '%s\n' 'invalid config was accepted' >&2
    exit 1
fi
[ ! -e "$tmp/injected" ] || exit 1
# Independent toolchain bootstrap, with backwards-compatible "on" profile.
printf 'WHP_DRIVER_BOOTSTRAP=on\n' > "$tmp/.whpconfig"
"$tmp/build.sh" config | grep -q 'Tool bootstrap:  llvm'
: > "$WHP_TEST_LOG"
"$tmp/build.sh" build
printf 'bootstrap\nsmoke\n' > "$tmp/expected"
cmp "$tmp/expected" "$WHP_TEST_LOG" >/dev/null || exit 1

printf 'WHP_DRIVER_BOOTSTRAP=openwatcom\n' > "$tmp/.whpconfig"
: > "$WHP_TEST_LOG"
"$tmp/build.sh" build
printf 'bootstrap-watcom\nsmoke\n' > "$tmp/expected"
cmp "$tmp/expected" "$WHP_TEST_LOG" >/dev/null || exit 1

printf 'WHP_DRIVER_BOOTSTRAP=both\n' > "$tmp/.whpconfig"
: > "$WHP_TEST_LOG"
"$tmp/build.sh" build
printf 'bootstrap\nbootstrap-watcom\nsmoke\n' > "$tmp/expected"
cmp "$tmp/expected" "$WHP_TEST_LOG" >/dev/null || exit 1

: > "$WHP_TEST_LOG"
"$tmp/build.sh" bootstrap-all
printf 'bootstrap\nbootstrap-watcom\n' > "$tmp/expected"
cmp "$tmp/expected" "$WHP_TEST_LOG" >/dev/null || exit 1

printf 'WHP_DRIVER_BOOTSTRAP=off\n' > "$tmp/.whpconfig"
printf '3\n3\ns\n' | "$tmp/build.sh" menuconfig >/dev/null
grep -qx 'WHP_DRIVER_BOOTSTRAP=openwatcom' "$tmp/.whpconfig"

printf 'WHP_DRIVER_BOOTSTRAP=unrecognized\n' > "$tmp/.whpconfig"
if "$tmp/build.sh" config >/dev/null 2>&1; then
    printf '%s\n' 'invalid toolchain bootstrap option was accepted' >&2
    exit 1
fi

# New Win16/DDK header probes must remain opt-in pass-through targets.
printf 'WHP_DRIVER_BOOTSTRAP=off\n' > "$tmp/.whpconfig"
: > "$WHP_TEST_LOG"
"$tmp/build.sh" display-win16-check
"$tmp/build.sh" display-ddk-check
printf 'display-win16-check\ndisplay-ddk-check\n' > "$tmp/expected"
cmp "$tmp/expected" "$WHP_TEST_LOG" >/dev/null || exit 1

printf '%s\n' 'WHP-Drivers: build.sh menuconfig regression checks passed'
