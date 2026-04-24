#!/bin/sh

set -eu

##############################################################################
# Packages to check
##############################################################################

PKGS="
gmake
gcc
git
ncurses
libffi
libX11
libXext
libXrender
libXcursor
mesa-libs
"

##############################################################################
# Ensure pkg is ready
##############################################################################

echo "==> updating pkg database"
pkg update -f >/dev/null

##############################################################################
# Check packages
##############################################################################

echo
echo "=================================================="
echo "==> checking package availability"
echo "=================================================="

for p in $PKGS; do
    echo
    echo "----------------------------------------"
    echo "checking: $p"
    echo "----------------------------------------"

    # Exact match
    EXACT=$(pkg search -e "^${p}$" 2>/dev/null || true)

    if [ -n "$EXACT" ]; then
        echo "exact match:"
        echo "$EXACT"
        continue
    fi

    echo "no exact match"

    # Partial / fuzzy matches
    echo "possible matches:"
    pkg search "$p" | head -n 10 || true
done

echo
echo "==> done"
