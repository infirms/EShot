# Linux Media and Yandex Fixes Design

## Goal

Fix Yandex visual search presentation and result loading, make KDE Wayland video/GIF recording negotiate correctly, prevent stale recording indicators, and list actual microphone inputs.

## Design

Yandex uses a bold white Y icon and opens the official image-view search route with `rpt=imageview`. Linux recording uses the portal-returned PipeWire node ID, even output dimensions, and a short startup-health gate before emitting `recordingStarted`.

Wayland GIF capture records a temporary H.264 MP4 through the same working portal pipeline, then converts it to GIF with FFmpeg and removes the temporary file. Linux microphone discovery parses `pactl list short sources`, excludes monitor sources, and exposes actual source names plus the default source.

## Verification

Focused unit tests cover the Yandex URL, PipeWire target selection, even dimensions, and microphone filtering. Runtime checks cover installed plugins, a live portal recording attempt, indicator cleanup on immediate failure, and AppImage packaging.
