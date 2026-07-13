# KDE Plasma Icon and Cursor Capture Fixes

## Scope

Fix two KDE Plasma 6 Wayland problems for the EShot v4.0.1 AppImage without resetting Plasma settings or changing the stable desktop ID `io.github.benoks.EShot`:

1. Kickoff may retain the previous EShot icon after the SVG contents change under the same icon name.
2. The capture overlay may contain the cursor from the instant Print Screen was pressed.

## Evidence and Root Causes

The installed desktop file and both user-installed SVG copies are unique and byte-identical to the repository SVG. The active Papirus-Dark theme has no EShot override. `kiconfinder6` resolves the hicolor SVG, the SVG renders successfully at 16, 22, 24, 32, 48, 64, 128, and 256 pixels, and a freshly opened KRunner process displays the new pen-nib image. The stale result is therefore an in-process Plasma icon pixmap cached under the unchanged icon name, not an SVG support or scale failure.

The application is launched from the desktop entry in a correctly named `app-io.github.benoks.EShot@...service` scope. Its Qt desktop file name is also `io.github.benoks.EShot`. However, the live AppImage process executable is `/tmp/.mount_EShot.../usr/bin/EShot`, while the installed desktop entry starts with `/usr/bin/env`. KWin 6.7 authorizes ScreenShot2 by comparing the canonical executable path with the first program in a matching desktop entry. It consequently rejects both `CaptureWorkspace` and `CaptureScreen` with `org.kde.KWin.ScreenShot2.Error.NoAuthorized` even though the desktop file declares `X-KDE-DBUS-Restricted-Interfaces=org.kde.KWin.ScreenShot2`.

The live D-Bus trace also proves that `include-cursor` is sent as a D-Bus boolean with value `false`. EShot currently discards the KWin error and silently continues to the XDG screenshot portal. The KDE portal result used by this path contains the cursor, producing the frozen ghost cursor.

## Design

### Icon cache isolation

Keep the desktop ID and application ID unchanged. Give the v4 artwork a new icon lookup key, `io.github.benoks.EShot-v4`, and install the SVG under that name in hicolor and pixmaps. Retain the legacy-named SVG as a compatibility alias for consumers that still request it. Set the desktop entry's `Icon` field to the versioned name so a running Plasma shell cannot reuse a pixmap cached under the previous key.

Do not generate raster PNG copies: the installed Qt/KDE SVG stack resolves and renders the source at every requested size. Add `StartupWMClass=EShot` because the KDE launcher deliberately starts EShot through XWayland; this improves window/task association but is not treated as a ScreenShot2 authorization fix.

### Capture backend visibility

Make every KWin call log its method, boolean options, D-Bus error name and message, returned metadata, validation failures, and successful image dimensions. Make the overlay log the selected backend and each fallback decision.

Represent failure details separately from a null pixmap so `NoAuthorized`, invalid metadata, timeouts, and read failures are distinguishable during live testing.

### Cursor-free KDE AppImage fallback

Continue trying KWin ScreenShot2 first because it is the fastest path when EShot is installed as a stable executable by a distribution. When a KDE AppImage is rejected, invoke the installed Spectacle executable in non-interactive background mode without its pointer option, write a workspace image to a unique temporary file, load it, and delete the temporary file.

For the XWayland virtual-overlay path, use the cursor-free Spectacle workspace image before any portal fallback. On KDE Wayland, never silently fall through from a rejected KWin/Spectacle path to the cursor-bearing screenshot portal or `QScreen::grabWindow`. If no cursor-free backend succeeds, return an empty result and log an actionable error. Preserve the existing XDG portal behavior for non-KDE desktops.

## Error Handling

- KWin errors include the exact D-Bus error name and message.
- KWin metadata is logged before validation.
- Spectacle startup, timeout, exit status, output-file, and image-load failures are logged separately.
- Temporary output is removed on every Spectacle exit path.
- KDE Wayland reports that no cursor-free backend is available instead of displaying a misleading cursor-bearing snapshot.

## Verification

- Add unit coverage for backend/fallback policy and package contract coverage for the versioned icon, compatibility icon, desktop identity, restricted interface, and `StartupWMClass`.
- Run the complete CTest suite and Linux package/integration scripts.
- Build `packages/EShot-v4.0.1-x86_64.AppImage`.
- Install/integrate that exact AppImage, stop all older EShot processes, and launch through the KDE desktop entry.
- Confirm the process identity and capture logs.
- Place the pointer over a visually distinctive location, invoke the registered Print Screen action, and verify the frozen overlay background has no old cursor while the live pointer remains movable.
- Search for EShot in KDE and verify the versioned pen-nib icon is displayed.
- Do not push to GitHub.
