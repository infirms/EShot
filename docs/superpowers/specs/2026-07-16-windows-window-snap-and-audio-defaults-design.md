# Windows Window Snap and Recording Settings Design

## Scope

Add ShareX-style window snapping to the capture overlay on Windows 10 and 11 only. Linux keeps its existing manual region selection and double-click monitor selection because reliable Wayland window enumeration requires desktop-specific integrations.

Make GIF and video frame-rate controls unambiguous, and improve the recording audio controls so new users understand which controls are active.

## Window selection

- Enumerate visible top-level Windows windows in Z order with `EnumWindows` when capture begins.
- Ignore the EShot overlay, hidden or cloaked windows, non-activatable tool windows, and invalid rectangles.
- Prefer `DwmGetWindowAttribute(DWMWA_EXTENDED_FRAME_BOUNDS)` for visible bounds and fall back to `GetWindowRect`.
- Convert native physical screen bounds into the overlay's logical multi-monitor coordinate system using the existing capture monitor geometry.
- Before a selection exists, moving the pointer over a candidate shows its bounds with the existing blue selection color.
- A click without meaningful pointer movement selects the highlighted window. Dragging past the normal drag threshold continues to create a manual region. Double-click continues to select the monitor.
- Window snapping is compiled out on Linux, so KDE and GNOME behavior cannot regress.

The implementation is independently written from the documented Win32 APIs. ShareX is used only as a behavioral reference because its GPL code cannot be copied into this MIT project.

## Frame-rate clarity

- Preserve the existing limits: GIF is 1-30 FPS and video is 1-60 FPS on every supported platform.
- Use distinct visible labels for GIF FPS and Video FPS in both the main settings and capture quick settings.
- Keep the video value flowing unchanged through `videoRecordingFps` to `VideoRecorder`, which clamps it to 60 and passes it to FFmpeg/GStreamer.

## Audio defaults and disabled state

- If neither the modern audio setting nor the legacy `videoAudioMode` setting exists, default desktop audio and microphone recording to enabled.
- Preserve every existing explicit user choice. Migrate a present legacy `videoAudioMode` value without treating a missing legacy key as `none`.
- Use the same default/migration policy in the main settings dialog, capture quick settings, and recording startup.
- When an audio checkbox is off, disable and visually dim its complete dependent section: source label, source selector, volume label, slider, and value display.
- Re-enable the complete section immediately when checked. The checkbox itself remains fully visible and interactive.

## Testing

- Unit-test Z-order candidate selection, invalid candidate filtering, click-versus-drag behavior, and coordinate conversion without requiring a live Windows desktop.
- Unit-test GIF/video FPS limits and preservation of 60 FPS video.
- Unit-test new-install audio defaults, legacy migration, and preservation of explicit disabled settings.
- Build the Windows targets in CI and run the existing Linux test suite locally to ensure the Windows-only feature does not change Linux capture behavior.

## References

- ShareX region capture behavior: https://github.com/ShareX/ShareX
- Win32 window bounds: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-getwindowrect
- DWM extended frame bounds: https://learn.microsoft.com/windows/win32/dwm/composition-ovw
