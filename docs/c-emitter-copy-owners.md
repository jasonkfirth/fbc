<!--
Project: FreeBASIC C ownership qualification
File: c-emitter-copy-owners.md
Purpose: Record generated scalar copy construction and real-project cleanup qualification.
Responsibilities: Explain the lifetime contract, diagnostic evidence and remaining guards.
This file intentionally does NOT claim general unwinding, canonical promotion or new queue totals.
-->

# Scalar copy ownership, November 5, 2026

The private compiler previously refuses managed records initialized by copying
another record. Its eligibility check requires one default-constructor call.
Valid code such as `Dim As Widget client = *w` instead calls the generated copy
constructor. This accounts for five of six guarded cleanup owners in the real
Scorched Earth graphics translation.

Private compiler `5b0de3db1950eb2170266b155e693543f38453206b8da3678def906328021986`
now recognizes either one default construction or one compiler-generated copy
construction for the complete destination. Copies take THIS and the source
record, and must match the type's registered copy constructor. User copy
constructors remain guarded. An unknown additional constructor also rejects
the shape; the check does not select one acceptable call among other calls.
A default constructor need not exist when the selected operation is a generated
copy constructor.

The existing zero-safe field contract and lifetime states are preserved. Storage
is cleared before registration; construction stays initializing until the call
returns. An interrupted generated copy may retire its owning fields without
entering a user destructor, including when a generated copy delegates to a user
LET operator. Live instances enter their user destructor once; interrupted
ordinary destruction finishes through field-only retirement. Complete-object
identity still distinguishes a first child at offset zero from its parent.
No runtime or public record ABI is changed.

The change extends the [managed-destructor candidate](c-emitter-managed-destructors.md),
not the retired [capture-region experiment](c-emitter-current-timing.md). It removes
five real guards: Widget client copies in three label routines, and tank copies
in `DrawHud` and `DrawTank`. A full function inventory confirms that six guarded
cleanup callbacks become one. The retained callback protects the fixed managed
`RtfViewParserState groupStack` array. A preliminary counter missed attributed
nonreturning definitions; `guard-verification.json` records the corrected count.

Qualification includes:

- A stable 224-unit compiler C fixed point, with 0 GCC and 1,652 Clang warnings.
- 64 fresh scalar-copy runs across both C producers/tools, GNU11/GNU23, O0/O2,
  checked modes and ST/MT archive links: 9,728 handled errors, 3,584 user
  destructor calls and 107,648 balanced allocations/releases. Emitted C has
  zero GCC warnings. Clang retains 64 global-startup warnings across the matrix.
- Eight further alignment/bounds/pointer-overflow instrumented copy runs,
  including interruption after every LET step and a copy whose type has no
  default constructor. Source strings and nested fixed string fields survive.
- Sixteen runs using the unchanged Widget headers and the exact tank declaration
  extracted from the game, eight instrumented. The observed layouts match the
  real translation's 840-byte Widget and 280-byte tank. Copies retain independent
  strings and embedded inventory while preserving borrowed pointers and callback
  fields; implicit destruction does not invoke GUI callback fields. Graphics
  initialization, rendering and image loading are not part of these controls.
- Another 48 instrumented runs cover natural layout, a first child at offset
  zero, and FIELD=1/2/4/8 layouts. All balance their allocations. The same six
  layouts also produce 24 Win32 program objects; these are after-only checks.
- 140 freshly executed ERROR-suite expectations: 132 successful runs plus eight
  expected illegal-RESUME exits, with matching outputs and no added warning class.
- 24 freshly emitted exact C identities binding the prior 192 default/user
  constructor and destructor executions to unchanged native inputs. Those 192
  executions are inherited evidence, not fresh executions in this batch.
- Seven remaining resource classes with 14 exact predecessor/current C identities
  and 112 expected refusals, including an explicit user copy-constructor case.
  A refusal is not a successful unwind.
- 32 paired object checks, including 16 Win32 objects. Before/after warning
  classes and counts match in every profile. Win32 execution is not claimed.
- Both backend suites, including mixed-header LTO controls, 14 semantic tests,
  the canonical structure guard, and strict lint of ten compiler/test providers
  with zero findings. The standalone generated real-record fixture retains one
  reviewed FBL012 recommendation about its required PASS output. Separate lint
  of all 14 generated layout/guard/real-record controls reports seven such
  recommendations and no unexpected findings.

The fresh successful qualification total is 268, excluding smoke runs and
expected refusals; 64 runs are instrumented. The directly counted copy/record
matrices balance 242,064 allocations/releases. Instrumented runs reuse eight
verified instrumented objects for the error frame, array retirement and allocation
observer. The entire runtime archive is not instrumented. ST/MT links are
single-thread tests and do not establish concurrent behavior.

The real graphics unit also passes the unchanged ordinary 90-second deadline:
GCC completes in 54.325 seconds with zero warnings; Clang completes in 35.904
seconds with its existing 74 padding and one reserved-identifier observation.
Single-run elapsed differences are not a demonstrated performance improvement.
No relaxed deadline, diagnostic suppression or rejected compile-cost attribute
is applied.

Additional controls retain ABI padding, singleton global startup/shutdown
registrations, GNU label diagnostics and two Win32 Clang possible-uninitialized
observations per compiler cut. The paired object matrix shows no increases.
GCC is warning-free in the main Win64 matrix; Clang startup observations remain
counted. Packed-layout controls also retain unaligned-member and padding
diagnostics. Passing alignment instrumentation does not erase those observations.

Reproduction scripts, input hashes, compressed C/logs, compiler diff and receipts
are under `build/c-emitter-copy-owners-20261105`. Initial harness checks are kept
separate: the predecessor has no supported object helper, and the external header
control needs an explicit compiler include directory. Neither required changes
to functional project sources. Exact project type bytes and headers are used;
assets and runtime archives are not copied.

Compiler builds use a 24 MiB retained / 32 MiB transient policy. The graphics
translation permits 32 MiB retained / 64 MiB transient physical allocation,
with at most two owned heavy workers and 512 MiB free on each volume. Installed
binaries and unrelated dirty sources remain unchanged. The candidate is private.

Fixed and dynamic object arrays, user copy constructors, inheritance/unions,
aliases, broader initialization shapes, profiling and native/platform partial
construction remain guarded or unqualified. The next concrete shared case is
the fixed managed array in the RTF parser, without changing the parser to avoid
its array. The last [paired census](c-emitter-packed-queues.md) retains its original
compiler/source cuts; no new whole-queue counts or promotion are claimed. PNG and
user-closed optional/native ERROR investigations remain excluded.

The later [fixed-array qualification](c-emitter-fixed-object-arrays.md)
repairs the remaining real parser guard through shared compiler array
cleanup. It preserves the scalar-copy contract and valid projects;
other resource and promotion limits above remain explicit.

<!-- end of c-emitter-copy-owners.md -->
