<!--
Project: FreeBASIC C backend verification
File: c-emitter-record-layout.md
Purpose: Explain named field offset qualification and the retained natural ABI.
Responsibilities: Record native, negative and selected-queue layout evidence.
This file intentionally does NOT claim bitfield or foreign runtime qualification.
-->

# C record layout checks, November 3, 2026

Whole-record size checks can miss a displaced field when trailing padding keeps
the total size unchanged. The C backend now also emits compile-time offset
assertions for named physical fields, including fields promoted from anonymous
records. Dynamic array pseudo-fields are skipped; their physical descriptor
fields are checked separately. Records emitted as bitfield byte bodies retain
their existing whole-size checks.

The change preserves field declarations, natural padding, alignment and aggregate
ABI types. A permanent negative regression displaces a middle field while
preserving the six-byte record size. Both C producers with GCC and Clang reject
the bad offset in four checks. Removing only that field's offset guard permits
all four same-size controls to compile. Corrupted records are never executed.
Four valid native programs and four Win32 objects pass. Removing the new guards
and normalizing assertion numbering reproduces the previous C in both producer
controls.

The complete current-source compiler passes 224 C fixed points, with zero GCC
and 2,529 Clang warnings. The updated backend regression suite passes. All 2,283
selected non-PNG profiles pass their original unsuppressed C diagnostics. Their
emitted C contains 216,795 offset-assertion occurrences, including earlier offset
checks; this counts profile occurrences, not distinct fields. Totals remain
2,141 GCC and 18,362 Clang warnings. Twelve PNG profiles remain excluded.

An earlier private experiment inserted explicit padding without changing
observed sizes or offsets. Forty native aggregate/callback/varargs programs
passed value checks, but eight mixed-declaration GCC LTO programs produced 64
type-mismatch diagnostics. Eight same-layout LTO controls had none. That rewrite
is rejected, and all 4,702 natural-padding observations remain visible. Receipts
are in `build/c-emitter-explicit-padding-20261103/abi-native/receipt.json` and
`abi-same-layout-control/receipt.json`.

The accepted checkpoint is `build/c-emitter-record-layout-20261103/summary.json`.
Its native and negative controls are in `guard-canonical/receipt.json`; queue
results are in `audit/results.sqlite3`. The stage has a 64 MiB cap, reuses scratch
files and runs native children at below-normal priority. Installed compiler and
runtime binaries and unrelated dirty source bytes are preserved. The broader
warning cleanup remains unfinished.

<!-- end of c-emitter-record-layout.md -->
