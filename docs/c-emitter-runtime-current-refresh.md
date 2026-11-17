<!--
Project: FreeBASIC C-backend qualification
File: c-emitter-runtime-current-refresh.md
Purpose: Record the current Linux runtime replay and open source owners.
Responsibilities: Bind native builds, execution outcomes and platform source cuts.
Other operating systems and application graphs retain their own verification.
-->

# Current runtime source cuts

Packet v51 applies the [VT100 C identifier repair](c-emitter-vt100-symbol.md).
Complete MT/non-MT builds remain GCC-clean and reach Clang 9,505 / 9,503,
removing 1,032 observations. All 2,072 objects match packet v50. The current
ABI checks pass 96 executions and all 478 existing recorded runtime outcomes
retain their expected results. The largest scratch tree uses 23,637,276 bytes
and is removed. The current Haiku header profiles also produce the same 24
previously executed objects with sixteen fewer Clang observations.

## Preceding platform and packet v50 checks

The [native platform owners](c-emitter-native-platform-owners.md) qualify seven
pending Haiku source versions and the native Windows UTF-16 header view. Ten
captured FreeBSD, OpenBSD and RISC OS versions retain their native replay.
The Haiku archive now selects the canonical executable-name implementation.

Packet v50 refreshes file EOF, integer-128 arithmetic and the conditional
long-double parser. Complete fresh Linux MT and non-MT builds compile 2,072
native object profiles. Both remain GCC-clean, with unchanged Clang totals of
10,021 / 10,019. All 478 recorded binary outcomes match their expectations,
including the 56 original-source failures, eight supplementary suite runs,
21,880 supplementary assertions and sixteen profiler-width runs.

The EOF and integer-128 source checks close. The parser receives separate
direct and runtime-entry checks because Linux's ordinary library build selects
its native CRT conversion. Sixteen Linux ILP32/LP64 before/after executions
pass 192,336 checks. Eight native Windows entry executions pass 96,168 parser
checks and 96,088 actual runtime-entry value comparisons. All four Win32
contexts select and execute the local fallback. All four Win64 contexts select
the UCRT native conversion path. Both use the unchanged public parser vectors.

This closes the shared parser source check. Seventeen captured Haiku, BSD and
RISC OS versions retain their native replay. The gfxlib2 Unicode header retains
its Windows 16-bit `wchar_t` view check. Four exact tested archives and startup
objects supply the numeric-format regression, totaling 6,141,552 bytes. All eight
files are removed after its completed native runs. The largest build scratch
tree uses 23,925,991 bytes. The workers remain serial, low priority and use
Make `-j1`.

The completed native receipts are `runtime-current-linux-v43-20261116.json.zlib`,
`native-nonmt-controls-v10-20261116.json.zlib` and
`runtime-limits-v6-20261116.json.zlib`, bound by
`runtime50-qualification-v2-20261116.json.zlib` to
`runtime-current-packet-v50-20261116.json.zlib`.

The parser evidence is recorded in `parser50-native-v1-20261116.json.zlib` and
`parser50-windows-v2-20261116.json.zlib`. The public
`build_scripts/test-longdouble-parser.py` runner supports direct vectors and
the actual runtime-entry path. It reports the selected parser and native
storage ABI. Its native-C flags follow the runtime's platform-default assembly
dialect. The first Windows attempt exposed an Intel-dialect option copied from
the emitted-BASIC profile into MinGW's AT&T inline math headers. The completed
matrix retains every warning option and uses the normal runtime-C dialect.

```text
python3 -B build_scripts/test-longdouble-parser.py --cc gcc --mode runtime-entry --bits 64 --optimization 2 --native-flags-json flags.json --receipt parser-entry.json
```

## Preceding packet v49

Packet v49 refreshes 23 common/runtime inputs and the graphics Unicode header
used by the padding components. Complete fresh Linux MT and non-MT builds
compile 2,072 native object profiles with zero GCC warnings. Clang reaches
10,021 observations for MT and 10,019 for non-MT, removing two from each
preceding build. All 478 recorded binary outcomes match their expectations,
including 56 original-source failures. All eight supplementary suite runs
pass 21,880 assertions, and all sixteen profiler-width runs pass.

The source cut also captures the newer DOS and RISC OS inputs. The existing
`dos-gcc-after-const-fix` audit independently rebuilt all 705 Make-selected DOS
runtime, gfxlib2 and sfxlib objects under unsuppressed GCC 12.2 diagnostics.
Every object has its diagnostic capture, all libraries have zero warnings,
and all 39 pending DOS input hashes match. Every runtime C, header and assembly
input matches packet v49. The DOS receipt records source hashes and compiler
version while its source inventory omits Make metadata hashes. DOS execution
qualification retains its maintenance owner.

Reusing that exact existing build closes the 39 DOS warning-replay checks.
Seventeen captured Haiku, BSD and RISC OS source versions retain native replay.
The graphics header has current Linux component evidence, with its 16-bit
`wchar_t` Windows view retaining its separate check. A later file-EOF source
version arrived after the completed immutable cut and retains owning replay.
The disjoint last-qualified GCC total remains 193.

Current receipts under `build/c-emitter-forward-views-20261105` are
`runtime-current-packet-v49-20261116.json.zlib`,
`runtime-current-linux-v42-20261116.json.zlib`,
`native-nonmt-controls-v9-20261116.json.zlib`,
`runtime-limits-v5-20261116.json.zlib`,
`runtime49-qualification-v1-20261116.json.zlib` and
`dos-platform-qualification-v1-20261116.json.zlib`.

# Preceding Linux runtime replay

Runtime packet v48 captures the complete 1,331-file inventory, including
eighteen changed inputs and the new Haiku runtime header. Eight changed common
or Unix inputs participate in the Linux builds. Complete fresh MT and non-MT
Make builds under GCC 14.2 and Clang 19.1.7 compile 2,072 object profiles.
Both configurations remain GCC-clean. Clang retains 10,023 observations for MT
and 10,021 for non-MT under the original unsuppressed options.

Both runtime archives change. GCC changes four objects per configuration,
covering PRINT USING, string-file input, file EOF and Unix serial support.
Clang changes the PRINT USING and file-EOF objects. The remaining object hashes
and both startup objects match the preceding packet-v47 builds.

All existing native control groups run again against the current sources and
archives. The receipts contain 478 binary invocations, including sixteen direct
console-view/focus boundary runs. Of these, 422 return success and 56
original-source controls reproduce their expected failing outcomes. Counting
each keyboard and other component invocation separately provides this total.
The earlier qualification retains its grouped execution-context accounting.

The replay includes narrow/wide formatting, serialization, Unicode, console
and pseudo-terminal behavior, file and TCP counts, pthread stack limits,
keyboard handling, termcap state and allocation cleanup. Eight supplementary
suite runs pass 21,880 assertions. Sixteen profiler-width runs pass their limit
checks. The direct 32/64-bit console-view runs cover 559,872 cases. GCC and
Clang run the existing matrices at O0 and O2.

The two Make workers run serially at low priority with `-j1`, each owning one
temporary source/build tree. Each tree stays below its 64 MiB cap and is
removed after verification. Installed compilers, archives and applications
retain their protected hashes.

## Platform source versions

The Linux replay closes the eight changed common/Unix source checks. Four DOS
and seven Haiku inputs retain their native platform replay. The Haiku count
includes the new header discovered by the complete source inventory guard.
One subsequent DOS scroll-source version is captured separately. Together
with the nine earlier FreeBSD/OpenBSD inputs, 21 platform source versions
retain native verification. Their exact bytes and hashes are stored with the
qualification. The five later Midisoft UI inputs retain their owning project
replay.

## Current format owners

The current complete GCC port packet contains 129 Coreutils nonliteral-format
observations. The new source ledger binds every observation to its current
function, C statement and C hash. Existing review receipts account for 115
observations. Their original execution scopes and six shared-formatter string
fallback dispositions remain recorded.

| Source owner | Observations | Remaining work |
| --- | ---: | --- |
| chmod and chown-core change descriptions | 2 | Compiler branch-join provenance |
| fprintftime, nstrftime, pinky and who | 4 | Time-format execution contracts |
| copy and cp | 2 | Forwarded-format caller contracts |
| csplit, nl and seq | 4 | Configured-format validation and output |
| numfmt floating builders | 2 | Runtime-built format branches and values |

The two finite compiler sites and twelve dynamic execution contracts remain
open. Numfmt also retains its separate ISO diagnostic for the apostrophe
format flag. The inventory introduces no diagnostic suppression or production
warning reduction. The disjoint GCC total remains 195, including Coreutils
161, SQLite 18, Templari 2 and the retained corpus 14. Compiler, fbctests,
Linux runtime and the four additional project groups retain GCC 0 at their
qualified source cuts.

Current receipts under `build/c-emitter-forward-views-20261105` are
`runtime-current-packet-v48-20261116.json.zlib`,
`runtime-current-linux-v41-20261116.json.zlib`,
`native-nonmt-controls-v8-20261116.json.zlib`,
`runtime-limits-v4-20261116.json.zlib`,
`current-format-ledger-v1-20261116.json.zlib` and
`runtime48-qualification-v1-20261116.json.zlib`.

<!-- end of c-emitter-runtime-current-refresh.md -->
