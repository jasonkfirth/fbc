<!--
Project: FreeBASIC compiler qualification
File: c-emitter-constructor-owners.md
Purpose: Record user-default-constructor cleanup and remaining acceptance limits.
Responsibilities: Bind private source, lifetime evidence, diagnostics and resource use.
This file intentionally does NOT claim general unwinding or canonical promotion.
-->

# User-constructor cleanup ownership, November 4, 2026

The private ERROR emitter previously refuses abandoned scalar records with user
constructors, even when their generated destructors visit only zero-safe fields.
The caller already clears storage before registration and marks construction as
initializing before calling the default constructor. Generated destruction can
therefore retire the initialized subset after an error interrupts construction.

The eligibility check now tests the destruction contract independently of the
default constructor's origin. User destructors remain excluded: they may inspect
values that an unfinished constructor has not established. The recursive field
check still rejects inheritance, anonymous unions, by-reference or uninitialized
fields and dynamic arrays whose elements require construction/destruction.
Exactly one direct default-constructor call is required. Parameterized calls,
aliases and top-level object arrays are not admitted by this component.

The owner state remains unseen, initializing or live. Ordinary destruction clears
the state before its call, so repeated scopes do not retire an earlier instance
twice. Partial generated destruction accepts initializing storage; user lifetime
semantics are not inferred from that rule. Existing guards remain enabled.

The prior private provider was cleanly rebased onto canonical anonymous-layout
compiler `7ec02c4e`. The rebase compiler is `473f8c3d`; the feature compiler is
`405d201e1e8a9607a46086ad1d91e230ecf85fd9eef065f0ee1c116a15661771`.
Both close all 224 C fixed points, changing only the HLC object, and retain zero
GCC / 1,652 Clang compiler observations. Canonical sources and installed binaries
are unchanged. Private ownership is not promoted.

Qualification includes:

- 64 fresh user-constructor runs across both C producers/tools, GNU11/GNU23,
  O0/O2, checked modes and ST/MT links: 10,240 handled errors, 4,096 partial outer
  transfers, 4,096 partial local exits and 247,936 balanced allocations/releases;
- 8 instrumented user-constructor runs and 8 instrumented constructor-local
  handler runs, plus 16 generated-constructor/default-array-field regressions,
  eight of them instrumented; all 96 fresh runs pass;
- 12 Win32 program objects; Win32 execution and concurrent MT execution are
  not claimed;
- two fresh expected predecessor refusals on the same new constructor fixture;
- five retained resource barriers with ten exact before/after C identities and
  80 actual expected refusals, which are not successful unwinds;
- both backend suites, including permanent mixed-header LTO checks, seven
  procedure-state tests, the compiler structure guard and strict lint of four
  BASIC files with zero lint warnings;
- 32 exact ERROR-suite C identities binding 140 historical execution expectations
  to unchanged native binaries. The 17 later native-header changes are mapped to
  the qualified pointer-view repair's exact before/after hashes. No fresh 140-run
  claim is made; the constructor matrices compile the private runtime unit and
  current array destructors explicitly against current headers.

All C checks use aggressive unsuppressed diagnostics. The main constructor matrix
has zero GCC/Clang warnings. Focused matrices are not whole-queue counts. The last
complete [paired census](c-emitter-packed-queues.md) still belongs to
bf893b74/f6b0905a, including its failed ordinary GCC deadline. No new performance
acceptance is claimed and rejected compile-cost variants remain unapplied.

Two proposed checks are resolved explicitly. Managed STRING/dynamic-array fields
already require or generate a default constructor in the current grammar; a
common managed aggregate with no constructor is not a separate missing case.
Destructor-only plain records remain a different unqualified lifetime class.
Constructors require modern FB, while RESUME requires a legacy dialect, so the
attempted constructor/RESUME fixture was invalid. Its drafts and diagnostics are
retained. The valid replacement handles an error inside the constructor and
exits normally, then checks later outer/local errors and balanced cleanup.

The warning-class review also finds that 179 of 195 canonical broad unused
parameters deliberately retain debug names (private: 177 of 193). The sixteen
nondebug cases remain open; no parameter-emission change or suppression is made.
Continuation ownership remains the higher-impact canonical family.

User destructors, dynamic object-array progress, inheritance/unions, alias and
broader initialization shapes, and native/platform partial-construction behavior
remain open. Private promotion also requires resolving the ordinary Scorched
Earth GCC time gate. PNG and user-closed optional/native ERROR checks remain
excluded. Existing valid examples/projects and concurrent Optical work are
preserved; only new compiler regression fixtures are added.

Reproduction controllers and exact source/tool/runtime hashes are retained in
`build/c-emitter-constructor-owners-20261104`. Compiler outputs and scratch use
E:, unchanged compiler objects are reused, and no runtime archive/assets are
copied. Aggregate stage storage across both volumes is capped at 32 MiB, with a
512 MiB free-space floor and at most two heavy workers. Superseded dialect
controls and the historical-header binding correction remain explicit evidence.

The final diagnostic review keeps native-module observations separate from
emitted-program counts. The zero-warning result above belongs to the main Win64
matrix. Win32 compile-only controls retain GNU indirect-goto diagnostics and two
Clang possible-uninitialized loop-variable observations. Six fresh emissions and
six Clang object checks reproduce these two observations in canonical 7ec02c4e,
the rebase and the feature compiler. Their C differs, so no whole-C identity is
claimed, but the constructor extension adds none of this diagnostic class. The
canonical header dispatch does not by itself close Clang's Win32 observation.
The diagnostic remains visible; Win32 continuation execution and a general fix
remain unqualified. Exact scopes, failed comparison assumptions and final results
are retained in `build/c-emitter-constructor-warning-review-20261104`.

<!-- end of c-emitter-constructor-owners.md -->
