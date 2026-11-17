<!--
Project: FreeBASIC C backend qualification
File: c-emitter-iif-formats.md
Purpose: Record native format contracts through closed conditional results.
Responsibilities: Bind source analysis, native values, diagnostic controls and queues.
Application deployment retains its separate build and runtime qualification.
-->

# Conditional native format contracts

The C backend retains literal formats selected by a direct pointer-valued IIF
expression in a native format argument. Each branch can supply a literal address or a matching native
gettext-family call. Plural calls contribute both original source formats.

Complete-body analysis checks the implicit result slot before AST loading
deletes its nodes. The slot must belong to that IIF, with exactly the two owned
branch assignments and its result read. Additional references reject the
contract. The existing read-only table then applies the same C qualification
to its declaration, stores and reads. Branch lowering keeps its ordinary
condition, selected call and pointer value.

The existing format identity helper receives the evaluated slot and the known
source formats. Native GCC and Clang can therefore check every possible
format. Unknown formats and writable literal storage retain their native
diagnostics. The implementation keeps ordinary private-pointer tracking and
its control-flow barriers.

## Native qualification

The fixed-point compiler is `93d2c494`. All 224 compiler C inputs reach an exact
fixed point with zero GCC warnings. Only `ir-hlc.bas` requires a fresh native
object in the first round. Compiler Clang retains 1,340 observations, comprising
1,327 padding and thirteen startup registrations. All 703 fbctests retain exact
C identities and their GCC 0 / Clang 771 totals.

The registered `c-iif-formats` regression covers literal, translated and plural
choices, plus a branch that consumes no argument. The 128 native executions
cover both C producers, O0/O2, Windows/Linux, both pointer widths and both native
compilers, with normal and debug builds. They perform 122,880 output, length,
truncation and buffer comparisons.
Every case checks one condition evaluation and the selected translation count.

The 448 native diagnostic compilations check valid inputs, wrong argument types
in either ordinary branch and either plural alternative, dynamic input and
writable literal storage. Expected errors remain visible. Ninety-six actual
objects qualify twelve ELF, COFF and Mach-O target contexts under both C
producers and optimization levels. Full target execution retains its separate
qualification.

Strict source-only lint of both compiler helpers and the BASIC regression has
zero findings. The full normal backend suite passes in 621.505 seconds. Its
first run identified a missing C-provider registration for the added fixture.
The shared provider/diagnostic registration now includes `iif-formats`, and the
complete rerun passes.

The first compiler build's source-pin guard caught an in-flight helper edit.
That receipt remains incomplete. The accepted build uses the final stable
canonical source bytes and reaches the complete emitted-C fixed point.

The first public-path debug check exposed a disabled conditional-format proof
under `-g`. Selection restored the preceding compiler. Complete-body analysis
now validates the implicit IIF slot in debug builds while ordinary named-local
tracking retains its debug restrictions. The expanded native matrix and normal
backend suite pass with this correction. Windows Clang 22 retains one debug
startup-registration warning before and after the repair. Linux Clang 19 has
zero such warnings. Paired checks preserve every other diagnostic class.

The qualified image is selected through `build/main-gcc/bin/fbc.exe`. Forty
public-path regression executions pass under the original aggressive native
options. Four further executions cover this fixture through default prefix and
include lookup, under both producers with normal and debug builds. Selection
and entry-point receipts verify the installed products and runtime archives.

## Complete Linux port replay

All 1,053 production emissions complete and preserve their original FBC warning
lines. All match the preceding conditional-format candidate exactly. Compared
with `409dc1ef`, 1,046 C files match exactly. The seven changed files contain
nine restored conditional format contracts. Removing identity metadata and
normalizing read-only pointer qualifications reconstructs the previous C tokens
for every changed module. Fresh before/after builds retain the original native
tools and aggressive unsuppressed flags.

| Queue | Profiles | GCC | Clang |
| --- | ---: | ---: | ---: |
| Coreutils | 498 | 187 | 1,831 |
| SQLite | 555 | 23 | 7,696 |

Coreutils loses nine nonliteral warnings per tool. GCC additionally exposes
the POSIX apostrophe grouping flag in numfmt's long-double format. Its grouping
control matches ordinary formatting in the C numeric locale. The flag is
documented by the native printf interface. [Linux man-pages printf reference](https://man7.org/linux/man-pages/man3/printf.3.html)

Clang additionally exposes an octal-format type observation in od. Original C
passes an unsigned character through default argument promotion. The emitted
explicit integer cast loses that original type in Clang's format checking.
Eight native controls compare both forms across all 256 byte values in each
available tested locale and check long-double grouping values. All executions
pass, totaling 4,096 byte and 320 floating comparisons. The explicit-cast form
alone produces the octal type observation. Redundant C default-promotion casts
remain a compiler follow-up, and this diagnostic remains counted.

Four of the earlier thirteen finite-format propagation candidates remain.
Twenty-four additional runtime format contracts retain their prior qualification
work. The wider 6,419-profile source refresh runs independently with original
diagnostic options and per-profile input checks.

## Current Templari source cut

The complete paired Win64 replay covers 659 profiles with GCC 2 / Clang 52,528
under each C producer through selected compiler `93d2c494`. Its source cut
contains 1,168 compiler inputs. Three input files differ from the preceding
conditional-format project cut. Two controls through that preceding compiler
reproduce the changed module's C exactly.

The preceding cut had 21 changed or additional files relative to its portable
parent. Seventy-six old-compiler emissions covered its 38 changed/new profiles
and reproduced their C exactly. The 120 additional Clang observations relative
to the earlier 658-profile cut follow those project input changes. Both
remaining GCC API conversions retain their prior qualification.

Concurrent project edits required a second affected-input refresh. The worker
now archives the actual opened input versions, allowing the qualified source
cut to retain its bytes when the live project changes afterward. Its profile
commands and native diagnostic flags remain in the bound paired database.

## Retained evidence

Artifacts are under `build/c-emitter-forward-views-20261105`.

| Evidence | Artifact |
| --- | --- |
| Current canonical fixed point | `iif-formats-selfhost-v4-20261115.json.zlib` |
| Complete compiler Clang graph | `iif-formats-compiler-clang-v2-20261115.json.zlib` |
| Full backend suite | `iif-formats-suite-v3-20261115.json.zlib` |
| Complete fbctests replay | `iif-formats-fbctests-v2-20261115.sqlite3` |
| Complete production emission replay | `iif-formats-port-replay-v2-20261115.sqlite3` |
| Production native warning comparison | `iif-formats-production-linux-v1-20261114.json.zlib` |
| Complete production token comparison | `iif-formats-production-review-v1-20261114.json.zlib` |
| Exact native fixture inputs | `iif-formats-native-packet-v2-20261115.json.zlib` |
| Windows values and diagnostics | `iif-formats-native-windows-v3-20261115.json.zlib` |
| Linux values and diagnostics | `iif-formats-native-linux-v4-20261115.json.zlib` |
| Newly exposed diagnostic controls | `iif-formats-extension-linux-v1-20261114.json.zlib` |
| Cross-target objects | `iif-formats-cross-targets-v2-20261115.json.zlib` |
| Selected development compiler and regression entry points | `iif-formats-selected-v2-20261115.json.zlib` |
| Default prefix and include lookup | `iif-formats-default-entry-v1-20261115.json.zlib` |
| Current Templari paired replay | `templari-paired-current-v6-20261114.sqlite3` |
| Portable Templari input delta and source controls | `iif-formats-templari-source-cut-v3-20261115.json.zlib` |

Native fixture packets contain exact emitted C and the C reference driver.
Production packets contain both versions of every changed C module. Workers
retain commands, native flags, source hashes and actual output, using one
automatically cleaned scratch directory at a time. The native fixture workers
use an 8 MiB scratch bound, production comparisons use 16 MiB, and the compiler
build uses a 32 MiB owned-file bound. The wider retained-corpus receipt has a
256 MiB owned-file bound because it archives changed generated C. Its resumed
controller preserves completed rows and records the transition from 64 MiB.

<!-- end of c-emitter-iif-formats.md -->
