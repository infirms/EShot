#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${script_dir}/runtime-common.sh"

ffmpeg=0; ocr=0; desktop=0; integrate_appimage=0; languages=eng
while (( $# )); do
  case "$1" in
    --ffmpeg) ffmpeg=1;; --ocr) ocr=1;; --desktop) desktop=1;;
    --integrate-appimage) integrate_appimage=1;;
    --ocr-languages) shift; [[ $# -gt 0 ]] || exit 2; languages="$1";;
    *) eshot_show_error "$(eshot_setup_text unknown_option "$1")"; exit 2;;
  esac
  shift
done

trap 'status=$?; printf "[EShot setup] exit status: %d\n" "${status}" >&2' EXIT
packages=()
manager=""
if (( ffmpeg || ocr || desktop )); then
  manager="$(eshot_package_manager)" || {
    eshot_show_error "$(eshot_setup_text unsupported_manager)"
    exit 1
  }
  read -r -a packages <<<"$(eshot_missing_selected_packages "${manager}" "${ffmpeg}" "${ocr}" "${languages}" "${desktop}")"
fi

printf '[EShot setup] attempted packages: %s\n' "${packages[*]:-(none)}" >&2
if (( ${#packages[@]} > 0 )) && ! command -v pkexec >/dev/null 2>&1; then
  eshot_show_error "$(eshot_setup_text missing_pkexec)"
  exit 1
fi

if (( ${#packages[@]} > 0 )); then case "${manager}" in
  pacman)
    pkexec pacman -S --needed --noconfirm "${packages[@]}"
    ;;
  apt)
    pkexec apt-get update
    pkexec apt-get install -y "${packages[@]}"
    ;;
  dnf)
    pkexec dnf install -y "${packages[@]}"
    ;;
esac; fi

if (( integrate_appimage )); then
  if [[ -z "${APPIMAGE:-}" || ! -x "${APPIMAGE}" ]]; then
    eshot_show_error "$(eshot_setup_text integration_unavailable)"
    exit 1
  fi
  "${APPIMAGE}" --integrate-only
fi
