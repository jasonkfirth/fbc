#!/usr/bin/env bash
# FreeBASIC classic AmigaOS examples
# File: amiga-run-exampleageddon.sh
# Purpose: Invoke the complete compile inventory and unattended guest workflow.
# Responsibilities: Preserve command arguments and return the workflow status.
# This file intentionally does NOT contain example classification or SDK policy.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec python3 "$SCRIPT_DIR/amiga-test.py" exampleageddon "$@"
# end of amiga-run-exampleageddon.sh
