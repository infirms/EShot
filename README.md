# EShot

Native screenshot, annotation, OCR, visual-search, upload, GIF, and video capture for Windows and Linux.

[![Latest release](https://img.shields.io/github/v/release/Benoks/EShot?label=release)](https://github.com/Benoks/EShot/releases/latest)
[![Build](https://github.com/Benoks/EShot/actions/workflows/build.yml/badge.svg)](https://github.com/Benoks/EShot/actions/workflows/build.yml)
[![Platforms](https://img.shields.io/badge/platform-Windows%2010%2F11%20%7C%20Linux-4b8bbe)](#platform-support)
[![Qt](https://img.shields.io/badge/Qt-6.x-41cd52)](https://www.qt.io/)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)

> [!IMPORTANT]
> The Linux build has currently been developed and tested only on KDE Plasma 6 with Wayland, primarily on CachyOS. Other desktop environments and compositors are not supported yet and are unlikely to work correctly. GNOME support is planned for a future release.

EShot keeps the complete screenshot workflow in one compact tray application: select a region, annotate it, copy or save it, extract text, search the image, upload it, pin it above other windows, or record it as GIF/MP4.

## Screenshots

| Windows | Linux (CachyOS / KDE Plasma 6) |
| --- | --- |
| <img src="image.png" alt="EShot on Windows" width="760"> | <img src="docs/images/cachyos-desktop.png" alt="EShot on CachyOS with KDE Plasma 6" width="760"> |

## Platform support

| Platform | Status | Distribution |
| --- | --- | --- |
| Windows 10/11 x64 | Stable | Installer and portable ZIP |
| Windows 11 ARM64 | Stable | Native ARM64 installer and portable ZIP |
| Linux x86_64 (KDE Plasma 6) | **Experimental** | AppImage, `.deb`, and portable archive |
| Other Linux desktops | Unsupported | Unlikely to work correctly |

## Features

- Region and monitor capture with multi-monitor and high-DPI handling
- Compact selection overlay with configurable actions and shortcuts
- Pen, arrow, line, rectangle, ellipse, text, highlighter, blur, counter, eraser, and eyedropper tools
- Undo/redo, selection locking, color controls, and configurable toolbar visibility
- Tesseract OCR with selectable language packs
- Google Lens or Yandex Images visual search for the selected region
- Screenshot uploads to Catbox, Uguu, Litterbox, TmpFiles.org, temp.sh, Allwebs, Radikal Cloud, Google Drive, and Yandex Disk
- Always-on-top pinned captures
- Selected-region GIF recording
- MP4 recording with configurable FPS, quality, duration, desktop audio, and microphone audio
- Real microphone-device selection on Linux and Windows
- Global capture and direct screenshot/GIF/video hotkeys
- Configurable pause, stop, and cancel recording shortcuts
- Settings import/export and automatic release checks
- Self-update support for installed AppImages

## Capture workflow

Start a capture from the tray icon, `Print Screen`, a custom global shortcut, a direct-action shortcut, or the command line. Select a region and use the overlay toolbar to annotate, OCR, search, upload, pin, copy, save, or begin recording.

Double-clicking a screen during selection captures that complete monitor. The overlay adapts to tight spaces and scaled multi-monitor layouts.

## Install

Download the latest build from [GitHub Releases](https://github.com/Benoks/EShot/releases/latest).

### Windows

1. Download `EShot_Setup_v<version>_x64.exe` or the ARM64 installer.
2. Run the installer and choose the optional FFmpeg/OCR components you need.
3. Launch EShot from the Start menu or system tray.

Portable x64 and ARM64 ZIP archives are also attached to each release.

### Linux: experimental KDE Plasma 6 build

1. Download `EShot-v<version>-x86_64.AppImage`.
2. Mark it executable if required:

   ```bash
   chmod +x EShot-v*-x86_64.AppImage
   ```

3. Open the AppImage.
4. Complete the graphical first-run wizard. It can install FFmpeg/GStreamer, PipeWire portal components, Tesseract, selected OCR languages, and application-menu integration through the system package manager.
5. Use **Activate Print Screen for EShot** if KDE currently assigns `Print Screen` to Spectacle.

The AppImage bundles EShot and Qt. Optional media, OCR, and desktop-integration packages remain system packages. Skipped dependencies can be installed later from **Settings → Open Linux dependency setup**.

Integrated AppImages are stored for the current user under `~/.local/opt/EShot`. When an update is available, EShot downloads the matching AppImage release asset, verifies its GitHub SHA-256 digest, replaces the installed AppImage, and restarts it. Native package builds should be updated through their package manager.

### Linux runtime notes

- Wayland screenshots and recordings use XDG Desktop Portal and PipeWire.
- KDE global shortcuts use KGlobalAccel; non-KDE desktops may fall back to the Global Shortcuts portal or X11 grabs.
- GIF recording uses GStreamer for portal capture and FFmpeg for final GIF encoding.
- MP4 recording requires a GStreamer AAC encoder when audio is enabled.
- Screen-sharing permission is requested by the desktop for each new portal recording session.

## Visual search

Choose **Google Lens** or **Yandex Images** in Settings. EShot uploads only the selected region to a temporary public image host and opens the chosen visual-search provider with that temporary URL.

Do not use visual search for private or sensitive screenshots.

## OCR

OCR is powered by Tesseract. The first-run wizard can install English, the system language, and additional language data. Missing languages remain visible but disabled so the missing dependency is clear.

## Upload services

Anonymous providers work without credentials. Google Drive and Yandex Disk require OAuth access tokens; Allwebs and Radikal Cloud require service API keys. Tokens are stored in EShot settings on the local machine.

### Google Drive token setup

1. Open [Google OAuth 2.0 Playground](https://developers.google.com/oauthplayground).
2. Select the Drive API v3 scope:

   ```text
   https://www.googleapis.com/auth/drive.file
   ```

3. Authorize the scope and exchange the authorization code for tokens.
4. Copy the `access_token`, or copy the complete JSON response.
5. Paste it into EShot's Google Drive token field.

OAuth Playground access tokens expire. Generate a new token if an upload later returns HTTP 401.

## Default shortcuts

Most shortcuts can be changed in Settings.

| Shortcut | Action |
| --- | --- |
| `Print Screen` | Start region capture |
| Double-click a screen | Capture the complete monitor |
| `Enter` / `Ctrl+C` | Copy selection |
| `Ctrl+S` | Save selection |
| `Esc` | Cancel or close |
| `P` | Pen |
| `A` | Arrow |
| `L` | Line |
| `R` | Rectangle |
| `C` | Circle |
| `T` | Text |
| `H` | Highlighter |
| `B` | Blur |
| `N` | Counter |
| `X` | Eraser |
| `D` | Semi-transparent rectangle |
| `I` | Eyedropper |
| `Ctrl+Z` / `Ctrl+Shift+Z` | Undo / redo |

## Command line

```powershell
# Windows
EShot.exe --capture
EShot.exe --save "C:\path\to\capture.png"
EShot.exe --silent
```

```bash
# Linux
EShot --capture
EShot --save "$HOME/Pictures/capture.png"
EShot --silent

# Native packages also install the lowercase launcher:
eshot --capture
```

## Build from source

Core requirements:

- CMake 3.16+
- C++17 compiler
- Qt 6 Core, GUI, Widgets, Network, and DBus on Linux

### Linux

Ubuntu/Debian and CachyOS/Arch dependency helpers are included:

```bash
./scripts/linux/install-ubuntu-deps.sh
# or
./scripts/linux/install-cachyos-deps.sh

./scripts/linux/check-linux-runtime.sh
./scripts/linux/build-linux.sh
./dist-linux/bin/EShot
```

Build the AppImage:

```bash
./scripts/linux/build-appimage.sh
```

### Windows

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
```

Build the installer with Inno Setup 6:

```powershell
iscc EShot_Setup.iss
```

## Third-party components

EShot uses Qt, Tesseract OCR, FFmpeg, GStreamer, PipeWire, XDG Desktop Portal, and Inno Setup. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for licensing details.

## License

EShot is released under the [MIT License](LICENSE).
