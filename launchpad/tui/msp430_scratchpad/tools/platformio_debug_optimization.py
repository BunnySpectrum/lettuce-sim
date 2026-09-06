"""Keep main.cpp at -O0 in debug-full; leave all other code at -Os."""

import os

Import("env")


PROJECT_DIR = os.path.realpath(env.subst("$PROJECT_DIR"))
UNOPTIMIZED_SOURCE = os.path.realpath(os.path.join(PROJECT_DIR, "src/main.cpp"))
OPTIMIZATION_FLAGS = {
    "-O0",
    "-O1",
    "-O2",
    "-O3",
    "-Og",
    "-Os",
    "-Ofast",
}


def use_unoptimized_main(build_env, node):
    source_path = os.path.realpath(node.srcnode().get_abspath())
    if source_path != UNOPTIMIZED_SOURCE:
        return node

    flags = [
        flag
        for flag in build_env.get("CCFLAGS", [])
        if str(flag) not in OPTIMIZATION_FLAGS
    ]
    return build_env.Object(node, CCFLAGS=flags + ["-O0"])


env.AddBuildMiddleware(use_unoptimized_main)
