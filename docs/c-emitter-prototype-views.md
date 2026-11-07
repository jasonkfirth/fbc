<!--
Project: FreeBASIC compiler qualification
File: c-emitter-prototype-views.md
Purpose: Explain deferred record dependencies in C procedure prototypes.
Responsibilities: Document pointer alignment, declaration identity and qualification.
This file intentionally does NOT claim complete warning cleanup or private promotion.
-->

# Deferred prototype record views, November 5

Procedure prototypes need compatible pointer types but do not need the complete
layout of every pointed-to record. The C emitter now keeps those dependencies
forward-declared until an actual object, field access or by-value operation
requires completion. This extends the existing pointer dependency rule without
changing public layouts or the source programs being compiled.

The pointer-view cache records whether an alias preceded record completion.
Prototype declarations keep their first alias spelling, including after the
record becomes complete. Exact declaration matching can then omit redundant
prototypes. Actual accesses use a completed-record alias with lowered alignment.
GCC can forget lowered alignment on an incomplete record typedef; Clang retains
it. Reusing the early alias for accesses would therefore break packed receivers
with GCC. The completed alias preserves those accesses while retaining the same
underlying compatible C type and ordinary pointer ABI.

The promoted changes are confined to `backend/c/ir-hlc.bas` and
`backend/c/c-pointer-views.bi`, plus the permanent backend regression hook.
The prototype-context counter is balanced around header generation and reset
for each output. Cache cleanup retains the existing string ownership rules.
No existing example, application or test program was changed for this repair.

The compiler built from the actual Root sources is `73073eab`. The separate
private compiler is `d2d583cc`; its ownership providers remain unpromoted. Both
224-unit compiler graphs reach a C fixed point with **0 GCC / 1,509 Clang**
observations: 1,496 padding and 13 startup registrations. The staged canonical
compiler is `d406302e`; rebuilding the Root source path changes one diagnostic
filename constant. All other compiler C output identities are preserved.

The eight-profile GTK, SDL3 and variant screen removes **138 Clang padding
observations per cut**, with no other warning-class changes. The completed
3,550-profile Exampleageddon census then removes **2,872 Clang padding
observations per cut**, with every other warning class unchanged. Canonical
counts are 3,538 GCC / 9,020 Clang; private counts are 126 / 5,599. The complete
broad recount separately removes 885 Clang padding observations per cut:
canonical 2,139 GCC / 7,314 Clang and private 247 / 5,461. Both broad cuts
accept all 2,251 stable profiles. The separately completed private Scorch GCC
compile uses unchanged C and flags; its earlier timeout observation remains
recorded without retaining the old 90-second acceptance threshold.
An earlier experiment omitting alignment views increased alignment warnings and
was rejected. The accepted implementation keeps alignment views and distinguishes
prototype spelling from completed access aliases.

Qualification covers:

- 168 instrumented C executions and eight GAS64 controls across both compiler
  cuts, including the new packed-receiver fixture and the existing declaration
  scope and record-dependency fixtures. The matrices cover both C producers,
  GCC/Clang, GNU11/GNU23 and O0/O2; dependency controls include mixed LTO.
- Four complete backend suites with the permanent regression hook, 14 semantic
  contracts and the compiler structure guard.
- 80 Win32 program objects, 16 C observer objects and eight mixed GCC LTO objects.
  This is object qualification; no Win32 execution is claimed.
- Strict lint of the actual Root/provider BASIC files: five files, zero errors,
  warnings or informational findings.
- 32 fresh ordinary ERROR C emissions matching the retained accepted outputs.
  These bind 140 earlier execution expectations; they were not executed again.
- Ten fixture C identities from the actual Root compiler matching the staged
  qualification, with no unnecessary repetition of native execution.

The new fixture puts a receiver one byte into allocated storage, checks direct
and indirect method access plus neighboring guard bytes, and frees the original
allocation. Its portable `CUInt` alignment check produces the same Win64 C bytes
as the original native-only cast in all 12 baseline/current contexts. The fixture
ordering was corrected to ensure prototypes actually precede record completion.
Both changes affect only the new regression, with earlier failures retained.

Evidence is in `build/c-emitter-forward-views-20261105` and its retained
`E:/fbc-c-audit-20261105-forward-views-*` work areas. Native temporary outputs are
reused, compressed retained data is bounded, and compilation is serial for the
body recount. Identical compiler and C-blob copies share hard links with their
paths and hashes preserved. Installed compilers and archives remain unchanged. PNG and the
user-closed optional investigations remain excluded.

<!-- end of c-emitter-prototype-views.md -->
