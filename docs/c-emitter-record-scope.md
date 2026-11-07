<!--
Project: FreeBASIC compiler qualification
File: c-emitter-record-scope.md
Purpose: Record the ownership rule for deferred C record declarations.
Responsibilities: Explain the shared repair, unchanged examples and proof boundaries.
This file intentionally does NOT claim a complete warning cleanup.
-->

# Deferred record declaration scope, November 5

The shared C emitter now completes a record in the outer C section where it
already declared that record's tag. Static storage and generated array helpers
can require an outer declaration for a BASIC procedure's local type. Completing
the body inside the function would introduce a distinct C type with the same
spelling. BASIC scope, record fields, alignment and public layouts stay intact.

The refreshed census exposed this defect in the private compiler's output for
the unchanged `tests/string/string-fixed-array.bas`. Both GCC and Clang rejected
four descriptor-pointer assignments. The canonical compiler already accepted
that unit. The shared fix restores private acceptance and preserves the unit's
zero GCC warnings and two Clang observations: padding and startup registration.
No existing example, project program or test was changed to accommodate it.

`hEmitStruct()` records the section of an emitted forward tag using the existing
per-output type metadata. `hEmitUDT()` respects that recorded outer section
when completing a tag already marked accessed. `hCLatestViewType()` selects the
newest metadata entry because local symbol storage can be reused. Alignment
views use the same lookup. Output reset and metadata release retain their
existing ownership; this change adds no separately owned string storage.

The canonical compiler built from the actual Root source is
`0d9eb0c4ddb0b6fea4432af3281a487eeb6162f6065fffe8c30c0d702bd72fb4`.
The corresponding private compiler is
`23a9ecb6ab610cdd9a3d90278cf80833f1d8fdad5885f7fb3ba6fb487461f687`.
The staged canonical compiler is `4d00e18d`; rebuilding the actual source path
changes only its diagnostic filename constant. Each cut retains all 224 C
fixed points and **0 GCC / 1,519 Clang** compiler warnings. The private ownership
providers remain private. Installed compiler and runtime archives are untouched.

The new `c-record-declaration-scope.bas` regression checks a static holder of a
local record pointer, writes through that pointer, and clears it before return.
It also checks dynamic arrays of local fixed-string records, retained bytes
after `REDIM PRESERVE`, initial space fill and array erasure. It repeats these
ordinary operations 32 times. Checked mode reproduces the original declaration
defect with the previous private compiler; both tools now accept it.

Qualification includes:

- 64 instrumented C executions and four GAS64 controls across the two compiler
  cuts, both C producers, GCC/Clang, GNU11/GNU23 and O0/O2. Instrumentation covers
  alignment, bounds and pointer arithmetic in the generated fixture, not the
  complete linked runtime.
- Eight final Win64 C identities matching the instrumented runs, including the
  compiler rebuilt from the real Root source path.
- 32 Win32 object checks covering both cuts, modes, producers, tools and C
  standards, with incompatible-pointer diagnostics treated as errors. Their
  ordinary padding and GNU-label observations remain visible. No Win32 execution
  is claimed.
- Exact Scorched Earth C identities for both final compilers, binding the
  completed Clang checks and the separate serial GCC passes at 60.836 and
  76.193 seconds under the original 90-second deadline.
- Four successful full backend suites with the permanent regression hook,
  the compiler structure guard and strict lint of all five affected/provider
  BASIC files. The suites add 32 ordinary C executions of the new regression.
- All 32 ordinary ERROR regression C profiles are byte-identical to their
  previously accepted outputs. This rebinds the retained 140 execution
  expectations; those programs were not executed again in this component.

The backend runner resolves the C tool paths and invokes the new helper next
to the existing record-dependency checks. The helper compiles both checked and
unchecked modes, retains strict type diagnostics and bounds temporary output
and each owned child process. The new fixture was initially written with a
local type requiring automatic construction; the compiler correctly rejected
that unsupported fixture design. The accepted fixture uses a plain record for
the holder and keeps the fixed-string array case separate.

Two controller preflight corrections are retained explicitly: BASIC `-w 3`
selects the frontend warning level and must not be rejected by a C suppression
guard; original output basenames must be retained when comparing generated
module constructor names. The completed Win32 checks were reused only after
exact C-byte and command validation. Neither correction changed compiler or
regression source behavior.

Evidence is under `build/c-emitter-record-scope-20261105` and its bounded
`E:/fbc-c-audit-20261105-record-scope-*` work directories. Source preimages,
promotion hashes, compiler graphs, native logs, byte-identity bindings and
resource receipts are retained. The [paired census](c-emitter-current-queues.md)
is a separate frozen compiler cut. This correctness repair does not claim an
additional whole-corpus warning reduction. PNG and closed optional work remain
excluded.

<!-- end of c-emitter-record-scope.md -->
