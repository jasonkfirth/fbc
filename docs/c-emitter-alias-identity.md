<!--
Project: FreeBASIC compiler verification
File: c-emitter-alias-identity.md
Purpose: Explain procedure-address comparison repairs and their qualification.
Responsibilities: Record C and GAS64 semantics, regressions and remaining limits.
This file intentionally does NOT claim completion of the warning campaign.
-->

# Procedure-address comparisons

The [numeric CRT conversion checkpoint](c-emitter-crt-conversions.md) found a
separate compiler defect: two BASIC declarations for one native linker entry
could compare unequal. The original `strtod` probe failed with Clang-generated
machine code and with GAS64. This repair closes that check without changing
the CRT declarations or installing a compiler.

The C backend now compares procedure addresses as native unsigned integers.
Two different C identifiers can have the same assembler label. Clang can fold
a direct comparison using the C identifiers' distinct function identities,
even when both names resolve to one linker address. Integer comparisons retain
the machine-address behavior required by BASIC. Procedure declarations and
call signatures remain typed, and each operand is evaluated once.

The address helper follows pointer casts, including casts through ANY PTR,
but stops at value-producing expressions and after 64 nodes. An explicitly
integer-valued expression keeps its requested signedness. In particular,
`CINT(procedure_pointer)` must still participate in signed comparisons.

GAS64 had three related defects. When both operands were symbol offsets, the
right operand remained a memory reference, so comparison read bytes at the
address instead of materializing the address. Both offsets now use address
loads into separate registers. Pointer ordering uses unsigned conditions.
Conversions that reuse an existing register value retain the destination
dtype and subtype, preventing a pointer's old type from replacing an explicit
signed integer conversion.

## Qualification

`tests/compiler-support/procedure-address-identity.bas` and its separate C
helper cover same-entry aliases, different functions, callback storage,
native C address getters, actual calls, null comparisons, Boolean results,
casts through ANY PTR, evaluation counts, unsigned pointer ordering, explicit
signed integer comparisons and data-symbol aliases. The artificial high
address used for ordering is never called or dereferenced.

The dedicated driver checks:

- 20 native executions: both C emitters with GCC and Clang under GNU11/GNU23
  at O0/O2, plus GAS64 at O0/O2 linked through both tools.
- 20 emitted-C objects, including four Win32 object checks, and two GAS64
  objects. Native reference-helper objects are checked separately.
- 12 foreign objects for Linux x86_64, ARMv7, AArch64 and Darwin AArch64.
  These are object checks, not executions on those targets.
- Four failing historical controls: the original C identity failure, the
  original GAS64 offset failure, unsigned pointer ordering and signed cast
  retention. The accepted compiler passes the corresponding checks.

The regular backend runner includes the native fixture for both C backends.
Both pass with the accepted compiler. The preceding compiler fails the new
Clang fixture, proving the runner exercises the defect. The original five-case
`strtod` probe also passes with the accepted compiler. Six existing integer-FRAC
and aggregate-return programs pass across the GCC and GAS64 backends.

The value-test driver accepts an explicit `--toolchain-prefix` and records
runtime hashes. Its initial attempt used this source tree's incomplete runtime
archive and failed to link. The completed run uses the existing staged runtime;
the source tree and installed runtime files are preserved.

The complete current compiler is rebuilt and its 224 emitted C files are
re-emitted with the resulting compiler to verify the C fixed point. All selected
project profiles are re-emitted with their original recipes and epochs. Changed
C is checked again with the original aggressive, unsuppressed flags; checks
are reused only when emitted C is byte-identical and protected inputs match.

## Diagnostics and boundaries

| Queue | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Current compiler | 224 | 0 | 4,802 |
| Unit tests / fbcunit | 703 | 16 | 14,347 |
| Production corpus/projects | 1,117 | 490 | 11,092 |
| Additional projects | 239 | 1,805 | 41,133 |
| Total | 2,283 | 2,311 | 71,374 |

The refresh recompiles 142 changed project C outputs, verifies 1,917
byte-identical project outputs and closes the 224-unit compiler C fixed point.
The other queues retain their preceding warning counts.

The compiler has zero GCC warnings and 4,802 Clang warnings. This repair adds
one `-Wunsafe-buffer-usage` occurrence where `astOpIsRelational` indexes the
existing AST operation table by the IR opcode. The retained diagnostic and
exact emitted expression are in `new-compiler-warning.json`. It is counted,
not suppressed. Data-alias comparisons in the regression also retain their
GCC tautological-comparison and Clang unreachable-code diagnostics.

Receipts are under `build/c-emitter-alias-identity-20261102`. The stage uses
one temporary C file and object per serial queue check, reuses unchanged
compiler objects and has a 64 MiB retained-size cap. Source preimages cover
the three changed compiler files and the two test drivers. Existing line
endings, unrelated dirty work and installed binaries are preserved.

The latest queue counts and completed receipt are linked from
[current status](c-emitter-current-status.md). Twelve PNG profiles remain
excluded. Nonlocal ERROR ownership, shared descriptor/pointer contracts,
public CRT header compatibility, other warnings, unselected source contexts
and compiler-backed semantic lint remain open. The full goal remains active.

<!-- end of c-emitter-alias-identity.md -->
