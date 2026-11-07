<!--
Project: FreeBASIC compiler qualification
File: c-emitter-managed-destructors.md
Purpose: Record scalar user-destructor ownership and its remaining limits.
Responsibilities: Explain lifetime states, shared field cleanup and qualification.
This file intentionally does NOT claim general exception unwinding or promotion.
-->

# Managed user-destructor ownership, November 5, 2026

Private compiler `a0c65bd6d78d7689f7728cdb9f86f666211c29ab5d2228ea1cedf020e03cc6ad`
extends the [user-constructor candidate](c-emitter-constructor-owners.md). It
admits qualified scalar default construction with a user destructor when the
owning fields support zero-safe generated retirement. Canonical compiler
`7ec02c4e`, installed binaries and existing examples/projects are preserved.

An unfinished constructor must not expose its storage to a user destructor.
Likewise, a destructor interrupted by ERROR must not be entered again. The
compiler now distinguishes unseen, initializing, live and destroying storage.
Only live storage enters the user body. Initializing or destroying storage
receives field-only retirement, using the same AST field-destruction builder
as ordinary destructor epilogues. AST statement normalization also preserves
discarded array-erasure calls and the temporary descriptors they require.

The lifetime guard applies to plain scalar records as well as records with
owned fields. Complete-object recognition checks the method's declaring type;
the first child at offset zero is not the enclosing object. Scope exits and
owner returns retire pending objects once, in reverse order. The owner stays
registered until this retirement finishes, allowing an error from ordinary
final destruction to select the enclosing handler. A second error during
abandoned-frame cleanup retains the existing runtime refusal contract.

One shared retirement helper serves each owner. The compiler creates it on
the first emitted call and emits its body after handler/cleanup callbacks.
A prototype alone does not create a dependency. A terminating procedure that
never calls this helper therefore emits neither its declaration nor its body,
including when opaque assembly retains other private procedures.

The retained failures explain why the final state rules are needed. Initial
revision R1 exits 91 after calling a user destructor on unfinished storage.
R2 exits 87 after losing field retirement on an interrupted inner-scope
destructor. R3 exposes the corresponding plain-record failure, exit 92.
R4 closes these runtime failures but adds an unused helper warning in each
C compiler. R5 closes the helper-liveness regression. These observations and
their source revisions remain available without rewriting the fixtures.

Final-candidate qualification includes:

- A stable 224-unit compiler C fixed point, with 0 GCC and 1,652 Clang
  observations, unchanged from the compiler baseline.
- 192 fresh ownership executions across both C producers/tools, GNU11/GNU23,
  O0/O2, checked modes and ST/MT archive links. They handle 19,456 errors and
  balance 331,136 observed allocations/releases, with zero emitted-C warnings.
- 48 alignment/bounds/pointer-overflow instrumented executions covering natural
  layout, a first child at offset zero and FIELD=1/2/4/8 records. All pass and
  balance 256,992 further allocations/releases. Eight explicit native units
  are freshly instrumented; the full runtime archive is not instrumented.
- 16 existing generated-constructor/default-array-field executions, eight
  instrumented, and 28 Win32 program objects. Win32 execution is not claimed.
- 140 fresh existing ERROR-suite execution expectations: 132 successful runs
  and eight expected illegal-RESUME exits. Current C differs from the old
  output, so this is fresh execution evidence rather than a C-identity claim.
  Outputs match and no warning class increases.
- Six remaining guarded resource classes, 12 exact R4/R5 C identities and 96
  expected refusals. These refusals are not successful resource unwinds.
- Both backend suites, including mixed-header LTO controls, 14 semantic tests
  and the canonical structure guard. The functional suites use the private
  compiler; production source guards inspect canonical sources.
- Strict lint of five private compiler providers and four permanent BASIC
  fixtures, with zero findings. Thirteen generated controls are also reviewed;
  six FBL012 recommendations concern required PASS output and are left intact.

The main Win64 warning-free matrix is separate from the additional layout and
Win32 diagnostics. Forty-eight fresh Clang object comparisons against canonical
`7ec02c4e` reproduce the less-aligned nested-record declaration and possible
uninitialized Win32 loop-variable classes. This candidate adds neither class.
Natural ABI/capture padding and GNU label/indirect-jump diagnostics remain
visible. No C identity, Win32 execution or warning suppression is inferred.
The opaque-assembly control removes one unused helper warning in each tool
while retaining its other unused-variable/procedure diagnostics. Its initial
zero-total-warning assertion was too broad; the exact comparison is preserved.

The first R2 native probe timed out compiling unchanged error-frame C before
the BASIC fixture was emitted or executed. After verifying that worker had
terminated, one bounded retry completed. A separate inventory race observed
GCC removing scratch output; completed rows were preserved and the build
resumed with distinct check names. Neither incident required example changes.

Reproduction scripts, exact input hashes, compressed C/logs and failed controls
are under `build/c-emitter-managed-destructors-20261104`; private outputs use
the matching `E:/fbc-c-audit-20261104-managed-destructors-*` directories. The
reviewable compiler diff is `compiler-changes.diff` in that audit directory.
Unchanged objects and verified native units are reused. Closed artifacts are
compressed transparently on NTFS with their bytes and paths verified unchanged.
The final resource policy caps actual allocated storage at 56 MiB, allows an
8 MiB transient reserve, requires 512 MiB free on each volume and permits at
most two heavy workers. Logical sizes are reported separately. No assets or
runtime archives are copied.

This remains a private ownership component. Nested user destructors, dynamic
object-array construction progress, inheritance/unions, aliases and broader
constructor shapes remain guarded or unqualified. Concurrent MT behavior,
profiling-specific lifetime behavior and native/platform partial-construction
semantics are not established by this matrix. The ordinary Scorched Earth GCC
time gate remains open, and rejected cold/noipa, opaque-zero and capture-grouping
experiments remain unapplied. No fresh whole-queue totals are claimed; the last
complete [paired census](c-emitter-packed-queues.md) retains its original cuts.
PNG and user-closed optional/native ERROR investigations remain excluded.

The later [current timing check](c-emitter-current-timing.md) supplies
passing ordinary GCC/Clang evidence for this exact private compiler on
the real graphics unit. The earlier timing limit above describes the
state at this component seal. Other ownership contracts and the lack
of a fresh whole-queue census remain explicit promotion limits.

<!-- end of c-emitter-managed-destructors.md -->
