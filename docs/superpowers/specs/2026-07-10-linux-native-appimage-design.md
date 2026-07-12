# EShot Native Linux AppImage Design

## Goal

Ship EShot as a single-click x86_64 AppImage that installs its required Linux
runtime packages with the user's authorization, integrates into KDE Plasma 6
and GNOME, preserves the Windows build, supports PrintScreen through native
desktop portals, and can replace itself safely when a newer GitHub Release is
published.

## Distribution Architecture

The release artifact is `EShot-v<version>-x86_64.AppImage`. The AppImage bundles
EShot, Qt 6, its Qt platform plugins, the desktop file, icon, and first-run
scripts. FFmpeg, Tesseract, GStreamer/PipeWire, and the correct
`xdg-desktop-portal` backend remain system packages because they integrate with
desktop services and multimedia devices.

On first launch, `AppRun` copies the AppImage to
`~/.local/opt/EShot/EShot.AppImage`, installs the desktop entry under
`~/.local/share/applications`, detects KDE or GNOME, and asks PolicyKit to
install missing runtime packages. It then starts the installed AppImage. Later
launches go directly to EShot without rebuilding from source.

The existing source-folder `.desktop` launcher remains a developer fallback,
not the user-facing release package.

## Desktop and PrintScreen Integration

EShot uses the standard `io.github.benoks.EShot` desktop identity. Linux global
shortcuts prefer `org.freedesktop.portal.GlobalShortcuts` on KDE and GNOME,
including X11 sessions when the portal is available. The portal asks the user
to approve the preferred `Print` trigger and resolves conflicts with Spectacle
or GNOME Screenshot through the desktop's native UI. Raw X11 grabs remain a
fallback for desktops without the portal.

Wayland screenshots use the Screenshot portal when direct Qt capture is
blocked. GIF and video recording use ScreenCast plus PipeWire/GStreamer.
Wayland permission dialogs are expected native security behavior and must not
be bypassed.

## Linux Self-Update

GitHub Releases publish the AppImage with a predictable architecture-bearing
name. `UpdateManager` selects `.exe` installers on Windows and `.AppImage`
assets on Linux. Linux downloads must match the GitHub asset size and SHA-256
digest before installation.

For an AppImage update, EShot writes a small replacement script into its cache,
starts it detached, exits, and lets the script atomically replace the current
AppImage, restore executable permissions, restart EShot with `--silent`, and
remove temporary files. If EShot is not running from a writable AppImage path,
the updater reports a clear error and leaves the download untouched.

## Build and Release Pipeline

GitHub Actions builds Linux on Ubuntu 22.04 for broad glibc compatibility,
stages a complete AppDir through CMake, bundles Qt with pinned linuxdeploy
tools, applies the custom `AppRun`, creates the AppImage, and runs an offscreen
GIF smoke test from the AppImage. Tag releases upload the AppImage alongside
the existing Windows installers and portable archives.

## Validation

Automated checks cover platform/architecture asset selection, SHA-256 parsing,
Linux replacement-script generation, portal trigger formatting, desktop
backend package selection, shell syntax, Windows regression build, Qt tests,
desktop/AppStream metadata, and AppImage smoke execution in Linux CI.

The final CachyOS KDE test remains the runtime acceptance gate for tray,
PrintScreen approval, capture, OCR, audio/video/GIF, autostart, update
replacement, and relaunch. GNOME behavior is covered by the same portal and
runtime scripts plus CI/static checks, but also benefits from a later live
GNOME session test.

## Constraints

- Preserve all existing Windows behavior and release assets.
- Target Linux x86_64 for this delivery.
- Do not require source compilation on normal AppImage launches.
- Ask for administrator authorization only when system runtime packages are
  missing.
- Never silently disable the desktop's security permission prompts.
- Keep current user changes local; do not push or commit the dirty workspace.
