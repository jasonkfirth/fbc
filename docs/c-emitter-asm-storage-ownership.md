<!--
Project: FreeBASIC C backend qualification
File: c-emitter-asm-storage-ownership.md
Purpose: Record checked assembly references and procedure-owned global storage.
Responsibilities: Explain dependency closure, regressions and native evidence.
The wider corpus retains its independently recorded source cuts.
-->

# Naked procedure private storage qualification, November 15

Development main selects `dffcd9a1` after its 224-unit GCC-clean emitted-C fixed
point and all 703 GCC-clean fbctests contexts pass. The complete normal backend
suite passes. Its final Linux fixture refresh receives sixteen additional
ordinary driver executions across Win32/Win64, both C producers, O0/O2 and debug.

File-scope ASM names private native objects directly. Their declarations now
carry the native `used` storage contract when those ASM references are recorded
by the existing procedure scan. Hoisted local statics retain their original
assembler aliases beside their encoded C identifiers. Local static declarations
and their types precede the naked function's entry label. This prevents a native
compiler from placing initializer bytes at the function entry after a file-scope
ASM section switch. [GCC basic assembly](https://gcc.gnu.org/onlinedocs/gcc/Basic-Asm.html),
[GCC common attributes](https://gcc.gnu.org/onlinedocs/gcc/Common-Attributes.html).

The permanent regression exercises scalar, fixed-array and record globals,
ASM-only storage, two local statics, repeated C/BASIC updates and two distinct
procedure-local record types sharing a BASIC name. Its Linux 32-bit helper uses
a normal PC thunk and GOT-relative storage operands. The native matrix covers
Windows and Linux, both widths and C producers, O0/O2 and debug/plain forms.
Linux includes default, PIE and PIC modes, including FBC's own 32-bit PIC option.
GNU LTO receives the same native options through compilation and linking.
All 504 repaired executions pass with 180,216 value observations
and zero generated-module GCC warnings under unsuppressed diagnostics.

The final fixture corrects its earlier cross-section displacement expression.
The original-failure receipts retain the two earlier compiler candidates, the
missing local alias, data emitted at the entry label and native harness failures.
The complete suite's earlier fixture cut remains archived. Its sole fixture
change is qualified through fresh ordinary driver executions and the expanded
Linux native matrix. Four freshly emitted leaf-CALL C identities also bind the
preceding sixteen native alignment observations. Eight opaque 32-bit controls
retain exact C. Development main executes the new regression through both C
producers, with `bc2e5878` available as its immutable rollback image.

An independent build updated repository `bin/fbc.exe` during qualification.
Its new hash is preserved in the installed-product refresh receipt. Development
main has its own selection target and rollback image.

Strict source-only lint covers the two compiler files and permanent fixture with
zero errors and one required PRINT-marker advisory. A fresh compiler-backed
attempt rejects the fixture's schema-27 model and reports two operational
errors. That semantic-model acceptance limit has its own captured receipt.

The complete project receipts use the `asm-global` prefix under
`build/c-emitter-forward-views-20261105`. Their aggregate is
`asm-global-qualification-v1-20261115.json.zlib`. The preceding sections retain
their recorded compiler and source cuts.


# Checked assembly and storage ownership

The selected development compiler is `9c05c8a6`, reached through
`build/main-gcc/bin/fbc.exe`. Its 224-unit emitted-C fixed point has zero GCC
warnings. The complete backend suite passes, and the public entry point passes
48 explicit-prefix regressions and four default-prefix/include executions.

The instruction check accepts `PUSH`, `POP`, `CPUID`, `XGETBV`, `FNSTCW` and
`FLDCW`. Operands pass through the existing symbol, register and local-label
checks. A C-style hexadecimal integer consists of adjacent `0` and hexadecimal
suffix tokens. Resolved symbols and intervening whitespace retain their own
reference handling.

An ASM variable reference records an edge from its owning procedure to private
global storage. Procedure and data nodes share the existing work-queue closure.
Static initializer trees contribute their variable and procedure edges before
global declarations are filtered. Rendering an initializer retains its data
owner while recording native helper dependencies. This preserves an initializer
that reaches a procedure whose own assembly uses another private global.

The storage index borrows C names from the procedure pool. Its index and entries
are released before the owning pool. Initializer traversal uses the existing
explicit AST work list, and graph traversal uses pooled queue links.

## Native qualification

Windows and Linux, 32/64-bit targets, both C producers, GCC/Clang, O0/O2 and
debug/normal forms pass 128 original/changed native executions. Twelve x87
precision/rounding states per execution give 1,536 value and control-word checks.
CPU register values match the independent C observer. XGETBV executes after its
operating-system support check. The unavailable-function observer records zero
calls from discarded helpers.

The regression includes a private global used only by live assembly, an inactive
ASM owner with an unavailable dependency, and a function-pointer initializer
that reaches ASM-only storage. Four normal public-path GCC/Clang executions
cover O0/O2. The inactive-owner native link control also passes after the repair.

The earlier `8ae0438a` candidate failed the ASM-only declaration test. The
`9a96f285` candidate then retained an inactive owner's external dependency.
Their sources, emitted C and failures remain in the receipts. The selected
compiler passes both controls.

All 703 fbctests retain GCC 0 / Clang 771. The compiler retains Clang 1,340.
The complete 1,053-profile port replay preserves every FBC warning line and
reaches coreutils GCC 168 / Clang 1,803 and SQLite GCC 21 / Clang 7,696.
The four changed port units remove nineteen GCC and 27 Clang observations;
all eight original/changed native object comparisons are byte-identical.
Templari's complete current paired graph has 668 profiles at GCC 2 / Clang 53,213.
Both C producers emit identical C for every profile.

## Recorded native boundary

The subsequent [checked register-effect qualification](c-emitter-asm-register-effects.md)
closes the PIC controls recorded here with selected compiler `8f51b482`.
Its native matrix also verifies stable automatic storage across PUSH/POP.

Linux 32-bit GCC rejects the original and changed general-register-clobbering
ASM under default PIE and explicit PIC at O0. Four paired failing compilations
retain that existing register-constraint defect. The 32-bit value matrix uses
explicit non-PIE compilation and linking. These results belong to the `9c05c8a6`
source cut.

The selected-compiler retained replay completes all 6,419 configured profiles
at GCC 12 / Clang 15,862, with no stale inputs or new frontend regressions.
It accepts 6,345 C object profiles and preserves 46 PNG exclusions, six included
providers, two closed assembler contexts and twenty known frontend refusals.
Its measured owned peak is 34,316,644 bytes within the 256 MiB bound.

## Evidence

The existing `build/c-emitter-forward-views-20261105` directory retains
`asm-storage-qualification-v2-20261115.json.zlib`,
`asm-storage-selected-v1-20261115.json.zlib`, the v4 self-host receipt, v3 suite
and queue receipts, v5 native receipts and both failed-candidate controls.
The source-only strict lint check covers five BASIC files with zero errors.
Its single PRINT-marker recommendation belongs to the executable regression's
required success output. Compiler-backed semantic consumer closure facts are
unavailable in the installed linter. This tool limitation is recorded separately
from the source-only lint and native compiler checks completed here.

<!-- end of c-emitter-asm-storage-ownership.md -->
