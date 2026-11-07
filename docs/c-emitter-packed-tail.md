<!--
Project: FreeBASIC compiler qualification
File: c-emitter-packed-tail.md
Purpose: Record packed managed record trailing-layout repair and evidence.
Responsibilities: Bind compiler, native access views, regression checks and limits.
This file intentionally does NOT claim the whole warning campaign is complete.
-->

# Packed managed record trailing layout, November 4, 2026

Valid records containing a STRING or dynamic-array descriptor followed by a byte
fail emitted-C size guards under FIELD=1, 2 and 4. Both GCC and Clang reproduce
the mismatch. BASIC retains natural offsets for direct descriptors, but uses
the requested packing for the enclosing record's trailing size. Natural C
descriptor fields previously raised the enclosing C alignment and trailing size.

The compiler now emits explicit gaps for flat packed managed records and applies
their effective BASIC packing to the enclosing C body. Named fields and every
size/offset assertion remain. Nested wrappers use the corrected C alignment.
BASIC field offsets, native layouts, generated constructor/destructor behavior
and public runtime storage types are unchanged. The previously qualified native
descriptor access views handle the actual unaligned addresses in packed parents
and successive array elements. No example, project or installed binary is edited.

The canonical compiler is `533686899364b1e45a9318fd570c278262301f03c0d11ff054b469b0efeb93fb`.
Its complete 224-unit C fixed point changes only the HLC object and retains
zero GCC and 1,652 Clang warnings. The symbol-layout edit only corrects the
alignment comment, preserving its line count and identical emitted C.

The permanent `c-packed-descriptor-tail.bas` fixture checks separate STRING and
array records, combined descriptors, fixed string arrays, trailing sentinels,
packed and natural wrappers, by-value copies and fixed/dynamic object arrays.
`c-packed-descriptor-init.bas` checks member defaults in shared, static and local
storage. Both are registered in the ordinary backend suite.

Qualification includes:

- 64 alignment-instrumented Win64 executions across four packings, both C
  producers, GCC/Clang, O0/O2 and ST/MT runtime links, with zero alignment reports;
- 88 Win32 native objects and 32 Win32 emitted-program objects, with machine
  headers checked; Win32 execution is not claimed;
- 32 C/GAS64 calls in both directions, including managed by-value arguments and
  record returns at every packing, plus eight initialized-storage executions;
- 50 freshly emitted canonical C variants matching the completed candidate
  checks byte-for-byte; this binds prior executions without counting new ones;
- both regular backend suites, seven procedure-state tests, the structure guard
  and strict BASIC lint on both compiler sources and both new fixtures.

The C profiles retain their aggressive unsuppressed flags. The test matrix still
reports legitimate GNU computed-goto, ABI padding and intentionally underaligned
packed-member diagnostics. There are no alignment reports or C compile errors.
Those synthetic matrix occurrences are not a corpus warning census. Public
packing is not weakened to hide diagnostics.

The first new test attempt used an unsupported direct REDIM expression on an
array element; a BYREF helper corrects the fixture. The initialized-storage test
uses valid member defaults rather than unsupported aggregate constructor syntax.
The native-provider wrapper's main-module selection was also corrected. Failed
attempt logs remain separate; they do not count as passed checks.

Anonymous containers, unions and bitfield byte bodies keep their existing path;
this stage does not claim to repair or qualify every such combination. General
managed/partial construction, user constructors/destructors, alias activation,
object-array construction progress and private ERROR ownership remain open.
The [paired census](c-emitter-packed-queues.md) belongs to bf893b74/f6b0905a,
not this subsequent compiler. Its ordinary private Scorched Earth GCC timeout
remains failed; the three rejected cost experiments are not retried or promoted.
PNG and user-closed optional/native ERROR checks remain excluded. Concurrent
Optical edits are preserved and remain outside the stable source boundary.

Sources, input hashes, compressed logs and exact preimages are recorded in
`build/c-emitter-packed-tail-20261104`. Compiler output and scratch files use E:.
The aggregate retained stage is capped at 32 MiB across C: and E:, with at most
two workers and a 512 MiB free-space floor. Unchanged compiler objects are reused;
runtime archives and assets are not copied.

The subsequent [anonymous-container repair](c-emitter-anonymous-layout.md)
extends this layout contract and preserves natural C declaration compatibility.
Its final compiler is qualified separately, with the flat variants unchanged.

<!-- end of c-emitter-packed-tail.md -->
