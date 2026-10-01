#!/usr/bin/env bash
# FreeBASIC classic AmigaOS tests
# File: amiga-run-fbctests.sh
# Purpose: Invoke the reproducible native fbctests workflow.
# Responsibilities: Preserve command arguments and return the workflow status.
# This file intentionally does NOT contain emulator or compiler implementation.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec python3 "$SCRIPT_DIR/amiga-test.py" fbctests "$@"
# end of amiga-run-fbctests.sh
