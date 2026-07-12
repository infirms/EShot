# Linux Media Fixes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Repair Yandex reverse image search, Wayland video/GIF recording, recording indicator state, and Linux microphone discovery.

**Architecture:** Add pure Linux recording helpers used by both recorders and UI device discovery. Keep portal selection in the existing recorder classes and gate the started signal on process health.

**Tech Stack:** C++17, Qt 6, KDE/xdg-desktop-portal, PipeWire, GStreamer, FFmpeg, CMake/CTest, AppImage.

## Global Constraints

- Use the portal-returned PipeWire node ID.
- Do not require the optional GStreamer `gifenc` plugin.
- Do not show recording UI for a pipeline that immediately exits.
- Exclude PulseAudio/PipeWire monitor sources from microphone choices.

---

### Task 1: Pure helpers and regression tests

- [ ] Add failing tests for Yandex `rpt=imageview`, node ID, even sizes, and microphone filtering.
- [ ] Run focused tests and verify expected failures.
- [ ] Implement the minimal helpers and rerun focused tests.

### Task 2: Recorder pipelines and state

- [ ] Switch video and GIF target objects to node ID and even dimensions.
- [ ] Replace `gifenc` with temporary MP4 plus FFmpeg conversion.
- [ ] Add an immediate-exit startup gate before `recordingStarted`.
- [ ] Verify recorder build and failure cleanup.

### Task 3: UI, icon, and package

- [ ] Replace the Yandex SVG with a bold white Y.
- [ ] Populate microphone combos from filtered system sources.
- [ ] Run focused tests and Linux package tests.
- [ ] Build and smoke-test `packages/EShot-v3.1.0-x86_64.AppImage`.
