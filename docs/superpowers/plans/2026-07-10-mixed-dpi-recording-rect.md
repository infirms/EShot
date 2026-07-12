# Mixed-DPI Recording Rectangle Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Keep GIF and video recording on the exact area selected on a non-primary Windows monitor whose scale differs from the primary monitor.

**Architecture:** Preserve the existing screenshot/annotation path and its physical recording rectangle. Add a small pure geometry helper that maps that physical rectangle back through the containing monitor's physical and Qt logical geometries, then retain the physical rectangle for the Windows recorders while using the converted logical rectangle for the on-screen indicator.

**Tech Stack:** C++17, Qt 6 Core/Gui/Widgets/Test, Win32 monitor APIs, CMake/CTest.

## Global Constraints

- Do not rewrite the recorder pipeline or the screenshot preview path.
- Preserve the uncommitted Linux-port work already present in the workspace.
- Do not commit or push.

---

### Task 1: Regression-test mixed-DPI mapping

**Files:**
- Create: `tests/CaptureGeometryTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `CaptureMonitorGeometry { QRect logical; QRect physical; qreal scale; }`
- Produces: tests for `displayRectFromPhysical(const QRect &, const QVector<CaptureMonitorGeometry> &)`

- [ ] Add a QtTest case where a 100%-scale primary monitor is right of a 125%-scale monitor.
- [ ] Assert that a physical selection on the left maps its origin and size back through 1.25.
- [ ] Assert that a physical selection on the 100% primary monitor remains unchanged.
- [ ] Configure and build the test target; verify it fails because the helper does not exist yet.

### Task 2: Implement the focused mapping helper

**Files:**
- Create: `src/capture/CaptureGeometry.h`
- Create: `src/capture/CaptureGeometry.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `QRect displayRectFromPhysical(const QRect &physicalRect, const QVector<CaptureMonitorGeometry> &monitors)`

- [ ] Select the monitor containing the selection center, falling back to the nearest monitor.
- [ ] Map the rectangle relative to that monitor's physical top-left and divide its edges by that monitor's scale.
- [ ] Return the input unchanged when no valid monitor map exists.
- [ ] Build and run the focused test; expect all cases to pass.

### Task 3: Route GIF and video through the physical rectangle

**Files:**
- Modify: `src/capture/CaptureOverlay.h`
- Modify: `src/capture/CaptureOverlay.cpp`
- Modify: `src/main.cpp`

**Interfaces:**
- Consumes: the helper and a monitor map populated from `QScreen` plus `GetMonitorInfoW`.
- Produces: physical `captureRect` for both recorders and unchanged logical `displayRect` for `RecordingIndicator`.

- [ ] Populate monitor maps during capture without changing snapshot painting.
- [ ] Make `selectedCaptureRect()` use the helper on Windows.
- [ ] Send `selectedCaptureRect()` from both direct video and hotkey/tray video paths.
- [ ] Run the focused tests and build the application.

### Task 4: Verify and hand off locally

**Files:**
- Update: `EShot_Release/` via the existing Release build output.

- [ ] Run CTest with failure output enabled.
- [ ] Build the full Release configuration.
- [ ] Run `git diff --check` on the touched source/test files.
- [ ] Copy the verified Release executable and required runtime files locally without committing or pushing.
