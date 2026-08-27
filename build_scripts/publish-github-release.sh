#!/usr/bin/env bash

##############################################################################
# FreeBASIC XL GitHub Release publisher
##############################################################################
#
# Publish package files already downloaded from a GitHub Actions run.
#
# Responsibilities:
#
#   * create the requested GitHub Release when it does not already exist
#   * give duplicate package basenames stable path-qualified names
#   * upload every selected package without duplicating its file data locally
#
# This file intentionally does NOT contain:
#
#   * GitHub Actions artifact download logic
#   * compiler or package build logic
#   * release-tag selection policy

set -euo pipefail

##############################################################################
# Helpers
##############################################################################

die() {
    echo "ERROR: $*" >&2
    exit 1
}

usage() {
    cat <<EOF
Usage: $0 TAG ASSET_ROOT [REPOSITORY]

Publish package files below ASSET_ROOT to TAG. REPOSITORY must use the
OWNER/NAME form and defaults to GITHUB_REPOSITORY.
EOF
}

##############################################################################
# Arguments and prerequisites
##############################################################################

[ "$#" -ge 2 ] && [ "$#" -le 3 ] || {
    usage >&2
    exit 2
}

TAG="$1"
ASSET_ROOT="$2"
REPOSITORY="${3:-${GITHUB_REPOSITORY:-}}"
UPLOAD_ROOT="${ASSET_ROOT%/}-upload"

[ -n "$TAG" ] || die "release tag is empty"
[ -d "$ASSET_ROOT" ] || die "asset root does not exist: $ASSET_ROOT"
[ -n "$REPOSITORY" ] || die "repository is empty"

case "$REPOSITORY" in
    */*) ;;
    *) die "repository must use OWNER/NAME form: $REPOSITORY" ;;
esac

command -v find >/dev/null 2>&1 || die "missing command: find"
command -v gh >/dev/null 2>&1 || die "missing command: gh"
command -v ln >/dev/null 2>&1 || die "missing command: ln"
command -v sort >/dev/null 2>&1 || die "missing command: sort"

[ ! -e "$UPLOAD_ROOT" ] || die "upload staging path already exists: $UPLOAD_ROOT"

##############################################################################
# Release creation
##############################################################################

if ! gh release view "$TAG" --repo "$REPOSITORY" >/dev/null 2>&1; then
    gh release create "$TAG" \
        --repo "$REPOSITORY" \
        --title "FreeBASIC XL $TAG" \
        --generate-notes
fi

##############################################################################
# Flat release-asset namespace
##############################################################################

mapfile -d '' SOURCE_ASSETS < <(
    find "$ASSET_ROOT" -type f \( \
        -name '*.deb' -o \
        -name '*.rpm' -o \
        -name '*.apk' -o \
        -name '*.hpkg' -o \
        -name '*.tgz' -o \
        -name '*.zip' -o \
        -name '*.tar.xz' -o \
        -name '*.pkg' -o \
        -name '*-setup.exe' \
    \) -print0 | sort -z
)

[ "${#SOURCE_ASSETS[@]}" -gt 0 ] || die "no package files were downloaded"

#
# GitHub Release assets share one flat namespace.  Keep familiar package
# names when they are unique, and qualify only duplicate basenames with their
# matrix path so no distribution replaces another.
#
declare -A ASSET_NAME_COUNTS=()

for asset in "${SOURCE_ASSETS[@]}"; do
    name="${asset##*/}"
    count="${ASSET_NAME_COUNTS[$name]:-0}"
    ASSET_NAME_COUNTS["$name"]=$((count + 1))
done

mkdir -p "$UPLOAD_ROOT"

for asset in "${SOURCE_ASSETS[@]}"; do
    name="${asset##*/}"
    upload_name="$name"

    if (( ASSET_NAME_COUNTS["$name"] > 1 )); then
        relative="${asset#"$ASSET_ROOT"/}"
        qualifier="${relative%/*}"
        qualifier="${qualifier//\//-}"
        upload_name="${qualifier}-${name}"
    fi

    [ ! -e "$UPLOAD_ROOT/$upload_name" ] || \
        die "release asset name collision: $upload_name"

    #
    # The Windows package artifact alone is several gigabytes.  A hard link
    # gives gh a flat upload directory without consuming a second copy of the
    # runner's limited workspace storage.
    #
    ln -- "$asset" "$UPLOAD_ROOT/$upload_name"
done

mapfile -d '' ASSETS < <(
    find "$UPLOAD_ROOT" -maxdepth 1 -type f -print0 | sort -z
)

echo "==> uploading ${#ASSETS[@]} package assets to $REPOSITORY release $TAG"

gh release upload "$TAG" "${ASSETS[@]}" \
    --repo "$REPOSITORY" \
    --clobber

##############################################################################
# end of publish-github-release.sh
##############################################################################
