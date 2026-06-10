#!/usr/bin/env bash
#
# Project: FreeBASIC package build scripts
# ----------------------------------------
#
# File: aur-stage-freebasic.sh
#
# Purpose:
#
#     Stage the repository's AUR package files into out/ so they can be copied
#     into an AUR checkout without touching the Docker package matrix.
#
# Responsibilities:
#
#     * copy the AUR-facing package files
#     * optionally regenerate .SRCINFO in the staged directory
#     * print the next manual AUR upload steps
#
# This file intentionally does NOT contain:
#
#     * package compilation
#     * AUR git push/commit operations
#     * Docker package matrix logic
#

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE_DIR="${ROOT}/contrib/pkg/aur/freebasic"
OUT_DIR="${ROOT}/out/aur/freebasic"
REFRESH_SRCINFO=0

usage() {
	cat <<'EOF'
Usage: build_scripts/aur-stage-freebasic.sh [--refresh-srcinfo]

Stages the FreeBASIC AUR package files under out/aur/freebasic.

Options:
  --refresh-srcinfo  Run makepkg --printsrcinfo in the staged package.
  -h, --help         Show this help.
EOF
}

while [ $# -gt 0 ]; do
	case "$1" in
		--refresh-srcinfo)
			REFRESH_SRCINFO=1
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			echo "unknown option: $1" >&2
			usage >&2
			exit 1
			;;
	esac
	shift
done

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

cp "${SOURCE_DIR}/PKGBUILD" "${OUT_DIR}/"
cp "${SOURCE_DIR}/.SRCINFO" "${OUT_DIR}/"
cp "${SOURCE_DIR}/.gitignore" "${OUT_DIR}/"
cp "${SOURCE_DIR}/README.md" "${OUT_DIR}/"
cp "${SOURCE_DIR}/update-srcinfo.sh" "${OUT_DIR}/"
chmod +x "${OUT_DIR}/update-srcinfo.sh"

if [ "${REFRESH_SRCINFO}" -ne 0 ]; then
	(
		cd "${OUT_DIR}"
		./update-srcinfo.sh
	)
fi

cat <<EOF
Staged AUR package files in:
  ${OUT_DIR}

Suggested next steps:
  1. Replace sha256sums=('SKIP') with the real release checksum before upload.
  2. Run ./update-srcinfo.sh after any PKGBUILD metadata change.
  3. Copy the staged files into ssh://aur@aur.archlinux.org/freebasic.git.
EOF

# end of aur-stage-freebasic.sh
