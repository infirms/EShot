# Windows Window Snap and Audio Defaults Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add Windows-only hover window selection, clarify GIF/video FPS limits, and make recording audio defaults and disabled states understandable.

**Architecture:** Keep native Win32 enumeration isolated behind a small provider, while pure selection and settings policies remain platform-independent and unit-testable on Linux. `CaptureOverlay` consumes logical window rectangles only. A shared recording settings policy supplies FPS limits and new-install audio defaults to both settings surfaces.

**Tech Stack:** Qt 6 Widgets and Test, C++17, Win32 User32 and DWM APIs, CMake.

## Global Constraints

- Do not change Linux window selection behavior.
- Do not copy GPL ShareX source into the MIT EShot repository.
- Preserve explicit existing audio choices.
- Do not push changes.

---

### Task 1: Window snap policy

**Files:**
- Create: `src/capture/WindowSnapPolicy.h`
- Create: `src/capture/WindowSnapPolicy.cpp`
- Create: `tests/WindowSnapPolicyTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `QRect topmostWindowAt(const QVector<QRect> &, const QPoint &, const QRect &)`
- Produces: `bool isWindowSnapClick(const QPoint &, const QPoint &, int)`

- [ ] Add tests proving first-containing Z-order selection, clipping/rejection of invalid rectangles, and click-versus-drag threshold behavior.
- [ ] Run `cmake --build build-linux --target eshot_window_snap_policy_tests -j2` and verify compilation fails because the policy is absent.
- [ ] Implement the minimal pure policy and add the test target.
- [ ] Run the policy test and verify it passes.

### Task 2: Native Windows window provider and overlay integration

**Files:**
- Create: `src/capture/WindowsWindowProvider.h`
- Create: `src/capture/WindowsWindowProvider.cpp`
- Modify: `src/capture/CaptureOverlay.h`
- Modify: `src/capture/CaptureOverlay.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `CaptureMonitorGeometry` and the Task 1 policy.
- Produces: `QVector<QRect> windowsForCaptureOverlay(WId, const QVector<CaptureMonitorGeometry> &, const QRect &)` in overlay-local logical coordinates.

- [ ] Add a coordinate conversion test for a scaled secondary monitor to the policy/provider helper surface and verify it fails.
- [ ] Implement Windows enumeration with `EnumWindows`, `IsWindowVisible`, `DWMWA_CLOAKED`, `DWMWA_EXTENDED_FRAME_BOUNDS`, tool-window filtering, and `GetWindowRect` fallback. Return an empty list outside Windows.
- [ ] Link `Dwmapi` on Windows.
- [ ] Populate candidates after the overlay native handle exists, ignore EShot's handle/process, and update the hovered rectangle on mouse movement.
- [ ] Paint the hovered rectangle in blue before selection. Commit it on a click, retain manual selection after drag, and keep double-click monitor selection.
- [ ] Run the window policy and capture geometry tests.

### Task 3: Recording settings policy

**Files:**
- Create: `src/recording/RecordingSettingsPolicy.h`
- Create: `src/recording/RecordingSettingsPolicy.cpp`
- Create: `tests/RecordingSettingsPolicyTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `gifRecordingFpsLimit()`, `videoRecordingFpsLimit()` and `initialAudioEnabled(bool, bool, bool, const QString &, RecordingAudioSource)`.

- [ ] Add tests for 30 FPS GIF, 60 FPS video, explicit false preservation, all legacy modes, and true defaults when no key exists.
- [ ] Build the test target and verify it fails because the policy is absent.
- [ ] Implement the policy and verify the tests pass.

### Task 4: FPS labels and audio visual state

**Files:**
- Modify: `src/core/TranslationManager.h`
- Modify: `src/ui/SettingsDialog.h`
- Modify: `src/ui/SettingsDialog.cpp`
- Modify: `src/capture/CaptureOverlay.h`
- Modify: `src/capture/CaptureOverlay.cpp`
- Modify: `src/main.cpp`

**Interfaces:**
- Consumes: Task 3 policy functions.

- [ ] Add translated `gifFpsLabel` and `videoFpsLabel` strings for all eight languages and use shared FPS limits in both settings surfaces.
- [ ] Change audio loading so missing modern and legacy keys default both sources on, while present settings are preserved.
- [ ] Store the defaults when first exposed so settings, quick settings, and recording startup agree.
- [ ] Group source labels/selectors and volume labels/controls so toggling a checkbox disables and visibly dims every dependent widget.
- [ ] Apply the same state immediately in the main settings and quick settings drawer.
- [ ] Run recording settings policy tests and translation completeness tests.

### Task 5: Verification and handoff

**Files:**
- Review all changed files.

- [ ] Run `git diff --check`.
- [ ] Run `cmake -S . -B build-linux -DBUILD_TESTING=ON` and `cmake --build build-linux -j2`.
- [ ] Run `ctest --test-dir build-linux --output-on-failure`.
- [ ] Inspect all `Q_OS_WIN` paths, CMake libraries, and workflow toolchains for MSVC and MinGW compatibility.
- [ ] Report the exact local commit and changed behavior without pushing.
