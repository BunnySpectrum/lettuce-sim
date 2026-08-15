#!/usr/bin/env bash

# Correlate the header lint, pre-link object check, and final-link map check.
# This is intentionally a presentation layer over the existing checks: their
# detection logic remains independent and can still be used directly in CI.

set -euo pipefail

script_dir=$(cd "$(dirname "$0")" && pwd)
project_dir=$(cd "$script_dir/.." && pwd)
build_path=$project_dir
map_file=
elf_file=
size_command=${SIZE:-msp430-elf-size}
platformio_command=${PLATFORMIO:-platformio}

usage() {
    cat <<'EOF'
Usage: show_duplicate_symbol_pipeline.sh [OPTION] --map MAP_FILE OBJECT_PATH...

Run the header lint, object-file duplicate check, and final map duplicate check,
then display one row per symbol so it can be followed through the build stages.
The PlatformIO sizedata target is rebuilt first and its output is suppressed.

Options:
  -p, --build-path DIR  Directory containing compile_commands.json
      --map MAP_FILE    GNU ld map from the final link (required)
      --elf ELF_FILE    Final ELF (default: firmware.elf beside MAP_FILE)
  -h, --help            Show this help message

OBJECT_PATH may be an object file or a directory searched recursively for .o
files. Prefer project object directories to avoid framework-heavy output.
EOF
}

while (($# > 0)); do
    case "$1" in
        -p|--build-path)
            [[ $# -ge 2 ]] || { printf '%s requires a value\n' "$1" >&2; exit 2; }
            build_path=$2
            shift 2
            ;;
        --map)
            [[ $# -ge 2 ]] || { printf '%s requires a value\n' "$1" >&2; exit 2; }
            map_file=$2
            shift 2
            ;;
        --elf)
            [[ $# -ge 2 ]] || { printf '%s requires a value\n' "$1" >&2; exit 2; }
            elf_file=$2
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
            printf 'Unknown option: %s\n\n' "$1" >&2
            usage >&2
            exit 2
            ;;
        *)
            break
            ;;
    esac
done

if [[ -z "$map_file" || $# == 0 ]]; then
    usage >&2
    exit 2
fi

if ! command -v "$platformio_command" >/dev/null 2>&1; then
    printf 'PlatformIO command not found: %s\n' "$platformio_command" >&2
    exit 2
fi

if ! (cd "$project_dir" &&
      "$platformio_command" run --target sizedata >/dev/null 2>&1); then
    printf 'PlatformIO sizedata build failed; run it directly for details.\n' >&2
    exit 2
fi

if [[ -z "$elf_file" ]]; then
    elf_file=$(dirname "$map_file")/firmware.elf
fi

if [[ ! -f "$elf_file" ]]; then
    printf 'ELF file not found: %s\n' "$elf_file" >&2
    exit 2
fi

if ! command -v "$size_command" >/dev/null 2>&1; then
    printf 'size command not found: %s\n' "$size_command" >&2
    exit 2
fi

scratch_dir=$(mktemp -d "${TMPDIR:-/tmp}/symbol-pipeline.XXXXXX")
trap 'rm -rf "$scratch_dir"' EXIT
lint_output=$scratch_dir/lint
object_output=$scratch_dir/object
map_output=$scratch_dir/map
size_output=$scratch_dir/size

# Findings are the normal exit status 1 for all three checks. Status 2 means a
# check could not run, so stop instead of presenting an incomplete table.
run_check() {
    local output_file=$1
    shift
    local status=0
    "$@" >"$output_file" 2>&1 || status=$?
    if ((status > 1)); then
        cat "$output_file" >&2
        exit 2
    fi
}

run_check "$lint_output" \
    "$script_dir/run_header_internal_object_check.sh" --build-path "$build_path"
run_check "$object_output" \
    "$script_dir/find_duplicate_object_symbols.sh" "$@"
run_check "$map_output" \
    "$script_dir/find_duplicate_map_symbols.sh" "$map_file"
run_check "$size_output" "$size_command" "$elf_file"

python3 - "$lint_output" "$object_output" "$map_output" "$size_output" <<'PY'
import re
import sys


def parse_lint(path):
    findings = {}
    pattern = re.compile(
        r"^(.+):(\d+):(\d+): (?:error|warning): "
        r"internal-linkage const object '([^']+)' .*"
        r"\[lettuce-header-internal-object\]$"
    )
    with open(path, encoding="utf-8", errors="replace") as stream:
        for line in stream:
            match = pattern.search(line.rstrip())
            if match:
                source, line_number, column, symbol = match.groups()
                location = f"{source}:{line_number}:{column}"
                findings.setdefault(symbol, set()).add(location)
    return {
        symbol: "; ".join(sorted(locations))
        for symbol, locations in findings.items()
    }


def parse_duplicates(path):
    findings = {}
    current = None
    with open(path, encoding="utf-8", errors="replace") as stream:
        for line in stream:
            match = re.match(r"DUPLICATE local object: (.+)$", line.rstrip())
            if match:
                current = match.group(1)
                continue
            if current is None:
                continue
            match = re.match(
                r"\s+Section: (\S+)\s+Copies: (\d+)\s+Total: (\d+) bytes"
                r"\s+Potential waste: (\d+) bytes",
                line,
            )
            if match:
                section, copies, _total, waste = match.groups()
                summary = f"{copies}x {section}, {waste}B waste"
                previous = findings.get(current)
                findings[current] = (
                    f"{previous}; {summary}" if previous else summary
                )
                current = None
    return findings


def parse_size(path):
    with open(path, encoding="utf-8", errors="replace") as stream:
        for line in stream:
            fields = line.split()
            if len(fields) >= 4 and all(field.isdigit() for field in fields[:4]):
                text_size, data_size, bss_size = map(int, fields[:3])
                return data_size + bss_size, text_size + data_size
    raise SystemExit(f"Could not parse size output from {path}")


lint = parse_lint(sys.argv[1])
objects = parse_duplicates(sys.argv[2])
final_map = parse_duplicates(sys.argv[3])
ram_size, flash_size = parse_size(sys.argv[4])
symbols = sorted(set(lint) | set(objects) | set(final_map), key=str.casefold)

headers = (
    "SYMBOL", "LINT", "OBJECT FILES", "FINAL MAP", "IMPACTING", "ORIGIN"
)
rows = []
for symbol in symbols:
    in_map = symbol in final_map
    rows.append((
        symbol,
        "warning" if symbol in lint else "-",
        objects.get(symbol, "-"),
        final_map.get(symbol, "-"),
        "Y" if in_map else "N",
        lint.get(symbol, "-"),
    ))

# Put symbols that consume duplicate space in the final image first, then sort
# alphabetically within the impacting and non-impacting groups.
rows.sort(key=lambda row: (row[4] != "Y", row[0].casefold()))

print(f"RAM:   {ram_size} bytes")
print(f"FLASH: {flash_size} bytes")
print()

if not rows:
    print("No findings from the lint, object, or map checks.")
    raise SystemExit

widths = [len(header) for header in headers]
for row in rows:
    for index, value in enumerate(row):
        widths[index] = max(widths[index], len(value))

def print_row(row):
    print(" | ".join(value.ljust(widths[index]) for index, value in enumerate(row)))

print_row(headers)
print("-+-".join("-" * width for width in widths))
for row in rows:
    print_row(row)

print()
print("- means that stage did not report the symbol as a duplicate.")
PY
