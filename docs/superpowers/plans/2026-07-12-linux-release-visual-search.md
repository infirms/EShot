# Linux Release and Visual Search Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Deliver selectable Google Lens/Yandex visual search, a graphical optional-dependency setup flow, and verified AppImage update/release behavior without regressing Windows builds.

**Architecture:** Put visual-search provider rules and Linux package selection in small testable helpers. Keep UI integration in the existing toolbar, settings dialog, and first-run wizard. Reuse the existing uploader, AppRun, PolicyKit installer, updater, and CI pipeline while making cancellation and optional package selection explicit.

**Tech Stack:** C++17, Qt 6 Core/Gui/Widgets/Network/Test, CMake/CTest, Bash, pacman/apt, PolicyKit, AppImageKit, GitHub Actions.

## Global Constraints

- Preserve Windows x64 and Windows ARM64 behavior and release assets.
- Linux release target is x86_64 and Plasma 6 is the live acceptance desktop.
- AppImage launches graphically; terminal knowledge is not required.
- English and the supported system OCR language are selected by default.
- Expected cancellation is silent; genuine failures remain visible.
- Never request or store an administrator password inside EShot.
- Preserve unrelated dirty-worktree changes.

---

### Task 1: Testable visual-search provider rules

**Files:**
- Create: `src/core/VisualSearch.h`
- Create: `src/core/VisualSearch.cpp`
- Create: `tests/VisualSearchTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `enum class VisualSearchProvider`, `visualSearchProviderFromSettings`, `visualSearchProviderKey`, `visualSearchDisplayName`, `visualSearchTooltip`, `visualSearchIconPath`, and `visualSearchResultUrl`.
- Consumes: a temporary public HTTPS image URL.

- [ ] **Step 1: Add failing Qt tests for default parsing, round-trip keys, provider metadata, and encoded Google/Yandex result URLs**

```cpp
QCOMPARE(visualSearchProviderFromSettings("unknown"), VisualSearchProvider::GoogleLens);
QCOMPARE(visualSearchProviderKey(VisualSearchProvider::YandexImages), QString("yandex"));
QVERIFY(visualSearchResultUrl(VisualSearchProvider::GoogleLens, image).toString().contains("lens.google.com"));
QVERIFY(visualSearchResultUrl(VisualSearchProvider::YandexImages, image).toString().contains("yandex.com/images"));
```

- [ ] **Step 2: Configure/build the test and verify RED because `VisualSearch.h` is absent**

Run: `cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build-linux --target eshot_visual_search_tests`

- [ ] **Step 3: Implement the minimal provider helper and register its production/test sources in CMake**

```cpp
enum class VisualSearchProvider { GoogleLens, YandexImages };
QUrl visualSearchResultUrl(VisualSearchProvider provider, const QUrl &imageUrl);
```

- [ ] **Step 4: Run `ctest --test-dir build-linux -R visual_search --output-on-failure` and verify GREEN**

### Task 2: Toolbar/settings integration and cancellation fix

**Files:**
- Modify: `src/ui/AnnotationToolbar.h`
- Modify: `src/ui/AnnotationToolbar.cpp`
- Modify: `src/ui/SettingsDialog.h`
- Modify: `src/ui/SettingsDialog.cpp`
- Modify: `src/capture/CaptureOverlay.h`
- Modify: `src/capture/CaptureOverlay.cpp`
- Modify: `src/core/TranslationManager.h`
- Create: `icons/yandex_search.svg`
- Modify: `resources/icons.qrc`
- Test: `tests/VisualSearchTests.cpp`

**Interfaces:**
- Consumes: Task 1 provider helpers and `QSettings("EShot", "EShot")/visualSearchProvider`.
- Produces: provider combo box, dynamic toolbar icon/tooltip, and current-operation-only browser launch/error behavior.

- [ ] **Step 1: Extend the failing tests with a request generation check showing stale generations are ignored**

```cpp
VisualSearchOperationState state;
const quint64 oldGeneration = state.begin();
const quint64 currentGeneration = state.begin();
QVERIFY(!state.isCurrent(oldGeneration));
QVERIFY(state.isCurrent(currentGeneration));
```

- [ ] **Step 2: Run the visual-search test and verify RED because operation state is absent**

- [ ] **Step 3: Add the generation helper, settings combo, provider icon/tooltip refresh, and translated labels**

```cpp
m_visualSearchProviderCombo->addItem(QStringLiteral("Google Lens"), QStringLiteral("google"));
m_visualSearchProviderCombo->addItem(QStringLiteral("Yandex Images"), QStringLiteral("yandex"));
```

- [ ] **Step 4: Replace hard-coded Lens URL creation with `visualSearchResultUrl`; disconnect/abort retired uploaders before deletion and ignore callbacks from stale generations**

```cpp
const quint64 generation = m_visualSearchOperations.begin();
if (!m_visualSearchOperations.isCurrent(generation)) return;
QDesktopServices::openUrl(visualSearchResultUrl(provider, QUrl(url)));
```

- [ ] **Step 5: Run the focused test and full CTest suite; verify GREEN**

### Task 3: Graphical Linux optional-dependency selection

**Files:**
- Create: `src/core/LinuxDependencySelection.h`
- Create: `src/core/LinuxDependencySelection.cpp`
- Create: `tests/LinuxDependencySelectionTests.cpp`
- Modify: `src/ui/FirstRunWizard.h`
- Modify: `src/ui/FirstRunWizard.cpp`
- Modify: `src/ui/SettingsDialog.h`
- Modify: `src/ui/SettingsDialog.cpp`
- Modify: `scripts/linux/runtime-common.sh`
- Modify: `scripts/linux/install-runtime-deps.sh`
- Modify: `packaging/linux/AppRun`
- Modify: `tests/linux/runtime-common-tests.sh`
- Modify: `tests/linux/apprun-integration-tests.sh`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `defaultOcrLanguageCodes`, `linuxDependencyArguments`, and installer CLI `--ffmpeg`, `--ocr`, `--ocr-languages <csv>`, `--desktop`.
- Consumes: system locale, distro package manager, feature checkboxes, and selected OCR language codes.

- [ ] **Step 1: Write failing Qt tests for English/system-language defaults and feature argument generation**

```cpp
QCOMPARE(defaultOcrLanguageCodes("tr_TR"), QStringList({"eng", "tur"}));
QCOMPARE(defaultOcrLanguageCodes("en_US"), QStringList({"eng"}));
QVERIFY(linuxDependencyArguments(true, true, {"eng", "tur"}, true).contains("--ocr"));
```

- [ ] **Step 2: Write failing Bash tests for pacman/apt FFmpeg-only, OCR-only, and language-specific package sets**

```bash
assert_contains 'tesseract-data-tur' "$(eshot_selected_packages pacman 0 1 'eng,tur')"
assert_contains 'tesseract-ocr-tur' "$(eshot_selected_packages apt 0 1 'eng,tur')"
```

- [ ] **Step 3: Run focused Qt/Bash tests and verify RED because selection helpers do not exist**

- [ ] **Step 4: Implement the C++ selection helper and package mapping with unsupported-language filtering**

```cpp
QStringList defaultOcrLanguageCodes(const QString &localeName);
QStringList linuxDependencyArguments(bool ffmpeg, bool ocr,
                                     const QStringList &languages, bool desktop);
```

- [ ] **Step 5: Add the Linux wizard page with default FFmpeg/OCR checks, English plus system language checks, skip/retry feedback, and a Settings entry to reopen it**

- [ ] **Step 6: Update the installer script to parse explicit selections and call `pkexec` only for missing selected packages**

```bash
pkexec pacman -S --needed --noconfirm "${packages[@]}"
pkexec apt-get install -y "${packages[@]}"
```

- [ ] **Step 7: Stop AppRun from performing an all-or-nothing pre-GUI install; leave AppImage integration and normal launch intact**

- [ ] **Step 8: Run dependency Qt tests, all Linux shell tests, `bash -n`, and full CTest; verify GREEN**

### Task 4: AppImage updater/release contract and end-to-end verification

**Files:**
- Modify: `tests/UpdateAssetSelectorTests.cpp`
- Modify: `tests/LinuxUpdateScriptTests.cpp`
- Modify: `tests/linux/package-contract-tests.sh`
- Modify: `.github/workflows/build.yml`
- Modify: `README.md`
- Modify: `LINUX_DEPENDENCY_SETUP.md`

**Interfaces:**
- Consumes: tagged GitHub Release assets, SHA-256 digest, current `APPIMAGE`, and the Task 3 setup flow.
- Produces: co-published Windows x64/ARM64 installers and Linux x86_64 AppImage with verified self-replacement.

- [ ] **Step 1: Add failing contract assertions for the exact Linux asset name, digest-required selection, AppImage smoke test, and Windows/Linux release upload paths**

```bash
grep -F 'packages/EShot-v${APP_VERSION}-x86_64.AppImage' .github/workflows/build.yml
grep -F 'packages/*.AppImage' .github/workflows/build.yml
```

- [ ] **Step 2: Run updater and package-contract tests; verify any missing release-contract assertions fail**

- [ ] **Step 3: Complete workflow/release wiring without changing existing Windows matrix assets and document graphical setup/update behavior**

- [ ] **Step 4: Run full Linux verification**

Run: `cmake --build build-linux --parallel && ctest --test-dir build-linux --output-on-failure && bash scripts/linux/verify-linux.sh`

- [ ] **Step 5: Build the AppImage and smoke-test it with runtime setup disabled**

Run: `bash scripts/linux/build-appimage.sh && ESHOT_SKIP_RUNTIME_SETUP=1 packages/EShot-v3.1.0-x86_64.AppImage --appimage-extract-and-run --test-gif`

- [ ] **Step 6: Run `git diff --check`, inspect all changed files, and confirm `main`, `codex/linux-port`, and `codex/linux-native-package` reachability before any branch deletion**
