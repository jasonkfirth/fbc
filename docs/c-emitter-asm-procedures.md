<!--
Project: FreeBASIC compiler verification
File: c-emitter-asm-procedures.md
Purpose: Explain private ASM procedure retention and ordinary library C names.
Responsibilities: Record native ABI invariants, regression controls and limits.
This file intentionally does NOT claim a complete corpus or selected release.
-->

# Private procedures referenced by inline assembly

A private procedure referenced only by a resolved Intel ASM operand was not
marked accessed. Its body could disappear before either C or native assembly
emission, leaving an undefined symbol. Keeping the AST body alone is
insufficient for C: GCC and Clang do not see a call embedded in an ASM string.

The parser now records that source use for every backend. The C-specific
observation remains outside `FBSYMBOL`, preserving the symbol layout used by
compiler objects and semantic tooling. The generated-C dependency graph records
ASM edges separately from ordinary C calls. A reachable ASM edge retains its
target's original native entry with `used`; ordinary C callees retain native
optimization. The existing conservative treatment of opaque ASM bodies stays
in place.

Parsed references in unused procedures must not introduce native link
dependencies. The closing graph pass removes an inactive ASM retention
attribute through its checked output span, including when opaque ASM prevents
body pruning. Debug-only source roots keep their existing retention policy.
Private naked procedures also receive a C prototype consistent with their
file-scope ASM definition, rather than a static declaration lacking a C body.

The public regressions are:

- `tests/compiler-support/c-asm-private-procedures.bas`: targets defined before
  and after the caller, normal ASM calls, naked tail calls and the original
  BASIC BYREF ABI.
- `tests/compiler-support/c-asm-inactive-procedures.bas`: parsed-only and
  emitted-but-unreachable ASM references, alongside a real optimized call.
  Its deliberately absent dependency exposes unwanted code retention. The
  emitted inactive source helper retains its baseline unused-function warning;
  that warning is compared without suppression.

The first attempted compiler repair passed the live-call tests but failed the
inactive-dependency control. It was not selected. The corrected compiler
passes 68 positive native executions across Windows widths, both C emitters,
both native C compilers, GNU11/GNU23 and O0/O2, including GAS/GAS64 cases. Twenty
candidate inactive-reference executions pass against twenty original-compiler
controls, with unchanged native warning classes. The optional readonly
inactive-dependency probe requires an optimization performed by the original
Win64 GCC build; it is not a portable missing-symbol test for Clang or Win32.
Actual readonly ABI calls are covered by the positive matrix on both widths
and both native compilers.

## Ordinary CRT and DJGPP exports

The known ordinary-export list now includes `_beginthreadex`, `_endthreadex`,
`__dpmi_yield`, `__dpmi_int`, `_farsetsel`, `_farnspokeb`,
`_go32_dpmi_lock_data` and `__crt0_glob_function`. Their generated C tokens use
the existing encoding and assembler-alias mechanism. Object import names stay
unchanged; unknown underscore names and compiler-recognized intrinsics keep
their existing policy.

Fourteen original contexts retain identical undefined object names in 28
before/after native object pairs, comprising 56 object builds. Their reserved-identifier warnings
fall from 24 to four, while GCC stays at zero. The four remaining observations
are recognized Interlocked intrinsic names. Real CRT thread creation, worker
arguments, exit status, joining and handle closure are checked on Windows
32-bit and 64-bit by `crt-thread-names.bas`.

## Qualification state

The combined candidate includes the independently arriving Xbox bitfield-layout
repair. It reaches a 224-unit GCC-clean emitted-C fixed point. Exact C and ASM
comparisons bind it to the positive/inactive value matrices and the original
library-name contexts without repeating unchanged native compilations.
The full GCC/Clang backend suite passes in 1,163.072 seconds, and all 703
retained fbctests profiles pass with zero GCC warnings. The compiler is selected
through `build/main-gcc`, with eight further selected-path value runs. Installed
release products have not been replaced. Newer Wii runtime-TLS edits remain
outside this artifact; its source snapshot preserves the qualified versions
without overwriting that concurrent work.

Local receipts reuse `build/c-emitter-forward-views-20261105/`:
`private-asm-selfhost-v3-20261113.json.zlib`,
`private-asm-values-v2-20261113.json.zlib`,
`private-asm-inactive-values-v4-20261113.json.zlib`,
`private-asm-combined-values-v3-20261113.json.zlib`,
`library-names-class-v2-20261113.json.zlib` and
`library-names-combined-v3-20261113.json.zlib`.
Selection is recorded in `private-asm-selected-v3-20261113.json.zlib`.
The ongoing corpus recount retains its earlier immutable compiler and source
cut, as described in [canonical corpus contexts](c-emitter-census-reproducibility.md).

<!-- end of c-emitter-asm-procedures.md -->
