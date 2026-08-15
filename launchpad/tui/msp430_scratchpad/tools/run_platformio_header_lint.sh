#!/usr/bin/env bash

# PlatformIO adapter. PlatformIO only generates the standard compilation
# database; the portable runner performs the actual clang-tidy analysis.

set -euo pipefail

script_dir=$(cd "$(dirname "$0")" && pwd)
project_dir=$(cd "$script_dir/.." && pwd)
environment=lpmsp430g2553

usage() {
    cat <<'EOF'
Usage: run_platformio_header_lint.sh [-e ENVIRONMENT] [RUNNER_OPTION...]

Generate compile_commands.json with PlatformIO, then run the portable
header-internal-object clang-tidy check.
EOF
}

if [[ ${1:-} == -h || ${1:-} == --help ]]; then
    usage
    exit 0
fi

if [[ ${1:-} == -e || ${1:-} == --environment ]]; then
    [[ $# -ge 2 ]] || { printf '%s requires a value\n' "$1" >&2; exit 2; }
    environment=$2
    shift 2
fi

(
    cd "$project_dir"
    pio run -e "$environment" -t compiledb
)

exec "$script_dir/run_header_internal_object_check.sh" \
    --build-path "$project_dir" "$@"
