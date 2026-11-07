<!--
Project: FreeBASIC runtime verification
File: c-emitter-array-objects.md
Purpose: Explain bounded reverse retirement of object-array elements.
Responsibilities: Record native/BASIC parity, ABI checks and qualification limits.
This file intentionally does NOT claim partial-construction or ERROR-owner unwinding.
-->

# Object-array retirement, November 4, 2026

The native runtime and its maintained BASIC translation now decrement the
remaining element count before forming the address passed to a destructor.
The former code calculated `elements - 1` for empty storage and stepped the
pointer before element zero after a complete retirement. Those unused pointer
formations are outside the native C allocation contract.

The physical origin and element stride are captured once. Each visited index
is below the original element count and at least `keep_idx`, so valid array
metadata bounds its byte offset within the allocation. Destruction stays in
strict reverse order. Empty storage and retained prefixes at least as large
as the array form no element address. Allocation ownership, descriptor layout,
callback ABI and constructor-completion behavior are unchanged.

The implementation files are `src/rtlib/arrays/array_destructobj.c` and
`C:/fb_corpus/fbrtLib/array_destructobj.bas`. Existing examples are unchanged.
The source-only repair does not install a runtime or replace compiler binaries.

## Qualification

The permanent `tests/compiler-support/runtime-array-objects.py` driver builds
the real native module and both BASIC C emissions. Its C observer checks exact
reverse callback order, retained bytes, payload deallocation, untouched metadata
and storage sentinels. It includes zero-length storage, null physical pointers,
zero virtual-origin bits, five element strides and oversized retained prefixes.

The current-source matrix passes 42 executions across GCC/Clang, GNU11/GNU23
and O0/O2. These include twelve instrumented executions and twelve links with
the multithread runtime. Two historical behavior controls also pass, for
19,888 API cases in total. The old defect forms an unused invalid pointer;
observable success alone therefore does not establish its absence. The source
change and count-before-address invariant remove both offending formations.

Eighteen Win32 objects pass and exported native/BASIC symbol sets match under
both C tools and widths. Win32 execution is not claimed. Eighty-nine SDK headers
are bound by dependency output. The existing descriptor-validity contract remains.

The separate BASIC fixture passes twelve real-language executions with the
canonical and private compiler/runtime pairs, both C emitters and both C tools.
It checks `REDIM PRESERVE`, `ERASE`, repeated empty erasure, fixed-array
reconstruction and automatic scope cleanup. Each execution constructs and
destroys eleven objects whose managed strings remain valid in their destructor.
Strict source-only Windows lint passes both changed/created BASIC files.

Independent before/current diagnostic comparisons introduce no warnings. The
native SDK Clang check loses one buffer-use observation. Its other SDK/header
observations stay visible. Both changed ST/MT corpus profiles remain at zero
GCC and one Clang natural-padding warning each. That native SDK reduction is
separate from the selected emitted-C totals, which remain 247 GCC/6,501 Clang
for the stable broad queue and 126/7,747 for Exampleageddon. No full queue sweep
was repeated: 8,590 unrelated non-Optical source hashes and the Make unit
membership remain unchanged, and the two affected recipes were freshly checked.

The first native edit added an implicit pointer-conversion diagnostic. Restoring
the existing explicit byte-pointer cast closed it before final qualification.
The first-pass results remain marked as superseded. A driver formatting/help-text
correction preserves its validation AST; its successful executions were not
replayed merely for presentation. The diagnostic guard runs independently on
the saved current-source matrix and additional GCC compile controls.

Receipts, source preimages and the final source cut are under
`build/c-emitter-object-retirement-20261104/`. The native matrix reuses its
small object set within 8 MiB; the whole component stays below 32 MiB with one
below-normal heavy worker. No assets or archive members are copied.

This repair is groundwork for object-array ownership. General managed/partial
construction, destructor errors and private ERROR promotion remain separate
open contracts. The explicit unsupported-cleanup boundary is unchanged. PNG
and the user-closed optional/native investigations remain excluded.

<!-- end of c-emitter-array-objects.md -->
