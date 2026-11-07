<!--
Project: FreeBASIC C backend diagnostic audit
File: c-emitter-exampleageddon.md
Purpose: Record the complete Exampleageddon warning inventory and compiler-only repairs.
Responsibilities: Preserve source classifications, diagnostic scope and reproducible evidence.
This file intentionally does NOT equate C compilation with example execution or library support.
-->

# Exampleageddon C warning queue

The November 3 inventory includes all **1,774** BASIC sources under `examples`.
Each has an ordinary profile and a checked `-exx` profile: **3,548** recorded
profile results. The initial sweep uses the accepted C-GOSUB compiler and the
existing aggressive, unsuppressed GCC/Clang flag sets. Both tools compile the
same actual emitted C. No example is linked or executed and no asset tree is
copied.

All **655 self-contained sources** compile successfully in both profiles and
under both C tools. Across the complete inventory, each profile records:

| Result | Sources |
| --- | ---: |
| Both C tools compile successfully | 1,617 |
| Frontend failure retained for classification | 136 |
| DOS-only inline assembly rejected by the Win64 C tools | 1 |
| PNG-related sources excluded | 17 |
| Included source represented by its owning source | 3 |

The frontend failures retain their original Exampleageddon classification and
complete output. Missing includes, documented compiler options, native-only
features and other target requirements must be distinguished from compiler or
library defects. These results do not establish that an example is wrong.
Previously functional examples remain regression inputs and are not changed
to hide diagnostics.

## Current measured warning counts

Counts below cover the 1,617 successfully compiled sources in each profile.
The raw baseline also retains diagnostics from the rejected DOS translation
unit; those are kept separate from the passing native C queue.

| Profile | GCC | Clang before compiler repair | Clang after compiler repair |
| --- | ---: | ---: | ---: |
| Ordinary | 101 | 4,441 | 3,954 |
| Checked `-exx` | 3,212 | 7,674 | 7,187 |

The first compiler-only repair renames opaque bitfield storage from
`__fb_struct_body` to `fb_c_struct_body`. This removes **974** Clang reserved
identifier warnings across **192** affected profiles. Every affected C output
is checked to differ only in that generated member name, and both C tools are
run again with unchanged flags. All other diagnostics remain identical.

The member is compiler-owned storage, not a BASIC field or linker symbol.
Layout, size, alignment, initializers and signatures remain unchanged.
Sixteen native bitfield value programs, a GAS control, the existing packed and
wide bitfield constructor/threadcall gate, four foreign object targets and
strict source lint pass. No example source or installed binary changes.

The separate unfinished continuation candidate is also measured against
sixteen small warning-producing examples: GCC warnings fall from 179 to zero
and Clang warnings from 182 to two, with all C compilations passing. Those
private results are not credited to the table above or the older corpus totals.

## Shared checked-initialization cause

All 54 checked examples reporting possibly uninitialized C values now have a
separate private continuation comparison. The unchanged examples compile with
both aggressive, unsuppressed C profiles. The compiler-generated local choices
remove all 84 GCC possible-uninitialized and 184 Clang conditional-uninitialized
observations, plus 1,120 continuation observations from each tool. The smallest
case initializes its profiling context before use; the old indirect dispatcher
nevertheless exposes a C control-flow edge past that initialization.

A follow-up removes the retired header dispatcher from the private emitter,
including its unused inline native handoff and API binding. It also incorporates
the accepted opaque-bitfield member spelling. One fresh compiler object and
223 unchanged objects close the affected module's C fixed point. All 54 example
outputs and six mixed-program C outputs are byte-identical to their sealed
private baseline except for the accepted member spelling. Eighty native mixed
programs pass again, and strict source-only lint covers both staged source files.

The first maintenance cut changes the 54-program subset from 1,220 GCC /
1,334 Clang observations to 20 / 77. It exposes seven unused cleanup-helper
warnings in four multithreading examples. The following compiler graph repair
removes those seven observations from each tool, leaving 13 GCC / 70 Clang.
Padding and other retained observations are still unsuppressed.

Generated callback and cleanup bodies now own graph nodes and ordered body and
prototype spans. Their ordinary C calls belong to those nodes, and raw cleanup
destructor calls record explicit dependencies. An unused source procedure no
longer leaves its cleanup body behind or makes its callback callees global
roots. All 54 example C comparisons contain only whole-line omissions; no
surviving emitted line changes and no example is edited.

The repaired compiler passes its affected-module fixed point, 80 native mixed
programs, 16 capture-retirement programs, 28 scalar-object lifetime runs and
seven procedure-state checks. A separate small array-owning object fixture
reproduces one unused helper before the repair and zero after it under both C
producers/tools at O0/O2. All 16 native value executions pass. Strict source-only
lint covers both modified compiler files and that fixture with zero findings.
The repair reuses 223 compiler objects and the unchanged runtime archives;
its receipt is `build/c-emitter-helper-ownership-20261103/summary.json`.
[Nested native/C pending state](c-emitter-c-resume-lowering.md#native-pending-error-state)
now passes its private Win64 resumption checks with unchanged application C.
Native abandonment and broader lifetime/target contracts continue to block
promotion. These private counts do not change the
accepted queue table. The receipt is
`build/c-emitter-continuation-cleanup-20261103/summary.json`.

## Source-handler owner follow-up

The [ordinary-entry compiler repair](c-emitter-c-resume-lowering.md#ordinary-owner-entry-reachability)
now checks all 22 source-handler profiles from 11 examples. Callback-only source
tails no longer force the original owner back to GNU indirect jumps. Direct,
fallthrough, GOSUB and opaque entries retain their original C bodies.

The 20 paired private profiles fall from 78 GCC / 128 Clang observations to
17 / 39, with no newly introduced diagnostic family/count. The other two
profiles of proguide/errors/on-local.bas previously failed in the private
frontend because modern C owners lacked the internal checkpoint intrinsic.
Both now compile unchanged; each retains one unused-label observation per C
tool. These newly available diagnostics are not included in the paired count.
Compiler, value, lifetime and procedure-state receipts are in
`build/c-emitter-owner-flow-20261103/summary.json`. This remains a private
comparison; the complete queue table and older corpus snapshots are unchanged.

## Resume-only cleanup follow-up

The [owner transfer-contract repair](c-emitter-c-resume-lowering.md#resume-only-owner-transfer-contracts)
removes the two remaining noreturn diagnostics by avoiding cleanup jumps that
resume-only callbacks cannot take. All noreturn declarations and definitions
remain unchanged across the 22 source-handler profiles. The private aggregate
changes from 19 GCC / 41 Clang observations to 19 / 39. No example is edited.
The separate native value matrix also repairs an old missing-label emission
case and preserves the documented difference between non-resumable and -exx
behavior. Receipts are in
`build/c-emitter-owner-noreturn-20261103/summary.json`; accepted queue totals
and promotion boundaries remain unchanged.

## Artifacts and bounds

- `build_scripts/audit-exampleageddon-c.py` discovers and classifies the full
  inventory, honors existing per-example frontend options, and reuses owned
  C/object scratch files. Each new source cut requires its matching compiler
  and source fingerprints; historical receipts remain immutable.
- `build/c-emitter-exampleageddon-20261103/plan.json` records membership,
  source hashes and the two profile definitions.
- `results.sqlite3` in that directory retains actual C, complete compressed
  frontend/C diagnostics and per-profile results. Identical C reuses exactly
  matching C-tool checks.
- `verification.json` records the complete queue, source preservation,
  passing-profile counts, remaining failures and compiler-repair overlay.
- `build/c-emitter-bitfield-name-20261103/` retains the compiler source edit,
  candidate receipt, all 192 diagnostic comparisons, value/layout checks and
  canonical-source promotion record.

The example queue retains about 19 MiB under a 64 MiB cap and uses one
below-normal compilation worker with bounded subprocess timeouts. Original
sources and installed binaries are hash-checked. PNG work and the previously
closed emission investigation remain excluded.

The broader compiler, corpus and game cleanup remains active. The older 2,251
profile baseline remains a separate snapshot pending a coherent refresh.

## Current owner-recovery qualification

The [November 4 recovery repair](c-emitter-c-resume-lowering.md#ordinary-recovery-and-explicit-exits-november-4)
closes the private loop and ordinary-profile checkpoint regressions, preserves
shared-code RESUME, and removes nine generated compiler-helper warnings. It
passes 140 checked/default execution expectations plus the complete backend and
procedure-state gates. The 54 unchanged checked examples emit identical C to the
previous private cohort, retaining 1 GCC / 70 Clang observations. No example is
edited or executed. The complete private census and accepted stable queues still
need their own current measurements; these cohort gains do not replace them.

## Complete private census, November 4

The qualified owner-recovery compiler now has a complete fresh emission census
of all 1,774 Exampleageddon sources and both profiles. Current private counts are:

| Profile | GCC warnings | Clang warnings |
| --- | ---: | ---: |
| Ordinary | 71 | 3,949 |
| Checked | 68 | 4,047 |
| Total | 139 | 7,996 |

All 3,548 classifications are unchanged from the frozen baseline. Each profile
has 1,617 passing C translations, 136 frontend failures, 17 PNG exclusions,
three fragments compiled by their owners, and the same DOS-only lowmemas.bas
C failure. Programs are not linked or executed by this census. Examples,
installed binaries and canonical compiler source are unchanged by the census.

The frozen earlier queue recorded 3,315 GCC and 12,121 Clang observations. The
current totals include the intervening accepted bitfield/pointer repairs and
the qualified private ERROR work; the difference is not an ERROR-only delta
and is not credited to accepted whole-project queues.

The dominant remaining Clang class is padding: 7,542 observations, or 94.3% of
the total. Preserve source record layouts and foreign ABI behavior. The next
shared class to investigate is procedure-pointer conversion: 278 strict Clang
cast observations, alongside 42 GCC function/object-pointer conversion
observations and 20 GCC function-type casts. Valid examples include GTK callback
erasure and the manual's procedure-pointer covariance/contravariance examples.
These require compiler/library analysis rather than rewriting examples to avoid
language features. A genuine bad C callback signature or format call must be
established separately before treating its example as wrong.

The private census also exposes six unused-variable and seven unused-label
observations per tool that were absent from the frozen queue, plus 102 extra
Clang padding observations in private capture records. These are recorded by
profile in warning-delta.json. The new unused observations remain compiler
promotion gates; a net warning reduction does not erase them.

Every profile is freshly emitted. Exact C/tool/flag matches reuse 2,162 previous
diagnostic checks per C tool; 927 checks per tool are fresh. Filename-, include-
or time-dependent preprocessing is excluded from cross-queue reuse. This saves
4,324 redundant C compilations without copying old objects or asset trees.
The census retains about 21 MiB under its 64 MiB cap, uses one below-normal
worker, and keeps 90-second child deadlines. Raw C, logs and bindings are in
build/c-emitter-exampleageddon-owner-recovery-20261104/results.sqlite3 and
verification.json. Canonical promotion and the C:\fb_corpus/game queue refresh
remain open. PNG and the user-closed optional emission investigation remain
excluded.

## Callback and owner-use follow-up, November 4

The [callback and owner-use repair](c-emitter-callback-erasure.md) supersedes the
preceding private census with 126 GCC / 7,747 Clang warnings across all 3,548
unchanged profiles. It removes 236 Clang callback casts and thirteen unused
observations per tool through the compiler, with no new queue diagnostics.
The full census retains 7,542 natural-padding observations. All 224 compiler
fixed points and backend/semantic/lint gates pass; prior callback/ERROR executions
are bound by exact current C identities and the new owner-use matrix passes its
52 execution expectations. Examples and installed binaries remain unchanged.
The previous thirteen unused observations are closed; broader private ERROR
promotion, callback-only canonical integration and stable corpus/project queue
refresh remain open. A separately reproduced default nested-call noreturn
inference gap remains explicitly recorded in the new fixture controls.

## Canonical generic-callback follow-up, November 4

The [callback-only integration](c-emitter-callback-erasure.md#canonical-callback-only-integration-november-4)
is now in the canonical compiler source, independently of the private ERROR work.
It removes 236 Clang casts across 118 unchanged Exampleageddon profiles, with
unchanged GCC counts and no new warnings. All 224 compiler fixed points,
permanent backend/value checks, semantic-origin lanes and structure/lint gates
pass. Installed binaries are unchanged. This is a cohort measurement; full
canonical Exampleageddon and corpus/project queue refreshes remain open.

## Full canonical refresh, November 4

The full canonical callback compiler census now measures 3,285 GCC and 10,911
Clang warnings across all 3,548 unchanged profiles: ordinary 86/3,838 and checked
3,199/7,073. Each lane retains 1,617 C passes, 136 frontend failures, seventeen
PNG exclusions, three included implementation fragments and the known DOS-only
lowmem C failure. No profile changes status relative to the previous census.

The private ERROR/callback compiler bff0e2ed remains separately measured at
126 GCC/7,747 Clang. Its largest shared reductions are GNU-label and initialized
automatic-value diagnostics. Natural padding stays enabled. The new f67cd20c
handler-return repair has focused proof and has not received a full queue recount.
See the [current comparison](c-emitter-current-status.md#full-queue-refresh-and-private-handler-returns-november-4)
for queue identities, private promotion gates and the concurrent Optical limit.

All admitted sources are freshly emitted: 3,508 frontend attempts, 3,167 exact
C/tool/flag diagnostic reuses and 69 fresh C pairs. Source, header, compiler,
runtime and tool hashes remain pinned. The canonical census retains under
9 MiB of its 64 MiB cap. Examples and installed binaries remain unchanged.

## Current private full recount, November 4

The full Exampleageddon queue is now freshly rebound to private compiler
96cdd5a9 after the return, generated-helper and GOSUB prologue repairs. It
remains at 126 GCC/7,747 Clang warnings across all 3,548 unchanged statuses.
All 3,508 admitted frontend attempts are fresh, and all 3,236 emitted C profiles
match the preceding private census exactly. There are no new profile warnings;
diagnostic reuse avoids repeating those C compilations or copying old objects.

The matching stable broad queue now measures 247 GCC/6,527 Clang, down from
270/6,552 for the preceding private compiler. These counts remain separate from
canonical 2,139/8,354 broad and 3,285/10,911 Exampleageddon counts. The
[current status](c-emitter-current-status.md#current-private-queue-recount-november-4)
records the nine concurrent Optical changes, remaining ERROR promotion limits,
and the verified dead-free class behind 24 additional Clang observations.
Examples and installed binaries are unchanged; this census stays below 64 MiB.

## Capture-storage batch full recount, November 4

Private compiler ffeefd3e now measures 247 GCC/6,501 Clang across the
stable broad queue and 126/7,747 across all 3,548 Exampleageddon profiles.
No per-profile warning observations or Exampleageddon statuses regress.
Fresh frontends and exact C/tool/flag matches bind the complete queues to the
cleanup and capture-storage batch. Examples and installed binaries are unchanged.
See the [current status](c-emitter-current-status.md#capture-storage-batch-full-recount-november-4)
for source cuts, class counts, runtime evidence and remaining promotion limits.

<!-- end of c-emitter-exampleageddon.md -->
