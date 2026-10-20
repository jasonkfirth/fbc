<!--
Project: FreeBASIC test suite
File: compiler-test-harness.md
Purpose: Explain saved-log reporting and failed-test retry ownership.
Responsibilities: Document commands, generated files, and focused regression coverage.
This file intentionally does NOT qualify the complete language or platform suites.
-->

# Compiler log tests and retries

The non-fbcunit suite uses `tests/log-tests.mk`. Each test log contains a
`TEST_MODE` header and exactly one `RESULT=PASSED` or `RESULT=FAILED` record.
The summary rejects missing or incomplete records and prints the last 100 lines
of each failed log. Summaries retain the log filename even when only one test
fails, so the details come from the log that contains the compiler diagnostics.

From `tests/`, run the normal suite or retry its saved failures with:

```sh
make log-tests FB_LANG=fb
make failed-tests FB_LANG=fb
```

The same commands accept `fblite`, `qb`, and `deprecated`. Omitting `FB_LANG`
runs all four dialects. `failed-tests` cleans, rebuilds, and summarizes only
the tests recorded in `failed-log-tests-<dialect>.inc`.

## Generated files and ownership

A normal run uses `log-tests-log-<dialect>.lst` and
`log-tests-obj-<dialect>.lst` for cleanup. Its failure summary is
`log-tests-results-<dialect>.log`.

A retry uses separate `log-tests-log-failed-<dialect>.lst`,
`log-tests-obj-failed-<dialect>.lst`, and
`log-tests-results-failed-<dialect>.log` files. These lists come from the saved
failure include. They preserve the logs and objects of passing tests, and a
later normal run still uses its full selection. Repeated failure-cache entries
produce only one cleanup or summary path.

If the failure include is missing, the harness reconstructs it from the
`TEST_MODE` headers in saved failed logs. This covers compile-only, runtime,
and multiple-module tests, including modes that expect compilation or execution
to fail. Each cleanup list has its own Make rule and can also be regenerated
independently of the source include.

The lists avoid whole-suite command lines that exceed shell limits. The
harness uses the usual Unix command tools, including `sort -u` to remove
duplicate paths. Haiku's grep compatibility helper supports the forced filename
output used by summaries and is compared with native grep by the focused test.

## Focused verification

From the repository root:

```sh
make compiler-test-harness-test
```

Or select an existing compiler without rebuilding it:

```sh
python build_scripts/test-compiler-test-harness.py --fbc bin/fbc
```

Use `bin/fbc.exe` on Windows. The regression creates one temporary tree and
removes it on exit. Saved-log fixtures exercise selection, cleanup, single and
multiple failure details, duplicate entries, empty retries, and switching back
to a full summary. Small compiler fixtures check success, expected failure,
incomplete records, and timeouts when GNU `timeout` is available. The Haiku
helper checks require GNU awk and run without a Haiku guest.

The 2026-10-20 Windows run passed all 25 checks, including the timeout case,
using the existing `bin/fbc.exe`. Shell syntax and `git diff --check` also
passed. The Haiku helper was checked locally against GNU grep; this batch did
not run on a native Haiku or DOS host.

These checks validate the harness. They do not replace a complete compiler
test run or native execution on other platforms.

<!-- end of compiler-test-harness.md -->
