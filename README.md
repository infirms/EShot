# EShot

Fast, lightweight native screenshot tool for Windows and Linux with annotations, OCR, uploads, pinned captures, GIF recording, and MP4 screen recording.

[![Latest release](https://img.shields.io/github/v/release/Benoks/EShot?label=release)](https://github.com/Benoks/EShot/releases/latest)
[![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11%20%7C%20Linux-lightgrey.svg)](#)
[![Qt](https://img.shields.io/badge/Qt-6.x-green.svg)](https://www.qt.io/)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

<p align="center">
  <img src="preview.png" alt="EShot preview" width="760">
</p>

EShot is built for people who want a quick screenshot workflow without a heavy desktop app getting in the way. Capture an area, mark it up, copy it, save it, upload it, recognize text, search it with Google Lens, pin it as a reference, or record it as GIF/MP4 from one compact tray app.

## Highlights

- Region capture with multi-monitor and high-DPI support
- Full-screen monitor capture by double-clicking a screen during selection
- Clean annotation tools for fast markup
- OCR powered by Tesseract with selectable language packs
- Google Lens search for the selected area
- Direct uploads to short-term and long-term image/file hosts
- Pinned screenshots that stay above other windows
- GIF recording for selected screen areas
- MP4 video recording powered by FFmpeg
- Desktop audio and microphone support for video recording
- Customizable global, direct-capture, recording, and in-capture shortcuts
- Toolbar visibility settings
- Windows Print Screen conflict detection and fix helper
- Native Linux capture path with X11 shortcuts and Wayland portal support
- Start with the desktop through Windows Task Scheduler or XDG autostart
- GitHub release update check
- Import/export settings

## Capture Workflow

Start capture from the tray icon, global hotkey, command line, or direct action hotkeys. After selecting an area, EShot opens a compact toolbar where you can annotate, copy, save, upload, OCR, search with Google Lens, pin, or start GIF/video recording.

The selection UI adapts to tight screen space, keeps the dimension label anchored to the selected area, and supports locking the selection while editing.

## Annotation Tools

EShot includes the tools most screenshot workflows need:

- Freehand pen
- Arrow and line
- Rectangle and ellipse
- Semi-transparent rectangle
- Multiline text with font and size controls
- Highlighter
- Mosaic blur with adjustable intensity
- Counter/number markers
- Eraser
- Eyedropper color picker
- Undo and redo

## OCR

EShot can extract text from a selected area using Tesseract OCR. The installer can include the OCR engine and optional language packs.

Supported OCR language options include:

- English
- Turkish
- Russian
- German
- French
- Spanish
- Italian
- Portuguese
- Polish
- Dutch
- Japanese
- Korean
- Simplified Chinese

Missing language packs are shown disabled in the OCR dialog instead of silently disappearing.

## Uploads

Upload screenshots directly from the capture toolbar. EShot supports anonymous/simple hosts as well as OAuth-based storage providers.

Supported services:

- Catbox
- Uguu
- Litterbox
- TmpFiles.org
- temp.sh
- Allwebs
- Radikal Cloud
- Google Drive

For OAuth providers, EShot accepts either a raw access token or the full redirect URL and extracts the token automatically. The upload dialog includes provider-specific help links for token setup.

Allwebs and Radikal Cloud use Chevereto API v1.1 and require an API key from the service account/API page. TmpFiles.org, temp.sh, Uguu, Litterbox, and anonymous Catbox uploads do not require a token.

### Google Drive token setup

Google Drive uploads need a short-lived OAuth `access_token`. The quickest setup path is Google OAuth 2.0 Playground:

1. Open [Google OAuth 2.0 Playground](https://developers.google.com/oauthplayground).
2. In the left panel, find **Drive API v3**.
3. Select this scope:
   ```text
   https://www.googleapis.com/auth/drive.file
   ```
4. Click **Authorize APIs** and approve access with your Google account.
5. Click **Exchange authorization code for tokens**.
6. Copy only the `access_token` value. It usually starts with `ya29.`.
7. Paste it into EShot's Google Drive token field and save it.

You can also paste the full JSON response from OAuth Playground; EShot extracts the `access_token` automatically. Access tokens expire, so if Google Drive upload later returns HTTP 401, generate a fresh token in OAuth Playground and save it again.

## Google Lens

The Google Lens button opens a visual search for the selected area. It is useful for finding matching images, identifying UI elements, checking product images, or searching from a cropped screenshot without saving it first.

## GIF Recording

Record a selected screen area as a GIF. You can configure:

- FPS
- Maximum duration
- Loop behavior
- Start delay
- Output folder
- Direct GIF recording hotkey

## Video Recording

Record a selected area as an MP4 video. Video recording uses FFmpeg and supports:

- Configurable FPS
- Configurable CRF quality
- Optional duration limit
- Desktop/system audio
- Microphone audio
- Audio volume controls
- Microphone device selection
- Pause, stop, and cancel hotkeys
- Output folder

## Pin to Screen

Pinned screenshots stay above other windows. This is useful for reference images, snippets, UI comparison, notes, forms, and quick visual memory while working in another app.

## Settings

The settings window includes:

- General behavior
- Separate save folders for screenshots, GIFs, and videos
- Filename pattern preview
- Notification controls
- Capture behavior
- GIF and video recording settings
- Audio recording controls
- Appearance options
- High contrast mode
- Black tray icon option for light Windows themes
- Interface and toolbar visibility
- Global capture hotkey
- Direct screenshot/GIF/video hotkeys
- Recording control hotkeys
- In-capture tool/action shortcuts
- Settings export and import

## Default Shortcuts

Most shortcuts can be changed in Settings.

| Shortcut | Action |
| --- | --- |
| `Print Screen` | Start capture |
| Double-click screen | Capture that full monitor |
| `Enter` | Copy selected capture |
| `Ctrl+C` | Copy selected capture |
| `Ctrl+S` | Save selected capture |
| `Esc` | Cancel or close overlay |
| `Shift+Enter` | New line while editing text |
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
| `Ctrl+Z` | Undo |
| `Ctrl+Shift+Z` | Redo |

## Install

### Windows

1. Download the latest Windows installer from the [Releases page](https://github.com/Benoks/EShot/releases/latest).
2. Run the setup file.
3. Select optional FFmpeg and OCR components if needed.
4. Start EShot from the Start menu, desktop shortcut, or tray icon.

If "Start with Windows" is enabled, EShot registers itself through Windows Task Scheduler.

### Linux

1. Download `EShot-v<version>-x86_64.AppImage` from the [Releases page](https://github.com/Benoks/EShot/releases/latest).
2. Make it executable if your file manager requires it, then double-click the AppImage.
3. In the graphical first-run wizard, choose the optional FFmpeg/GIF-video,
   Tesseract/OCR language, and desktop portal features you want. The package
   manager may request administrator approval; you can also skip this step.
4. Approve EShot's preferred PrintScreen shortcut in the KDE/GNOME shortcut dialog.
5. Start EShot from the application menu on later launches.

The AppImage bundles EShot and Qt. Its first-run setup installs missing FFmpeg,
Tesseract, GStreamer/PipeWire, and the KDE or GNOME portal backend through
PolicyKit. The application itself is installed for the current user under
`~/.local/opt/EShot`.

Skipped optional features can be installed later from **Settings > Open Linux
dependency setup**. EShot updates an AppImage in place: when a newer GitHub
release is available, choose **Update now** in Settings or the tray menu. EShot
downloads the exact `EShot-v<version>-x86_64.AppImage` release asset, requires
its GitHub SHA-256 digest to match, replaces the AppImage identified by the
`APPIMAGE` environment variable, and restarts it. Package-manager builds do not
self-replace; update those through the package manager.

On Wayland, screenshot, recording, and global-shortcut permissions are handled
by the desktop portal. EShot also prefers the native GlobalShortcuts portal on
KDE/GNOME X11 sessions and retains direct X11 grabs as a fallback.

## Command Line

Windows:

```powershell
EShot.exe --capture
EShot.exe --save "C:\path\to\capture.png"
EShot.exe --silent
```

Linux:

```bash
EShot --capture
EShot --save "$HOME/Pictures/capture.png"
EShot --silent
# deb installs also provide:
eshot --capture
```

## Build From Source

Requirements:

- Qt 6
- CMake
- C++17 compatible compiler

Windows-only:

- Windows 10 or Windows 11
- Inno Setup, only for installer builds

Linux-only:

- X11 development headers for X11 global shortcuts
- FFmpeg for X11 video recording
- GStreamer with PipeWire plugins for Wayland GIF/video recording
- `xdg-desktop-portal` and a desktop portal backend for Wayland capture

```powershell
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

Linux helper scripts:

```bash
./scripts/linux/install-ubuntu-deps.sh
# or on CachyOS/Arch:
./scripts/linux/install-cachyos-deps.sh
./scripts/linux/check-linux-runtime.sh
./scripts/linux/build-linux.sh
./dist-linux/bin/EShot
# Build the release AppImage:
./scripts/linux/build-appimage.sh
```

Installer:

```powershell
iscc EShot_Setup.iss
```

## Third-Party Components

EShot uses:

- Qt for the desktop UI
- Tesseract OCR for text recognition
- Tesseract tessdata language files for OCR language support
- FFmpeg for video recording
- GStreamer and PipeWire for Wayland recording
- xdg-desktop-portal for native Wayland capture permissions
- Inno Setup for Windows installer packaging

See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for details.

## License

EShot is released under the MIT License. See [LICENSE](LICENSE).
