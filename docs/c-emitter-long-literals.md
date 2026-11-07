<!--
Project: FreeBASIC compiler verification
File: c-emitter-long-literals.md
Purpose: Record long string storage and its C diagnostic qualification.
Responsibilities: Explain lowering, byte and code-unit checks, and audit scope.
This file intentionally does NOT claim foreign runtime or whole-corpus completion.
-->

# Long literal storage, November 2, 2026

The C backend emits array initializers for string data exceeding 4,095
characters. ISO C's minimum supported string length applies after adjacent
quoted tokens are concatenated, so splitting a long literal into quoted pieces
does not resolve this warning. Short strings retain their existing spelling.
Assembler directives and symbol aliases still use the quoted syntax their
interfaces require.

`ir-hlc.bas` owns the limit and numeric narrow initializer. It retains embedded
zeros, converts high bytes explicitly to C `char`, and emits the final zero.
The same initializer handles long static ZSTRING values. The existing numeric
wide initializer now also handles long UTF-16 literals and explicitly splits
supplementary code points into surrogate pairs. UTF-32 retains one code unit per
code point.

`c-literal-storage.bi` gives long expression literals module-owned storage,
using the existing symbol identity, address mutability and reachability rules.
The change does not coalesce separately folded BASIC literal symbols. Repeated
calls returning one literal retain that literal's address and writable storage.
The test checks global pointer contents separately from procedure-local literal
identity.

The complete compiler is
`build/c-emitter-long-literals-20261102/current/fbc.exe`, SHA-256
`65b7bb4bc58d9c2256f991a18ed086e131336046643382be77422ede385c13d9`.
The source map differs from the preceding wide-search compiler only in these
two C backend files. Its HLC object passes the original unsuppressed GCC and
Clang profiles. A full current compiler build reproduces the candidate binary,
and all 224 compiler C fixed-point checks pass.

## Qualification

`tests/compiler-support/c-long-literals.bas` checks 4,095/4,096-byte boundaries,
every byte of a repeated binary pattern, embedded zeros, values above 127,
static record members, global pointers, dynamic ASCII string copies, long wide
data, terminators and writes through returned literal addresses.

The driver passes 20 native runs and 24 native object checks. Both C backends
and both C compilers run GNU11/GNU23 at O0/O2, plus an unsigned plain-char mode.
Win32 has object checks. Eight foreign objects cover Linux x86-64, ARMv7,
AArch64 and Darwin AArch64 through both C backends. Their emitted UTF-32 arrays
are checked against all expected code units, including the final zero. Foreign
execution is not claimed.

The previous compiler reproduces the long-string warning. The regular backend
suite passes with the new fixture, and its new unsuppressed diagnostic guard
rejects the previous compiler. Strict source-only lint reports zero errors and
warnings; structure checks pass 393 documented files and 21 host graphs.

The selected refresh re-emits all 2,283 non-PNG profiles. Changed C is compiled
with the original unsuppressed GCC and Clang profiles. Unchanged C retains its
prior accepted diagnostic receipts after verifying byte identity.

| Queue | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Current compiler | 224 | 0 | 4,801 |
| Unit tests / fbcunit | 703 | 16 | 14,347 |
| Production corpus/projects | 1,117 | 490 | 11,110 |
| Additional projects | 239 | 1,805 | 41,133 |
| Total | 2,283 | 2,311 | 71,391 |

Five GCC and five Clang `-Woverlength-strings` occurrences are removed from
Stunts and the corpus linter. Clang also reports one new compiler
`-Wunsafe-buffer-usage` occurrence for the numeric narrow initializer's byte
read. Its loop is bounded by the decoded literal length, and the final zero
is emitted separately. This observation is retained without suppression;
the net Clang reduction is four. No other diagnostic group changes. These are
selected build contexts, not all source files or all possible configurations.
No Stunts graphics session or corpus-linter application run is claimed.

Receipts are retained under `build/c-emitter-long-literals-20261102` with a
64 MiB stage limit. Installed tools, canonical corpus/project sources and
unrelated dirty work are preserved. Twelve PNG profiles remain excluded.
Nonlocal ERROR ownership, descriptor/pointer contracts, other diagnostics,
unselected contexts and compiler-backed semantic lint remain open.

<!-- end of c-emitter-long-literals.md -->
