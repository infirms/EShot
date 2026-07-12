#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
build_script="${repo_root}/scripts/linux/build-appimage.sh"
workflow="${repo_root}/.github/workflows/build.yml"
desktop_launcher="${repo_root}/scripts/linux/desktop-launch.sh"
installed_launcher="${repo_root}/scripts/linux/eshot-launcher"
source_desktop_entry="${repo_root}/EShot-Linux.desktop"
release_version_script="${repo_root}/scripts/linux/apply-release-version.sh"

grep -F 'EShot-v${version}-x86_64.AppImage' "${build_script}" >/dev/null
grep -F 'linuxdeploy/releases/download/1-alpha-20251107-1/linuxdeploy-x86_64.AppImage' "${build_script}" >/dev/null
grep -F 'linuxdeploy-plugin-qt/releases/download/1-alpha-20250213-1/linuxdeploy-plugin-qt-x86_64.AppImage' "${build_script}" >/dev/null
grep -F 'appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage' "${build_script}" >/dev/null
grep -F 'packaging/linux/AppRun' "${build_script}" >/dev/null
grep -F -- '--appimage-extract-and-run' "${workflow}" >/dev/null
grep -F 'packages/EShot-v${APP_VERSION}-x86_64.AppImage' "${workflow}" >/dev/null
grep -F 'release-assets/*.AppImage' "${workflow}" >/dev/null
grep -F 'packages/*.AppImage' "${workflow}" >/dev/null
grep -F 'release-assets/EShot-Setup-x64/*.exe' "${workflow}" >/dev/null
grep -F 'release-assets/EShot-Setup-arm64/*.exe' "${workflow}" >/dev/null
grep -F 'if: startsWith(github.ref, '"'"'refs/tags/v'"'"')' "${workflow}" >/dev/null
grep -F 'scripts/linux/apply-release-version.sh' "${workflow}" >/dev/null

version_fixture="$(mktemp)"
trap 'rm -f "${version_fixture}"' EXIT
printf 'project(EShot VERSION 3.1.0 LANGUAGES CXX)\n' >"${version_fixture}"
resolved_version="$(bash "${release_version_script}" v9.8.7 "${version_fixture}")"
[[ "${resolved_version}" == "9.8.7" ]]
grep -F 'project(EShot VERSION 9.8.7 LANGUAGES CXX)' "${version_fixture}" >/dev/null
if bash "${release_version_script}" nightly "${version_fixture}" >/dev/null 2>&1; then
  echo 'invalid release tag was accepted' >&2
  exit 1
fi
grep -F 'install-user.sh' "${desktop_launcher}" >/dev/null
grep -F 'ESHOT_WAYLAND_XWAYLAND_OVERLAY=1' "${installed_launcher}" >/dev/null
grep -F 'Terminal=false' "${source_desktop_entry}" >/dev/null
grep -F 'StartupNotify=false' "${source_desktop_entry}" >/dev/null
grep -F 'X-KDE-StartupNotify=false' "${source_desktop_entry}" >/dev/null
grep -F 'export NO_STRIP=1' "${build_script}" >/dev/null
grep -F 'qmake-appimage-wrapper.sh' "${build_script}" >/dev/null
grep -F 'qt-plugin-dir.sh' "${build_script}" >/dev/null
if grep -F '/usr/lib/qt6/plugins' "${build_script}" >/dev/null; then
  echo 'AppImage build hardcodes an Arch-only Qt plugin path' >&2
  exit 1
fi
fixture_bin="$(mktemp -d)"
trap 'rm -f "${version_fixture}"; rm -rf "${fixture_bin}"' EXIT
cat >"${fixture_bin}/qtpaths6" <<'EOF'
#!/usr/bin/env bash
[[ "$1" == "--plugin-dir" ]] && printf '/opt/qt/lib/plugins\n'
EOF
chmod +x "${fixture_bin}/qtpaths6"
[[ "$(PATH="${fixture_bin}:${PATH}" bash "${repo_root}/scripts/linux/qt-plugin-dir.sh")" == /opt/qt/lib/plugins ]]
desktop-file-validate "${source_desktop_entry}"

printf 'package contract tests passed\n'
