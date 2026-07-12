# Linux Release, Visual Search, and First-Run Setup Design

## Goal

Ship EShot for Windows x64, Windows ARM64, and Linux x86_64 from the same tagged
release. A Plasma 6 user must be able to download the AppImage, double-click it,
choose optional media/OCR capabilities in a graphical first-run flow, and use
capture, visual search, and self-update without terminal knowledge.

## Existing Problem and Evidence

The current Google Lens action first uploads the selected image to Litterbox and
only opens the browser after that upload returns a public URL. This extra network
round trip explains the visible delay on Linux. The action also aborts an existing
uploader when it is invoked again or its overlay is destroyed. A normal Qt abort
is reported as `Operation canceled`, so cancellation can surface as a misleading
network error instead of being treated as control flow.

The existing Linux work already builds an AppImage, integrates it under the
user's local application directory, selects Linux GitHub release assets, verifies
their SHA-256 digest, replaces the running AppImage atomically, and restarts it.
Its runtime installer is currently all-or-nothing and runs before the graphical
application, which does not provide the requested feature/language choices.

## Visual Search Architecture

Introduce a small platform-neutral visual-search helper with two providers:
`GoogleLens` and `YandexImages`. The persisted setting is
`visualSearchProvider`, defaulting to Google Lens for existing and new users.
The helper owns provider labels, tooltip text, icon selection, and result URL
construction so the overlay, toolbar, settings UI, and tests do not duplicate
provider rules.

The existing toolbar control remains one action and one shortcut. At toolbar
creation/refresh time it reads the selected provider. Google Lens uses the
existing search icon and tooltip; Yandex Images uses a visually matching `Y`
icon and a Yandex tooltip. Settings contains a combo box labelled "Visual search
engine" with exactly Google Lens and Yandex Images.

Both providers require a browser-accessible image URL for the stable URL-based
flow. Before upload, EShot creates a search-specific temporary image capped to a
reasonable long edge and encoded at high quality. This reduces upload time while
preserving text and line-art readability. The temporary upload remains private
to this action and expires through the temporary host's existing retention.

Each request receives an operation generation. Starting a newer request silently
cancels and retires the older generation. User/application cancellation never
shows a warning. Only a failure belonging to the current generation shows a
translated error. A successful current request opens the selected provider URL
once and releases all temporary state. Browser launch failure is reported
separately from upload failure.

## Graphical Linux First-Run Setup

On Linux, extend the existing `FirstRunWizard` with a dependency page instead of
opening a terminal. It offers:

- FFmpeg/media support, selected by default.
- OCR support, selected by default.
- OCR languages, with English and the supported system language selected by
  default. Other packaged languages can be selected explicitly.
- Desktop integration and shortcuts, selected by default when running an
  AppImage.

The wizard explains that the base screenshot application works without optional
media/OCR packages. Advanced users may skip setup or reopen the dependency page
from Settings later.

Package selection is data-driven. A testable helper maps feature and language
choices to pacman package names on Arch/CachyOS and apt package names on
Ubuntu/Debian. Only missing packages are passed to the installer. The application
invokes the existing packaged installation script with explicit feature/language
arguments; the script uses `pkexec`, so the desktop shows its normal graphical
administrator authorization dialog. No password is read by EShot and no package
is installed without the user's confirmation.

Canceling authorization returns to the wizard with a clear message and leaves
EShot usable. Successfully installed features are detected again rather than
assumed. The selected setup choices and completion state are persisted so normal
launches do not repeat the wizard, while Settings can rerun it after package or
language changes.

The AppImage includes EShot, Qt, its required Qt plugins, icons, desktop metadata,
and the setup scripts. System services and command-line media/OCR tools remain
system packages because bundling their complete codec, portal, and language
ecosystems would make the image large and harder to update safely.

## Linux Update and Release Behavior

Automatic update checks continue to use the latest GitHub Release. A Linux
AppImage install selects only the matching `x86_64.AppImage` asset, requires the
release SHA-256 digest, downloads to the user cache, stages a sibling `.new`
file, waits for the old process to exit, atomically replaces it, preserves the
executable bit, and relaunches it. Failure before replacement leaves the current
AppImage intact.

If EShot is running from a development tree, tar archive, or system package
instead of `APPIMAGE`, it must not overwrite arbitrary files. Settings explains
that the package must be updated through its installation method and offers the
release page where appropriate.

Tagged CI releases build and test Windows x64, Windows ARM64, and Linux x86_64
independently, then publish the existing Windows installers together with the
Linux AppImage (and existing Linux package artifacts). A Linux job failure must
not silently replace or rename Windows assets.

## Error Handling and Privacy

- Expected cancellation is silent; genuine network, HTTP, invalid-response, and
  browser-launch failures have distinct translated messages.
- Visual search makes it explicit in the UI/help that the selected image is sent
  through a temporary public image host before Google or Yandex receives its URL.
- Dependency installation logs the attempted package names and exit status but
  never logs credentials.
- Unsupported distributions do not run guessed commands; the wizard shows the
  exact detected missing tools and links to manual instructions.
- Update verification is mandatory on Linux and a digest mismatch never reaches
  replacement.

## Testing and Acceptance

Automated tests cover provider setting defaults, Google/Yandex result URL
construction, tooltip/icon selection, stale-operation suppression, and expected
cancellation. Package-selection tests cover pacman and apt, FFmpeg-only,
OCR-only, combined installation, English plus system-language defaults, and
unsupported languages. Existing Linux runtime, AppRun, updater, portal, capture
geometry, and Windows behavior tests remain green.

Release validation includes a Linux Release build, full CTest run, shell syntax
and package-contract tests, desktop/AppStream validation, AppImage construction,
and an extracted AppImage smoke test. A live Plasma 6 acceptance pass verifies
double-click launch, graphical authorization, selective package installation,
PrintScreen permission, capture, Google Lens, Yandex Images, and AppImage
self-update. Windows x64 and ARM64 CI builds verify that the shared changes do not
regress the existing installers.

## Git Delivery

The local `main`, `codex/linux-port`, and `codex/linux-native-package` branches
currently point to the same base commit; the Linux implementation is uncommitted
workspace state rather than divergent branch history. During implementation,
unrelated user changes are preserved. After all checks pass, the completed work
is committed in reviewable units and `main` is fast-forwarded to the verified
result. Redundant local branches are deleted only after their commits are proven
reachable from `main`; remote branches are not deleted without explicit user
authorization.
