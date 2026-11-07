<!--
Project: FreeBASIC runtime qualification
File: c-emitter-runtime-type-tags.md
Purpose: Record the shared runtime C tag namespace repair and ABI evidence.
Responsibilities: Separate internal tags, public typedefs, linker symbols and
source-delivery proof while retaining unsuppressed native diagnostic counts.
This file intentionally does NOT claim experimental promotion or another-host testing.
-->

# Runtime C type tags

The shared runtime headers used implementation-reserved C structure and enum
tags. Forty-two observed tags contributed 16,109 Clang ST observations. They now
use non-reserved names, mostly by moving the leading underscore to the end.
Three private serial-record tags for other platform branches follow the same
rule. Public typedef names, members, enum values, access-view alignment and
linker names are unchanged. No diagnostic option is suppressed.

The consistent token rename touches 25 C/header files, including the internal
graphics infrastructure mock which completes the runtime's opaque handle types.
Every edited source can be restored exactly by the inverse token map. The
descriptor boundary-test extractor now identifies its public typedef rather
than depending on the private tag spelling. Application examples are unchanged.

The C/C++ search covered the current sources, tests, C:/fb_corpus and
C:/Nextcloud/games, excluding generated build directories and PNG paths. Its
external matches belong to independent runtime copies in FreeBASIC forks and
OHRRPGCE; they use their own headers and remain unchanged. The current graphics
mock is the only additional active source consumer found. The installation
rules distribute the BASIC inc tree, rather than these runtime-internal C
headers. This does not promise compatibility for an external C++ mangled ABI
constructed from private tags; supported C entry points retain their names and
layouts. Native C++ consumers use the public typedefs and C linkage.

| Native runtime profile | GCC before / after | Clang before | Clang after |
| --- | ---: | ---: | ---: |
| ST, 522 members | 1,067 / 1,067 | 42,814 | 26,705 |
| MT, 522 members | 1,067 / 1,067 | 42,821 | 26,712 |
| Startup | 2 / 2 | 78 | 47 |

All 2,090 direct native checks pass. Only Clang reserved-identifier counts change:
32,249 fewer observations across the two variants and startup. Every one of the
1,045 GCC objects is byte-identical to the preceding qualified string-header
runtime. The existing normal archives and compiler therefore remain the exact
same binaries. Their previous export, self-host, backend-suite and smoke evidence
is reused through object and artifact identities, not reported as fresh runs.

Independent ABI records retain public typedef sizes, alignment and selected
member offsets. They and the graphics mock produce eight identical before/after
object pairs across GCC/Clang and Win32/Win64. C++ consumers produce another four
identical pairs and pass four Win64 executions against the same runtime archive.
Sixteen checks of the two current canonical provider sources produce eight more
identical object pairs. The string-header matrix again passes 24 executions, six
unchanged negative controls and six Win32 objects. The runtime structural check
passes all 126 source-graph cases.

The structural guard now rejects reserved C tags in common runtime headers.
It ignores comments and quoted examples. Its only SDK tag exception is
struct _stat in fb_file.h, where the file API references the UCRT-owned type.
Six positive/negative controls cover new definitions, opaque declarations,
double-underscore tags, quoted text and the scoped SDK exception.

## Source delivery

The existing portable-source.zip remains unchanged. The small
E:/fbc-c-audit-20261105-forward-views-r2-qualification/crt-candidate/canonical/runtime-warning-source-update.zip
updates 21 runtime inputs and their manifest. It includes both the earlier
string-header conversion repair and this tag repair. All 608 composed source
inputs match the provider used by the native checks. The update contains no
native object, runtime archive, executable or old object cache.

Extract the original source archive into a fresh short directory, then extract
the update there before the first build. From that directory:

```powershell
python -B verify-runtime-warning-update.py
python -B build.py --tool-dir C:/path/to/ucrt64/bin
python -B verify-smoke.py --tool-dir C:/path/to/ucrt64/bin
```

The compiler sources and C bootstrap retain their frozen cut. Source-byte
composition is verified locally; another-machine build and the external corpus
replay have not been demonstrated. Other platform tag references are updated
consistently, without claiming their SDK execution. The source update retains
the experimental compiler's existing promotion limits. Installed binaries and
the original portable working package are unchanged.

Evidence is retained in runtime-tag-native.json/SQLite, runtime-tag-probes-*.json,
runtime-tag-cpp.json/SQLite, runtime-tag-canonical.json/SQLite,
runtime-tag-consumers.json.zlib, runtime-tag-guard-controls.json and
runtime-warning-source-update.json in the existing E: qualification work area.
The retained component remains bounded at 160 MiB. Emitted-C corpus queue totals
remain separate and unchanged; the overall campaign is unfinished.

<!-- end of c-emitter-runtime-type-tags.md -->
