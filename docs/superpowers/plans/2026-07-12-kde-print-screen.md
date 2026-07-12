# KDE Print Screen Integration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make EShot own Print Screen reliably on KDE Plasma 6 and select Wayland recording dependencies by default on Wayland.

**Architecture:** Add a KDE KGlobalAccel backend with stable EShot action IDs and activation handling, selected before the portal on KDE. Make the wizard transfer Spectacle's key only after EShot registration succeeds.

**Tech Stack:** C++17, Qt 6 Core/DBus/Widgets/Test, KDE KGlobalAccel D-Bus, CMake/CTest, AppImage.

## Global Constraints

- Preserve Windows behavior and non-KDE portal behavior.
- Never remove Spectacle's Print shortcut unless EShot registration succeeds.
- Preserve Spectacle's remaining shortcuts.
- Default portal dependencies to selected only when `XDG_SESSION_TYPE=wayland`.

---

### Task 1: KDE global shortcut backend

**Files:**
- Create: `src/core/LinuxKGlobalAccelShortcuts.h`
- Create: `src/core/LinuxKGlobalAccelShortcuts.cpp`
- Modify: `src/core/HotkeyManager.h`
- Modify: `src/core/HotkeyManager.cpp`
- Modify: `CMakeLists.txt`
- Test: `tests/LinuxPortalShortcutTests.cpp`

**Interfaces:**
- Produces: `LinuxKGlobalAccelShortcuts::isKdeDesktop(QString)`, `setShortcuts(QHash<int,QPair<UINT,UINT>>)`, `shortcutActivated(int)`.
- Consumes: EShot hotkey IDs and Win32-compatible modifier/key values.

- [ ] **Step 1: Write failing tests** for KDE detection, action IDs, and backend preference.
- [ ] **Step 2: Run** `cmake --build build --target eshot_linux_portal_shortcut_tests && ctest --test-dir build -R linux_portal_shortcut --output-on-failure`; expect failure because the KDE backend does not exist.
- [ ] **Step 3: Implement minimal backend** using `doRegister`, `setShortcut`, `getComponent`, and `globalShortcutPressed` on `org.kde.kglobalaccel`.
- [ ] **Step 4: Run the focused test** and expect PASS.

### Task 2: Safe wizard transfer and Wayland default

**Files:**
- Modify: `src/ui/FirstRunWizard.cpp`
- Modify: `src/core/LinuxDependencySelection.h`
- Modify: `src/core/LinuxDependencySelection.cpp`
- Test: `tests/LinuxDependencySelectionTests.cpp`

**Interfaces:**
- Produces: `defaultLinuxPortalSelection(QString sessionType)`.
- Consumes: `HotkeyManager::reRegisterCaptureHotkey(0, VK_SNAPSHOT)` as the registration success gate.

- [ ] **Step 1: Write failing tests** proving Wayland defaults true and X11 defaults false.
- [ ] **Step 2: Run** the Linux dependency test and expect failure because the helper is absent.
- [ ] **Step 3: Implement minimal helper and wizard flow**; register EShot first, then remove Spectacle's Print assignment, and report any failure.
- [ ] **Step 4: Run focused and complete CTest suites** and expect PASS.

### Task 3: Live KDE and AppImage verification

**Files:**
- Modify only if runtime evidence exposes a defect.
- Output: `packages/EShot-v3.1.0-x86_64.AppImage`

**Interfaces:**
- Consumes: integrated desktop file and live KDE session bus.
- Produces: verified AppImage and visible EShot KGlobalAccel component.

- [ ] **Step 1: Build the AppImage** with the repository packaging script.
- [ ] **Step 2: Launch the integrated AppImage with no duplicate EShot process** and inspect journal output for registration errors.
- [ ] **Step 3: Verify EShot appears in KGlobalAccel and Print activation reaches EShot.**
- [ ] **Step 4: Run package smoke tests** and retain the artifact under `packages`.
