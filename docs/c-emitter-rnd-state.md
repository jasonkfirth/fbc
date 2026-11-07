<!--
Project: FreeBASIC runtime qualification
File: c-emitter-rnd-state.md
Purpose: Record the private seed-fill helper and native warning qualification.
Responsibilities: Bind source ownership, recurrence, normal archives and limits.
This file intentionally does NOT claim whole-campaign or foreign-host completion.
-->

# RND seed-vector helper ownership

The shared math header previously defined a seed-fill implementation that only
the ordinary and Windows CE RND providers use. It now resides in math/rnd_state.h,
included by those two providers. The public state layout, declarations, algorithm
constants and linker names are unchanged. Carrying the previous word locally
avoids rereading the output vector. An empty fill now touches no memory.

| Native profile | GCC before / after | Clang before / after |
| --- | ---: | ---: |
| ST, 522 members | 1,009 / 1,009 | 15,686 / 14,651 |
| MT, 522 members | 1,009 / 1,009 | 15,693 / 14,658 |
| Startup | 2 / 2 | 26 / 24 |

All 2,090 native checks pass. This removes 2,072 repeated unsafe-buffer-usage
observations from unrelated translation units and the redundant read. One indexed
write remains counted in each actual RND provider. No other warning class changes
and no diagnostic is suppressed. The 1,043 unchanged GCC objects remain exact;
only the ST/MT RND objects change. Counts are observations, not independent bugs.

Thirty-two before/after C/C++ runs cover 36 seed/length combinations, an independent
64-bit modular-arithmetic reference and output guards. Lengths straddle the actual
624-word vector and include one-word and 1,000-word fills. The old zero-length write
is recorded; the new no-write and null/zero-length cases pass. Eight Win32 objects
and eight ARM, RISC-V and big-endian header/constant probes pass. These cross probes
are compilation checks, not foreign-machine executions. The actual Windows CE
provider's relative include resolves; a complete CE SDK build/run is not claimed.

Both normal archives preserve 522-member order and every export. Sixteen GCC/Clang
C/C++ executions compare old/new ST/MT public APIs across twelve algorithm/seed
cases, each with 4,096 outputs and a repeated-seed check. Seed vectors and FAST32
outputs match independent arithmetic; every before/after output record matches.
The relinked frozen compiler is byte-identical to the preceding qualified
image 3dd76a4d5fc0; its 224 exact self-host C outputs and both
complete backend suites are inherited from that exact executable, without rerunning
unchanged compiler checks.

The runtime structural check passes 126 graph cases and three collision guards
across 1,253 files and 21 groups. Its initial missing-colon header-marker failure
is retained. The comment-only correction preserves four exact preprocessed-C and
native object pairs, binding the complete native sweep to the final header.

The 76,038-byte rnd-state-source-update.zip combines this repair with the
UTF word repair. Extract portable-source.zip, string-length-source-update.zip,
then rnd-state-source-update.zip into one fresh short directory before building.
The separate utf-word update is not needed. Run the unchanged source verifier,
build.py and verify-smoke.py as described in [the portable build](c-emitter-portable-build.md).
All 610 effective runtime inputs verify through a temporary junction/hardlink
namespace; no further source tree or binary cache is copied. The package remains
a frozen Win64 compiler/runtime cut before unrelated later main-compiler edits.

Installed binaries, valid examples and unrelated dirty work are preserved. No PNG
work is involved. Retained allocation stays below 160 MiB and integration scratch
below 192 MiB. Temporary namespaces, archives, objects and programs are retired.
Another-host rebuilding, full external corpus replay from the package and
experimental promotion remain unverified. Main emitted-C totals stay broad
2,139 GCC / 6,966 Clang and Exampleageddon 3,495 / 8,916 at their measured cuts.
The overall warning-cleanup campaign remains active.

Evidence: rnd-state screen/native/archive/header-marker/structure/source-update
receipts in the existing qualification area and rnd-state-seal.json under
build/c-emitter-forward-views-20261105.

<!-- end of c-emitter-rnd-state.md -->
