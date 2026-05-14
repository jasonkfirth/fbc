#!/usr/bin/env bash

##############################################################################
# Alpine emulated-native package test matrix wrapper
##############################################################################

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"

exec "$SCRIPT_DIR/emulated-native-matrix/alpine-test-freebasic-matrix.sh" "$@"

##############################################################################
# end of alpine-test-freebasic-matrix.sh
##############################################################################
