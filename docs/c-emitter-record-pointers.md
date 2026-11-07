<!--
Project: FreeBASIC compiler qualification
File: c-emitter-record-pointers.md
Purpose: Record shared C record dependency pruning and its verification.
Responsibilities: Distinguish omitted unused declarations from actual ABI layouts.
This file intentionally does NOT claim a refreshed whole-corpus warning census.
-->

# Pointer-only record dependencies, November 5

The [completed paired census](c-emitter-current-queues.md) now measures the
common-coverage reduction. The later [declaration-scope repair](c-emitter-record-scope.md)
preserves outer tag identity when completing a local BASIC record. The compiler
identities and sample-only limits below describe the original component.

The C emitter now writes a forward declaration when a record body or one of
its callback types needs only another record's pointer. It completes that
record normally when a by-value field, real object, access or alignment view
requires its layout. The change is in `hEmitStruct()` in
`src/compiler/backend/c/ir-hlc.bas`. Required fields, padding, alignment and
calling conventions are unchanged. No existing example program was edited.

This addresses the largest remaining Clang warning class through a shared
emission rule. It does not pack, reorder or hide diagnostics on real records.

| Measured body | Before Clang padding | After Clang padding |
| --- | ---: | ---: |
| 224 compiler units, per compiler cut | 1,639 | 1,506 |
| Eight unchanged GTK, variant and SDL3 profiles, per cut | 389 | 310 |

The compiler totals are now **0 GCC / 1,519 Clang**, including the unchanged
13 required startup observations. Each cut retains all 224 C fixed points.
The eight-profile comparison preserves its four GCC and five Clang function
cast observations. Both variant-library samples drop from 16 padding warnings
to zero. These sample reductions are not extrapolated to the whole corpus.

The canonical compiler built from the promoted root source is
`80f288608f189c3f7b193524ba89a6707dd88dc9b17e1f998be1282ad8caa536`.
The private compiler is
`fa3648f496a96d50c09641e0133fde390dcb66817f41840750f4fe11ea75b918`.
Its earlier ownership changes remain private. Both staged fixed points also
reproduced their one-unit bootstrap binaries exactly. All 436 regenerated
compiler objects matched the corresponding bootstrap graph objects; a separate
root-source rebuild changes the compiler's own diagnostic filename constant.

The new permanent regression checks opaque pointer fields, recursively linked
records, later completion, local type reuse, pointer strides, packed access,
callbacks and ordinary C aggregate arguments and returns. The current compiler
pair passes 40 instrumented executions across both C producers, GCC/Clang,
GNU11/GNU23 and O0/O2, including eight mixed-declaration GCC LTO checks.
Twelve baseline controls also pass. Twenty earlier canonical prototype runs
are retained separately, and the four backend suites add 24 ordinary runs of
the permanent regression. Instrumentation covers these generated programs and
their C observers, not the whole runtime archive.

Both cuts also pass Win32 object compilation with both producers, both C
tools and both standards, including eight combined GCC LTO objects across the
pair. No Win32 execution is claimed. Four backend suites, 14 private semantic
checks, the structure guard and strict lint of all four affected/provider BASIC
files pass. The existing ERROR suite is freshly reexecuted: 132 successful
runs and eight expected illegal-RESUME exits, with no new warning classes.

The dedicated observer intentionally uses C11 static assertions and natural
record padding. Their Clang pre-C11 compatibility and padding observations are
retained. The fixture initially initialized an opaque pointer field in BASIC;
that ordinary operation correctly requested a complete alignment view. The
accepted control lets C initialize the header before any read and checks the
pointer-only declaration rule without that additional request.

Two verification jobs timed out during observed host CPU contention. The
unchanged compiler-unit command then passed in 7.215 seconds after the live
host sample showed idle capacity. Its original 90-second deadline and flags
were retained. Both backend suites subsequently pass. A source-binding attempt
also correctly detected the changed diagnostic filename when moving a source
copy back into the root tree; the final root compiler is rebuilt and all 224
C identities are freshly checked. Failed attempts remain in the receipts.

Evidence is in `build/c-emitter-record-pointers-20261105`, with owned scratch
on E:. Two heavy workers, reusable per-job C/object outputs, a 512 MiB volume
free-space floor and a 64 MiB retained component limit bound this batch.
127 byte-identical native files share immutable hard links, removing
13,578,518 duplicate logical bytes. Cross-volume matches remain separate.
Closed metadata and binaries were compressed with unchanged byte hashes.
Installed compiler/runtime archives and unrelated dirty work are preserved.

The [last complete paired census](c-emitter-current-queues.md) still predates
the identifier/build-context and record-pointer repairs. Rebind the complete
bodies before publishing new queue totals. The separate confirmed variant
library mismatch is repaired in the [native contract batch](c-emitter-variant-abi.md).
The public/implementation raw extents were 16/24 on Win64, with containing
records of 17/32 bytes. Win32 raw extents agreed at 16, but containing records
occupied 17/24 bytes. The shared header now uses the native contract in both
modes; existing caller programs remain unchanged.

PNG, concurrent Optical and user-closed optional investigations remain outside
this batch. Whole-campaign completion and private promotion remain open.

<!-- end of c-emitter-record-pointers.md -->
