<!--
Project: FreeBASIC C backend
File: c-emitter-callback-erasure.md
Purpose: Document typed storage for generic callbacks and ERROR owner emission.
Responsibilities: State contracts, complete warning counts and remaining qualification.
This file intentionally does NOT claim canonical ERROR promotion or a zero-warning queue.
-->

# Generic callback and owner-use qualification, November 4

The private compiler lowers dynamic conversions between a typed procedure pointer
and a generic, zero-argument SUB pointer through its existing representation-copy
path. Both signatures must have the same calling convention. Separate exact-type
carriers, a size assertion and typed null handling preserve pointer identity. The
operand is evaluated once at its expression site, including lazy expressions.
The generic pointer is a stored address: a foreign dispatcher must restore the
actual callback signature before invocation. This does not adapt an incompatible
call. Already-compatible generic signatures, typed-to-typed conversions, differing
conventions and static constant initializers retain their existing lowering.

The ERROR emitter now retains original label names only when an emitted branch,
address, recovery entry or cleanup transfer requires them. Ordinary fallthrough
still emits the source statements. Unknown/GOSUB graphs keep the conservative
label behavior. Generated callbacks omit an unused capture alias only after
checking their emitted declarations and body. They explicitly discard the unused
ABI argument after the declarations; debugger builds retain the binding. Actual
capture use, callback arguments and owner cleanup contracts are preserved.

No existing example is changed. The complete fresh emission census covers all
1,774 Exampleageddon sources in ordinary and checked profiles:

| Profile | GCC warnings | Clang warnings |
| --- | ---: | ---: |
| Ordinary | 65 | 3,825 |
| Checked | 61 | 3,922 |
| Total | 126 | 7,747 |

The preceding private census recorded 139 GCC / 7,996 Clang observations. The new
compiler removes 236 strict function-pointer-cast warnings from Clang and thirteen
unused-variable/label warnings from each tool, with no new queue observations.
The 131-profile affected cohort is checked first; all remaining emitted C is
verified against the exact sealed baseline. C bytes and raw diagnostics remain
in their existing SQLite receipts, referenced by the small full-queue manifest.
The census does not link or execute examples and preserves all 3,548 statuses:
per profile, 1,617 C passes, 136 frontend failures, seventeen PNG exclusions,
three included fragments and the same DOS-only lowmemas.bas C failure.

All 224 compiler C fixed points pass, with zero GCC and 1,652 Clang observations
(1,639 padding and thirteen global initializations). Current C identities bind
the prior 72 callback executions, four native controls and 140 ERROR expectations
without counting them as fresh runs. The callback fixture checks exact foreign
calls, pointer identity, nulls, single evaluation and lazy branches; separate
static/typed-boundary controls deliberately retain their original cast warnings.
Eight earlier compiler controls also remain recorded. The additional owner-use
matrix freshly runs forty successful C programs, eight expected status-5
rejections of RESUME without metadata, and four same-activation native controls.
It checks normal returns, shared labels and unused-context handlers. Both C
producers, GCC/Clang, GNU11/GNU23 and O0/O2 are represented. The backend suite,
seven procedure-state checks and lint of all created BASIC providers/fixtures
pass. The thirteen affected source profiles also pass GNU11 diagnostics.

Four default-profile owner-test Clang compilations retain one missing-noreturn
and one unreachable-code warning each, reproduced with the preceding compiler.
The fixture invokes a non-returning worker inside a condition. The AST termination
test currently recognizes direct statement calls and does not infer that nested
expression as terminal. This remaining compiler issue is recorded with both raw
controls; it is not suppressed or counted as a new corpus regression. Checked
executions resume successfully. The earlier all-native cross-procedure failure
is outside this matrix and is not retried.

Clang retains 7,542 natural-padding observations, 97.4 percent of this queue.
Packing or adding fields to suppress them can break the native/foreign aggregate
ABI; the earlier padding experiment failed LTO compatibility checks. Keep these
diagnostics visible. Forty-two strict function-pointer casts, forty-two GCC
static function/object-pointer observations and twenty GCC typed-function casts
remain distinct boundaries. Valid FreeBASIC variance examples require a compiler
call-ABI solution; a representation copy alone cannot repair their C calls.

The candidate is build/c-emitter-callback-erasure-20261104/compiler-r4/fbc.exe
(SHA-256 bff0e2ed51117fac97f7f30060417127480fc9223ef1da05b2a81e1acac96992).
Use its summary.json current_provider_map and matching prefix-thin-r2 runtime.
Canonical sources and installed binaries are unchanged. ERROR promotion still
requires broader managed/partial-lifetime and allocation qualification, unproved
callee/native crossings and wider targets. The next integration step is to
separate the callback-only change onto the canonical pointer-order compiler,
add permanent regression coverage and refresh the corpus/project queues. Those
accepted queue totals have not been replaced by this private Exampleageddon run.

At most two below-normal heavy workers are used, with bounded child deadlines
and a 32 MiB component cap. Objects and C/diagnostic receipts are shared; no
runtime archive members or project assets are copied. Retired executables have
hash journals. PNG and the user-closed optional emission work remain excluded.

## Canonical callback-only integration, November 4

The dynamic generic-callback change is now in the canonical C emitter and its
new c-callback-representation.bi helper. Same-convention typed/generic callback
values use exact typed storage, null handling and expression-site evaluation.
The foreign dispatcher restores the actual signature before calling it. Typed
variance casts and static initializer conversions keep their previous spelling
and warnings; four before/after C identities verify these boundaries. Existing
examples are unchanged. Private ERROR providers and runtime changes are separate.

The current canonical compiler is
build/c-emitter-callback-canonical-20261104/compiler/fbc.exe, SHA-256
93ea32d4db7d6dbb030fbfd548fc9cc1b5daa82c4c9d5d92838732464eb3ebd9.
It uses the existing build/c-emitter-c-gosub-20261103/prefix runtime. All 224 C
fixed points pass; only HLC is recompiled and 223 exact objects are reused.
Compiler diagnostics remain zero GCC and 1,652 Clang. A footer-only correction
is separately bound to byte-identical HLC C after the fixed-point build.

All 118 affected Exampleageddon profiles are freshly emitted and compiled with
both the previous and new canonical compilers. GCC stays at 108 observations;
Clang drops from 4,289 to 4,053, removing 236 callback casts with no new warning
observations. These are affected-cohort counts. The full 126/7,747
Exampleageddon measurement above still belongs to the private ERROR candidate;
the full canonical and stable corpus/project queues need coherent refreshes.

Permanent foreign-callback coverage passes 32 dedicated native C executions
across both generators, GCC/Clang, GNU11/GNU23, O0/O2 and ordinary/checked
profiles. It covers callback calls, identity, nulls, single evaluation and lazy
branches. The general backend suite includes four additional focused runs and
passes all existing gates. Two GAS controls and eight Win32 objects pass. The
historical compiler reproduces eleven strict-cast warnings in the new fixture;
the current compiler has none. Four semantic-origin tests pass in each of GCC,
Clang and LLVM lanes. Structure passes 397 documented files and 21 host graphs;
all three changed/created BASIC files lint clean, including the corrected footer.

Combined backend and semantic invocations originally hit their 180-second
deadlines and receive no pass credit. Splitting full callback matrices by C
dialect and semantic tests by backend completes the same checks within bounds.
The general backend runner selects a focused callback profile for routine use.
Receipts, preimages, source journals and limits are in this stage's summary.json
and verification.json. Installed binaries and runtime archives remain unchanged;
the component is below 32 MiB with at most two heavy workers. PNG and the
user-closed optional emission work remain excluded.

## Broad comparison and completed-cleanup returns, November 4

The full canonical and private queue refreshes are now recorded in the
[current status](c-emitter-current-status.md#full-queue-refresh-and-private-handler-returns-november-4).
The stable broad total is 2,139 GCC/8,354 Clang canonically and 270/6,552 for
private compiler bff0e2ed. The private reduction includes all 1,820 GNU-label
and 70 GCC/110 Clang initialization observations. Canonical Exampleageddon is
3,285/10,911; private Exampleageddon is separately measured at 126/7,747.
Examples, game sources and installed binaries remain unchanged.

The next private compiler, f67cd20c, repairs outlined handler returns after
array or UDT cleanup. Its plan assigns a separate actual-exit recovery target
only when the handler graph reaches that label after cleanup. The callback
keeps the real label, returns the existing owner-recovery action and the owner
dispatches to its actual exit label. Existing safe early-cleanup boundaries
remain distinct, preventing double destruction. Runtime/public layouts and
APIs are unchanged. Four providers are staged under c-emitter-owner-return-20261104;
the compiler remains private with the existing thin-r2 runtime.

The unchanged Stunts module loses both return-type warnings per tool and adds
none. Sixty-four C executions and two native controls verify values and exactly
once normal cleanup. All 224 compiler fixed points, backend checks, seven
procedure-state tests and seven-file lint pass. Thirty-two exact C identities
bind the previous 140 ERROR expectations. Indirect destructor-RESUME fails
identically in the before/after controls and remains explicitly unqualified.
The broad totals precede this focused repair; unused cleanup helpers and
jump/declaration placement remain promotion gates. No whole-goal completion or
canonical ERROR promotion is claimed.

## Generated ERROR helper ownership follow-up, November 4

The next private compiler removes all eighteen extra unused cleanup functions
across thirteen unchanged project profiles. Generated callbacks now follow
their owner's dependency graph in debug builds rather than being retained as
source-defined procedures. Both C tools add no cohort warnings. All 224 fixed
points, separate GCC/Clang backend lanes, seven procedure-state tests and
two-file lint pass. Existing return/ERROR expectations are bound by forty exact
C identities. The [current status](c-emitter-current-status.md#generated-cleanup-ownership-follow-up-november-4)
records compiler be90ed1e and the unchanged thin-r2 runtime. Broad counts still
belong to bff0e2ed; GOSUB prologue placement and other ERROR promotion limits
remain open. Functional examples and installed binaries are unchanged.

## GOSUB prologue follow-up, November 4

The private compiler 96cdd5a9 now places complete owner setup before the GOSUB
dispatcher, preserving native parameter conversions and exactly once capture
retirement. The unchanged quirk/gosub unit loses three jump warnings per tool
and two Clang declaration-order observations. Eighty-four fresh successful
executions, four expected illegal-RESUME controls, all 224 fixed points and
backend/semantic/lint gates qualify the component. Current broad queue totals
await a full private recount; examples and installed binaries remain unchanged.
See the [current status](c-emitter-current-status.md#gosub-owner-prologue-follow-up-november-4)
for the retained native-parameter declaration debt and ERROR promotion limits.

## Nonreturning cleanup follow-up, November 4

Private compiler d1c35ed7 removes all 24 extra Clang unreachable observations
across the fifteen affected unchanged profiles. Unsupported-resource callbacks
omit a free after their nonreturning barrier and its unused capture alias;
GNU11 and C23 retain a correct ignored-context ABI. Supported cleanup behavior
and the explicit unsupported-resource boundary are preserved. All 224 fixed
points, normal/boundary runs, backend/semantic/lint gates and prior C identity
bindings pass. Full queue totals remain the 96cdd5a9 census until rebind.
See the [current status](c-emitter-current-status.md#nonreturning-cleanup-follow-up-november-4).

## Nonresumable GOSUB capture storage, November 4

Private compiler ffeefd3e removes the obsolete unconditional GOSUB heap-capture
requirement. Actual ERROR/retry checkpoints and opaque assembly retain heap
storage. Automatic storage borrows an early-initialized GOSUB context safely
through runtime unwinding. All 224 fixed points, 112 current runtime cases,
backend/semantic/lint gates and 44 prior C identities pass. Existing examples
and installed binaries are unchanged. Full queue totals await a measured rebind.
See the [current status](c-emitter-current-status.md#nonresumable-gosub-capture-storage-november-4).

<!-- end of c-emitter-callback-erasure.md -->
