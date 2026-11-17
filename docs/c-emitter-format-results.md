<!--
Project: FreeBASIC C-backend verification
File: c-emitter-format-results.md
Purpose: Record native format checking across evaluated translation results.
Responsibilities: Bind the compiler repair, CRT declarations and native evidence.
-->

# Evaluated translation results

[Stored translation contracts](c-emitter-stored-translations.md) extend this
earlier result-register qualification through validated private pointer stores.

The selected development compiler is `cd32db5b`. Its immediate rollback is
`acc4a892`. Direct C-declaration calls to `gettext`, `dgettext` and `dcgettext`
can carry their literal source-format contract through an evaluated result
register. A compiler-owned identity helper returns the captured pointer and
exposes the original literal through `format_arg(2)`. Translation calls execute
at their original positions. GCC documents this attribute as the connection
between a transformed format and its source argument. [GCC function attributes](https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html)

The emitter validates the native name, C calling convention, result type,
parameter count and parameter types. Source-defined routines, indirect calls,
incompatible return declarations and runtime format operands retain their
existing paths. The formatter checks continue to diagnose wrong argument
types and dynamic formats. The permanent regression covers saved pointers,
indirect calls, source definitions, mutable runtime formats and incompatible
result declarations through both native C compilers.

## Complete production replay

| Queue | Units | GCC before | GCC after | Clang before | Clang after |
| --- | ---: | ---: | ---: | ---: | ---: |
| Coreutils Linux x86-64 | 498 | 776 | 216 | 2,412 | 1,852 |
| SQLite Linux x86-64 | 555 | 23 | 23 | 7,696 | 7,696 |

Both queues are emitted again through the final compiler. All 1,053 C outputs
match the fully compared pilot outputs exactly, and every FBC warning line is
preserved. Original aggressive GCC 14.2 and Clang 19.1.7 options remain in the
saved commands. Each native compiler loses 560 coreutils observations. GCC's
nonliteral-format class falls from 702 to 153 across the complete graph.

There are 133 affected units. Removing the exact three-line identity helper and
its 3,685 calls reconstructs every preceding C file byte for byte. Each erased
call reads a compiler result register and forms a static literal address. All
133 Clang objects and 128 GCC objects match their preceding bytes exactly.
Five GCC objects differ following the equivalent identity expressions. Their
native object and optimized-IR inspections accompany the source-equivalence
proof and runtime controls.

## Native values and declarations

Thirty-two native C-startup executions cover Windows and Linux at 32 and
64 bits, both C producers, both native C compilers and O0/O2. Two BASIC modules
are emitted in one invocation, checking per-output helper state. The observers
check translated text and call order. The ordinary backend fixture also checks
the returned pointer against its native provider. Twenty-eight negative native
format controls cover seven input classes through both producers and both
native C tools. The independent semantic-model controls preserve all three
translation aliases and produce identical C with model output enabled.

The Linux checks exposed mutable format parameters in the SDK's bounded
formatter declarations. Twenty-five declarations across thirteen platform
headers now use `Const ZString Ptr` for `snprintf` and `vsnprintf`. Destination
parameters remain writable. The declarations pass 64 before/after emissions
across sixteen platform/width profiles. Native Linux headers reject eight old
prototype controls and accept eight repaired controls. The permanent host-native
check compares the emitted declarations and returned procedure-pointer types
against `<stdio.h>`. Windows profiles retain their preceding emitted types.
Foreign platform results here are declaration-emission checks.

The compiler retains its exact 224-unit C fixed point and zero GCC warnings.
Its complete current Clang graph has 1,326 padding and thirteen global-constructor
observations. There are 103 exact native reuses and 121 fresh Clang builds.
All 703 fbctests outputs retain exact C identities and GCC zero / Clang 771.
The promoted compiler also re-emits all 115 omaGUI profiles with exact preceding
C identities, GCC zero and Clang 5,566. Its 168 GUI owning-source proofs now use
the promoted compiler. The wider ledger retains 1,414 proofs and 1,523 pending
contexts on its explicitly recorded compiler cuts.
The final complete GCC/Clang backend suite passes in 630.583 seconds. A fresh
canonical-source replay checks the later header/comment corrections against
all 224 qualified C identities.
The selected development path passes 28 actual program executions. Sixteen
additional baseline/repaired host executions cover release/debug and O0/O2.

Strict source-only lint of the emitter and formatter fixtures reports zero
findings. The SDK scan retains one existing `FBL-PP-001` observation on OpenBSD's
`TMP_MAX` hexadecimal literal. Its original header reproduces the same finding.
The installed semantic-lint consumer's previously recorded capability mismatch
remains open.

## Reproduction and remaining scope

The ordinary `build_scripts/test-compiler-backends.py` entry point runs the
formatter fixture and diagnostic controls. The paired files are
`tests/compiler-support/c-format-results.bas`, `.c` and `.py`, with the native
declaration probe in `crt-format-declarations.bas`. The existing
`build/c-emitter-forward-views-20261105` directory contains the
`format-result-*-20261114` source cuts, complete replay databases, native
comparison packet, value controls and selected-path receipt.

The remaining coreutils GCC classes comprise 153 nonliteral formats, 21 unused
functions, fifteen qualifier casts, eight format-overflow observations, eight
builtin declaration mismatches, five possible null dereferences, four format
truncations, one other format observation and one returned-local-address
observation. SQLite retains its separate 23-observation queue. Wider source
coverage and the 128-written-cast parser stack check remain open.

Native comparisons reuse one C/object/log slot, run serially at reduced
priority and retain compressed evidence. Automatic approval review rejected
cleanup of a stopped suite's 240,883-byte temporary directory at
`E:\fbc-c-audit-20261105-forward-views-r2-qualification\crt-candidate\canonical\tool-temp\fbc-backends-pttsp5rb`,
returning only `blocked by policy`. That directory remains in place. Installed
release binaries and runtime archives retain their hashes.

<!-- end of c-emitter-format-results.md -->
