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
printf 'smoke\nwatcom-smoke-vxd\ninf-check\ntest-format\ntest-le\nwatcom-smoke-vxd\n' > "$tmp/expected"
cmp "$tmp/expected" "$WHP_TEST_LOG" >/dev/null || exit 1
WHP_DRIVER_PROFILE=radeon6410d "$tmp/build.sh" build
printf 'smoke\nwatcom-smoke-vxd\ninf-check\ntest-format\ntest-le\nwatcom-smoke-vxd\nsmoke-radeon6410d\n' > "$tmp/expected"
cmp "$tmp/expected" "$WHP_TEST_LOG" >/dev/null || exit 1

# New profile must use only the network smoke; all now includes network.
: > "$WHP_TEST_LOG"
WHP_DRIVER_PROFILE=rtl8111e "$tmp/build.sh" build
printf 'smoke-rtl8111e\n' > "$tmp/expected"
cmp "$tmp/expected" "$WHP_TEST_LOG" >/dev/null || exit 1
: > "$WHP_TEST_LOG"
WHP_DRIVER_PROFILE=all "$tmp/build.sh" build
printf 'smoke\ntest-format\ntest-le\nwatcom-smoke-vxd\nsmoke-radeon6410d\nsmoke-rtl8111e\n' > "$tmp/expected"
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
"$tmp/build.sh" minivdd-check
"$tmp/build.sh" inf-check
printf 'display-win16-check\ndisplay-ddk-check\nminivdd-check\ninf-check\n' > "$tmp/expected"
cmp "$tmp/expected" "$WHP_TEST_LOG" >/dev/null || exit 1

# Comparable QEMU/Water workflow commands are intentionally host-only
# dispatch: the fake backend must see exact order and NO duplicate tests.
: > "$WHP_TEST_LOG"
WHP_DRIVER_PROFILE=rtl8111e "$tmp/build.sh" test-fast
printf 'test-format\ntest-ndis31-audit\ntest-ndis31-libs\ntest-ndis31-adapter\n' > "$tmp/expected"
cmp "$tmp/expected" "$WHP_TEST_LOG" >/dev/null || exit 1
: > "$WHP_TEST_LOG"
WHP_DRIVER_PROFILE=rtl8111e "$tmp/build.sh" test
printf 'inf-check\ntest-format\ntest-le\ntest-ndis31-audit\ntest-ndis31-libs\ntest-ndis31-adapter\nsmoke-rtl8111e\n' > "$tmp/expected"
cmp "$tmp/expected" "$WHP_TEST_LOG" >/dev/null || exit 1
: > "$WHP_TEST_LOG"
WHP_DRIVER_PROFILE=radeon6410d "$tmp/build.sh" test-fast
printf 'test-format\ninf-check\n' > "$tmp/expected"
cmp "$tmp/expected" "$WHP_TEST_LOG" >/dev/null || exit 1
: > "$WHP_TEST_LOG"
WHP_DRIVER_PROFILE=all "$tmp/build.sh" test
printf 'inf-check\ntest-ndis31-audit\ntest-ndis31-libs\ntest-ndis31-adapter\nsmoke\ntest-format\ntest-le\nsmoke-vxd\nsmoke-radeon6410d\nsmoke-rtl8111e\n' > "$tmp/expected"
cmp "$tmp/expected" "$WHP_TEST_LOG" >/dev/null || exit 1

# The read-only build plan and real backend execution share target order.
: > "$WHP_TEST_LOG"
WHP_DRIVER_PROFILE=all WHP_DRIVER_BOOTSTRAP=both \
    "$tmp/build.sh" plan > "$tmp/plan"
[ ! -s "$WHP_TEST_LOG" ] || exit 1
grep -Fx 'WHP-Drivers: selected build plan (all)' "$tmp/plan" >/dev/null
cat > "$tmp/expected-plan" <<'PLAN'
WHP-Drivers: selected build plan (all)
  ./build bootstrap
  ./build bootstrap-watcom
  ./build smoke
  ./build test-format
  ./build test-le
  ./build smoke-vxd
  ./build smoke-radeon6410d
  ./build smoke-rtl8111e
PLAN
cmp "$tmp/plan" "$tmp/expected-plan" >/dev/null || exit 1
WHP_DRIVER_PROFILE=all WHP_DRIVER_BOOTSTRAP=both "$tmp/build.sh" build
sed -n '2,$s/^  \.\/build //p' "$tmp/expected-plan" > "$tmp/expected"
cmp "$tmp/expected" "$WHP_TEST_LOG" >/dev/null || exit 1

# Incremental overrides auto-bootstrap for the invocation only;
# never overwrites the user's saved toolchain preference.
cp "$tmp/.whpconfig" "$tmp/expected-config"
: > "$WHP_TEST_LOG"
WHP_DRIVER_BOOTSTRAP=both "$tmp/build.sh" incremental
printf 'smoke\n' > "$tmp/expected"
cmp "$tmp/expected" "$WHP_TEST_LOG" >/dev/null || exit 1
cmp "$tmp/.whpconfig" "$tmp/expected-config" >/dev/null || exit 1

# Cleanup is narrowly scoped; symlink and alternate build-tree paths fail.
mkdir -p "$tmp/.build/llvm-native/whp-tests"
touch "$tmp/.build/llvm-native/whp-tests/test-output"
touch "$tmp/.build/llvm-native/keep"
"$tmp/build.sh" clean-tests > /dev/null
[ ! -e "$tmp/.build/llvm-native/whp-tests" ] || exit 1
[ -f "$tmp/.build/llvm-native/keep" ] || exit 1
touch "$tmp/victim"
ln -s "$tmp/victim" "$tmp/.build/llvm-native/whp-tests"
if "$tmp/build.sh" clean-tests >/dev/null 2>&1; then
    printf '%s\n' 'clean-tests followed a symlink' >&2
    exit 1
fi
[ -f "$tmp/victim" ] || exit 1
rm "$tmp/.build/llvm-native/whp-tests"
if WHP_LLVM_BUILD_DIR="$tmp/custom" "$tmp/build.sh" clean-tests >/dev/null 2>&1; then
    printf '%s\n' 'clean-tests accepted an external build tree' >&2
    exit 1
fi

# Refuse unexpected args and keep older backend pass-through unchanged.
if "$tmp/build.sh" plan extra >/dev/null 2>&1; then exit 1; fi
: > "$WHP_TEST_LOG"
"$tmp/build.sh" ndis31-link-inventory --lib "$tmp/ndis.lib"
printf 'ndis31-link-inventory --lib %s/ndis.lib\n' "$tmp" > "$tmp/expected"
cmp "$tmp/expected" "$WHP_TEST_LOG" >/dev/null || exit 1

printf '%s\n' 'WHP-Drivers: build.sh menuconfig regression checks passed'
