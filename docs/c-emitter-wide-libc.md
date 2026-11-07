<!--
Project: FreeBASIC compiler verification
File: c-emitter-wide-libc.md
Purpose: Record alignment-checked native wide-string calls and their evidence.
Responsibilities: Define preserved ABI behavior, target qualification and limits.
This file intentionally does NOT claim all foreign calls accept packed storage.
-->

# Native wide-string calls, November 2, 2026

The corpus runtime's wide helpers call `wcslen` and `wcsncmp` through BASIC
pointers. Their C pointer views promise alignment one, while the native wide
functions require naturally aligned code units. Clang reported 104 warnings
at those calls. The compiler now emits an internal adapter for direct calls
with the matching native C signature.

Aligned pointers retain the original library call. A runtime alignment test
precedes `__builtin_assume_aligned`; that assumption is never applied to the
packed path. The packed path copies one complete code unit into an aligned
local and stops at the appropriate terminator, difference or comparison limit.
Address stepping uses the backend's machine-address integer model. It forms no
native wide lvalue at an unaligned address and performs no allocation.

The native code-unit type and size/count/address types come from the target
C compiler. The adapter's alignment-one typedef preserves compatible function
types. Its declaration and body belong to the existing private reachability
graph, so dead callers do not leave unused adapters behind.

Taking a native function's address is unchanged. Indirect calls through that
address retain the native C pointer preconditions. Source-defined procedures
and incompatible declarations are not replaced. The signature gate excludes
the current DOS/Android byte-wide bindings instead of guessing a native ABI.

## Evidence

The complete current compiler is
`build/c-emitter-wide-libc-20261102/current/fbc.exe`, SHA-256
`c69accc3d86c99dbf294560da4146b4f011b74e474d690dbdda8ead6859bd3f9`.
The one changed compiler object passes the original aggressive GCC/Clang
diagnostics. All 224 compiler C outputs are checked again; the full compiler
build reproduces the candidate binary and the subsequent C fixed point passes.

The standalone contract fixture passes 16 native executions, 20 native object
checks, eight foreign-target objects and six declaration controls. These cover
both C backends, both C compilers, GNU11/GNU23 and O0/O2. A native C companion
checks that `@wcslen` still equals the C library address. Same-name BASIC bodies,
including a definition after its first caller, keep their source behavior in C
emission. Wrong native return types retain native diagnostics.

The unchanged corpus `fb_unicode.bi` passes eight more native cases and ten
object checks with its actual owning header/configuration. Its bounded reference
tests cover all eight tested byte offsets, empty and nonempty strings, high code
units, comparison prefixes and differing data after a terminator. The aligned
scan still uses the native fast path. Coarse clock samples are retained as
observations, not a calibrated performance claim.

Ten target IR checks confirm alignment-one code-unit loads in the fallback,
including Win32/Win64 and 32-bit wide units on Linux x86-64, ARMv7, AArch64 and
Darwin AArch64. Foreign object/IR acceptance does not claim foreign execution.
The regular backend suite passes. Its new structural guard rejects the previous
compiler at the expected missing-adapter check.

The selected queue refresh re-emits all 2,283 non-PNG profiles. Eighty-eight
production C outputs change and pass both original unsuppressed diagnostic
profiles. All 703 unit and 239 additional-project outputs remain byte-identical.

The subsequent [wide-search extension](c-emitter-wide-search.md) supersedes this
checkpoint table and resolves its eight remaining alignment warnings.

| Queue | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Current compiler | 224 | 0 | 4,800 |
| Unit tests / fbcunit | 703 | 16 | 14,347 |
| Production corpus/projects | 1,117 | 490 | 11,118 |
| Additional projects | 239 | 1,810 | 41,138 |
| Total | 2,283 | 2,316 | 71,403 |

Exactly 104 Clang alignment warnings disappear. Other warning groups and GCC
counts remain unchanged. Counts are occurrences across selected build contexts.
At this checkpoint, eight remaining production alignment warnings concern
`wcschr` in trim/search operations. The linked extension resolves them.

## Alternatives and boundaries

A private BASIC-loop replacement for the shared corpus helpers passes its
code-unit tests, but trades each alignment warning for a buffer-usage warning.
It is rejected. The original corpus header remains byte-identical; the private
experiment and its exact source snapshots remain in
`build/c-emitter-wide-helpers-20261102`.

Strict source-only lint is clean for the changed compiler and all BASIC fixtures.
Compiler-backed semantic lint remains unqualified.

The summary, compressed diagnostics and target IR are under
`build/c-emitter-wide-libc-20261102`. Installed binaries and unrelated dirty work
are preserved. The stage retains about 22 MiB under a 64 MiB cap; the rejected
experiment retains less than 1 MiB. PNG profiles remain excluded. Nonlocal ERROR
ownership, descriptor/pointer contracts, other diagnostics and unselected source
contexts remain part of the active campaign.

<!-- end of c-emitter-wide-libc.md -->
