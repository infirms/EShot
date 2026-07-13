# KDE Plasma Icon and Cursor Fixes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make EShot v4.0.1 show its pen-nib icon in KDE launchers and guarantee a cursor-free frozen capture background on KDE Plasma 6 Wayland.

**Architecture:** Keep the stable desktop/application ID, but use a versioned icon lookup key to bypass Plasma's in-process pixmap cache. Keep KWin ScreenShot2 as the preferred backend, add explicit diagnostics around it, and use Spectacle's authorized cursor-free background capture as the KDE AppImage fallback while prohibiting cursor-bearing portal/QScreen fallback on KDE Wayland.

**Tech Stack:** Qt 6 Core/Gui/Widgets/DBus/Test, KDE Plasma 6 ScreenShot2, Spectacle CLI, CMake/CTest, Bash AppImage packaging tests.

## Global Constraints

- Target version is exactly `4.0.1`.
- Keep desktop ID and Qt desktop file name exactly `io.github.benoks.EShot`.
- Do not reset Plasma settings or caches globally.
- Do not push to GitHub.
- Final artifact is `packages/EShot-v4.0.1-x86_64.AppImage`.

---

### Task 1: Lock the Linux fallback and icon contracts with failing tests

**Files:**
- Create: `src/core/LinuxScreenshotPolicy.h`
- Create: `src/core/LinuxScreenshotPolicy.cpp`
- Create: `tests/LinuxScreenshotPolicyTests.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/linux/package-contract-tests.sh`
- Modify: `tests/linux/apprun-integration-tests.sh`

**Interfaces:**
- Produces: `LinuxScreenshotPolicy::isKdeWaylandSession(const QString &, const QString &) -> bool`
- Produces: `LinuxScreenshotPolicy::spectacleWorkspaceArguments(const QString &) -> QStringList`
- Produces: `LinuxScreenshotPolicy::allowCursorBearingFallback(bool) -> bool`

- [ ] **Step 1: Add tests asserting KDE Wayland detection, cursor-free Spectacle arguments, portal rejection on KDE Wayland, `Icon=io.github.benoks.EShot-v4`, `StartupWMClass=EShot`, and installation of both versioned and compatibility SVG names.**

- [ ] **Step 2: Run the focused Qt and Bash tests and verify they fail because the policy target, versioned icon contract, and second installed icon do not yet exist.**

- [ ] **Step 3: Implement only the pure policy functions and register their test target in CMake.**

- [ ] **Step 4: Run `cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release`, build `eshot_linux_screenshot_policy_tests`, and verify the focused policy tests pass while package tests still fail at the unimplemented packaging assertions.**

### Task 2: Make icon lookup cache-safe without changing application identity

**Files:**
- Modify: `packaging/linux/io.github.benoks.EShot.desktop`
- Modify: `EShot-Linux.desktop`
- Modify: `scripts/linux/install-user.sh`
- Modify: `packaging/linux/AppRun`
- Modify: `CMakeLists.txt`
- Modify: `tests/linux/package-contract-tests.sh`
- Modify: `tests/linux/apprun-integration-tests.sh`

**Interfaces:**
- Consumes: the existing `packaging/linux/io.github.benoks.EShot.svg` artwork.
- Produces: installed `io.github.benoks.EShot-v4.svg` plus legacy `io.github.benoks.EShot.svg` compatibility copies.

- [ ] **Step 1: Change both desktop templates to `Icon=io.github.benoks.EShot-v4` and add `StartupWMClass=EShot`, leaving the desktop filename and restricted-interface declaration unchanged.**

- [ ] **Step 2: Install the same source SVG under both icon names in hicolor and pixmaps from user installation and AppImage integration paths.**

- [ ] **Step 3: Add CMake install rules for the versioned hicolor and pixmap files while retaining the legacy hicolor file.**

- [ ] **Step 4: Run package contract and AppRun integration tests and verify all icon/desktop assertions pass.**

### Task 3: Expose ScreenShot2 failures and metadata

**Files:**
- Modify: `src/core/LinuxPortalScreenshot.cpp`
- Modify: `src/core/LinuxPortalScreenshot.h`

**Interfaces:**
- Produces: exact logs prefixed `[LinuxScreenshot]` for method, typed options, error name/message, metadata, image-read status, and selected backend.
- Preserves: `grabScreen`, `grabWorkspace`, and `grab` public return types so existing callers remain source-compatible.

- [ ] **Step 1: Add a shared internal KWin reply reader so `CaptureScreen` and `CaptureWorkspace` report identical diagnostics and metadata validation.**

- [ ] **Step 2: Ensure the `include-cursor` and `native-resolution` values remain `bool` QVariants and include their metatype/value in the request log.**

- [ ] **Step 3: Log portal request/response details and image dimensions without changing non-KDE portal behavior.**

- [ ] **Step 4: Build EShot and run the complete CTest suite to catch API or DBus regressions.**

### Task 4: Add the cursor-free Spectacle fallback and forbid unsafe KDE fallback

**Files:**
- Modify: `src/core/LinuxPortalScreenshot.cpp`
- Modify: `src/core/LinuxPortalScreenshot.h`
- Modify: `src/capture/CaptureOverlay.cpp`
- Test: `tests/LinuxScreenshotPolicyTests.cpp`

**Interfaces:**
- Consumes: `LinuxScreenshotPolicy::spectacleWorkspaceArguments()` and `isKdeWaylandSession()`.
- Produces: `LinuxPortalScreenshot::grabSpectacleWorkspace(QWidget *, int) -> QPixmap`.

- [ ] **Step 1: Implement Spectacle discovery with `QStandardPaths::findExecutable`, a unique `QTemporaryFile` output name, `QProcess`, timeout/termination handling, image loading, and unconditional temporary-file cleanup.**

- [ ] **Step 2: Invoke Spectacle with `--fullscreen --background --nonotify --output <path>` and never pass `--pointer`.**

- [ ] **Step 3: In the XWayland virtual-overlay path, select KWin workspace first, then Spectacle workspace; do not continue to KWin per-screen plus portal after a KDE Wayland authorization failure.**

- [ ] **Step 4: Preserve portal fallback on non-KDE sessions and log the final selected backend or the absence of a cursor-free backend.**

- [ ] **Step 5: Run focused policy tests, the full CTest suite, and Linux package/integration tests.**

### Task 5: Build, integrate, and live-verify v4.0.1

**Files:**
- Generate: `packages/EShot-v4.0.1-x86_64.AppImage`
- Update only through integration: `~/.local/opt/EShot/EShot.AppImage`, `~/.local/share/applications/io.github.benoks.EShot.desktop`, and EShot icon files.

**Interfaces:**
- Produces: the final local AppImage and runtime evidence from the exact same hash.

- [ ] **Step 1: Run `scripts/linux/verify-linux.sh`, `tests/linux/package-contract-tests.sh`, and `tests/linux/apprun-integration-tests.sh`.**

- [ ] **Step 2: Run `scripts/linux/build-appimage.sh` and verify the expected v4.0.1 filename, executable bit, desktop/icon payload, and AppImage smoke test.**

- [ ] **Step 3: Stop every old EShot process, integrate the newly built AppImage, verify installed/package SHA-256 equality, and launch the installed desktop file through KDE.**

- [ ] **Step 4: Verify the process scope, desktop identity, `StartupWMClass`, KWin `NoAuthorized` diagnostic, and successful Spectacle backend selection.**

- [ ] **Step 5: Put the live pointer over a known high-contrast location, invoke Print Screen, and visually verify that the overlay has no frozen pointer while the real pointer remains movable.**

- [ ] **Step 6: Search EShot in KRunner/Kickoff and verify the versioned pen-nib icon; refresh only the Plasma shell process if its already-open view must be reloaded, without deleting user configuration.**

- [ ] **Step 7: Record final `git status`, diff summary, test results, artifact path/hash, and do not push.**
