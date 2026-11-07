<!--
Project: FreeBASIC C backend verification
File: c-emitter-json-contract.md
Purpose: Record the corpus JSON pointer-contract and array handoff repairs.
Responsibilities: State diagnostic deltas, actual runtime scope and source bounds.
This file intentionally does NOT claim complete JSON or corpus acceptance.
-->

# JSON corpus repair, November 2, 2026

The canonical source is
`C:/fb_corpus/fb-ext-lib/src/modules/json/libext-json-object.bas`.
The private parser helpers now receive a read-only buffer pointer by value.
They return cursor positions through `end_` and never rebind the caller's
pointer. Their former BYREF signatures required unsafe nested qualifier
conversions in emitted C. The public JSON interfaces remain unchanged.

Array construction now transfers exactly the parsed entries into a matching
`NEW[]` allocation owned by `JSONarray`. The previous allocation added one byte
while its copy loop included an extra pointer-sized workspace entry. Empty
arrays no longer allocate or iterate an unallocated workspace. Allocation
failure releases the parsed values. String and nested-array branches retain
the next unread delimiter instead of advancing over it; the comma branch
consumes delimiters even when no scalar token is pending.

The exact Win64 production profile loses four GCC `-Wcast-qual` warnings and
four Clang warnings of the same class. Every other diagnostic group is
unchanged. No warning suppressions were added. The composed selected production
totals are now **498 GCC / 11,250 Clang**, across 1,117 non-PNG profiles.
The compiler, unit and additional project cohorts retain their preceding
declaration-repair results. PNG remains excluded.

`build/c-emitter-json-contract-20261102/summary.json` seals the evidence:

- 16 baseline object-input runs and 16 repaired runs across both C producers,
  GCC/Clang, GNU11/GNU23 and O0/O2, using the real JSON modules and dependencies.
- Four further native runs from the canonical source through the reusable
  regression runner. Its test translation unit gates qualifier warnings.
- 32 production object checks covering old/new sources, Win32/Win64,
  GCC/Clang, GNU11/GNU23 and O0/O2. Win32 execution is not claimed.
- Strict source-only lint finds the same three pre-existing findings: a legacy
  maintenance marker, presentation whitespace and missing section banners.
  The new BASIC regression has no findings. Semantic lint is not claimed.

Baseline array execution was excluded because its storage handoff was already
inconsistent. The repaired runtime controls exercise empty arrays, string
arrays, nested arrays, recursive objects, unchanged input and cursor results.
These controls do not qualify every JSON grammar case, file-loading behavior
or BSON conversion. The private preimage replay uses a different debug file
path, so byte identity with the earlier canonical C is not claimed.

The production source-pin comparison finds only this JSON repair and the
previously qualified UDP repair. No other recorded non-PNG source textually
includes this implementation. The other 1,116 production profiles therefore
retain their preceding checks. Existing JSON object binaries, installed
compiler/runtime binaries and compiler sources remain unchanged. No compiler
rebuild or library installation was required. Scratch files and private helper
objects were retired; retained JSON receipts occupy less than one MiB.

The corpus regression is documented in
`C:/fb_corpus/fb-ext-lib/tests/c-backend-regression/json-buffer-contract.md`.
The wider warning campaign and its recorded ERROR-frame, pointer/descriptor
and source-context work remain open.

<!-- end of c-emitter-json-contract.md -->
