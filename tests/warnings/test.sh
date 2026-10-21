#!/usr/bin/env bash
# FreeBASIC test suite: test.sh
# Purpose: Check maintained warning reports without changing their references.
# Responsibilities: Preserve the historical shell entry point and environment.
# This file intentionally does NOT generate compiler output in the source tree.

# Use --update only when reviewing an intentional diagnostic change.
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
exec "${PYTHON:-python3}" "$script_dir/../../build_scripts/check-warning-reports.py" "$@"

# end of test.sh
