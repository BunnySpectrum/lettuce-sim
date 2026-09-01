Import("env")

env.AddCustomTarget(
    name="size-breakdown",
    dependencies="$PIOMAINPROG",
    actions=[
        env.VerboseAction(
            "$SIZECHECKCMD",
            "Size breakdown for $PIOENV",
        ),
        env.VerboseAction(
            env.DumpSizeData,
            "Writing size data for $PIOENV",
        ),
    ],
    title="Size Breakdown",
    description="Build the firmware, print its sections, and write sizedata.json",
)
