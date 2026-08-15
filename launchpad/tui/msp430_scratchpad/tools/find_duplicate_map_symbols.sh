#!/usr/bin/env bash

# Find duplicate local C++ data objects that survived the final GNU ld link.
# This parser targets maps produced with per-variable sections (-fdata-sections).

set -euo pipefail

cxxfilt_command=${CXXFILT:-msp430-elf-c++filt}
demangle=1

usage() {
    cat <<'EOF'
Usage: find_duplicate_map_symbols.sh [OPTION] MAP_FILE

Find duplicate local C++ objects in the final .data, .bss, .noinit, and .rodata
output sections of a GNU ld map. Exit status is 1 when duplicates are found.

Options:
      --demangle     Demangle C++ names (default)
      --no-demangle  Keep mangled symbol names
  -h, --help         Show this help message

Environment:
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

if (($# != 1)); then
    usage >&2
    exit 2
fi

map_file=$1
if [[ ! -f "$map_file" ]]; then
    printf 'Map file not found: %s\n' "$map_file" >&2
    exit 2
fi

if ((demangle)) && ! command -v "$cxxfilt_command" >/dev/null 2>&1; then
    printf 'c++filt command not found: %s\n' "$cxxfilt_command" >&2
    exit 2
fi

scratch_dir=$(mktemp -d "${TMPDIR:-/tmp}/duplicate-map-symbols.XXXXXX")
trap 'rm -rf "$scratch_dir"' EXIT
records=$scratch_dir/records
report=$scratch_dir/report

# Track only final data output sections, which excludes similarly named entries
# in the map's "Discarded input sections" area. This includes writable RAM and
# read-only flash data. An input section may place its address/size/object on
# the same line or on the following line.
awk '
    function emit_record(input_section, address, size, object,    base, symbol) {
        if (input_section !~ /^\.(data|bss|noinit|rodata)\./) return
        if (match(input_section, /_ZL[^[:space:]]*/) == 0) return
        symbol = substr(input_section, RSTART, RLENGTH)
        if (size !~ /^0x[[:xdigit:]]+$/) return
        if (size ~ /^0x0+$/) return
        if (input_section ~ /^\.data\./) base = ".data"
        else if (input_section ~ /^\.bss\./) base = ".bss"
        else if (input_section ~ /^\.noinit\./) base = ".noinit"
        else base = ".rodata"
        print symbol "\t" base "\t" size "\t" address "\t" object
    }

    /^\.[^[:space:]]/ {
        pending_section = ""
        if ($1 == ".data" || $1 == ".bss" || $1 == ".noinit" ||
            $1 == ".rodata")
            output_section = $1
        else
            output_section = ""
        next
    }

    output_section != "" && $1 ~ /^\.(data|bss|noinit|rodata)\./ {
        pending_section = $1
        if ($2 ~ /^0x[[:xdigit:]]+$/ && $3 ~ /^0x[[:xdigit:]]+$/ && NF >= 4) {
            object = $4
            for (i = 5; i <= NF; i++) object = object " " $i
            emit_record(pending_section, $2, $3, object)
            pending_section = ""
        }
        next
    }

    output_section != "" && pending_section != "" &&
    $1 ~ /^0x[[:xdigit:]]+$/ && $2 ~ /^0x[[:xdigit:]]+$/ && NF >= 3 {
        object = $3
        for (i = 4; i <= NF; i++) object = object " " $i
        emit_record(pending_section, $1, $2, object)
        pending_section = ""
    }
' "$map_file" | LC_ALL=C sort -t $'\t' -k1,1 -k2,2 -k5,5 >"$records"

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
            printf "  %s  %d bytes  %s\n", addresses[i], hex_to_dec(sizes[i]), objects[i]
        print ""
        findings++
    }

    {
        key = $1 SUBSEP $2
        if (previous_key != "" && key != previous_key) flush_group()
        if (key != previous_key) {
            delete sizes
            delete addresses
            delete objects
            copies = 0
            symbol = $1
            section = $2
            previous_key = key
        }
        copies++
        sizes[copies] = $3
        addresses[copies] = $4
        objects[copies] = $5
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
