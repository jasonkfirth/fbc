#!/usr/bin/env bash

##############################################################################
# Debian/Ubuntu emulated-native matrix wrapper
##############################################################################

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"

exec "$SCRIPT_DIR/emulated-native-matrix/debianubuntu-build-freebasic-matrix.sh" "$@"

##############################################################################
# end of debianubuntu-build-freebasic-matrix.sh
##############################################################################
