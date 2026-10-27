<!--
Project: FreeBASIC compiler verification
File: c-emitter-members.md
Purpose: Describe typed C member recovery and its qualification boundary.
Responsibilities: Record layout rules, expression ownership and remaining warnings.
This file intentionally does NOT claim that warning cleanup is complete.
-->

# Typed C member access

The C emitter now retains declared record fields instead of reconstructing every
field through a byte-pointer cast. The current 189-module compiler graph still
has 8,384 GCC warnings and 19,228 Clang warnings with the aggressive unsuppressed
profiles. This repair is one completed part of the active cleanup.

## Access and ownership rules

`backend/c/c-member-access.bi` matches a constant byte displacement to a declared
field with the same value representation. It handles nested records, fixed array
elements and the shared FBSTRING data/length/size layout. Integer aliases must
have the same size type. Floating/integer reinterpretation is excluded. Pointer
fields retain their declared subtype; byte-text pointer compatibility is limited
to the existing char/ubyte representation.

The search recovers a declared pointer origin through the access-type casts and
constant byte arithmetic introduced by the IR. It checks displacement overflow
and bounds, and stops at integer address arithmetic. Pointer arithmetic needs a
specific check because `typeCBop` records its result as a pointer-sized integer
even though C still evaluates the pointer operand. Multidimensional indices
follow the actual individual C array dimensions, including unequal dimensions.

Packed records, bitfield byte bodies, inline fixed-string storage, dynamic-array
placeholders and unmatched representations retain the existing access paths.
The search does not mutate symbols. Failed searches leave the expression tree
untouched. Successful searches release the replaced cast/arithmetic wrappers
and retain the typed base once, preserving qualifiers and side effects.

BASIC constant records use writable C storage during runtime initialization.
After the frontend has checked a store, the member view permits that store
without removing qualifiers from pointer-valued fields. Read and address
views retain the record's const qualification.

Recovered member types use the same spelling as the emitted C fields. In
particular, an array descriptor for `cva_list` may contain pointers to the
compiler's record representation even when the original BASIC value uses
the C compiler's array-shaped `__builtin_va_list` type.

The member expression owns its path string. Its second child stores displacement
metadata for the existing alignment checks and is released with the tree; that
child is never evaluated in the emitted C. The existing EXPRNODE layout is kept.
The member class also participates in flushing, freeing and expression dumping.

## Qualification

Receipts are under `build/c-emitter-audit-20261022/typed-members-v48/`.
All 378 current-source GCC/Clang object stages pass. With 378 pinned baseline
controls, the comparison contains 756 accepted object stages. Eight identical
after-C objects reuse their existing compilation receipts.

All 378 public-definition sets are unchanged. Complete public-symbol sets match
for 376 pairs. The two HLC objects add the same internal `SYMBGETREALSIZE` import,
resolved by the existing compiler definition. The original failed complete-set
comparison is retained, and the exact import classification is recorded in
`compiler-matrix/validated-results.json`. Both complete 189-object compilers link
against the pinned standard Win64 runtime.

| Compiler-graph diagnostic | v47 | v48 |
| --- | ---: | ---: |
| GCC total warnings | 30,625 | 8,384 |
| Clang total warnings | 63,442 | 19,228 |
| Cast alignment, either compiler | 22,864 | 1,561 |
| GCC cast qualifiers | 3,161 | 2,184 |
| Clang cast qualifiers | 1,457 | 480 |
| Clang unsafe-buffer usage | 25,227 | 3,267 |
| Uninitialized warnings, either compiler | 0 | 0 |

Clang declaration-after-statement observations increase from 6,904 to 6,920
because of the new helper locals. Both compilers also report five additional
sign conversions. GCC unused-but-set observations increase from nineteen to
48. These observations remain in the totals.

Typed array addresses expose five implicit const-discard diagnostics in generated
record-copy loops. Their source element temporaries are declared as mutable
pointers even though the source record is const. The old byte casts discarded
that qualifier explicitly. Correcting the copy-loop temporary types remains
required work; the new diagnostic category is retained, not suppressed.
Declaration placement and the other warning families are also unfinished.

The standalone `tests/quirk/c-typed-members.bas` passes eight before/after native
checks with GCC and Clang at O0/O2. It covers nested records, unequal array
dimensions with nonzero bounds, const references, pointer fields, callbacks,
copies, single evaluation, inherited fields, anonymous unions, packed fields,
bitfields, const initialization and floating/integer raw views. Eight Win32
objects pass with matching before/after symbols; Win32 execution is not claimed.
Four source models validate and form two byte-identical pairs. Clang static
analysis reports no findings for either version of this focused fixture.

Both freshly linked compilers reproduce the executed member C and source models.
The canonical regression also reproduces the executed C. These stages add
sixteen native replays without recompiling identical C.

The existing runtime/callback regressions add eight fresh native checks and four
fresh Win32 object checks. Their four current source-model pairs match exactly.
Older controls crossed the October 24/25 boundary: their only model changes are
the builtin date strings. Fresh controls reproduce the exact previously executed
C and the exact current models. Original failures and calendar deltas remain in
`legacy-contracts/`.

The real fbsound CPU/DSP profiles pass eight O0/O2 objects, and the assembly
contracts pass sixteen native checks. Four Make-selected fbfrog modules pass
sixteen before/after object checks. Their GCC warnings fall from 2,506 to 496 and
Clang warnings from 5,595 to 1,716. This is affected-project qualification, not a
new full-corpus sweep.

## Remaining work

The remaining alignment and qualifier warnings, generated copy-loop pointer
qualifiers, aggregate initialization, sign conversions, declaration placement,
array virtual-origin contract and general
error-handler ownership/lifetime lowering remain open. The private v43 error
outliner must be rebased onto this source epoch before further integration.
The separate eleven semantic-tooling header/footer failures are preserved; none
belongs to these three adopted source files.

Installed binaries and runtime archives are preserved. No installation, commit
or push is part of this repair. Validated model files are compressed losslessly,
and the external corpus is reused in place.

<!-- end of c-emitter-members.md -->
