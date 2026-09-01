"""Apply -O0 only to application-owned code in the debug-full build."""

import os

Import("env")


PROJECT_DIR = os.path.realpath(env.subst("$PROJECT_DIR"))
UNOPTIMIZED_ROOTS = tuple(
    os.path.realpath(os.path.join(PROJECT_DIR, path))
    for path in ("src", "lib/tui", "lib/byte_stream")
)
OPTIMIZATION_FLAGS = {
    "-O0",
    "-O1",
    "-O2",
    "-O3",
    "-Og",
    "-Os",
    "-Ofast",
}


def is_below(path, root):
    try:
        return os.path.commonpath((path, root)) == root
    except ValueError:
        return False


def use_unoptimized_debug_code(build_env, node):
    source_path = os.path.realpath(node.srcnode().get_abspath())
    if not any(is_below(source_path, root) for root in UNOPTIMIZED_ROOTS):
        return node

    flags = [
        flag
        for flag in build_env.get("CCFLAGS", [])
        if str(flag) not in OPTIMIZATION_FLAGS
    ]
    return build_env.Object(node, CCFLAGS=flags + ["-O0"])


env.AddBuildMiddleware(use_unoptimized_debug_code)
