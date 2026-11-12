<!--
Project: FreeBASIC scalar conversion backends
File: c-emitter-qualified-conversions.md
Purpose: Explain precision selection for read-only floating values.
Responsibilities: Record C helper and LLVM intrinsic contracts and qualification.
This file does not change source constness, public ABI or rounding policy.
-->

# Read-only scalar precision

Const qualification describes access to storage. It must not change the
precision used to round a floating value to an integer. The C expression layer
can retain qualification on an indexed or member read, including inferred
read-only views of ordinary BASIC arrays. Comparing that complete type directly
with `Single` incorrectly selected a Double conversion helper.

The C backend now selects its helper from the scalar type while preserving
qualification on the expression used to read memory. Signed 32-bit conversion
uses `fb_F2I`, signed 64-bit and unsigned 32-bit conversion use `fb_F2L`, and
unsigned 64-bit conversion uses `fb_F2UL`. Native signed/unsigned 128-bit
conversion selects the corresponding Single helper too. Double values retain
their Double helpers. Pointer levels remain part of the type comparison.

LLVM had the same full-type comparison in rounding and unary math selection.
It could emit a `nearbyint.f64` or `sqrt.f64` call with a `float` operand, which
the LLVM verifier rejects. Those selections now match the unqualified SSA value
type. The Double assertion is normalized consistently. No caller warning is
appropriate for a valid read through a Const Single pointer.

## Candidate qualification

Candidate SHA256 is
`56cc73187b5208e476725afd1e7df9cd5f90944c7ae86c02aefb8747dbc2179a`.
All four existing queues now bind this candidate. The selected development
executable remains pinned for the already-running JRPG audit. Installed release
binaries and normal archives have not been replaced by this batch.

The old compiler produces seven native Clang promotion errors in the regression
and invalid LLVM IR in a separate read-only square-root reduction. The candidate
passes 24 focused checks, including six real executions across GCC/Clang O0/O2,
GAS64 and LLVM, and eight native object checks across both C producers and
Win32/Win64. The fixture checks indexed reads once per conversion, reference and
member reads, ties-to-even, and the Double-only integer 16777217. The 128-bit
conversion controls apply on the native 64-bit target.

The compiler self-hosts all 224 units at an exact emitted-C fixed point with
zero GCC warnings. Both complete C backend suites pass in 298.379 seconds,
including 462 source-warning checks. Strict lint reports no findings in the
three owned BASIC files. The completed 208-profile extracted-example queue
removes all 47 Clang double-promotion observations, moving from GCC 3 / Clang 70
to GCC 3 / Clang 23 without changing the examples. The complete existing
6,418-profile cohort accepts 6,312 C units with GCC 198 / Clang 15,969. Compiler
and fbctests remain GCC-clean. Expanded project qualification is still pending
and its partial counts are separate. The selected executable has not been moved
under the already-running JRPG audit.

Evidence is in `build/c-emitter-forward-views-20261105/`: the
`qualified-conversions-baseline-`, `qualified-math-llvm-baseline-`,
`qualified-conversion-selfhost-v2-`, `qualified-conversion-values-v2-`, and
`qualified-conversion-gates-v2-` packets; the
`current-extracted-examples-qualified-conversion-20261112.sqlite3` database;
the complete `qualified-conversion-queue-` packets/database; and
`qualified-conversion-cohort-check-20261112.json.zlib`. The first C-only
candidate's failed LLVM control is retained separately and was never selected.

<!-- end of c-emitter-qualified-conversions.md -->
