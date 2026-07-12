# Native Linux AppImage Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a one-click KDE/GNOME AppImage with native PrintScreen portals and verified Linux self-updates while preserving Windows behavior.

**Architecture:** Keep platform-neutral release selection and update-script construction in testable C++ helpers. Package the installed CMake tree as an AppDir, add a first-run `AppRun`/runtime dependency layer, then produce and smoke-test the AppImage in Linux CI.

**Tech Stack:** C++17, Qt 6 Core/Gui/Widgets/Network/DBus/Test, CMake/CTest, Bash, xdg-desktop-portal, PipeWire/GStreamer, linuxdeploy, AppImageKit, GitHub Actions.

## Global Constraints

- Preserve all existing Windows behavior and release assets.
- Target Linux x86_64 for this delivery.
- Do not require source compilation on normal AppImage launches.
- Ask for administrator authorization only when system runtime packages are missing.
- Never silently disable the desktop's security permission prompts.
- Keep current user changes local; do not push or commit the dirty workspace.

---

### Task 1: Cross-platform release asset selection

**Files:**
- Create: `src/core/UpdateAssetSelector.h`
- Create: `src/core/UpdateAssetSelector.cpp`
- Create: `tests/UpdateAssetSelectorTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `UpdateAsset selectUpdateAsset(const QJsonArray &, UpdatePlatform, const QString &architecture)` and `QString normalizedSha256Digest(const QString &)`.
- Consumes: GitHub Release asset objects containing `name`, `browser_download_url`, `size`, and `digest`.

- [ ] **Step 1: Add failing Qt tests for Windows x64, Windows ARM64, Linux x86_64, wrong architecture, and SHA-256 normalization**

```cpp
QCOMPARE(selectUpdateAsset(assets, UpdatePlatform::Linux, "x86_64").name,
         QString("EShot-v3.2.0-x86_64.AppImage"));
QCOMPARE(normalizedSha256Digest("sha256:ABCDEF"), QString("abcdef"));
```

- [ ] **Step 2: Configure and run only the selector test; verify RED because the helper does not exist**

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:\Qt\6.8.2\msvc2022_64
cmake --build build --config Release --target eshot_update_asset_tests
```

- [ ] **Step 3: Implement the minimal selector and add the test target to CMake**

```cpp
struct UpdateAsset {
    QString name;
    QString url;
    QString sha256;
    qint64 size = 0;
    bool isValid() const { return !name.isEmpty() && !url.isEmpty(); }
};
```

- [ ] **Step 4: Rebuild and run the selector test with the Qt runtime on PATH; verify GREEN**

```powershell
$env:PATH="C:\Qt\6.8.2\msvc2022_64\bin;$env:PATH"
ctest --test-dir build -C Release -R update_asset --output-on-failure
```

- [ ] **Step 5: Record a local checkpoint with `git diff --check` without committing**

### Task 2: Verified AppImage self-update

**Files:**
- Create: `src/core/LinuxUpdateScript.h`
- Create: `src/core/LinuxUpdateScript.cpp`
- Create: `tests/LinuxUpdateScriptTests.cpp`
- Modify: `src/core/UpdateManager.h`
- Modify: `src/core/UpdateManager.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `UpdateAsset` from Task 1 and the current `APPIMAGE` environment path.
- Produces: `QString buildLinuxUpdateScript(qint64 pid, const QString &currentPath, const QString &downloadPath)` and Linux launch/replacement behavior in `UpdateManager`.

- [ ] **Step 1: Add failing tests for shell quoting, wait-for-process, atomic replacement, chmod, restart, and cleanup**

```cpp
const QString script = buildLinuxUpdateScript(42, "/home/a b/EShot.AppImage", "/tmp/new.AppImage");
QVERIFY(script.contains("while kill -0 42"));
QVERIFY(script.contains("chmod 0755"));
QVERIFY(script.contains("mv -f --"));
QVERIFY(script.contains("--silent"));
```

- [ ] **Step 2: Build and run the new test; verify RED because script generation is missing**

- [ ] **Step 3: Implement the script helper and connect `UpdateManager` to `UpdateAssetSelector`**

```cpp
const UpdatePlatform platform =
#ifdef Q_OS_WIN
    UpdatePlatform::Windows;
#else
    UpdatePlatform::Linux;
#endif
```

- [ ] **Step 4: Verify every Linux download with `QCryptographicHash::Sha256`, then launch the detached replacement script only for a writable AppImage path**

- [ ] **Step 5: Run update asset, update script, and existing geometry tests; verify GREEN**

- [ ] **Step 6: Build the full Windows Release target to prove the `.exe` updater remains intact**

### Task 3: KDE/GNOME runtime setup and one-click AppRun

**Files:**
- Create: `scripts/linux/runtime-common.sh`
- Create: `scripts/linux/install-runtime-deps.sh`
- Create: `tests/linux/runtime-common-tests.sh`
- Create: `packaging/linux/AppRun`
- Modify: `scripts/linux/install-cachyos-deps.sh`
- Modify: `scripts/linux/check-linux-runtime.sh`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `eshot_desktop_backend`, `eshot_runtime_packages`, and a first-run AppRun that installs to `~/.local/opt/EShot`.
- Consumes: `XDG_CURRENT_DESKTOP`, `XDG_SESSION_DESKTOP`, `pacman`/`apt-get`, `pkexec`, `APPIMAGE`, and XDG user directories.

- [ ] **Step 1: Write failing Bash tests for Plasma, GNOME, unknown desktop, and package-manager selection**

```bash
XDG_CURRENT_DESKTOP=KDE assert_eq "kde" "$(eshot_desktop_backend)"
XDG_CURRENT_DESKTOP=GNOME assert_contains "xdg-desktop-portal-gnome" "$(eshot_runtime_packages pacman)"
```

- [ ] **Step 2: Run the Bash test with Git for Windows Bash; verify RED because `runtime-common.sh` is absent**

```powershell
& 'C:\Program Files\Git\bin\bash.exe' tests/linux/runtime-common-tests.sh
```

- [ ] **Step 3: Implement desktop/package detection and the PolicyKit runtime installer; verify the Bash tests GREEN**

- [ ] **Step 4: Implement AppRun integration: copy the AppImage, install the desktop file/icon, update desktop caches when available, run dependency setup once, and launch EShot**

- [ ] **Step 5: Install the runtime scripts into `lib/eshot` from CMake and run `bash -n` over every Linux script**

### Task 4: Portal-first PrintScreen behavior

**Files:**
- Create: `tests/LinuxPortalShortcutTests.cpp`
- Modify: `src/core/LinuxPortalGlobalShortcuts.h`
- Modify: `src/core/LinuxPortalGlobalShortcuts.cpp`
- Modify: `src/core/HotkeyManager.cpp`
- Modify: `src/main.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: public static `LinuxPortalGlobalShortcuts::preferredTrigger(UINT, UINT)` and portal-first Linux shortcut registration.
- Consumes: the `io.github.benoks.EShot` desktop identity and portal `Activated` signals.

- [ ] **Step 1: Add failing tests for `Print`, modified keys, letters, digits, function keys, and unsupported keys**

```cpp
QCOMPARE(LinuxPortalGlobalShortcuts::preferredTrigger(0, VK_SNAPSHOT), QString("Print"));
QCOMPARE(LinuxPortalGlobalShortcuts::preferredTrigger(MOD_CONTROL | MOD_ALT, 'P'), QString("CTRL+ALT+p"));
```

- [ ] **Step 2: Build/run the shortcut test; verify RED because the formatter is private**

- [ ] **Step 3: Expose the tested formatter, set the Qt desktop file name, and prefer the GlobalShortcuts portal on Linux with X11 grabbing only as fallback**

- [ ] **Step 4: Run shortcut and full Qt tests; verify GREEN and rebuild Windows Release**

### Task 5: Reproducible AppImage and release contract

**Files:**
- Create: `scripts/linux/build-appimage.sh`
- Modify: `scripts/linux/package-linux.sh`
- Modify: `scripts/linux/verify-linux.sh`
- Modify: `.github/workflows/build.yml`
- Modify: `docs/linux-port-test.md`
- Modify: `README.md`

**Interfaces:**
- Produces: `packages/EShot-v<version>-x86_64.AppImage` and the same asset in tagged GitHub Releases.
- Consumes: CMake install tree, pinned linuxdeploy/Qt plugin/appimagetool URLs and SHA-256 checksums.

- [ ] **Step 1: Add a failing packaging contract check that requires the AppImage asset name in scripts and workflow**

```bash
grep -F 'EShot-v${version}-x86_64.AppImage' scripts/linux/build-appimage.sh
grep -F 'release-assets/*.AppImage' .github/workflows/build.yml
```

- [ ] **Step 2: Implement the pinned AppImage build: stage AppDir, run linuxdeploy with Qt, apply custom AppRun, run appimagetool, and verify output is executable**

- [ ] **Step 3: Add AppImage smoke execution using `--appimage-extract-and-run --test-gif` and upload/release steps to GitHub Actions**

- [ ] **Step 4: Update user documentation to say: download, double-click, approve packages/PrintScreen, then use EShot from the application menu**

- [ ] **Step 5: Run shell syntax, metadata checks available on Windows, all Qt tests, Windows Release build, `--test-gif`, and `git diff --check`**

- [ ] **Step 6: Re-read the design requirement by requirement and report the remaining live-Linux acceptance checks without claiming they ran on Windows**
