<!--
Project: FreeBASIC C-backend verification
File: c-emitter-plural-formats.md
Purpose: Record preserved plural format contracts and the current warning queues.
Responsibilities: Bind emitted expressions, diagnostics, native values and selection.
Broader application execution and corpus ownership retain separate evidence.
-->

# Plural translation format contracts

[Stored translation contracts](c-emitter-stored-translations.md) extend this
qualification to saved results and validated intermediate copies. The current
regression accepts proven saved formats and retains wrong-type and call-barrier
diagnostics for all three plural APIs.

The selected development compiler is
`9c1ebb28862fa41014e46c377d0c31a1348a57b089d9d7a13c5ecb0825aa6d62`.
`build/main-gcc/bin/fbc.exe` selects that image. Immediate rollback is `cd0cc1b6`.
Thirty-two program executions pass through the public development path.

| Qualified graph | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Compiler | 224 | 0 | 1,339 |
| fbctests | 703 | 0 | 771 |
| Coreutils Linux x86-64 | 498 | 197 | 1,841 |
| SQLite Linux x86-64 | 555 | 23 | 7,696 |

The compiler reaches an exact 224-unit C fixed point. All 703 fbctests retain
their preceding C exactly. Both complete port graphs are re-emitted through
the current compiler, with 1,040 exact C identities and thirteen changed units.
Every original FBC warning line is retained. The full normal GCC/Clang backend
suite passes in 602.383 seconds. Compiler Clang observations comprise 1,326
padding and thirteen startup-registration observations.

The compiler structure check also caught a missing required exclusion field in
the preceding literal-offset helper's header. That field is corrected. All 224
compiler contexts are emitted again from the current source and match their
qualified C exactly. The structure checker passes 415 documented files and 21
host source graphs.

## Preserving both source contracts

The existing evaluated-result helper carries a literal `gettext` format beside
the translated pointer. Plural translations had lost that contract when their
results entered C registers. The shared recognizer now accepts `ngettext`,
`dngettext` and `dcngettext` when both source formats are literals and their
external C declarations match the target ABI.

Plural counts use C unsigned long. The proof checks the LP64 width on 64-bit
Unix targets and the 32-bit width on Windows and 32-bit targets. Category
arguments retain signed C-int width. Local procedures, indirect calls, wrong
result types and incompatible declarations retain their ordinary lowering.

GCC supports multiple format_arg attributes for plural functions. A native
pilot found that Clang 22 checks the first attribute and misses a bad argument
in the second format. The emitted identity helper therefore receives a
conditional choice of the two literal addresses. Its selector reads the
already evaluated translation register. Both native compilers check both
formats. The helper returns the original translated pointer and ignores that
metadata argument. It repeats no source call or count expression.
[GCC format_arg contract](https://gcc.gnu.org/onlinedocs/gcc-11.4.0/gcc/Common-Function-Attributes.html)

The twelve-case native pilot records both representations and the missed Clang
control. The permanent `c-plural-format-results.bas`, `.c` and `.py` regression
uses the normal backend runner. Its forty cases run through both C producers
and both native tools, totaling 160 checks. Twelve valid cases pass and 148
invalid or unproven cases retain native diagnostics. Controls cover wrong
arguments in either literal, dynamic source forms, saved results, indirect
calls and incompatible count, category or return declarations. Four runs on
the preceding compiler retain the original missing-contract failure.

## Native values and production objects

Sixty-four C-startup executions cover Windows/Linux, 32/64-bit targets, both
C producers, both native tools, O0/O2 and preceding/current compiler revisions.
Each checks all three translation functions at counts zero, one, two and the
target's unsigned-long maximum. Formatted values, call counts and original
translation storage pass 1,536 comparisons. Every repaired generated module
has zero GCC and Clang warnings under its recorded aggressive options.

Thirteen production units contain 35 new identity wrappers. Removing those
wrappers and their newly needed helper declaration reconstructs every original
C file exactly. Native before/after compilation retains 25 of 26 objects byte
for byte. The remaining GCC `dd` object differs only inside its text section.
Its disassembly changes the order of two signed range tests in `IREAD`, with
the same early-exit target and unchanged registers between tests. All other
object bytes, public symbols and undefined symbols match exactly.

Coreutils loses two nonliteral-format observations in `dd` and two in `uptime`
from each native compiler. All other warning classes retain their counts.
Strict source-only lint reports zero findings across the three changed compiler
files and the new BASIC fixture.

## Reproduction and remaining work

Receipts live under `build/c-emitter-forward-views-20261105`.

| Evidence | File |
| --- | --- |
| Native attribute pilot | `plural-format-pilot-v1-20261114.json.zlib` |
| Compiler fixed point | `plural-format-selfhost-v1-20261114.json.zlib` |
| Complete compiler Clang graph | `plural-format-compiler-clang-v1-20261114.json.zlib` |
| Complete fbctests replay | `plural-format-fbctests-v1-20261114.sqlite3` |
| Complete port replay | `plural-format-ports-v1-20261114.sqlite3` |
| Production and Linux values | `plural-format-native-result-v1-20261114.json.zlib` |
| Windows values | `plural-format-native-values-v1-20261114.json.zlib` |
| Permanent diagnostic driver | `plural-format-diagnostics-v1-20261114.json.zlib` |
| Original C reconstruction | `plural-format-C-identities-v1-20261114.json.zlib` |
| GCC object comparison | `plural-format-dd-object-v1-20261114.json.zlib` |
| Full normal backend suite | `plural-format-suite-v1-20261114.json.zlib` |
| Current warning inventory | `plural-format-current-inventory-v1-20261114.json.zlib` |
| Current-source header identities | `plural-format-header-reconciliation-v1-20261114.json.zlib` |
| Incremental source delivery | `plural-format-source-cut-v2-20261114.json.zlib` |
| Public-path executions | `plural-format-selected-v1-20261114.json.zlib` |
| Final verification | `plural-format-final-v2-20261114.json.zlib` |

The nine-file source overlay records its compiler Git base, exact source
bytes and the preceding literal-offset delivery. Apply its recorded parent
overlays first. Unchanged production C and native records reference the existing
qualified database, keeping the evidence compact. Controllers reuse one owned
temporary directory per job and remove temporary objects after qualification.

Coreutils retains 142 nonliteral-format warnings and 55 observations in other
GCC classes. The reviewed runtime builder accounts for 99 nonliteral sites.
Forty-three other sites still need source-contract review, including translated
results carried through local variables. Nine runtime-builder coverage gaps
remain recorded in the formatter report. Full application execution, upstream
coreutils regeneration, broader corpus replay, parser stack control and the
semantic-linter consumer remain pending. Templari retains its separately
qualified two GCC observations and pending full Clang census. The wider cleanup
remains active.

<!-- end of c-emitter-plural-formats.md -->
