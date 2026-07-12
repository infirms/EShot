#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
source "${repo_root}/scripts/linux/runtime-common.sh"

failures=0

assert_eq() {
  local expected="$1"
  local actual="$2"
  local label="$3"
  if [[ "${expected}" != "${actual}" ]]; then
    printf 'FAIL %s: expected <%s>, got <%s>\n' "${label}" "${expected}" "${actual}" >&2
    failures=$((failures + 1))
  fi
}

assert_contains() {
  local needle="$1"
  local haystack="$2"
  local label="$3"
  if [[ " ${haystack} " != *" ${needle} "* ]]; then
    printf 'FAIL %s: <%s> not found in <%s>\n' "${label}" "${needle}" "${haystack}" >&2
    failures=$((failures + 1))
  fi
}

assert_not_contains() {
  local needle="$1"
  local haystack="$2"
  local label="$3"
  if [[ " ${haystack} " == *" ${needle} "* ]]; then
    printf 'FAIL %s: <%s> unexpectedly found in <%s>\n' "${label}" "${needle}" "${haystack}" >&2
    failures=$((failures + 1))
  fi
}

assert_eq "kde" "$(XDG_CURRENT_DESKTOP=KDE XDG_SESSION_DESKTOP= eshot_desktop_backend)" "KDE backend"
assert_eq "gnome" "$(XDG_CURRENT_DESKTOP=GNOME XDG_SESSION_DESKTOP= eshot_desktop_backend)" "GNOME backend"
assert_eq "kde" "$(XDG_CURRENT_DESKTOP= XDG_SESSION_DESKTOP=plasma eshot_desktop_backend)" "Plasma session backend"
assert_eq "gtk" "$(XDG_CURRENT_DESKTOP=sway XDG_SESSION_DESKTOP=sway eshot_desktop_backend)" "fallback backend"

kde_pacman="$(XDG_CURRENT_DESKTOP=KDE eshot_runtime_packages pacman)"
gnome_pacman="$(XDG_CURRENT_DESKTOP=GNOME eshot_runtime_packages pacman)"
gnome_apt="$(XDG_CURRENT_DESKTOP=GNOME eshot_runtime_packages apt)"

assert_contains "xdg-desktop-portal-kde" "${kde_pacman}" "KDE portal package"
assert_not_contains "xdg-desktop-portal-gnome" "${kde_pacman}" "KDE excludes GNOME portal"
assert_contains "xdg-desktop-portal-gnome" "${gnome_pacman}" "GNOME portal package"
assert_not_contains "xdg-desktop-portal-kde" "${gnome_pacman}" "GNOME excludes KDE portal"
assert_contains "gst-plugin-pipewire" "${gnome_pacman}" "Arch PipeWire GStreamer package"
assert_contains "gstreamer1.0-pipewire" "${gnome_apt}" "Debian PipeWire GStreamer package"
assert_contains "ffmpeg" "${kde_pacman}" "FFmpeg runtime"
assert_contains "tesseract" "${kde_pacman}" "Tesseract runtime"

pacman_ffmpeg="$(eshot_selected_packages pacman 1 0 '')"
pacman_ocr="$(eshot_selected_packages pacman 0 1 'eng,tur,bogus')"
apt_ocr="$(eshot_selected_packages apt 0 1 'eng,tur,bogus')"
assert_contains "ffmpeg" "${pacman_ffmpeg}" "pacman FFmpeg-only"
assert_not_contains "tesseract" "${pacman_ffmpeg}" "pacman FFmpeg-only excludes OCR"
assert_contains "tesseract-data-tur" "${pacman_ocr}" "pacman Turkish OCR"
assert_not_contains "ffmpeg" "${pacman_ocr}" "pacman OCR-only excludes FFmpeg"
assert_not_contains "tesseract-data-bogus" "${pacman_ocr}" "unsupported pacman OCR language"
assert_contains "tesseract-ocr-tur" "${apt_ocr}" "apt Turkish OCR"
assert_not_contains "tesseract-ocr-bogus" "${apt_ocr}" "unsupported apt OCR language"
apt_chinese="$(eshot_selected_packages apt 0 1 'chi_sim,chi_tra')"
assert_contains "tesseract-ocr-chi-sim" "${apt_chinese}" "apt simplified Chinese OCR"
assert_contains "tesseract-ocr-chi-tra" "${apt_chinese}" "apt traditional Chinese OCR"
assert_not_contains "tesseract-ocr-chi_sim" "${apt_chinese}" "apt package names use hyphens"

assert_eq "pacman" "$(ESHOT_PACKAGE_MANAGER=pacman eshot_package_manager)" "forced pacman"
assert_eq "apt" "$(ESHOT_PACKAGE_MANAGER=apt eshot_package_manager)" "forced apt"
assert_eq "/home/test/.local/opt/EShot/EShot.AppImage" \
  "$(HOME=/home/test XDG_DATA_HOME= eshot_installed_appimage_path)" "default AppImage install path"
assert_eq "/home/test/share/applications/io.github.benoks.EShot.desktop" \
  "$(HOME=/home/test XDG_DATA_HOME=/home/test/share eshot_desktop_file_path)" "XDG desktop path"

missing_gnome="$(XDG_CURRENT_DESKTOP=GNOME ESHOT_INSTALLED_PACKAGES='ffmpeg tesseract' \
  eshot_missing_runtime_packages pacman)"
assert_contains "xdg-desktop-portal-gnome" "${missing_gnome}" "missing GNOME portal is detected"
assert_not_contains "ffmpeg" "${missing_gnome}" "installed FFmpeg is excluded"

if (( failures > 0 )); then
  exit 1
fi

printf 'runtime-common tests passed\n'
