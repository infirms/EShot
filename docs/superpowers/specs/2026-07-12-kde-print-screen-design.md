# KDE Print Screen Integration Design

## Goal

On KDE Plasma 6, EShot must appear as a real global-shortcut component and receive Print Screen. Spectacle's plain Print assignment must only be removed after EShot registration succeeds. On Wayland, screen-recording portal dependencies default to selected.

## Design

EShot will use KDE's KGlobalAccel D-Bus API as the preferred shortcut backend on KDE. The backend registers stable EShot action identifiers, applies the requested key sequences, listens for KGlobalAccel activation signals, and reports synchronous registration success to `HotkeyManager`. Existing portal handling remains for non-KDE desktops and X11 remains the fallback where available.

The setup button first registers EShot's Print action. Only after confirmation does it remove plain Print from Spectacle while preserving Spectacle's other shortcuts. If either operation fails, the UI reports failure and avoids a misleading success state.

## Testing

Pure helpers cover KDE detection, action identifiers, shortcut filtering, and Wayland defaults. Runtime verification checks KGlobalAccel component visibility and activation on the current Plasma session. The complete CTest and AppImage smoke suites must pass.
