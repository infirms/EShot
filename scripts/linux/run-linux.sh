#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
app="${repo_root}/dist-linux/bin/EShot"

if [[ ! -x "${app}" ]]; then
  "${repo_root}/scripts/linux/build-linux.sh"
fi

exec "${app}" "$@"
