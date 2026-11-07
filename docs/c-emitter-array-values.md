<!--
Project: FreeBASIC C backend verification
File: c-emitter-array-values.md
Purpose: Explain native-width scaling before actual array value accesses.
Responsibilities: Record physical storage, instrumentation, ABI and queue qualification.
This file intentionally does NOT claim general source bounds or lifetime correctness.
-->

# Native array values, November 3, 2026

The earlier address repair preserved native-width arithmetic when forming array
addresses. Actual dynamic fixed-array reads and writes still used C subscripting.
That performs pointer scaling in C, where an overflowing logical index can be
undefined even when native unsigned scaling wraps to a valid owned physical slot.

A three-element LONG array reproduces the difference. Its high-bit index scales
to the second element on GAS64. Both old C producers appear to work without
instrumentation, but both trap under Clang array/pointer checks. No access leaves
the declared physical array. This is a code-generation difference, not evidence
that every ordinary array warning described a bad source access.

Dynamic value accesses now scale and add in the native unsigned integer width
before converting the result address to the real element pointer type. Both
operands occur once. Existing pointer views preserve packed alignment, pointer
depth and qualifiers. The complete dereference is parenthesized so array-held
callbacks remain callable. Constant accesses retain their declared C indexing;
declarations, record layouts, descriptors and external ABI types are unchanged.

The permanent `c-array-values.bas` regression covers scalar widths, floating
values, pointers, callbacks, packed six-byte records, larger records, unaligned
member arrays, strings, local negative/positive/minimum bounds and one evaluation.
Its canonical receipt passes:

- 24 native C programs across both producers, GCC/Clang, GNU11/GNU23 and O0/O2,
  including Clang array, pointer-overflow and alignment instrumentation;
- four checked native programs and four expected bounds-error cases;
- three GAS64 controls and four Win32 objects;
- a historical instrumented trap control.

Checked BASIC bounds behavior is retained: a large logical index is rejected
under `-exx` before the physical access. Unchecked mode preserves native scaling.
The regression does not execute outside owned storage. Foreign objects are not
foreign runtime executions.

The current compiler passes all 224 C fixed points with zero GCC and 1,652 Clang
warnings. The updated backend suite and eight semantic tests pass. The original
eight-test semantic batch reached its 180-second limit; two bounded partitions
complete every method while preserving the already-passing native/backend gates.

All 2,283 selected non-PNG profiles pass their original unsuppressed C checks.
Clang access observations fall from 9,323 to zero; every other warning class is
unchanged. Totals are 2,141 GCC and 8,763 Clang warnings. All 216,795 existing
offset-assertion occurrences still compile. Twelve PNG profiles remain excluded.
This is not a whole-program proof of source bounds, allocation limits or lifetime.

A separate ordinary-index loop uses a one-MiB working set. Twenty alternating
native executions produce matching results. Candidate/previous median elapsed
ratios are about 1.015 for GCC and 1.001 for Clang on this host. The first short,
noisier measurements remain preserved. This is one workload, not an application
performance claim.

The accepted receipt is `build/c-emitter-array-values-20261103/summary.json`.
The directory also contains canonical compiler, native, semantic partition,
benchmark and whole-queue receipts. Physical C diagnostic mapping was qualified
separately in `build/c-emitter-access-shapes-20261103/`: Clang's display-only
`-fno-diagnostics-use-presumed-location` maps 4,101 observations to visible fixed
arrays without changing C bytes or warning counts. The earlier ambiguous logical
line snippets remain rejected.

Installed binaries and unrelated dirty source bytes are preserved. The stage
has a 64 MiB cap, reuses scratch outputs and runs at most two heavy workers at
below-normal priority. Superseded private binaries are compressed only after
decompression/hash verification. The remaining warning cleanup stays open.

<!-- end of c-emitter-array-values.md -->
