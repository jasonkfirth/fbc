<!--
Project: FreeBASIC C-backend verification
File: c-emitter-cleanup-dependencies.md
Purpose: Record generated field-destructor retention and complete Templari counts.
Responsibilities: Bind compiler changes, native lifetimes and reproducible sources.
Application deployment retains its own build and execution qualification.
-->

# Managed fields across separate member definitions

The Templari expansion exposed a generated-C error in an added production
module. A cleanup helper called an implicit child destructor whose declaration
and body had already been discarded. The enclosing user members were defined
in another source module, so their field calls were absent from this module's
AST when procedure reachability settled.

Local scalar records with user destructors now record their generated child
destructor dependencies at declaration time. A dedicated cleanup-root flag
keeps these AST bodies available through flushing. The C dependency graph then
retains the bodies referenced by actual cleanup helpers. A discarded-owner
control verifies that unused generated child destructors are still removed.

The selected development compiler is `409dc1ef`, available through
`build/main-gcc/bin/fbc.exe`. The immutable image is
`build/c-emitter-forward-views-20261105/cleanup-selfhost-v1-20261114.exe`.
The preceding `f64ca95a` image supplies rollback.

## Complete paired warning counts

| Queue | Profiles | GCC | Clang |
| --- | ---: | ---: | ---: |
| Compiler | 224 | 0 | 1,340 |
| fbctests | 703 | 0 | 771 |
| Templari Win64 client/server | 658 | 2 | 52,408 |
| Coreutils Linux | 498 | 195 | 1,839 |
| SQLite Linux | 555 | 23 | 7,696 |

Templari has 1,316 producer emissions across its 658 profiles. Both producers
have the same native totals. Clang records 51,731 padding observations, 675
startup registrations and two strict function-pointer conversions. The two
GCC API-conversion observations retain their earlier Win32/Win64 native
qualification. Natural record layout and startup registrations retain their
diagnostics under the original unsuppressed flags.

Concurrent Templari edits required an affected-input refresh. The accepted cut
includes the two subsequently added source profiles and verifies every opened
input of every retained row. Its portable snapshot contains 1,167 source and
header inputs in 12,515,530 raw bytes, including the recorded include context.

The whole compiler reaches an exact 224-unit emitted-C fixed point. Its Clang
count comprises 1,327 padding observations and thirteen startup registrations.
The repair exposes one additional padding observation. All 703 fbctests and
1,053 port profiles reproduce their previous C exactly. Their original FBC
warning lines are preserved.

## Cleanup and allocation evidence

Eight original native compilation controls reproduce the missing destructor
using the small fixture and the actual Templari module. The fixture puts user
members in a separate BASIC source file. It contains a scalar child and a fixed
array of children with managed string fields.

Twenty native executions cover ordinary, debug, assembly-conservative, local
handler and outer-handler paths. Each execution checks 48 parent constructions
and the expected 32 user-destructor calls. Interrupted constructors raise error
24 and interrupted destructors raise error 25. Eight Linux executions observe
338 allocations and 338 releases each, with zero outstanding allocations.

Sixteen Win32/Win64 objects check discarded owners. Ninety-six cross-target
objects cover both producers, O0/O2, two modules and twelve target contexts
across ELF, COFF and Mach-O. The normal backend suite passes in 767.119 seconds.
Strict source-only lint has zero findings across all six changed BASIC files.
Compiler structure checks cover 416 documented files and 21 host graphs.

Linux allocation checks use a freshly built Make-selected MT runtime. Its
source packet preserves the exact build inputs. The archive build records
5,059 native-source GCC observations, including 3,239 sign conversions and
1,628 padding observations. Subsequent concurrent TCP source edits are recorded
against that snapshot. The byte-transfer and main-module selection corrections
in the Linux harness retain their failure records and successful replay.

## Remaining format contracts

The three floating converter modules pass eight native executions and 261,120
comparisons with libc formatting. Flags, case, field width, truncation, signed
zero, subnormals, finite limits, infinities and NaNs are included. All six
producer/module C inputs match their qualified production inputs exactly.
These checks close three additional runtime contracts, leaving 24.

The formatter's twelve current closure emissions match its earlier qualified
C inputs exactly. The nine unexecuted dispatch sites now have explicit
dispositions. Six generic string sites are bypassed by the parser's normalized
string conversions and the formatter's earlier string branches. Three require
precision fields on character or pointer conversions and retain their recorded
nonstandard-input boundary. Forty-eight native diagnostic controls warn for
those forms, and 48 valid-width controls retain format checking. The earlier
native value matrix executes 90 of the 99 sites. C23's precision rules are
recorded in the WG14 draft. [WG14 N3096](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3096.pdf)

Thirteen finite-format compiler candidates, the 24 additional runtime contracts,
the wider source-coverage ledger and the retained-corpus refresh remain active.

## Reproduction evidence

Receipts live under `build/c-emitter-forward-views-20261105`.

| Evidence | File |
| --- | --- |
| Fixed-point compiler | `cleanup-selfhost-v1-20261114.json.zlib` |
| Complete compiler Clang graph | `cleanup-compiler-clang-v1-20261114.json.zlib` |
| Complete backend suite | `cleanup-suite-v1-20261114.json.zlib` |
| Complete fbctests replay | `cleanup-fbctests-v1-20261114.sqlite3` |
| Complete production port replay | `cleanup-port-replay-v1-20261114.sqlite3` |
| Coherent Templari paired census | `templari-paired-current-v3-20261114.sqlite3` |
| Portable Templari source inputs | `cleanup-templari-source-cut-v1-20261114.json.zlib` |
| Windows native cleanup values | `cleanup-windows-v1-20261114.json.zlib` |
| Linux allocation observations | `cleanup-linux-native-v3-20261114.json.zlib` |
| Exact Linux runtime source inputs | `cleanup-linux-packet-v3-20261114.json.zlib` |
| Debug, assembly and discarded owners | `cleanup-controls-v1-20261114.json.zlib` |
| Cross-target objects | `cleanup-cross-targets-v1-20261114.json.zlib` |
| Floating converter values | `floating-format-native-v1-20261114.json.zlib` |
| Formatter boundary diagnostics | `formatter-coverage-native-v1-20261114.json.zlib` |
| Combined qualification | `cleanup-qualification-v1-20261114.json.zlib` |
| Compiler source overlay | `cleanup-source-cut-v1-20261114.json.zlib` |
| Development selection and rollback | `cleanup-selected-v1-20261114.json.zlib` |
| Final reconciliation | `cleanup-final-v1-20261114.json.zlib` |

Replay the source overlays in their recorded parent order. The project snapshot
uses Templari, FBC and PREFIX namespaces. Recorded profile commands provide the
source list, defines, include order, target and diagnostic flags. Substitute
the receiving workspace and toolchain prefix paths when replaying elsewhere.
The runtime packet supplies its complete Make-selected source context.

Native builds run serially, and the runtime archive build uses two jobs in the
two-core guest. The runtime build's measured owned files peak at 13,743,946
bytes. Each worker removes its scratch directory after recording its result.

The subsequent [native character-adapter qualification](c-emitter-wide-adapters.md)
reuses the exact runtime source packet for full before/after builds. Its explicit
native type conversions remove 3,096 warnings per tool with byte-identical runtime
objects. That comparison records later concurrent source changes independently.

<!-- end of c-emitter-cleanup-dependencies.md -->
