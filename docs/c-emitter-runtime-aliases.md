<!--
Project: FreeBASIC compiler verification
File: c-emitter-runtime-aliases.md
Purpose: Explain the runtime declaration repair and its qualification.
Responsibilities: Record the target naming rules, focused tests and queue receipts.
This file intentionally does NOT claim completion of the full warning campaign.
-->

# Runtime procedure aliases, November 2, 2026

The text-type unit profile emitted each of `fb_UStrFromWstr` and
`fb_UStrToWstr` twice. The compiler intrinsic and explicit `extern "rtlib"`
declaration had exactly the same C signature. The latter additionally carried
an assembler label repeating the default object name, preventing the existing
exact declaration cache from recognizing the duplicate. GCC reported two
`-Wredundant-decls` warnings in the 64-bit profile.

`hNeedAlias()` now compares the required assembler name with the default C
object name, including the target underscore prefix. Encoded C identifiers
retain their aliases. All x86 conventions other than CDECL retain the previous
alias handling, including stdcall hidden-result decoration, Pascal and fastcall.
The procedure ABI model continues to observe the original required object name.
The declaration cache still compares exact text; it does not infer compatibility
between different signatures.

## Evidence

The private compiler is
`build/c-emitter-ustring-declarations-20261102/current/fbc.exe`, SHA-256
`c4e2e26a5a28d579785b6aed13c691114e6721114b8c7cecc18bdf56b7f51663`.
Its build re-emits all 224 current compiler modules, recompiles changed C with
the original unsuppressed GCC/Clang flags and reuses unchanged objects by C hash.
The resulting binary equals the initial single-object candidate byte for byte.

The focused qualifier compares old and new compilers in 16 native runs and 28
object checks. It exercises implicit and explicit Unicode conversions, a
reserved-word alias, direct calls and the required Win32 symbols `_for`,
`_stdcall_identity@4`, `_stdcall_plain` and `@fastcall_identity@4`. Native cases
cover GCC and Clang, GNU11 and GNU23, and O0/O2. The 32-bit objects are inspected
without claiming foreign execution. That target already deduplicated these
runtime declarations and retains its original stdcall aliases.

The regular backend runner passes with both C compilers, including the new
`runtime-aliases` fixture. Existing procedure-declaration/startup-order and
negative layout-assertion tests pass. Strict source-only lint reports no errors
or warnings in the changed BASIC files. The structure checker passes 392
documented files and 21 host source graphs.

The authoritative queue refresh is
`build/c-emitter-ustring-declarations-20261102/audit/receipt.json`. It checks the
224-module compiler C fixed point, re-emits every selected non-PNG project
profile and runs the original unsuppressed diagnostics on each changed C output.
Source, tool, runtime and installed-binary hashes are checked around that run.

The later [private read-only entry repair](c-emitter-readonly-entries.md)
supersedes the following checkpoint with a fresh recount and 39 fewer Clang
warnings.

| Queue | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Current compiler | 224 | 0 | 4,830 |
| Unit tests / fbcunit | 703 | 16 | 14,354 |
| Production corpus/projects | 1,117 | 490 | 11,222 |
| Additional projects | 239 | 1,810 | 41,140 |
| Total | 2,283 | 2,316 | 71,546 |

The repair removes two GCC warnings. Clang counts are unchanged. These totals
are occurrences across selected build profiles, not unique source defects.
The sealed summary is
`build/c-emitter-ustring-declarations-20261102/summary.json`.

## Boundaries

PNG remains excluded. Unselected source contexts, nonlocal ERROR ownership,
shared descriptor/pointer contracts and compiler-backed semantic lint remain
separate unfinished work. Accepted emitted C is not a claim of full application
runtime qualification. Installed compilers and runtimes remain unchanged.

The repair uses one scratch C/object pair and compressed diagnostic receipts.
The complete repair stage is limited to 64 MiB of retained files.

<!-- end of c-emitter-runtime-aliases.md -->
