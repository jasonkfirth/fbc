<!--
Project: FreeBASIC compiler verification
File: c-emitter-initializers.md
Purpose: Explain static C aggregate initialization and its qualification boundary.
Responsibilities: Document member selection, zero defaults, array shape and wide constants.
This file intentionally does NOT claim warning-free compiler or platform acceptance.
-->

# Static C aggregate initializers

The C backend names record members in both supplied values and trailing defaults.
This preserves BASIC's flattened access to anonymous structure and union members
without asking the C compiler to infer omitted braces. Member names come from the
same symbols used to declare the C fields. The existing declarations, layout
assertions and public emitter callbacks remain in place.

`src/compiler/backend/c/ir-hlc.bas` owns this translation. Each initializer scope
records its symbol, record type, array role and number of emitted elements. Scope
nodes belong to the current initializer and are released at its end or before the
next initializer. Allocation failure stops declaration emission. Parser recovery
after an existing BASIC error discards unfinished scopes without adding an
internal compiler error.

## Arrays and omitted members

An array of pointers must be classified as an array before considering its pointer
element type. Nested defaults initialize one complete element at each C array
dimension; C supplies zeroes for the remaining array elements. Record elements
name their initializable fields, skipping alternate union fields and synthetic
dynamic-array fields. Layout padding belongs to the C compiler.

The dimension count comes from the emitted declarator, which also includes fixed
strings' character-array dimension. It uses bounded string operations rather than
indexing the declaration buffer.

Synthesized dynamic-array descriptors need the same treatment. Their dimension
table is an array of records, so its empty initializer must retain both levels:

```c
.DIMTB = { { .ELEMENTS = 0, .LBOUND = 0, .UBOUND = 0 } }
```

This changes the spelling of the zeroed table. The supplied descriptor dimensions,
flags, element length and pointer values retain their original values. The focused
control declares an unallocated array, checks its empty bounds, then redimensions
it to two dimensions with negative and positive lower bounds and checks its values.
User-written empty initializer lists are rejected by the BASIC parser; the
descriptor reduction exercises the actual synthesized initializer instead.

## Wide constants

A 128-bit static value must be appended before its separator. The old callback
put the separator first, producing a leading comma and joining a following narrow
field to the wide expression. The regression includes unsigned arrays, a signed
all-bits-set value between narrow fields, and an omitted wide member.

Win64 uses its existing native C wide type. The Win32 representation retains its
two 64-bit words. This repair does not change either representation or its ABI.

## Qualification

The `compiler-warnings-v63` receipts retain the original C rejection, intermediate
brace failures, source preimages, complete diagnostics and provider hashes. The
R4 focused gate passes twelve GCC/Clang object profiles across Win32/Win64 and six
Win64 native runs at `-O2`. Four changed BASIC files pass strict non-semantic lint
with zero errors, warnings or information findings. The malformed-input control
still fails with source diagnostics and no internal error.

The compiler comparison keeps the same 189 modules, two C producers and two
unsuppressed warning profiles. Byte-identical C retains its pinned compilation
proof; changed C is compiled again. The saved prefix of an interrupted comparison
is verified before resumption. Completion of this initializer gate does not close
other warning families, native Win32 qualification, source-model checks, other
backends, or the full fbctests, Exampleageddon, OMA and corpus platform matrix.
The private error-owner prototype is excluded from these compiler and run counts.

The final comparison has 3,919 GCC and 17,485 Clang warnings, down from the pinned
5,681/19,255 baseline. The initializer warning families are zero. Sixteen changed
C profiles are recompiled and 362 retain exact-C proofs; fourteen of the changed
profiles also retain identical object bytes. The Clang HLC object is identical to
the linked component. These are compiler-cohort results, not a clean whole-goal
result. The newly added implementation's own declaration and buffer diagnostics
remain explicit follow-up work.

<!-- end of c-emitter-initializers.md -->
