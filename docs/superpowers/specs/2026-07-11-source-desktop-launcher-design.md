# Source Desktop Launcher Fix

## Goal

Make `EShot-Linux.desktop` launch the source-folder Linux launcher when it is
opened from Plasma, without relying on shell syntax that violates the Desktop
Entry `Exec` field grammar.

## Design

The source desktop entry will invoke the repository's `desktop-launch.sh`
through `/bin/bash` using the current SSD path. The script already finds the
repository root from its own location, so it does not need a shell `cd`, command
substitution, or `%k` argument from the desktop entry.

The desktop entry keeps `Terminal=true`, so the existing dependency install,
build, and AppImage launch output remains visible to the user.

## Validation

Extend the Linux package-contract shell test to require a valid source desktop
entry and run `desktop-file-validate` against it. The test will fail on the
current malformed `Exec` value and pass once the entry contains the standard
launcher command.

No Windows source files or packaged AppImage desktop metadata are changed.
