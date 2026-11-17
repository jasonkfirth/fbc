<!--
Project: FreeBASIC C-backend verification
File: c-emitter-local-formats.md
Purpose: Record native format contracts carried by private pointer values.
Responsibilities: Bind compiler changes, alias controls and production evidence.
Broader corpus coverage retains its separately recorded qualification.
-->

# Native format checks through private pointer stores

The subsequent [ordinary-call qualification](c-emitter-call-formats.md) and
[stored translation qualification](c-emitter-stored-translations.md) extend the
boundaries described at this earlier compiler cut.

The C backend now carries a proven literal format beside a private pointer's
evaluated value. The complete optimized procedure is checked with the existing
read-only pointer analysis. Address escapes, BYREF exposure, pointee writes and
opaque operations reject the binding. Direct stores update the known literal.
Labels, branches, calls and memory operations invalidate the tracked value.

At a native printf-family or strftime format argument, an identity helper
returns the actual stored pointer. Its `format_arg` attribute carries the
original literal into GCC and Clang's format checks. The source assignment,
pointer storage and argument evaluation remain present.

This private-format qualification uses compiler `f64ca95a`. The subsequent
[cleanup dependency repair](c-emitter-cleanup-dependencies.md) selects `409dc1ef`
through `build/main-gcc/bin/fbc.exe`. The private-format image is stored in
`build/c-emitter-forward-views-20261105/format-locals-selfhost-v2-20261114.exe`.
It reaches an exact 224-unit emitted-C fixed point with zero GCC warnings.
Complete compiler Clang qualification retains 1,339 observations, comprising
1,326 padding and thirteen startup registrations. All 703 fbctests reproduce
their original C exactly and retain GCC 0 / Clang 771.

The normal GCC/Clang backend suite passes in 713.392 seconds. Its registered
`c-format-locals` fixture covers literal replacement, native values, stored
pointer identity and foreign aliases. Standalone diagnostic controls preserve
wrong argument types, dynamic values, address escapes, call barriers, copy
boundaries, static storage and control-flow joins.

## Helper names and native aliases

An initial candidate used `fb_FormatValue` for its helper. A valid foreign
declaration with that alias could bind to the helper instead of the external
definition. The actual native control compiled successfully and returned the
wrong result. Its receipt records exit 2.

The corrected helper uses the backend's disjoint C namespace and an
assembler-local label. The C identifier is `fb_c_format_value`, and its local
assembler identity is `.Lfb_c_format_value`. Foreign declarations using both
`fb_FormatValue` and `fb_c_format_value` retain their own calls and results.
The same original alias control now returns exit 0.

Twenty-two actual Clang object builds cover both C producers and eleven target
contexts. These include Linux x86-64/AArch64, Darwin x86-64/AArch64, FreeBSD
x86-64/AArch64, OpenBSD, NetBSD and Windows x86/x86-64/ARM64. They exercise ELF,
Mach-O and COFF object emission with the private assembler label. These are
cross-compilation checks. Runtime evidence comes from Windows and Linux.

## Native values and remaining coverage

Sixty-four before/after executions cover Windows/Linux, 32/64 bits, both C
producers, both native compilers and O0/O2. They pass 1,920 formatting comparisons
against native snprintf and 960 pointer-identity checks. Thirty-two executions
use the repaired compiler. Both external aliases execute their separate native
providers. Repaired generated modules have zero GCC and Clang warnings.

The complete namespace-safe production replay covers 1,053 profiles. Of these,
1,051 emit exactly their previous C. The changed du and ls modules each carry
one proven X_TIMESTYLE_MATCH format. Their four native before/after object
pairs match byte for byte under GCC and Clang. Removing the metadata helper
and its two call wrappers reconstructs both original C files exactly.
Coreutils reaches GCC 195 / Clang 1,839 and SQLite retains GCC 23 / Clang 7,696.
Every FBC warning line is preserved.

The Windows and Linux diagnostic matrices pass 288 native checks. These include
32 accepted literal cases, 32 retained type errors and 224 retained nonliteral
errors. Strict source-only lint has zero findings across the three BASIC files.
Compiler structure checks cover 416 documented files and 21 host graphs.
Development selection executes 34 public-path regression programs through both
C producers and verifies the protected installed-product hashes.

The two resolved sites belong to the fifteen finite-format propagation
candidates from the preceding source review. Thirteen candidates remain,
together with 27 additional runtime contracts at this cut. The subsequent
[cleanup and runtime qualification](c-emitter-cleanup-dependencies.md) closes
three runtime contracts, records dispositions for the nine formatter gaps and
completes Templari's Clang census. Thirteen compiler candidates, 24 runtime
contracts and the broader source-coverage/corpus refresh remain active.

## Reproduction evidence

Receipts live under `build/c-emitter-forward-views-20261105`.

| Evidence | File |
| --- | --- |
| Current fixed-point compiler | `format-locals-selfhost-v2-20261114.json.zlib` |
| Complete compiler Clang graph | `format-locals-compiler-clang-v2-20261114.json.zlib` |
| Complete normal backend suite | `format-locals-suite-v2-20261114.json.zlib` |
| Complete fbctests replay | `format-locals-fbctests-v2-20261114.sqlite3` |
| Actual initial helper-name failure | `format-locals-name-control-v1-20261114.json.zlib` |
| Corrected helper-name execution | `format-locals-name-control-v2-20261114.json.zlib` |
| Windows native value matrix | `format-locals-values-windows-v2-20261114.json.zlib` |
| Linux native value matrix | `format-locals-values-linux-v2-20261114.json.zlib` |
| Cross-target object matrix | `format-locals-cross-targets-v2-20261114.json.zlib` |
| Current native diagnostics and production objects | `format-locals-native-audit-v2-20261114.json.zlib` |
| Complete production replay | `format-locals-port-replay-v2-20261114.sqlite3` |
| Combined qualification | `format-locals-qualification-v2-20261114.json.zlib` |
| Reproducible source overlay | `format-locals-source-cut-v2-20261114.json.zlib` |
| Selected development path and rollback | `format-locals-selected-v2-20261114.json.zlib` |
| Final evidence reconciliation | `format-locals-final-v2-20261114.json.zlib` |

Compiler builds reuse exact qualified objects and remove their owned raw C and
objects after linking. Each native job uses one automatically cleaned scratch
directory. The Linux work reuses the existing compile-audit root and runs
serially at reduced priority.

<!-- end of c-emitter-local-formats.md -->
