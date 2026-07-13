# Linux AppImage Autostart Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make EShot's XDG autostart entry survive logout and reboot by targeting the stable outer AppImage instead of its temporary mounted payload.

**Architecture:** A focused `LinuxAutoStartPolicy` unit selects the stable executable path from the AppImage environment, with the normal executable as a fallback. `SettingsDialog` uses the same result both when writing and when checking the autostart desktop entry.

**Tech Stack:** C++17, Qt 6 Core/Test, XDG autostart desktop entries, CMake/CTest, Bash package tests.

## Global Constraints

- Preserve the KDE Plasma Wayland XCB environment used by the existing entry.
- Preserve `--silent` startup behavior.
- Never persist a `/tmp/.mount_*` AppImage payload path when `APPIMAGE` identifies the stable outer image.
- Keep unpacked and development builds working through the current executable path fallback.
- Do not push to GitHub.
- Rebuild `packages/EShot-v4.0.1-x86_64.AppImage` and update the local integrated copy.

---

### Task 1: Stable Linux Autostart Executable Policy

**Files:**
- Create: `src/core/LinuxAutoStartPolicy.h`
- Create: `src/core/LinuxAutoStartPolicy.cpp`
- Create: `tests/LinuxAutoStartPolicyTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: the `APPIMAGE` value and `QCoreApplication::applicationFilePath()` supplied by `SettingsDialog`.
- Produces: `QString LinuxAutoStartPolicy::executablePath(const QString &appImagePath, const QString &applicationFilePath)`.

- [x] **Step 1: Write the failing policy tests**

Cover an AppImage outer path replacing `/tmp/.mount_EShot/usr/bin/EShot`, an empty `APPIMAGE` fallback, and a missing outer path fallback using a temporary test file for the valid case.

- [x] **Step 2: Run the focused test to verify RED**

Run: `cmake -S . -B build-autostart -DBUILD_TESTING=ON && cmake --build build-autostart --target eshot_linux_autostart_policy_tests --parallel 1 && ctest --test-dir build-autostart -R '^linux_autostart_policy$' --output-on-failure`

Expected: configuration/build failure because `LinuxAutoStartPolicy` does not exist yet, or the new assertions fail because path selection is not implemented.

- [x] **Step 3: Implement the minimal policy**

Return the absolute AppImage path only when the provided value is non-empty and exists as a file; otherwise return the application executable path unchanged.

- [x] **Step 4: Run the focused test to verify GREEN**

Run the command from Step 2.

Expected: `linux_autostart_policy` passes.

### Task 2: Use the Stable Path in Settings

**Files:**
- Modify: `src/ui/SettingsDialog.cpp:468-570`
- Modify: `tests/linux/package-contract-tests.sh`

**Interfaces:**
- Consumes: `LinuxAutoStartPolicy::executablePath(...)` from Task 1.
- Produces: an autostart `Exec` line targeting the outer AppImage and an enabled-state check against that same path.

- [x] **Step 1: Add a failing source contract**

Require `SettingsDialog.cpp` to resolve the Linux path through `LinuxAutoStartPolicy` and require the versioned icon key `Icon=io.github.benoks.EShot-v4`.

- [x] **Step 2: Run the package contract test to verify RED**

Run: `bash tests/linux/package-contract-tests.sh`

Expected: failure because `SettingsDialog` still uses `QCoreApplication::applicationFilePath()` directly and writes the legacy icon key.

- [x] **Step 3: Implement the settings integration**

Include the policy header, resolve from `qEnvironmentVariable("APPIMAGE")` plus `QCoreApplication::applicationFilePath()`, use it in both Linux branches, and retain the existing desktop escaping and KDE Wayland environment prefix.

- [x] **Step 4: Run focused tests to verify GREEN**

Run: `ctest --test-dir build-autostart -R '^linux_autostart_policy$' --output-on-failure && bash tests/linux/package-contract-tests.sh`

Expected: both pass.

### Task 3: Repair, Package, and Verify

**Files:**
- Modify at runtime: `~/.config/autostart/io.github.benoks.EShot.desktop`
- Build artifact: `packages/EShot-v4.0.1-x86_64.AppImage`

**Interfaces:**
- Consumes: the corrected EShot binary and package scripts.
- Produces: a rebuilt integrated AppImage and a valid user XDG autostart entry.

- [x] **Step 1: Run full verification**

Run a clean/reconfigured build, `ctest --output-on-failure`, `bash tests/linux/package-contract-tests.sh`, and `bash tests/linux/apprun-integration-tests.sh`.

Expected: every test passes.

- [x] **Step 2: Build and integrate v4.0.1**

Run: `bash scripts/linux/build-appimage.sh` followed by `packages/EShot-v4.0.1-x86_64.AppImage --integrate-only`.

Expected: the package and `~/.local/opt/EShot/EShot.AppImage` exist and have identical SHA-256 hashes.

- [x] **Step 3: Repair and exercise the current autostart entry**

Start the final integrated application, enable/rewrite autostart using the corrected stable path, and execute the XDG autostart desktop entry manually.

Expected: its `Exec` contains `/home/emirhan/.local/opt/EShot/EShot.AppImage`, contains no `/tmp/.mount_`, and EShot remains running silently.

- [x] **Step 4: Commit the implementation**

Commit only the source, tests, and plan changes. Do not push.
