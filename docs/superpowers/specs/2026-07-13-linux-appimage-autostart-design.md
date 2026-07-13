# Linux AppImage Autostart Design

## Problem

On Linux, EShot writes `QCoreApplication::applicationFilePath()` into its XDG
autostart desktop entry. Inside an AppImage this is the temporary mounted
payload path, such as `/tmp/.mount_EShot.../usr/bin/EShot`. The mount disappears
when the process or session ends, so KDE cannot execute that path at the next
login.

The failure is confirmed by the current autostart entry and the user journal:
KDE tried the stale `/tmp/.mount_EShot.../usr/bin/EShot` path and the generated
autostart service exited with status 127. The running integrated AppImage exposes
the stable outer image through `APPIMAGE=/home/emirhan/.local/opt/EShot/EShot.AppImage`.

## Design

Add a small Linux autostart policy unit that resolves the executable used by
the XDG desktop entry:

1. Use the non-empty `APPIMAGE` path when it identifies an existing AppImage.
2. Otherwise use `QCoreApplication::applicationFilePath()` so unpacked and
   development builds retain their current behavior.

Both autostart creation and `isAutoStartEnabled()` must use the same resolved
path. This prevents the settings checkbox from comparing a stable desktop entry
against the temporary mounted payload path.

The generated entry will preserve the KDE Wayland XCB environment and
`--silent` behavior. Its icon lookup key will match the versioned icon currently
installed by v4 (`io.github.benoks.EShot-v4`). No unrelated startup or capture
behavior changes are included.

## Existing Installation Repair

After building the corrected application, replace the current user autostart
entry with one whose `Exec` line targets the integrated AppImage at
`~/.local/opt/EShot/EShot.AppImage`. Reintegrate the final AppImage so the menu
entry and installed executable also refer to the final artifact.

## Testing

Add unit coverage for AppImage-path preference and executable fallback. Extend
the Linux package contract/integration coverage to reject temporary AppImage
mount paths in generated autostart behavior. Verify the new test fails before
implementation, then passes afterward.

Run the complete CTest suite and Linux package tests, rebuild
`packages/EShot-v4.0.1-x86_64.AppImage`, install it locally, and start the XDG
autostart desktop entry manually. Confirm the resulting process comes from the
stable installed AppImage and the desktop entry contains no `/tmp/.mount_`
reference.
