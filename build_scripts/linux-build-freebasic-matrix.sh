#!/usr/bin/env bash

##############################################################################
# Linux emulated-native matrix wrapper
##############################################################################

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"

exec "$SCRIPT_DIR/emulated-native-matrix/linux-build-freebasic-matrix.sh" "$@"

##############################################################################
# end of linux-build-freebasic-matrix.sh
##############################################################################
