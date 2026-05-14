#!/usr/bin/env bash

##############################################################################
# Alpine emulated-native matrix wrapper
##############################################################################

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"

exec "$SCRIPT_DIR/emulated-native-matrix/alpine-freebasic-matrix-build.sh" "$@"

##############################################################################
# end of alpine-freebasic-matrix-build.sh
##############################################################################
