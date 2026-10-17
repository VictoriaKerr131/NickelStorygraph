#!/usr/bin/env sh
# Post-build artifact checks for the NickelStorygraph Qt6 port.
#
# Test 2 — libstorygraph.so: correct ABI (ARM32), soname, exported handlers,
#          and no Qt5 linkage.
# Test 3 — packaging: Kobo.tgz extracts the hook under imageformats/ (relative
#          to /usr/local/Kobo), and the .adds payload carries the CLI + config.
#
# Run after `just build package` (or `just build-package`), on Linux with the
# binutils toolchain (readelf/nm) on PATH.
set -eu

# Portable ELF readers: Linux CI has readelf; macOS has llvm-objdump (via
# xcrun) but no llvm-readelf. `file` exists on both and reports the machine.
dynsec_cmd() {
  if command -v readelf >/dev/null 2>&1; then readelf -d "$1";
  elif command -v greadelf >/dev/null 2>&1; then greadelf -d "$1";
  elif command -v llvm-objdump >/dev/null 2>&1; then llvm-objdump -p "$1";
  elif command -v xcrun >/dev/null 2>&1; then xcrun llvm-objdump -p "$1";
  else echo "FAIL: no ELF dynamic-section reader"; exit 1; fi
}

SO=hook/libstorygraph.so
TGZ=hook/Kobo.tgz
ADDS=mnt/onboard/.adds/NickelStorygraph

echo "== Test 2: libstorygraph.so =="
[ -f "$SO" ] || { echo "FAIL: $SO missing"; exit 1; }

machine=$(file -b "$SO")
echo "  file: $machine"
printf '%s' "$machine" | grep -q 'ARM' \
  || { echo "FAIL: expected ARM ELF, got '$machine'"; exit 1; }

dynsec_cmd "$SO" | grep -q 'SONAME.*libstorygraph.so' \
  || { echo "FAIL: wrong or missing SONAME"; exit 1; }

for sym in _nh_ReadingController__setVolume \
           _nh_ReadingMenuView__constructor \
           _nh_MainNavView__constructor \
           _nh_SupportedLocales__supportedLocales; do
  nm -D --defined-only "$SO" | grep -q " $sym\$" \
    || { echo "FAIL: handler $sym not exported"; exit 1; }
done

if nm -D "$SO" | grep -q '@Qt_5'; then
  echo "FAIL: binary references Qt5 symbols (should be Qt6)"
  exit 1
fi

echo "  OK: .so checks passed"

echo "== Test 3: packaging =="
[ -f "$TGZ" ] || { echo "FAIL: $TGZ missing"; exit 1; }
tar -tzf "$TGZ" | grep -qx 'imageformats/libstorygraph.so' \
  || { echo "FAIL: Kobo.tgz does not contain imageformats/libstorygraph.so"; exit 1; }

[ -f "$ADDS/cli" ] || { echo "FAIL: $ADDS/cli missing"; exit 1; }
[ -f "$ADDS/config_example.ini" ] || { echo "FAIL: $ADDS/config_example.ini missing"; exit 1; }

echo "  OK: packaging checks passed"
echo "ALL CHECKS PASSED"
