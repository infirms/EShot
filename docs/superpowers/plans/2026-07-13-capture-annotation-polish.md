# Capture Annotation Polish Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Deliver a locally testable v4.0.1 AppImage with screen-fixed annotations, resize-handle priority, optional tool persistence, and repaired KDE icon integration.

**Architecture:** CaptureOverlay will author and render annotations in overlay coordinates, using the selection rectangle only for clipping and export translation. A small pure policy helper will centralize resize-handle precedence and tool restoration defaults so they can be tested without driving the full overlay UI. Existing QSettings and Linux integration scripts remain the persistence and installation boundaries.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake/CTest, Bash, AppImage.

## Global Constraints

- Do not push commits.
- `rememberLastAnnotationTool` defaults to `false`.
- Ordinary selection-interior dragging remains disabled while a drawing tool is active.
- Linux remains experimental and KDE Plasma 6 is the acceptance desktop.

---

### Task 1: Capture interaction policy

**Files:**
- Create: `src/capture/CaptureInteractionPolicy.h`
- Create: `src/capture/CaptureInteractionPolicy.cpp`
- Create: `tests/CaptureInteractionPolicyTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `shouldReleaseToolForResize(bool handleHit, int currentTool)` and `initialAnnotationTool(bool remember, int storedTool, int noneTool)`.

- [ ] Write tests proving a resize handle releases an active tool, an interior click does not, persistence defaults to None, and opt-in persistence restores the stored tool.
- [ ] Run the focused test and verify it fails because the helper is absent.
- [ ] Implement the minimal pure helper and add its CMake target.
- [ ] Run the focused test and verify it passes.

### Task 2: Overlay coordinate and input behavior

**Files:**
- Modify: `src/capture/CaptureOverlay.cpp`
- Modify: `src/capture/CaptureOverlay.h`
- Modify: `src/ui/AnnotationToolbar.cpp`
- Modify: `src/ui/AnnotationToolbar.h`

**Interfaces:**
- Consumes: Task 1 policy functions.
- Produces: overlay-space annotation authoring, clipping, export translation, and `AnnotationToolbar::clearToolSelection()`.

- [ ] Add a regression test for selection movement preserving annotation overlay coordinates and verify RED.
- [ ] Change drawing, hit testing, moving, text/counter placement, overlay rendering, and final export to use overlay coordinates; translate only while painting the cropped result.
- [ ] Resolve resize handles before drawing input, clear the active tool, and begin resize.
- [ ] Restore or clear the initial tool according to QSettings when each capture starts; save the last tool only when persistence is enabled.
- [ ] Run focused and full CTest suites.

### Task 3: Settings and KDE icon integration

**Files:**
- Modify: `src/ui/SettingsDialog.h`
- Modify: `src/ui/SettingsDialog.cpp`
- Modify: `scripts/linux/install-user.sh`
- Modify: `tests/linux/package-contract-tests.sh`

**Interfaces:**
- Produces: `rememberLastAnnotationTool` checkbox and deterministic KDE cache refresh.

- [ ] Extend package/settings contract tests and verify RED.
- [ ] Add the capture-tab checkbox, load/save/export/import wiring, defaulting to false.
- [ ] Ensure icon installation removes stale icon variants and refreshes hicolor/KService caches after installing the canonical SVG.
- [ ] Run Linux shell tests and full CTest.

### Task 4: Version and local AppImage

**Files:**
- Modify: `CMakeLists.txt`
- Produce: `packages/EShot-v4.0.1-x86_64.AppImage`

**Interfaces:**
- Consumes: all previous tasks.
- Produces: local v4.0.1 acceptance artifact.

- [ ] Change the project version to `4.0.1`.
- [ ] Run a clean Release build, full CTest, shell syntax and package-contract tests.
- [ ] Build the AppImage using `/tmp` build/AppDir paths to avoid NTFS lock contention.
- [ ] Run the AppImage smoke test and report its absolute path and SHA-256 without pushing.
