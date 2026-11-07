<!--
Project: FreeBASIC compiler verification
File: c-emitter-deep-reachability.md
Purpose: Record deep procedure reachability and debug-storage qualification.
Responsibilities: Explain traversal limits, storage semantics and current evidence.
This file intentionally does NOT claim complete corpus or foreign runtime coverage.
-->

# Deep C procedure reachability

The Stunts viewer's large global initializer reached the C procedure scanner's
256-level recursion limit. That limit was treated like opaque assembly and kept
every private procedure in the module. A diagnostic compiler identified the
depth fallback; the selected viewer output contains no actual inline assembly.

`ast-node-proc.bas` now uses a reusable heap stack for inspection and call
marking. It visits ordinary AST children, profiling expressions and temporary
string copy-back trees. The pending stack is limited to 65,536 entries and each
walk to 1,048,576 visits, including string-list items. Assembly, naked procedures
and exceeded budgets retain conservative full emission. The stack is released
on both successful analysis and fallback. The procedure graph's existing roots
and work queue retain their original meaning.

Removing dead procedures exposes private globals that debug builds deliberately
keep. Merely emitting their declarations does not retain optimized object
storage: both GCC and Clang discard otherwise unreferenced private data at O2,
even with debug information. The C storage index now roots those debug globals
and their initializer dependencies and emits `__attribute__((used))` on the
retained definitions. This requires real object storage; it does not disable an
unused warning. Release builds still omit unobserved private storage. Public,
owned and thread-local definitions keep their existing policies.

## Qualification

The noninstalled compiler is
`build/c-emitter-unused-procedures-20261102/accepted/fbc.exe`, SHA-256
`e17df33573b5a9cc5d9eac0d2e4e61ea14781f161b41926f0209f6a47694ba0e`.
All 224 compiler outputs close the C fixed point. Their diagnostics remain
zero GCC and 4,802 Clang warnings.

The permanent deep-initializer fixture constructs 512 string-owning entries,
checks their runtime contents and contains a dead private call chain accessing
private array data. Both C producers, GCC/Clang, GNU11/GNU23 and O0/O2 pass 32
native executions. Forty main objects include eight Win32 checks; eight further
objects cover Linux x86_64, ARMv7, AArch64 and Darwin AArch64. Object symbols
confirm the private data survives every debug optimization profile and is absent
in release. Twelve historical and annotation-removal controls establish the old
depth fallback and the loss of optimized data without the retention attribute.
Foreign and Win32 object checks do not establish execution on those targets.

The existing reachability suite passes again, including assembly fallback,
deep expressions, a 600-call chain, static and nested destructors, string
copy-back and GAS64 execution. The ordinary GCC/Clang backend suite passes its
new generated-C guard and native fixture. The historical compiler is rejected
by that guard because its unreachable private body remains.

The structure check passes 393 documented files and 21 host graphs. Strict
source-only fblint checks the three compiler files and new BASIC fixture with
zero errors, warnings or advice. The separate semantic lint attempt could not
obtain two required compiler contracts; it remains an unresolved semantic check
and is not included in the source-only result.

## Selected warning queues

Five additional-project C outputs change: Stunts viewer/input, Optical VBDOS
Studio, and WSR5 Raider/omaGUI runtime. They are recompiled under the original
unsuppressed GCC and Clang profiles. Another 2,054 project outputs are verified
byte-identical and retain their bound object checks. Twelve PNG profiles remain
excluded. This is selected-profile object qualification, not complete game
execution or project build coverage.

| Queue | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Compiler | 224 | 0 | 4,802 |
| Unit tests | 703 | 16 | 14,347 |
| Production | 1,117 | 485 | 11,087 |
| Additional projects | 239 | 1,683 | 39,758 |
| Total | 2,283 | 2,184 | 69,994 |

The reduction is 122 GCC and 1,375 Clang warning occurrences. Counts include
repeated source profiles and are not counts of distinct defects. No new warning
category is suppressed. Installed binaries and unrelated source bytes are
preserved. The packet's retained artifacts are capped at 64 MiB.

Evidence is in `build/c-emitter-unused-procedures-20261102/summary.json`, its
`audit/`, `contracts/`, compiler lineage, preservation proofs and test logs.
The [current status](c-emitter-current-status.md) records the remaining campaign
work. The overall warning-cleanup goal remains incomplete.

<!-- end of c-emitter-deep-reachability.md -->
