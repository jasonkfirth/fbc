<!--
Project: FreeBASIC emitted-C verification
File: c-emitter-crt-conversions.md
Purpose: Record the corpus CRT conversion pointer-contract repair.
Responsibilities: Explain native storage, production callers and current receipts.
This file intentionally does NOT claim whole-corpus or foreign runtime completion.
-->

# Native numeric conversion output pointers, November 2, 2026

The corpus runtime's compatibility header now owns explicit bridges for
`strtod`, `strtof` and `strtoul`. Their native declarations use `char **` output
storage, expressed as `ZSTRING PTR PTR`. The internal BASIC-facing helpers keep
the legacy `BYTE PTR PTR` argument. A non-NULL output uses a native local
`char *`, then copies the returned pointer representation into the caller's
slot. The caller's previous slot is never loaded, and packed slots require no
native pointer-object dereference. A NULL output is passed directly to the CRT.

`crt_extra/stdlib.bi` owns those declarations and transfers. Explicit calls in
`dev_com.bas`, `file_input_float.bas`, `str_convfrom.bas` and
`str_convfrom_uint.bas` select the helpers. The exported runtime function names,
BASIC parsing rules, native conversion results and native entry symbols remain
the same. The public compiler CRT headers and compiler sources are unchanged.
Source headers/footers identify the modified modules, preserving their LF
endings and unrelated source bytes.

The compiler remains
`build/c-emitter-long-literals-20261102/current/fbc.exe`, SHA-256
`65b7bb4bc58d9c2256f991a18ed086e131336046643382be77422ede385c13d9`.

## Qualification

The direct contract fixture checks all eight output-slot offsets, non-pointer
initial slot bytes, surrounding canaries, exact returned addresses, NULL
outputs, no conversion, fractional values, radix selection and unsigned
overflow. Native entry addresses are compared with typed pointers returned by
a separate C translation unit including the real SDK's `stdlib.h`.

The owner fixture includes the actual three VAL/INPUT conversion modules. Its
bounded token source exercises floating and integer INPUT branches, while VAL
checks decimal text, BASIC radix prefixes, D exponents and the existing 0x
rejection. The serial module receives fresh C object checks. Serial-device and
file-device execution are outside this receipt.

Both C backends and C compilers pass GNU11/GNU23, O0/O2 and ST/MT configurations:
64 ordinary native runs and 80 native objects, including 16 Win32 objects.
Sixteen additional SDK views check exact function-type compatibility with the
real declarations and execute again, for **80 native runs and 96 native/SDK
objects** overall. Eight foreign objects cover Linux x86-64, ARMv7, AArch64 and
Darwin AArch64 through both C backends. Foreign execution is not claimed.

The SDK header dependencies and runtime archives are pinned. The unchanged
public CRT header remains a negative control and still reports three library
redeclaration warnings when used directly. Its broader compatibility contract
is separate from these internal runtime helpers.

Strict source-only lint reports zero errors and five existing presentation
whitespace warnings. Their actual source bytes match the preimages. Three old
missing-final-newline warnings are resolved by the file footers. The fixtures
and new bridge introduce no lint findings.

## Selected queue refresh

All 2,059 non-compiler, non-PNG profiles are re-emitted. The shared header changes
940 production C outputs, all freshly checked by GCC and Clang using their
original unsuppressed profiles. The remaining 1,119 emitted outputs are
byte-identical. The 224 compiler receipts are rebound to their unchanged full
source, compiler and tool inputs without rebuilding the compiler.

The earlier receipt had omitted 14 corpus compatibility headers from its
protected input map. This refresh binds the complete current corpus header
set, including those 14, and re-emits every project profile rather than using
the historical omission as input-identity evidence.

An unrelated edit to `tests/services/android-poll.bas` arrived after the refresh.
It is absent from the selected source list, and the filename-reference scan
finds only its own identifying comments. The edit is preserved. The original
refresh inputs remain in `inputs_at_refresh`; the current protected map and
`unselected-input-rebind.json` record the subsequent hash without claiming a
new compile of that unselected standalone test.

| Queue | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Current compiler | 224 | 0 | 4,801 |
| Unit tests / fbcunit | 703 | 16 | 14,347 |
| Production corpus/projects | 1,117 | 490 | 11,092 |
| Additional projects | 239 | 1,805 | 41,133 |
| Total | 2,283 | 2,311 | 71,373 |

All twelve selected Clang `-Wincompatible-library-redeclaration` occurrences
are removed. No other diagnostic group changes. Twelve PNG profiles remain
excluded. Receipts are under `build/c-emitter-crt-conversions-20261102`, with a
32 MiB retained-stage limit. Installed binaries and unrelated dirty work are
preserved. The overall warning campaign remains incomplete.

## Alias-address check at this checkpoint

A small additional probe declares the same `strtod` native entry under two
BASIC names, then compares their addresses. Both C backends pass when their C
is compiled by GCC. Both fail when compiled by Clang, and GAS64 also fails.
The unchanged compiler and the probe's emitted C are retained in the observation
receipt; the probe imports no corpus header. The later
[procedure-address repair](c-emitter-alias-identity.md) closes this check with
the accepted compiler; the failures here remain historical controls.

The final CRT identity tests obtain the SDK pointer through a separate C
translation unit. They verify actual native addresses without relying on a
same-translation-unit comparison of distinct C identifiers with one assembler
label. The failed direct SDK comparison is also retained in the earlier test
database; it is not counted as a passing check.

Nonlocal ERROR ownership, descriptor/pointer contracts, public CRT header
compatibility, other diagnostics, unselected source contexts and compiler-backed
semantic lint remain separate open requirements.

<!-- end of c-emitter-crt-conversions.md -->
