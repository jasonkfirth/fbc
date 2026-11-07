<!--
Project: FreeBASIC compiler verification
File: c-emitter-wide-search.md
Purpose: Record native wide-character search alignment and pointer-return checks.
Responsibilities: Explain the adapter extension, owning modules and current receipts.
This file intentionally does NOT claim foreign runtime execution or whole-corpus completion.
-->

# Native wide-character search, November 2, 2026

The checked-call adapter now covers direct `wcschr` calls with the native C
signature. Naturally aligned inputs still call the C library. Packed inputs
copy one native code unit at a time, return the first matching original address,
and return NULL at an unmatched terminator. Searching for zero returns the
terminator's address. The returned pointer keeps the native API's mutable
pointer type and the backend's minimum-alignment view.

The signature gate checks the scalar search-character type and mutable pointer
return as well as the input pointer. Incompatible const-return declarations are
left to the C compiler's diagnostics. Same-name BASIC bodies, including later
definitions, are retained. Native procedure addresses are unchanged; callbacks
through those addresses still require inputs satisfying the native C contract.

The complete compiler snapshot is
`build/c-emitter-wide-search-20261102/current/fbc.exe`, SHA-256
`451b753fc68f0a5516d75ea9f40ffd8f5b41e98fefe4164e0c993e7d4445e1b2`.
Only `c-wide-libc.bi` changes in the compiler source map. Its owning HLC object
passes the original unsuppressed GCC/Clang profiles, the full compiler build
reproduces the candidate binary, and all 224 compiler C fixed-point checks pass.

## Qualification

The standalone fixture passes 16 native runs, 20 native object checks, eight
foreign objects and six declaration-control contexts. It checks first matches,
misses, terminators, high units, native function-address identity, and writes
through returned packed pointers. Both C backends and both C compilers are
checked in GNU11/GNU23 and O0/O2 native modes. The extended declarations test
same-name definitions and incompatible search return types.

The corpus fixture includes the actual `strw_ltrimany.bas`, `strw_rtrimany.bas`
and `strw_trimany.bas` implementations. It passes 32 native runs and 40 object
checks across both backends, C compilers and ST/MT configurations. Independent
source/pattern offsets cover all native alignment combinations. Cases include
left/right/both trims, high units, empty patterns, fully trimmed strings, empty
strings and the functions' NULL-source behavior. Every allocated result is
checked and freed. No production corpus implementation or header is changed.

Ten target IR checks verify alignment-one code-unit loads and alignment-one
stores of the test value through returned pointers. These cover Win32/Win64 and
Linux x86-64, ARMv7, AArch64 and Darwin AArch64. Foreign acceptance is object/IR
qualification only. The regular backend suite passes; its strengthened guard
rejects the preceding compiler because that compiler lacks the search adapter.

The selected refresh re-emits all 2,283 non-PNG profiles. Six production C files
change and pass both original unsuppressed diagnostics. The other production
outputs, all 703 unit outputs and all 239 additional-project outputs are
byte-identical to the preceding accepted C.

| Queue | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Current compiler | 224 | 0 | 4,800 |
| Unit tests / fbcunit | 703 | 16 | 14,347 |
| Production corpus/projects | 1,117 | 490 | 11,110 |
| Additional projects | 239 | 1,810 | 41,138 |
| Total | 2,283 | 2,316 | 71,395 |

Eight Clang alignment warnings disappear without changing other diagnostic
groups. Combined with the preceding [length/comparison repair](c-emitter-wide-libc.md),
the selected queues now contain no `-Walign-mismatch` occurrences. Other native
APIs, native callback input contracts, unselected contexts and the broader
descriptor/pointer work are separate acceptance boundaries.

Strict source-only lint is clean. Structure checks pass 393 files and 21 host
graphs. The summary and compressed receipts are under
`build/c-emitter-wide-search-20261102`, limited to 64 MiB of retained files.
Installed binaries, canonical corpus modules/headers and unrelated dirty work
are preserved. Twelve PNG profiles remain excluded. The overall campaign,
including nonlocal ERROR ownership and compiler-backed semantic lint, remains open.

<!-- end of c-emitter-wide-search.md -->
