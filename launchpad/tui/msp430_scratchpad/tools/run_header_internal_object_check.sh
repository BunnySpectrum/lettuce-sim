#!/usr/bin/env bash

# Run the project clang-tidy plugin using any standard compilation database.

set -euo pipefail

script_dir=$(cd "$(dirname "$0")" && pwd)
project_dir=$(cd "$script_dir/.." && pwd)
build_path=$project_dir
if [[ $(uname -s) == Darwin ]]; then
    plugin=$script_dir/clang-tidy/build-plugin/LettuceTidy.dylib
else
    plugin=$script_dir/clang-tidy/build-plugin/LettuceTidy.so
fi
clang_tidy=${CLANG_TIDY:-}

usage() {
    cat <<'EOF'
Usage: run_header_internal_object_check.sh [OPTION] [SOURCE...]

Run lettuce-header-internal-object using compile_commands.json. When SOURCE is
omitted, every source file in the compilation database is checked.

Options:
  -p, --build-path DIR  Directory containing compile_commands.json
      --clang-tidy CMD  clang-tidy executable
      --plugin FILE     LettuceTidy plugin library
  -h, --help            Show this help message
EOF
}

while (($# > 0)); do
    case "$1" in
        -p|--build-path)
            [[ $# -ge 2 ]] || { printf '%s requires a value\n' "$1" >&2; exit 2; }
            build_path=$2
            shift 2
            ;;
        --clang-tidy)
            [[ $# -ge 2 ]] || { printf '%s requires a value\n' "$1" >&2; exit 2; }
            clang_tidy=$2
            shift 2
            ;;
        --plugin)
            [[ $# -ge 2 ]] || { printf '%s requires a value\n' "$1" >&2; exit 2; }
            plugin=$2
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        --)
            shift
            break
            ;;
        -*)
            printf 'Unknown option: %s\n' "$1" >&2
            exit 2
            ;;
        *)
            break
            ;;
    esac
done

if [[ -z "$clang_tidy" ]]; then
    for candidate in \
        /opt/homebrew/opt/llvm/bin/clang-tidy \
        /usr/local/opt/llvm/bin/clang-tidy \
        "$(command -v clang-tidy 2>/dev/null || true)"; do
        if [[ -n "$candidate" && -x "$candidate" ]]; then
            clang_tidy=$candidate
            break
        fi
    done
fi

if [[ -z "$clang_tidy" || ! -x "$clang_tidy" ]]; then
    printf 'clang-tidy not found; pass --clang-tidy or set CLANG_TIDY.\n' >&2
    exit 2
fi

if [[ ! -f "$plugin" ]]; then
    printf 'Plugin not found: %s\n' "$plugin" >&2
    exit 2
fi

compilation_database=$build_path/compile_commands.json
if [[ ! -f "$compilation_database" ]]; then
    printf 'Compilation database not found: %s\n' "$compilation_database" >&2
    exit 2
fi

scratch_dir=$(mktemp -d "${TMPDIR:-/tmp}/header-internal-object.XXXXXX")
trap 'rm -rf "$scratch_dir"' EXIT
all_diagnostics=$scratch_dir/diagnostics

sources=("$@")
if ((${#sources[@]} == 0)); then
    source_list=$scratch_dir/sources
    python3 - "$compilation_database" "$project_dir" >"$source_list" <<'PY'
import json
import os
import sys

with open(sys.argv[1], encoding="utf-8") as database_file:
    entries = json.load(database_file)

project_dir = os.path.realpath(sys.argv[2])
sources = set()
for entry in entries:
    source = entry["file"]
    # PlatformIO records framework sources as absolute paths and project/library
    # sources as paths relative to each entry's working directory.
    if not os.path.isabs(source):
        sources.add(os.path.realpath(os.path.join(entry["directory"], source)))
        continue
    real_source = os.path.realpath(source)
    if os.path.commonpath((project_dir, real_source)) == project_dir:
        sources.add(real_source)

for source in sorted(sources):
    print(source)
PY
    while IFS= read -r source_file; do
        sources+=("$source_file")
    done <"$source_list"
fi

status=0
for source_file in "${sources[@]}"; do
    "$clang_tidy" \
        --load="$plugin" \
        --checks=-*,lettuce-header-internal-object \
        --warnings-as-errors=lettuce-header-internal-object \
        --header-filter='(^|/)(src|include|lib)/' \
        --quiet \
        -p "$build_path" \
        "$source_file" >>"$all_diagnostics" 2>&1 || status=1
done

# A header is parsed once per translation unit, so the same source diagnostic
# can naturally appear more than once. Print each project warning once while
# retaining any unrelated compiler errors that prevented analysis.
awk '
    /\[lettuce-header-internal-object,-warnings-as-errors\]$/ {
        normalized = $0
        sub(/,-warnings-as-errors/, "", normalized)
        if (!seen[normalized]++) print normalized
        next
    }
    /(^|: )(error|warning):/ || /Error while processing/ { print }
' "$all_diagnostics"

exit "$status"
