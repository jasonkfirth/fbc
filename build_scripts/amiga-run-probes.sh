#!/usr/bin/env bash
# FreeBASIC classic AmigaOS qualification
# File: amiga-run-probes.sh
# Purpose: Invoke native I/O, graphics, sound, and floating-point checks.
# Responsibilities: Preserve command arguments and return qualification status.
# This file intentionally does NOT contain emulator or compiler implementation.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec python3 "$SCRIPT_DIR/amiga-test.py" probes "$@"
# end of amiga-run-probes.sh
