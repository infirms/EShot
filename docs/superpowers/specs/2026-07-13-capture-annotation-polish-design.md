# Capture Annotation Polish Design

## Goal

Polish the v4.0.1 capture overlay so annotations behave like marks placed on the desktop image, resize handles take precedence over active drawing tools, tool persistence is optional and disabled by default, and KDE shows the installed EShot icon.

## Annotation Coordinates

Annotation points remain in overlay/screen coordinates. Moving or resizing the selection rectangle must not translate existing annotations. Rendering and final capture crop use the current selection rectangle only as a viewport, so marks remain fixed to the underlying screenshot and only the portion inside the final selection is exported.

## Resize Handle Priority

Mouse presses on selection edge or corner handles are resolved before annotation-tool input. When a handle is hit, the active annotation tool is changed to `None`, its toolbar selection is cleared, and resize begins. An active tool still prevents dragging the selection from its ordinary interior; only explicit resize handles receive this escape behavior.

## Tool Persistence

Settings gains a `rememberLastAnnotationTool` boolean. Its default is `false`. With the setting disabled, every new capture overlay starts with no annotation tool selected. With it enabled, the last selected tool is restored using the existing settings store. Tool style settings such as color and width remain unaffected.

## KDE Icon Integration

The Linux desktop entry continues to use the stable `io.github.benoks.EShot` icon name. Installation must place the SVG at the matching hicolor theme path, refresh the desktop and icon caches when available, and overwrite stale per-user integration files. Package metadata and contract tests verify the identity and installed asset path.

## Testing and Delivery

Add focused tests for screen-fixed annotation coordinates, resize-handle priority, and the default/opt-in tool persistence policy. Extend Linux package contract coverage for the desktop icon identity and cache refresh. Run the focused tests, full CTest suite, Linux shell tests, then build a local v4.0.1 AppImage in `packages/`. Do not push any commits.

## v4.0.1 Test Feedback

Annotations render across the entire dimmed overlay so users can still see portions outside the current selection, while export remains cropped to the selection. KDE Wayland screenshots explicitly request `include-cursor=false` to prevent the pre-capture pointer from being frozen into the background. Newly introduced annotation persistence and toolbar-section labels use TranslationManager entries for all eight supported languages, with native Turkish characters.
