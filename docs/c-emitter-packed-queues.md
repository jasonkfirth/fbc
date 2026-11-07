<!--
Project: FreeBASIC compiler qualification
File: c-emitter-packed-queues.md
Purpose: Publish current paired warning counts and their acceptance boundary.
Responsibilities: Separate completed diagnostic information from an ordinary timeout.
This file intentionally does NOT promote private ERROR ownership.
-->

# Current paired C queues, November 4, 2026

This census binds the exact compiler cuts below. The later
[trailing-layout repair](c-emitter-packed-tail.md) is qualified separately;
these are not fresh corpus counts for that subsequent compiler.

The [packed descriptor compiler/library repair](c-emitter-packed-descriptors.md)
is in canonical source. Both compiler cuts freshly process all selected broad
and Exampleageddon profiles. Existing examples and installed binaries are unchanged.

| Queue | Profiles | Canonical GCC / Clang | Private GCC / Clang |
| --- | ---: | ---: | ---: |
| Compiler | 224 | 0 / 1,652 | 0 / 1,652 |
| Unit tests / fbcunit | 703 | 15 / 798 | 15 / 798 |
| Production corpus / projects | 1,117 | 443 / 2,545 | 86 / 2,204 |
| Additional stable projects | 207 | 1,681 / 3,359 | 146 / 1,847 |
| Stable broad total | 2,251 | 2,139 / 8,354 | 247* / 6,501 |
| Exampleageddon | 3,548 | 3,285 / 10,911 | 126 / 7,747 |

Canonical compiler: `bf893b7466560ef09c3b2d6be4729671eba6fba564fe092729ffcb9b4ce5af01`.
Private compiler: `f6b0905a0cf11112f455b2afd5aedf41c525ddddf5a1d5e788c50a990007f585`.
Counts are warning occurrences, not independent source defects. The private
compiler remains unpromoted.

*The private Scorched Earth graphics C exceeds its ordinary 90-second GCC
deadline. Two isolated ordinary runs also exceed that limit. A separate,
single-worker diagnostic run of identical C completes with zero warnings in
83.783 seconds, adding only `-ftime-report` and using a bounded 180-second
diagnostic deadline. This supplies complete warning information for those exact
C bytes. It does not pass or replace the failed ordinary acceptance gate.

All 2,251 stable canonical broad profiles pass C compilation. Both Exampleageddon
status sets match their predecessors, including the existing Windows-target
inapplicable DOS assembly example and classified frontend failures. Its 1,774
sources and all 3,548 profiles remain represented. Twelve broad and 34 Exampleageddon
PNG profiles are excluded. Six included-by-owner profiles remain classified.

The 32 Optical profiles are observed separately, at two GCC and 79 Clang
warnings per compiler cut. Concurrent Optical edits remain outside the stable
headline. Current Make membership still contains 224 compiler units and 699
selected test roots, with four fbcunit support profiles. No roots were replaced
by a small passing subset.

Diagnostic reuse requires identical emitted C, tools and flags. Every needed
frontend is freshly run; changed C is compiled under the original aggressive,
unsuppressed GCC and Clang profiles. The new census stores compressed row data
and only newly compiled C payloads, with one reusable C/object scratch pair per
worker. No assets or runtime archives are copied.

## Compile-cost disposition

Cold/noinline cleanup callbacks, noipa cleanup callbacks and an opaque boundary
around the 21 newly added managed-initialization calls each miss the unchanged
GCC deadline. None is applied to compiler source. Their remaining Clang and
runtime/compiler-integration checks are explicitly retired after GCC rejection.
The earlier rejected capture-field grouping is not retried.

The timing report attributes 66.20 of 82.66 reported seconds to function
expansion/code generation. Its completed diagnostics and the failed ordinary
runs are retained separately. No deadline exception or source workaround is
promoted. General managed/partial construction, other packed record shapes and
private ERROR ownership acceptance remain unfinished.

Receipts, source hashes, cache provenance, graph checks and dispositions are in
`build/c-emitter-packed-queues-20261104` and
`build/c-emitter-cleanup-cold-20261104`. Census work and compiler temporaries use
E:, with a shared 64 MiB census cap, at most two heavy workers and a 512 MiB
source-volume free-space floor. Closed results are relocated with verified
hashes and original-path junctions, retaining one physical copy.

The later [anonymous C record repair](c-emitter-anonymous-layout.md) is now
qualified separately. This census retains its original bf893b74/f6b0905a cuts.

<!-- end of c-emitter-packed-queues.md -->
