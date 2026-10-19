#!/usr/bin/env bash
# Bundle the Windows runtime DLLs a packaged executable needs and verify
# that nothing is missing.
#
# The Windows release used to ship vioavr.exe without libwinpthread-1.dll,
# so it failed to start with 0xC0000135 (STATUS_DLL_NOT_FOUND). This script
# makes that class of bug impossible to ship: every non-system dependency of
# every executable has to be either statically linked or sitting next to it.
#
# Usage: tools/package_windows.sh <package-dir> [dll-search-dir ...]
#
#   <package-dir>       directory with the executables to check
#   [dll-search-dir]    where to look for a missing DLL (default: the PATH)
#
# Exit status: 0 when every executable is complete, 1 otherwise.
set -uo pipefail

PKG_DIR="${1:?usage: package_windows.sh <package-dir> [dll-search-dir ...]}"
shift || true
SEARCH_DIRS=("$@")
if [ ${#SEARCH_DIRS[@]} -eq 0 ]; then
    SEARCH_DIRS=("/mingw64/bin" "/usr/bin")
fi

# DLLs that Windows always provides. The names are compared without the
# .dll suffix and without case, so "KERNEL32.dll" matches "kernel32".
SYSTEM_DLLS="kernel32 kernelbase ntdll ntoskrnl user32 gdi32 advapi32 shell32
shlwapi ole32 oleaut32 comctl32 comdlg32 ws2_32 wsock32 bcrypt crypt32
winmm imm32 wininet urlmon version setupapi netapi32 rpcrt4 msvcrt ucrtbase
msvcp_winpthread"

is_system_dll() {
    local base
    base="$(echo "${1%.dll}" | tr '[:upper:]' '[:lower:]')"
    case " $SYSTEM_DLLS " in
        *" $base "*) return 0 ;;
    esac
    # API sets are always provided by the loader.
    case "$base" in
        api-ms-win-*|ext-ms-*) return 0 ;;
    esac
    return 1
}

dll_dependencies() {
    objdump -p "$1" 2>/dev/null | awk '/DLL Name/ { print $3 }'
}

find_dll() {
    local name="$1" dir
    for dir in "${SEARCH_DIRS[@]}"; do
        if [ -f "$dir/$name" ]; then
            echo "$dir/$name"
            return 0
        fi
    done
    return 1
}

status=0
while IFS= read -r -d '' exe; do
    exe_dir="$(dirname "$exe")"
    while read -r dep; do
        [ -n "$dep" ] || continue
        if is_system_dll "$dep"; then
            continue
        fi
        if [ -f "$exe_dir/$dep" ]; then
            continue
        fi
        if src="$(find_dll "$dep")"; then
            echo "bundling $dep -> $exe_dir"
            cp -f "$src" "$exe_dir/" || {
                echo "  ERROR: could not copy $src"
                status=1
                continue
            }
            continue
        fi
        echo "  MISSING: $(basename "$exe") needs $dep, which is neither static nor next to it"
        status=1
    done < <(dll_dependencies "$exe")
done < <(find "$PKG_DIR" -name '*.exe' -print0)

if [ "$status" -eq 0 ]; then
    echo "All executables in $PKG_DIR are self-contained."
else
    echo "Some executables in $PKG_DIR still have unsatisfied dependencies." >&2
fi
exit "$status"
