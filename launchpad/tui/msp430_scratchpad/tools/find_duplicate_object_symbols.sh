#!/usr/bin/env bash

# Find local C++ objects emitted into RAM or read-only storage by multiple files.
# This is a pre-link lint check: it reads .o files and does not use a linker map.

set -euo pipefail

objdump_command=${OBJDUMP:-msp430-elf-objdump}
cxxfilt_command=${CXXFILT:-msp430-elf-c++filt}
demangle=1

usage() {
    cat <<'EOF'
Usage: find_duplicate_object_symbols.sh [OPTION] PATH...

Search each .o file or directory recursively for duplicate local C++ objects
in .data, .bss, .noinit, or .rodata. Exit status is 1 when duplicates are found.

Options:
      --demangle     Demangle C++ names (default)
      --no-demangle  Keep mangled symbol names
  -h, --help         Show this help message

Environment:
  OBJDUMP             objdump command (default: msp430-elf-objdump)
  CXXFILT             c++filt command (default: msp430-elf-c++filt)
EOF
}

while (($# > 0)); do
    case "$1" in
        --demangle)
            demangle=1
            shift
            ;;
        --no-demangle)
            demangle=0
            shift
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

if (($# == 0)); then
    usage >&2
    exit 2
fi

if ! command -v "$objdump_command" >/dev/null 2>&1; then
    printf 'objdump command not found: %s\n' "$objdump_command" >&2
    exit 2
fi

if ((demangle)) && ! command -v "$cxxfilt_command" >/dev/null 2>&1; then
    printf 'c++filt command not found: %s\n' "$cxxfilt_command" >&2
    exit 2
fi

scratch_dir=$(mktemp -d "${TMPDIR:-/tmp}/duplicate-object-symbols.XXXXXX")
trap 'rm -rf "$scratch_dir"' EXIT
object_list=$scratch_dir/objects
records=$scratch_dir/records
report=$scratch_dir/report

# Expand directory arguments here so callers may inspect an entire build tree or
# restrict the check to project objects such as .pio/build/BOARD/src.
for input_path in "$@"; do
    if [[ -d "$input_path" ]]; then
        find "$input_path" -type f -name '*.o' -print
    elif [[ -f "$input_path" && "$input_path" == *.o ]]; then
        printf '%s\n' "$input_path"
    else
        printf 'Not an object file or directory: %s\n' "$input_path" >&2
        exit 2
    fi
done | LC_ALL=C sort -u >"$object_list"

if [[ ! -s "$object_list" ]]; then
    printf 'No object files found.\n' >&2
    exit 2
fi

# objdump identifies a local object with the fields "l O". Limit the lint to
# nonzero objects in data sections and to _ZL names, the Itanium C++ ABI prefix
# for namespace-scope entities with internal linkage. .rodata catches duplicate
# constants that consume flash rather than RAM.
while IFS= read -r object_file; do
    "$objdump_command" --syms "$object_file" |
        awk -v object_file="$object_file" '
            $2 == "l" && $3 == "O" &&
            $4 ~ /^\.(data|bss|noinit|rodata)(\.|$)/ &&
            $5 !~ /^0+$/ && $NF ~ /^_ZL/ {
                if ($4 ~ /^\.data(\.|$)/) section = ".data"
                else if ($4 ~ /^\.bss(\.|$)/) section = ".bss"
                else if ($4 ~ /^\.noinit(\.|$)/) section = ".noinit"
                else section = ".rodata"
                print $NF "\t" section "\t" $5 "\t" object_file
            }
        '
done <"$object_list" | LC_ALL=C sort -t $'\t' -k1,1 -k2,2 -k4,4 >"$records"

# Group by mangled name and output section. Potential waste assumes that one
# largest instance is intentional and the remaining instances are duplicates.
awk -F '\t' '
    function hex_to_dec(value,    digits, result, i, digit) {
        digits = "0123456789abcdef"
        value = tolower(value)
        sub(/^0x/, "", value)
        result = 0
        for (i = 1; i <= length(value); i++) {
            digit = index(digits, substr(value, i, 1)) - 1
            result = result * 16 + digit
        }
        return result
    }

    function flush_group(    i, size, total, largest) {
        if (copies < 2) return
        total = 0
        largest = 0
        for (i = 1; i <= copies; i++) {
            size = hex_to_dec(sizes[i])
            total += size
            if (size > largest) largest = size
        }
        printf "DUPLICATE local object: %s\n", symbol
        printf "  Section: %s  Copies: %d  Total: %d bytes  Potential waste: %d bytes\n", \
            section, copies, total, total - largest
        for (i = 1; i <= copies; i++)
            printf "  %s bytes  %s\n", hex_to_dec(sizes[i]), objects[i]
        print ""
        findings++
    }

    {
        key = $1 SUBSEP $2
        if (previous_key != "" && key != previous_key) flush_group()
        if (key != previous_key) {
            delete sizes
            delete objects
            copies = 0
            symbol = $1
            section = $2
            previous_key = key
        }
        copies++
        sizes[copies] = $3
        objects[copies] = $4
    }

    END {
        flush_group()
        if (findings == 0) print "No duplicate local objects found."
    }
' "$records" >"$report"

if ((demangle)); then
    "$cxxfilt_command" <"$report"
else
    cat "$report"
fi

if grep -q '^DUPLICATE ' "$report"; then
    exit 1
fi
