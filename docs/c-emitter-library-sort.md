<!--
Project: FreeBASIC C backend library qualification
File: c-emitter-library-sort.md
Purpose: Record the fb-ext-lib sorting callback repair and native evidence.
Responsibilities: Explain callback types, count handling and source diagnostics.
The library owns its implementation and registered regression tests.
-->

# Typed sorting callbacks

Three expanded fb-ext-lib profiles diagnose the default quickSort callback's
function-pointer cast. Its typed element pointers differ from the CRT's
`const void *` parameters. The library also declared the CRT callback result as
native Integer, which is 64 bits on Win64. C qsort requires an int result.

The custom-comparator overload has a separate defect. It casts `@comp`, the
address of its procedure-pointer parameter, into a callable procedure pointer.
The retained original C shows the address of that parameter entering the
data-to-code pointer transport. A callable callback value must come from `comp`.

## Library changes

`ext/algorithms/detail/common.bi` now declares unsigned native-width counts and
a Cdecl callback returning Long. The generic CRT comparator alias uses the same
result type. The typed public comparator API continues to return Integer.

Default sorting continues through CRT qsort. A private typed adapter accepts
the CRT's const-void pointers and returns its 32-bit result. It reuses the
existing default comparator, whose results are -1, 0 and 1.

Custom comparators run through typed in-place partitioning. Each invocation
keeps its own comparator and partition state. It recurses into the smaller
partition and iterates over the larger partition, bounding recursion by the
number of bits in SizeType. Pivot tracking uses existing array elements and
SWAP. Each partition uses a fixed number of scalar and pointer temporaries.

Unsigned indices and address calculations preserve the complete SizeType count
range. Element-span multiplication is checked before calculating addresses.
Empty array/range calls return before forming an element address. The entry
points also return early for null storage, null custom callbacks and overflowing
spans. Qualification replaced the first pilot's signed-index restriction with
unsigned address calculations.

The generic span-overflow comparison is instantiated only for elements larger
than one byte. A byte element cannot overflow the unsigned count when multiplied
by its size. Normal header instantiation exposed FBC warning 56 for the redundant
byte comparison, so the macro now handles that case at preprocessing time.

The existing `tests/sort/qsort.bas` retains its original test and adds custom,
nested, empty-array, subrange and signed/unsigned-byte regressions. All three
edited files have source preimages and compare-before-write publication guards.

## Qualification

The final matrix supplies sixteen actual executions across Win32/Win64, both
C producers, both native compilers and O0/O2. Each checks 1,300 sort cases,
including ordered, reversed, duplicate and deterministic random inputs. It also
checks reentrant comparators, nonzero lower bounds, subranges, managed String
members, signed/unsigned bytes, empty arrays and guarded invalid inputs. Win64
callbacks return values with high bits set. A native CRT call checks the corrected
generic callback alias. The peak owned scratch is 228,174 bytes.

The three original warning owners are recompiled before and after under their
original diagnostic flags. Their strict-cast observations disappear. Clang
retains two startup-registration observations across these owners. The original
declaration headers and original test preimage supply the before controls.
Eight additional native object builds check the final registered regression on
both Windows widths and both C producers/native tools, with zero FBC warnings.

The accurate callback declaration also exposes return mismatches to FBC. A
direct Win64 callback assignment using Integer emits warnings 4 and 41,
including the function-pointer return-type mismatch. Long callbacks compile
cleanly on both widths. The 32-bit Integer callback has the same width and also
compiles cleanly. The diagnostic controls use compiler emission.

Twenty-eight retained header readers receive 56 fresh native builds. The
subsequent unsigned/default-CRT and byte-span refinements each requalify the
sole retained reader with two further native builds. The original retained count
remains **12 GCC / 15,785 Clang**. The expanded library's updated complete count
is tracked in
[current status](c-emitter-current-status.md).

Strict lexical lint of the three published files and runtime fixture reports
zero errors and seven pre-existing whitespace observations on preserved lines.
Required-semantic lint remains unverified. The installed consumer rejects the
current compiler's diagnostic capability records, including on the small
C-long regression control. The same failure occurs when consuming the fresh
sorting export directly. The installed linter executable retains its identity.

Evidence uses the existing `build/c-emitter-forward-views-20261105` work area.
The final runtime receipt is `sort-typed-values-v7-20261114.json.zlib`. The final
registered-test object receipt is
`sort-canonical-unit-objects-v3-20261114.json.zlib`. Source publication uses
`sort-typed-published-v1-20261114.json.zlib` and
`sort-typed-published-v2-20261114.json.zlib`, followed by the byte-span and
regression update in `sort-typed-published-v3-20261114.json.zlib`.
The source-diagnostic receipt is
`sort-callback-source-diagnostics-v1-20261114.json.zlib`. Earlier pilots retain
their recorded failures and source corrections.

<!-- end of c-emitter-library-sort.md -->
