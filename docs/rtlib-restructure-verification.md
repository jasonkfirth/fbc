<!--
Project: FreeBASIC runtime library
File: rtlib-restructure-verification.md
Purpose: Record runtime restructuring inputs, checks and native test scope.
Responsibilities: Distinguish structural preservation from executed qualification.
This file intentionally does NOT contain unverified platform parity claims.
-->

# Runtime restructuring and verification

Work started on 2026-10-19 from `e5d0ca918`. The runtime inventory contained
1,256 tracked files, including 1,241 C, C++, assembly and header files and
456 portable C implementations in the root directory.

The portable implementations now belong to 21 explicit subsystem directories.
Six private headers/data templates follow their owning implementations. Shared
`fb_*.h` contracts retain their established root paths. Platform, shared Unix
and architecture implementations now live under `src/rtlib/drivers`; common
startup objects remain in `static`. A [driver map](../src/rtlib/drivers/README.md)
describes the native layers without crowding the portable subsystem list.
The [maintainer guide](../src/rtlib/README.md) documents ownership, ABI rules,
startup, shutdown, memory lifetimes, file-device dispatch and locking limits.
Source headers and footers identify all 1,241 native runtime source/header
files while preserving the existing implementation notes.

`mk/rtlib-sources.mk` owns portable discovery and whole-file platform
replacement. Portable filenames and override selection are preserved. Generic
archive members use Make's deterministic basename order instead of native
directory collation; platform layers retain their existing wildcard order.
Make rejects duplicate archive member names within a layer and in the selected
runtime. All four object variants derive from the same source list. The special
Win64 GOSUB unwind-table rule follows its relocated object. Deep private
headers participate in the normal runtime dependency list.
The required runtime include root is scoped to runtime objects and survives
caller-supplied `CPPFLAGS`. A focused build compiled one separate probe object
with both the caller macro flag and the required include path, without changing
the qualified archives.

The new `make rtlib-structure-test` belongs to the quick and full workflows.
It checks source identities, local includes, documented subsystem ownership,
126 target/architecture/X11 graphs, all four object variants and synthetic
replacement/collision cases. These graph checks require no cross SDK and do
not claim execution on those platforms.

Original hashes, relocation records and ordered Make graph selections are
retained under `out/rtlib-maintenance-20261019`. The implementation comparison
ignores comments and reverses the recorded include relocations before hashing
tokens. It is intended to detect accidental behavior edits during the move.

## Structural preservation

The original and reorganized Make graphs select the same source membership in
all 126 cases. A same-host comparison uses empty original source names, the
original Make fragment and the current Make fragment. This avoids treating
Windows/Unix wildcard collation differences as source-selection changes.
Portable archive members now have a deterministic basename order. The native
runtime archive checks also require unique strong exported symbols.

All 1,241 native implementation/header token streams match the original after
removing comments, empty documentation conditionals and recorded include
relocations. The comment-only path updates in `inc/fbc-int/profile.bi` and
`inc/fbc-int/fbcall.bi` each passed strict Windows lint with zero diagnostics.
Two `fbportio.h` headers are shared with BASIC tools. Their documentation uses
preprocessor-skipped blocks because BASIC cannot parse C comments. Native C
macro dumps match the originals on all three hosts, and two small Windows
BASIC fixtures compile and run with those headers. The structure checker
protects both documentation wrappers. The unchanged legacy port-I/O tester
still fails a full frontend probe on its existing `__FB_ARGC__` references.
That utility is outside fbctests and Exampleageddon. The include fixtures do
not load a driver or access hardware.
The NuttX mini-runtime staging audit and shell syntax check passed after
updating its reduced console source tree and include search root. A probe also
executes the harness's actual flat timer-source rewrite and preprocesses the
result. The DOS export-generator Makefile resolves its relocated BASIC source
in a build dry-run. These are staging checks, not new embedded or DOS execution
claims.

## Native qualification

Each host uses one owned source snapshot. Make builds and example sweeps use
two jobs. The
compiler, rtlib, fbrt, gfxlib2, gfxlib3 and sfxlib were built for the initial
restructuring. The driver follow-up reused those unchanged compiler binaries
and rebuilt the runtime and companion libraries. Each compiler reports
`1.20.5-1`. The log-test validator requires exactly one
PASSED record for every selected test, independently of the outer Make exit.

| Gate | Windows x86-64 | Midcom Linux x86-64 | FreeBSD VM134 x86-64 |
| --- | ---: | ---: | ---: |
| Runtime and companion library build | PASS | PASS | PASS |
| Runtime structure | 126 graphs, PASS | 126 graphs, PASS | 126 graphs, PASS |
| fbcunit tests | 2,441 | 2,441 | 2,441 |
| fbcunit assertions, zero failures | 1,621,936 | 1,622,047 | 1,622,044 |
| Language tests, zero failures | 1,834 | 1,834 | 1,834 |
| Self-contained examples, zero failures | 654/654 | 654/654 | 654/654 |

The language totals comprise 1,740 FB, 86 QB, three fblite and five deprecated
tests on all three hosts. The assertion count difference is the existing
platform-specific test coverage; each selected assertion passed.

The secure-random and platform-service test helpers also construct native
source paths. Their focused checks pass on all three hosts. These include
controlled provider failures and single-thread idle
behavior, without displaying desktop notifications.

Exampleageddon considered all 1,773 example sources on each host. The driver
follow-up compiled 1,591 on Windows, 1,574 on Linux and 1,583 on FreeBSD. All
654 self-contained executions passed on each host, with zero execution failures
or timeouts. Other rows include platform-specific, external-library,
interactive, helper, audio and intentional-failure examples. Their classified
compile failures do not imply universal third-party library support and are
not silently counted as successful executions.

The first example sweep exposed a harness defect: SETDATE and SETTIME were
classified as self-contained and changed Midcom's clock when run as root.
The RTC retained the correct time; the system clock was restored to current
UTC and checked against that RTC. Those two examples now remain compile-only,
including with `--run-all`. Two regression tests cover classification and the
actual run decision without executing clock-changing code. All six harness
contract tests passed. Completed sweeps record current UTC before and after
execution. Midcom's system clock and RTC also matched current UTC after the
initial qualification.

Compiler SHA-256 values:

| Host | SHA-256 |
| --- | --- |
| Windows | `36aa84c8d7a76e6636e29596efc31c7f1ca89502c853fd6d71a6a7cae89fd187` |
| Midcom | `b98339b282a8b44e226af4e5db7abaf4f4c94e3fa9a53511fa3ccf904e37531b` |
| FreeBSD | `e756b9f4f3e52e8983bcbdb660ea67389e86aca50a596579b1ddbd31b3b48c5f` |

Full native summaries, archive hashes, stage exits, source hashes and graph
receipts are retained under `out/rtlib-maintenance-20261019`. The driver follow-up
is complete on all three hosts. Compact
`*-drivers-receipts.tar.gz` bundles
retain all 1,834 language-result records per host without copying binaries
or build directories.

The final source identity check covers 10,499 files on all three snapshots. One
generated test-object list is explicitly excluded from source identity and is
regenerated by the harness. The original source bundle plus one consolidated
2.2 MB overlay reproduces the frozen payload after applying its recorded old
path removals. Replay verification checks every input hash without creating
another source tree. Builds use two jobs per host. Obsolete objects at the old
driver paths are removed after retaining receipts.

Concurrent graphics restructuring edits appeared after the initial inventory.
They are preserved in the shared checkout. Native qualification identifies
the final frozen inputs in `drivers-qualified-inputs.json`, whose SHA-256 is
`276d92de8aebd4393c296dd680b3bcbcdd5775d9bbff4e576edbe335c48bac99`.
The driver follow-up captures the current graphics driver layout as well.
The final comment wrappers have identical C macro output on all three hosts;
the runtime builds therefore retain their original C implementation. The
focused BASIC include fixtures check the language boundary that C builds do
not exercise. Later source edits require their own validation.

## Strict source audit follow-up

The 2026-10-20 Windows audit ran Cppcheck over 1,690 first-party C/C++ files,
including 1,157 rtlib, 167 gfxlib2, 70 gfxlib3 and 165 sfxlib files. The
warning, performance and portability checks passed. Clang also built all four
library groups with the standard strict warning set promoted to errors. The
archive rule uses a response file so the expanded runtime object list stays
within Windows command-line limits.

Strict source-only fblint covered all six FreeBASIC build tools in these trees
with zero errors or warnings. Compiler-backed semantic analysis accepted the
DOS generator; five other helper roots remain unavailable to that mode
because the frontend rejects their module-level argument references. This is
not counted as a semantic pass for those files. The optional exhaustive and
addon Cppcheck experiments were advisory and are not part of the passing gate.

<!-- end of rtlib-restructure-verification.md -->
