<!--
Project: FreeBASIC C backend qualification
File: c-emitter-asm-reference-analysis.md
Purpose: Explain checked Intel ASM references and generated-helper ownership.
Responsibilities: Record original-module diagnostics, source boundaries and qualification.
This file intentionally does NOT claim complete ASM storage analysis or foreign execution.
-->

# Checked ASM procedure references, November 13

The C backend can now omit unrelated private procedures beside a checked subset
of Intel assembly. The check accepts ordinary arithmetic, memory operations and
branches whose names are resolved BASIC symbols, registers or labels inside one
joined ASM template. It does not change the assembly text or its constraints.

Raw labels are collected only across the adjacent ASM/source-annotation region
that `astLoadASM` joins. An intervening BASIC statement ends that region. Calls,
directives, prefixes, unknown names, ASM GOTO, naked procedures, other syntax and
other CPU families retain the conservative module-wide procedure policy.
Resolved procedure operands still use the preceding native-ABI retention repair.

This is a procedure-reference proof. Storage and parameter analysis retain their
own conservative boundaries. In particular, the check does not authorize removal
of arbitrary variables accessed through assembly registers or hidden addresses.

## Generated numeric helpers

Removing unused procedure bodies exposed helper definitions that had only those
bodies as callers. Floating conversion, floating equality, integer ABS and
evaluated-value helpers now participate in the existing dependency graph. A call
records its owning procedure; each definition owns a checked header span. Live
ordinary callers, initializers and retained debug bodies keep their helpers.
Helpers belonging only to discarded bodies disappear with those bodies.

## Original Win32 module

The unchanged fb-ext-lib `libext-gfx-rotozoom.bas` is freshly emitted by both C
producers and compiled with the original aggressive GCC/Clang options. Eight
before/after object builds form four comparisons. Each pair has the same native
global/undefined symbol set and identical complete disassembly.

| Native tool | Before | After | Remaining classes |
| --- | ---: | ---: | --- |
| GCC | 65 | 4 | Four unread assigned variables |
| Clang | 66 | 5 | The same four variables and one padding observation |

All 61 unused-function observations disappear per tool, including removal of
four newly exposed conversion/equality helper definitions. No library source,
example, native instruction, import, export or warning option is changed.
This Win32 scope overlaps other library coverage and must not be added to the
retained Win64 census.

## Unread library state

The subsequent library repair removes `ODSTPTR`, `DSTBYTEPTR`, `SRCBPP` and
`DSTBPP`, including their unused assignments. The `Image.bpp()` implementation
only reads the image format. Its two discarded calls also disappear, so this
source repair requires behavioral comparison in addition to native acceptance.

Both C producers now compile the Win32 module with **zero GCC warnings** and
one Clang padding observation. Eight original/cleaned object builds preserve the
global and undefined symbol sets. Sixteen actual Win32 executions cover both C
producers, both native tools and O0/O2. Each executes 500 in-memory transforms
through the real Image constructors, accessors and destructor. Original and
cleaned versions produce the same pixel checksum and 8,188 changed pixels.
Cases vary rotation, scale, clipping position and transparent pixels. The
library source passes strict compiler-backed fblint with zero findings.

Evidence is `roto-unused-values-v3-20261113.json.zlib` and
`roto-unused-pixels-v5-20261113.json.zlib` in the existing work directory. The
runtime check owns at most 503,347 bytes of disposable products. The original
12,775-byte source is retained in `roto-unused-source-before-v1-20261113.json.zlib`.

## Qualification and current source build

The reference fixture passes 96 actual values across Win32/Win64, both C
producers, both native tools, GNU11/GNU23 and O0/O2. Thirty-two checked cases
have zero native warnings. Sixty-four opaque controls preserve their one
unused-function warning and eight exact preceding-compiler C identities.

The numeric fixture adds 32 actual values. It checks rounding, integer ABS,
floating comparisons and retained evaluation while unused Single helpers
disappear. Two FBC unread-variable warning lines remain per emission; native
warnings are zero. Sixteen of these values are carried from the completed
Win32 checkpoint after correcting a Win64 helper-name expectation.

The final compiler is built directly from all current canonical compiler
sources. Its 224-unit fixed point has zero GCC warnings and no frozen source
overrides. This includes the current Wii TLS/overalignment compiler edits;
native Wii runtime qualification is outside this host receipt.

All 703 fbctests profiles pass at zero GCC and 779 Clang observations. Seven
hundred and two retain exact preceding C; one independently updated platform
test receives fresh native compilation. The expanded complete backend suite
passes in 1,426.310 seconds. Twenty-six further exact C identities bind the
canonical compiler to 192 earlier value executions and the original-module
objects. Those identities are not counted as new executions.

`build/main-gcc/bin/fbc.exe` now selects
`b1554e3942d43295794a879f132b01f54d72d5c20cba29580cbe470b95a2fcac`.
Twelve additional values pass through that selected path. Immediate rollback
is the preceding static-local compiler `c7f06b1d`. Installed releases and
runtime archives are preserved.

Evidence reuses `build/c-emitter-forward-views-20261105/`:
`asm-reference-roto-v3-20261113.json.zlib`,
`asm-reference-values-v3-20261113.json.zlib`,
`numeric-helper-values-v3-20261113.json.zlib` and the
`asm-reference-canonical-` selfhost, gates, values, fbctests and selected receipts.
The canonical source cut preserves sixteen dirty build inputs in 265,378 bytes,
together with their Git predecessor and complete input hashes.

The [current status](c-emitter-current-status.md) separates these completed
checks from the remaining whole-corpus and Clang work.

<!-- end of c-emitter-asm-reference-analysis.md -->
