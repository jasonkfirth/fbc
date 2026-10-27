<!--
Project: FreeBASIC compiler verification
File: c-emitter-pointers.md
Purpose: Record pointer arithmetic and generated copy-source qualification.
Responsibilities: Explain qualifiers, overload compatibility and remaining defects.
This file intentionally does NOT claim that the overall warning cleanup is complete.
-->

# Pointer arithmetic and copy sources

The canonical source includes v56 pointer-expression result types, v55 pointer
fields, v54 unaligned scalar/record transfers and v52 typed physical indexing.
It retains the v49 pointer/copy-source repairs. The current compiler graph still
has 5,709 GCC warnings and 19,236 Clang warnings under the aggressive unsuppressed
profiles. Shared array descriptors, general pointer-view formation and other
structural work remain open. The overall cleanup remains unfinished.

## Source contracts

`backend/c/c-pointer-arithmetic.bi` owns the two pointer helpers. A byte view keeps
const on the pointed-to object. Pointer-value constness is separate and does not
make the addressed bytes const. Exact element steps retain their pointer type
and subtype through `exprNewBOP`'s pointer-result bookkeeping. Numeric
expressions continue to use `typeCBop`.

Scaling requires a complete positive element size and an integer constant with
an exact remainder of zero. An unsigned displacement above LONGINT's range is
not reinterpreted as negative. Division avoids negation and multiplication
overflow. Variable or non-divisible displacements, void/function/incomplete
pointees and pointer differences retain their existing paths. Array bounds and
the virtual-origin descriptor contract are not repaired by this change.

The fixed-array copy generator in `symbols/symb-comp.bas::hAssignList` retains
field pointee qualifiers and gives ordinary read-only source elements a const
pointer view. When an element has a mutable copy LET overload, the generator
keeps the existing mutable source view. This includes types with both mutable
and const overloads: changing that view would select a different operation.
The existing shared AST field-builder contract is preserved.

The rejected broader field-builder change and the rejected unconditional const
source change remain under `pointer-copies-v49/rejected-const-propagation/` and
`rejected-mutable-selection/`. The first rejects valid compiler semantic types.
The second bypasses the mutable LET operation and fails the native copy counter.
Neither rejected change was adopted into canonical source.

## v49 pointer/copy qualification

Existing receipts remain in
`build/c-emitter-audit-20261022/pointer-copies-v49/`.
The 189-module compiler graph passes 378 current GCC/Clang object stages. With
378 pinned controls the comparison contains 756 accepted stages. Three hundred
identical after-C objects reuse their existing object proofs; 78 are compiled
from changed C. All 378 complete public-symbol sets match. Both complete
compiler executables link against the pinned standard Win64 runtime.

| Compiler-graph diagnostic | v48 | v49 |
| --- | ---: | ---: |
| GCC total warnings | 8,384 | 8,124 |
| Clang total warnings | 19,228 | 18,974 |
| Cast alignment, either compiler | 1,561 | 1,450 |
| GCC cast qualifiers | 2,184 | 2,038 |
| Clang cast qualifiers | 480 | 334 |
| Implicit qualifier discards, either compiler | 5 | 0 |
| Uninitialized warnings, either compiler | 0 | 0 |

The historical v49 comparison above used different literal-type policies. GCC
explicitly used `-Wwrite-strings`, whereas Clang used `-Weverything`. A read-only
driver inspection with the pinned Clang executable
(`b4b8cfd80069564af4ebbca1efb5c490f8db7d9e19ca80370d8ec97220fde062`)
confirmed that `-Weverything` alone does not pass `-fconst-strings` to its C
frontend; adding `-Wwrite-strings` does. Both inspections used `-###` and syntax
checking arguments, without running the frontend or requesting object output.
Thus the two retained qualifier counts do not cover the same literal-type
policy. The existing receipts and counts remain unchanged as historical proof.

The v51 gate now qualifies all 189 retained C modules with explicit Clang
`-Wwrite-strings` in addition to `-Weverything`. It exposes 1,717 additional
`-Wcast-qual` observations: 2,051 qualifier observations and 20,691 total Clang
warnings. Every object is byte-identical to its accepted v49 counterpart, so
this profile change does not require a new link or repeat native executions.
The objects and complete diagnostics are retained at
`E:/fbc-c-audit-01a166ce/clang-literals-v51/results.json`.

The canonical auditor now requests the explicit flag. Its own script hash is
part of the cache identity, so older warning receipts cannot be silently reused.
An explicit `--artifact-root` separates an external artifact tree from the
compiler prefix; the default remains the checkout's build directory. The 11
focused auditor checks pass, including an actual literal-const warning probe,
cache reuse/input-change rejection, timeout handling, header precedence and
rejection of artifact roots in the compiler source tree. The source preimages
needed by v50 receipts are retained on E:, and the helper functions those
receipts used are unchanged. The applied v51 checkpoint is
`E:/fbc-c-audit-01a166ce/checkpoint-clang-literals-v51.json`, SHA-256
`0511a928a1de7d88e22ae18ebd7bdfb9436ad4f7ed4e250484ce411cd335e316`.

Two additional sign-conversion observations remain in each compiler's counts.
Clang declaration-after-statement observations increase from 6,920 to 6,926.
These observations are retained and remain unfinished work.

## Behavior and source models

`tests/quirk/c-pointer-offsets.bas` covers positive, negative and one-past element
steps, const pointers, runtime displacements, pointer-to-pointer strides, byte
fallbacks and single evaluation. `tests/quirk/copy-overload.bas` covers mutable-only
and dual LET overloads, plus an explicit readonly call. The existing typed-member
regression is also repeated. Each fixture passes eight before/after native
checks at O0/O2 with GCC and Clang, plus eight Win32 object checks. Win32 execution
is not claimed.

Both full compiler executables reproduce all three fixtures' executed C and
source models, adding 24 native replays. The two adopted canonical regressions
also reproduce that C and add sixteen native replays.

The pointer and overload source-model pairs match exactly. The member fixture
has precisely seventeen changed model rows: one compiler-generated source
temporary and seven nodes gain its pointed-to const bit, with matching typed
metadata. Graph edges, user declarations and value payloads are unchanged.
`qualification/validated-results.json` retains and validates this exact delta;
the failed raw-byte comparison remains in `qualification/results.json`.

The LLVM controls emit all three fixtures. Pointer and overload IR match exactly.
The member copy's qualifier conversion changes its LLVM instruction numbering
and adds identity pointer bitcasts. Its old/new LLVM outputs both compile, link
and execute successfully at O0/O2. This is focused shared-generator qualification,
not a claim about every LLVM program or target.

Test registration changes source-file hashes and byte locations in the models.
Fresh before/after models for the registered overload test match exactly, and all
eight emitted C outputs match the previously executed controls. The required
helper header marker is separately rechecked with both full compilers and both
C profiles, producing identical qualified compiler C.

## v49 projects, lint and structural observations

The affected project check accepts 44 object stages. Six changed fbfrog objects
are rebuilt. Thirty-eight unchanged fbsound, assembly, runtime/callback and
fbfrog object proofs are retained only after exact C and source/tool/runtime
checks. The selected fbfrog warning totals remain 496 GCC and 1,716 Clang.

Strict non-semantic lint reports five canonical files with zero errors, warnings
or informational diagnostics. The compiler structure guard retains the same
eleven header/footer observations in unrelated semantic-tooling files; it has
no new owned-source failures.

The v49 Clang static analysis reports no findings for either overload fixture version.
The pointer fixture retains one finding in both v48/v49 versions at the pointer-array
read. The second element is initialized through an integer address expression,
which obscures the array write from the analyzer. The v52 repair below recovers
this array access. Native success alone is not used
to declare the broader analysis clean.

The relevant paths are `exprNewVREG` and `exprNewVREGAddr` in `ir-hlc.bas`.
They convert C-array bases to integers before applying offsets. That fallback
also protects existing virtual-zero and WebAssembly lower-bound behavior, so it
cannot simply be removed. The v52 repair recovers physical element accesses
before that fallback and preserves integer arithmetic for dynamic addresses.
It does not change the shared descriptor contract described below.

The read-only continuation audit inspected the retained GCC diagnostics for all
189 modules. Of 1,700 sign-conversion observations, 1,654 occur on lines that
combine a declared C-array base with unsigned integer address arithmetic, across
94 modules. Another 248 qualifier observations share that line pattern. These
are diagnostic-line clusters, not a claim that every observation on such a line
has the same cause. The two new sign observations in the stride helper use this
same indexed datatype-table path; changing that helper's local types would not
repair the underlying array lowering.

The member matcher also deliberately requires an exact pointer representation
or matching integer size type. Reads of a declared text pointer as a runtime
`const void*`, and same-width signed/unsigned field views, can therefore fall
back to byte-address loads. The next implementation needs to distinguish a
converted rvalue from storage and address views. It must preserve the current
store/address rules rather than treating all pointer or integer representations
as interchangeable. No new implementation of that distinction is qualified yet.

Remaining work includes other array layouts and the shared virtual-origin
contract, remaining alignment/qualifier warnings, aggregate initialization,
sign conversions, declaration placement, and general error-owner lifetime and
transfer lowering. Mutable-overload copy compatibility still needs its existing
source view; qualifier diagnostics in those legacy wrappers are not covered by
the compiler graph's zero implicit-discard count. The private error outliner
must be rebased onto v54 before further integration.

Related source review found two array failure-path requirements. The implicit
dynamic-array copy previously built its end pointer before checking whether the
source had zero bytes. The runtime permits an empty source with a NULL allocation
pointer. The v57/v58 repair below now qualifies this empty path without NULL
pointer arithmetic. In `src/rtlib/arrays/array_redim.c`, the space-fill path
previously called `memset` immediately after `malloc`, before the common
allocation-failure check. That preimage matches the pinned v43 runtime source
(`a5789902f237f03a1a27a4833c16420fd543d9246b1789bb732d75773ec62109`).
The canonical source now guards the fill and retains the common out-of-memory
result; its postimage is
`577ac5adab710c27cda0711b9d00db6b90bc11a8fc1478a7828d50fd9140d1e7`.
The v50 source repair is now qualified on Win64 with GCC and Clang at O0/O2.
Installed compiler and runtime providers remain unchanged.

`tests/rtlib/array-allocation.c` and `array-allocation-hooks.h` add a deterministic
16-case regression fixture. It exercises four clear modes, with/without an
element constructor and with/without allocator failure. Only the array
allocator translation unit substitutes allocation and fill calls. The hooks
record a NULL fill instead of executing it, while error-context allocation
continues to use the real runtime allocators. Normal cases check initialization,
constructor counts and descriptor metadata; failures require the out-of-memory
result without initialization or constructor calls. All four old native
controls report exactly one NULL-fill attempt and fail the intended assertion;
all four guarded controls pass all 16 cases without a NULL-fill attempt.

The focused qualification compiles the pinned preimage and current source at
O0/O2 with GCC and Clang, using explicit `-Wwrite-strings` for Clang. It compiles
the fixture normally and instruments only the array allocator by defining
`FB_ARRAY_ALLOC_INSTRUMENT` and force-including the hook header. It uses the
pinned v43 headers and links the replacement allocator object before that runtime's
`fbrt0.o`/`libfb.a`. No archive or source-tree copy is needed. The old control
reports one NULL-fill attempt and fails; the guarded source passes all 16 cases
with no NULL-fill attempt. Ordinary, uninstrumented objects pass both-width
compiler and public-symbol checks: four before/after symbol sets are identical.
The fault-control receipt is
`E:/fbc-c-audit-01a166ce/array-allocation-v50r2/results.json`.

The ordinary BASIC caller `tests/quirk/c-array-allocation.bas` separately checks
repeated fixed-string REDIM space filling, zeroed integer arrays and constructor
values/counts. Both full v49 compiler producers emit exactly matching C and
source models for each C backend. Four user-code objects, compiled at O0/O2
with GCC and Clang, link against both ordinary allocator versions and pass all
eight native executions. The BASIC source also has a strict non-semantic
fblint summary of one file with zero errors, warnings and information messages.
Its receipt is
`E:/fbc-c-audit-01a166ce/array-allocation-basic-v50r2/results.json`.
The exact-current-source qualification/adoption checkpoint is
`E:/fbc-c-audit-01a166ce/checkpoint-array-allocation-v50.json`, SHA-256
`4d08be6ab2f30407660304f9c825cfbf492595732e59aef0056367d2ca176bf5`.
The shared descriptor virtual-origin contract remains open. The later v57/v58
qualification covers empty-copy arithmetic and repeated allocation failures.

No compiler or runtime was installed, committed or pushed. Existing C: evidence
is retained in place. Subsequent task-owned scratch, build and cache outputs are
to use E: with the existing 128 MiB cohort cap and 5 GiB free-space reserve.
The continuation root is `E:/fbc-c-audit-01a166ce`. The storage recovery receipt
was independently hashed before admission, and current disk, memory and process
guards passed before the new cohort. Canonical project source and these reports
remain in their established checkout.

## Typed fixed-array access, v52

`backend/c/c-array-index.bi` owns recovery of a physical element index from the
IR's scaled byte index and symbol bias. Matching storage emits `ARRAY[index]`
and declared member paths. Native unsigned arithmetic preserves wrapped index
normalization without negating an INTEGER_MIN lower bound. The helper borrows
the original index tree until every layout check succeeds, then transfers its
ownership, preserving exactly one evaluation. Other ranks, dynamic/bydesc
storage, special string/va_list layouts and incompatible byte views retain the
existing fallback.

Taking a dynamic array-element address has a separate contract. BASIC permits
forming a virtual zero-index address outside a nonzero-bound allocation without
accessing that address. Emitting `&ARRAY[index]` unconditionally introduced a
bounds-sanitizer trap in the first prototype. That rejected binary and its failing
controls remain in `array-index-v52/rejected-virtual-address/`. The adopted
expression keeps the owned element metadata but emits native integer address
arithmetic for a dynamic address; ordinary element reads/writes remain typed.
Both GCC and Clang virtual-address controls pass after this correction.

| Compiler-graph diagnostic | v49 with v51 literal policy | v52 |
| --- | ---: | ---: |
| GCC total warnings | 8,124 | 6,820 |
| Clang total warnings | 20,691 | 20,593 |
| GCC sign conversion | 1,700 | 413 |
| Clang sign conversion | 1,709 | 418 |
| GCC cast qualifiers | 2,038 | 2,014 |
| Clang cast qualifiers | 2,051 | 2,027 |
| Cast alignment, either compiler | 1,450 | 1,450 |
| Clang unsafe buffer usage | 3,267 | 4,470 |
| Uninitialized warnings, either compiler | 0 | 0 |

Clang's buffer observations increase when indexing becomes visible. They remain
in the full diagnostics; this repair adds no warning suppression. All 189
modules qualify with each C compiler. Of 378 after-object stages, 204 are fresh
and 174 retain exact-C object proofs. All 378 complete global symbol sets,
including imports, match their controls. Both full compiler producers link.

Five focused fixtures pass 80 object stages, 40 native controls and 20 exact
before/after source-model pairs across both widths and C backends. These include
24 fresh objects and 12 fresh native runs; 56 object and 28 native proofs are
retained after exact-C comparison. Both full producers reproduce 40 exact C and
source-model outputs. Three adopted canonical files have strict non-semantic
fblint summaries with zero errors, warnings or informational diagnostics.

The selected fbsound, fbfrog, assembly and legacy controls accept another 44
object stages: 31 fresh, including Clang's explicit `-Wwrite-strings` profile,
and 13 exact-C retained proofs. Four native runs are fresh; 20 retain exact C or
object proofs. Four legacy source models match their retained controls exactly.
Clang's focused pointer-fixture path analysis changes from one undefined-pointer
finding to zero. This is a bounded analysis result, not a clean corpus claim.

Actual emitted wasm32 functions also pass a Node-driven dynamic-index control
before and after the change. It supplies indices externally and checks member
loads, member writes, nonzero bounds and virtual/one-past addresses. The direct
SDK LLVM compile/link needs no runtime libraries or cache writes. A separate
full emcc attempt remains incomplete because the frozen installed SDK cache
lacks `libclang_rt.builtins.a`; the full FreeBASIC JS runtime is not qualified.

`tests/quirk/c-array-index-unaligned.bas` recorded a separate case unresolved by v52.
A dynamic byte-array address cast to a LONGINT pointer still uses the raw
fallback. Both old and v52 output trap with GCC and Clang alignment sanitizers
under the normal required flags. The relevant word read/write C is identical
before and after this repair. Retaining origin/alignment through incompatible
storage views and lowering unaligned scalar transfers safely required a separate
repair. The adopted v54 controls below now cover this reduction.

All new evidence is under `E:/fbc-c-audit-01a166ce/array-index-v52/`: the focused
`qualification-r2/results.json`, `compiler-matrix/results.json`,
`full-contract/results.json`, `projects/results.json`, `analysis/results.json`,
`wasm-index-exports/results.json`, and `alignment-probe/results.json` retain
their exact inputs and limits. `source-adoption.json` records the three adopted
files and preserved HLC preimage. Completed model TSVs are stored as lossless
gzip with restoration hashes in `compressed-models.json`; project/full-contract
models are compressed immediately. No installed provider was replaced.

## Unaligned scalar and record transfers, v54

The adopted [alignment repair](c-emitter-alignment.md#typed-alignment-views-adopted-v54)
preserves declared field paths with compatible alignment-one C typedef views.
Whole-record value reads copy into aligned temporaries; storage/address paths
preserve BYREF identity. Copy/fill builtin arguments explicitly use byte pointers
so Clang does not attach a false natural-alignment promise to raw storage.

The complete compiler graph falls from 6,820 to 5,736 GCC warnings and from
20,593 to 19,558 Clang warnings, using the current canonical source/header
preimages. All 378 global symbol sets match. Both full producers reproduce
80 C/source-model outputs, and 26 selected project profiles pass 44 fresh objects,
24 native runs and four legacy source models. The registered Win64 gate passes
20 current sanitizer runs and preserves 20 corresponding old-output traps.
Warnings remain active; broader corpus, descriptor and error-owner work remains.

## Pointer fields and byte-copy qualifiers, v55

Byte-copy load/store temporaries now retain the IR value's pointer-target
qualifiers. Their own binding remains writable. The member matcher also keeps
pointer depth when distinguishing character arrays from character-pointer
fields. Ordinary text pointers and fixed-string array descriptor `.DATA`/`.PTR`
fields therefore retain declared C member access; raw fixed strings retain
their separate array/byte path. Descriptor layout and virtual origins do not
change.

The compiler graph has zero implicit qualifier discards, down from six per C
compiler. Overall totals fall to 5,709 GCC and 19,235 Clang warnings, while
explicit const-cast warnings rise to 2,117 per compiler as readonly value types
are preserved. The [qualified alignment report](c-emitter-alignment.md#pointer-field-qualifiers-v55)
records the 88 full-producer C/model comparisons, new negative regression,
fresh descriptor controls and retained project proofs. Remaining warnings,
pointer-expression bookkeeping and shared descriptors still require work.

## Pointer expression result types, v56

A byte-pointer addition could retain an unsigned-integer result type in the C
expression tree. Its emitted C was still a pointer expression. A later IR
conversion to that same integer type therefore disappeared, leaving an invalid
pointer/integer comparison in `c-pointer-offsets.bas`.

`ir-hlc.bas::exprNewBOP` now treats C pointer arithmetic separately from numeric
promotion. Pointer-plus-integer and pointer-minus-integer results keep the
pointer's dtype and record subtype; commuted addition keeps the right operand's
pointer type. A direct C pointer difference has signed native-integer metadata.
The final vreg conversion can then insert an integer-address conversion when
requested by the IR. The exact-stride emitter path uses this common bookkeeping
instead of patching its result afterward. Numeric `typeCBop` behavior is unchanged.
The member helper's explanatory comment no longer relies on the old mismatch.

The new `c-pointer-expression-types.bas` reduction covers record subtypes,
pointee qualifiers, commuted addition, negative differences, one-past addresses,
single evaluation and mixed integer promotions. The frontend still lowers
ordinary BASIC pointer differences through integer-address nodes. Those runtime
controls preserve that behavior; they do not claim direct coverage of the
internal C pointer-difference constructor branch.

The canonical qualification retains these separate checks:

- All 378 compiler profiles preserve complete exported/imported global-symbol
  sets. Of these, 346 reuse exact C; 32 compile freshly, and 26 fresh objects
  match their prior object bytes. All 2,201 graph input pins are rechecked.
- Both complete GCC/Clang producers reproduce 96 exact C/full-model outputs
  across twelve fixtures, both pointer widths and both C backends.
- The eight changed array-index/pointer-field profiles pass sixteen ordinary
  objects and eight Win64 native runs with undefined-behavior traps enabled.
- Twenty-six selected project profiles qualify two fresh fbfrog objects and
  retain 42 exact-C objects, 24 native proofs and four exact legacy models.
- The registered gate rejects the prior compiler at the comparison constraint
  and passes sixteen current objects/eight native sanitizer controls. All eight
  canonical fixture C outputs match the qualified private reductions exactly.
- The changed HLC source, member helper and new BASIC fixture pass strict
  non-semantic lint with zero errors, warnings or informational findings.

Eight focused before/after full models match, including four independently
validated new-fixture models. Earlier expression-only packets remain separate.
Four focused Win64 GCC/Clang analyzer checks report zero analyzer findings.
The compiler graph remains at 5,709 GCC warnings; Clang moves from 19,235 to
19,236, with two added unsafe-buffer observations and one fewer declaration-order
observation. Explicit const casts remain at 2,117 per compiler and implicit
qualifier discards remain zero. These diagnostics are retained without hiding
any warning family.

The standalone regression command is:

```text
python build_scripts/test-c-emitter-pointer-expressions.py --fbc FBC --gcc GCC --clang CLANG --runtime-dir WIN64_RUNTIME --output NEW_DIRECTORY
```

The output directory must be new. The gate retains full diagnostics and verified
compressed artifacts, checks Win32/Win64 objects, and executes Win64 controls.
It does not qualify other ABIs, array-decay aliases, descriptor virtual origins,
empty-copy NULL arithmetic or cross-procedure error handlers. Broader current
corpus qualification and the other warning families remain open.

Evidence is under `E:/fbc-c-audit-01a166ce/pointer-expression-types-v56/`.
`source-adoption.json`, `compiler-matrix/results.json.gz`,
`full-contract-inputs/results.json.gz`, `changed-contract/results.json.gz`,
`full-producers/results.json.gz`, `projects/results.json.gz`,
`canonical-gate-verification.json` and `canonical-lint.json` retain the exact
scope and inputs. Verified provider/object/model archives and compact record
streams preserve their contents within the existing 128 MiB retained-artifact
cap. The source preimages are referenced rather than duplicated. Installed
compiler/runtime files were not replaced.

## Empty dynamic-array copies and typed callbacks, v57/v58

`symbols/symb-comp.bas::hAssignDynamicArray` checks the destination storage after
`fb_ArrayRedimTo`, then checks source storage before forming the source end
pointer. Both checks branch to the existing loop exit. Empty sources and failed
allocations therefore skip element access without NULL pointer arithmetic.
Nonempty copies, bounds, self-assignment and element lifetimes retain their
existing behavior.

`rtlib/arrays/array_redimto.c` calculates storage bias only for a nonempty source,
before destructor callbacks. On allocation failure it calls `fb_ArrayResetDesc`
before returning out-of-memory. The old runtime left a positive byte count after
losing storage, so a later assignment retried an invalid descriptor. Repeated
failure controls require a reset descriptor, one allocation attempt and the
expected error without extra constructor/destructor calls. Descriptor layout
and exported definitions are unchanged; the runtime object gains the expected
reference to the existing reset helper.

The stronger ordinary-copy sanitizer check also exposed a distinct C defect:
an element method declared `void(Type*)` was called by the runtime as
`void(void*)`. Compatible machine argument passing does not make these C
function types compatible. The original Clang control traps on Windows and the
Linux diagnostic identifies the incompatible indirect call.

`backend/c/c-array-callbacks.bi` now owns parameter spelling and per-unit
adapters for the six array constructor/destructor runtime entry points. Each
adapter has the runtime signature and explicitly converts its element argument
to the original method's declared pointer type before calling it directly.
Null callbacks use integer zero. The cache owns emitted names and type spellings
until the next emission unit or backend shutdown; it does not borrow released
local symbols. Prototypes precede callers and bodies follow the original typed
procedures. These helpers add no BASIC procedure or exported ABI definition.

`tests/quirk/c-array-copy-values.bas` covers empty and nonempty fields, strings,
two-dimensional bounds, self-assignment and constructor/destructor/LET behavior.
The two failure fixtures use `tests/rtlib/array-copy-hooks.c` and its header to
inject failure only into the replacement copy allocator. The ordinary runtime
and user C both retain undefined-behavior instrumentation, including Clang's
function-type checks. The public Windows gate is:

```text
python build_scripts/test-c-emitter-array-copies.py --fbc FBC --gcc GCC --clang CLANG --runtime-dir WIN64_RUNTIME --output NEW_DIRECTORY
```

An optional `--baseline-runtime-source OLD_ARRAY_REDIMTO_C` retains the failing
repeated-copy controls. The complete matrix checks 24 ordinary Win32/Win64
objects, twelve Win64 native runs and eight old-runtime controls. The final
callback refinement freshly checks sixteen changed objects, eight native runs
and four controls; the other eight objects, four runs and four controls retain
their exact-C proofs. No Win32 native execution is claimed.

Both full compiler producers retain all complete public-symbol sets. Twenty-four
affected full source-model comparisons and four legacy project models change
only `__FB_BUILD_DATE__` and `__FB_BUILD_DATE_ISO__` from the verified October 25
compiler identity to October 26. The comparison validates those named records
and requires every other binary byte to match. It does not ignore general
metadata. Twenty-six selected project C outputs remain byte-identical, reusing
44 qualified objects and 24 native proofs.

Evidence is under `E:/fbc-c-audit-01a166ce/array-callbacks-v58/`:
`compiler-matrix-r3/results.json.gz`, `full-providers-r3/results.json.gz`,
`array-copies-r3/results.json.gz`, `source-models-r3/results.json.gz`,
`selected-projects-r3/results-date-qualified.json.gz` and
`canonical-source-adoption-r3.json`. The earlier allocator controls include
28 native runs on Linux, FreeBSD and Haiku; the incompatible-callback Linux
failure and its corrected native control remain separate receipts.

The compiler warning totals remain 5,681 GCC and 19,255 Clang observations.
Aggregate initialization, declaration placement, qualifier conversions, shared
descriptor origins, general error transfer, broader corpus/project coverage
and the remaining analyzer checks are unfinished. No compiler or runtime was
installed. The retained scratch cap and free-space reserve are unchanged.

## Read-only load and copy views, v65

Raw C loads use a const-qualified view of the stored object. The loaded value
then retains its original IR type, including the qualifiers of any pointer
stored inside it. A const container therefore does not make that pointer's
target const. This access role also covers cases where lowering has lost a
source qualifier that remains present in the emitted C declaration.

Writes and address-taking request an lvalue view explicitly. Builtin variadic
operations also consume their argument-list objects as lvalues; the source of
`cva_copy` is an object reference rather than an ordinary value load. Its state
remains independent of the destination, as the existing variadic control checks.

Byte copies always give their source a read-only byte view. The destination
keeps its existing writable-storage rules. Packed and unaligned transfers retain
their byte-copy path, sizes and layout. These changes belong to
`c-pointer-arithmetic.bi` and the raw access/copy paths in `ir-hlc.bas`.

The new packed-record control copies plain data and a managed string from a
const source, then changes the destination and checks that the source retains
its values. Its two qualifier diagnostics per profile become zero. Together
with the existing variadic control, it passes four native Win64 runs and eight
GCC/Clang Win32/Win64 object profiles. A separate Clang Linux x86_64 object checks
the true builtin argument-list array form; it is object-only evidence. Four
changed or exercised BASIC files pass strict non-semantic lint without findings.
The first candidate's remaining scalar-load qualifier failure is retained.

The comparable 189-module cohort passes all 378 profiles, recompiling 160
changed C outputs and retaining 218 exact-C proofs. Of the fresh compilations,
158 reproduce their previous object bytes. Complete global-symbol sets match.
Qualifier diagnostics fall from 2,119 to 2,013 in each compiler; all other warning
family counts remain unchanged. Totals are 3,813 GCC and 10,181 Clang warnings.
Declaration and initializer warning families remain zero. The running Clang
compiler already contains byte-identical versions of all 189 qualified objects,
so its link is retained. The GCC producer is linked from its qualified objects
and reproduces all 378 C outputs exactly without another C compilation.

The remaining qualifier, buffer, ABI/lifetime and broader corpus/platform gates
remain open. No installed compiler or runtime is promoted by this repair.

## Native byte offset corrections, v66

The C backend combines a proven native signed index and a constant byte
correction before taking a pointer step. For example, `*(p + i - 2)` can
address an array's last element even though `p + i` is outside that array.
Emitting the correction as integer arithmetic avoids that artificial
intermediate pointer. The backend's existing `-fwrapv` contract retains
native signed arithmetic.

The proof accepts ordinary mutable or const byte pointers and signed
integer values whose actual C width matches the target pointer width. It
checks symbol declarations or explicit casts, rather than trusting the IR
type alone. Existing conversions of wider or unsigned inputs remain before
the addition. Constant corrections must fit the native signed range. Other
pointer types, unknown expression forms and targets with a pointer size
other than four or eight bytes retain their existing path.

The rewrite retains the owned address tree and its casts, transfers the
correction once, and introduces no symbols or repeated evaluations. The
existing cast optimizer can then remove redundant cast/restore pairs.
An intentional conversion to a mutable byte pointer still emits its cast
and its qualifier diagnostic. This is an address-expression repair; it does
not prove that a source program's final address is within bounds.

`tests/pointers/byte-offset-correction.bas` checks reads and writes at both
array ends, large offsets, target-width conversions, single evaluation and
an explicit mutable view. The old and revised producers each pass two
Win64 native runs and four GCC/Clang Win32/Win64 object profiles. Clang's
buffer diagnostics fall from sixteen to nine in each focused profile;
qualifier diagnostics fall from seven to two in both compilers. The retained
diagnostics include the intentional mutable conversion and an index form
outside this proof. Three source files pass strict non-semantic lint with
zero findings. Win32 native execution and the broader platform gates remain
open.

The comparable 189-module cohort completes all 378 profiles with 3,813 GCC
and 10,125 Clang warnings. Only Clang's buffer family changes, from 3,973 to
3,917. Declaration and initializer families remain zero. Sixteen changed C
profiles are recompiled, 362 retain exact-C proofs, and thirteen fresh
compilations reproduce their previous object bytes. Complete global-symbol
sets match. Both complete producers link their 189 qualified objects. Thirty-six
full semantic models pass independent source/location validation, and the two
complete producers each reproduce twelve baseline models and C outputs byte
for byte. No metadata is normalized. Each producer's four byte-offset fixture
C outputs also match the qualified controls, retaining their object and two
Win64 native proofs.

Both complete producers also reproduce all 378 compiler C profiles exactly,
with zero further C compilations. The evidence is retained under
`E:/fbc-c-audit-01a166ce/compiler-warnings-v63/pointer-offsets-r8/`.

## Native offset expression proof, v67

The byte-offset proof also accepts declared native signed fields, signed
literals, unary negation/complement and native signed arithmetic, bitwise and
shift expressions. Both operands of an admitted binary expression must have
a proved native signed C type. Comparison results and arbitrary C text do not
establish that type. Explicit casts retain their conversion point, and member
nodes use the actual field type recovered from the declaration.

Each address has a budget of 64 visited expression nodes. Running out of that
budget retains the existing path. The proof allocates no symbols, evaluates no
operands and does not copy the expression tree. Existing shift-count masking
and the backend's signed wrapping contract remain in force.

`tests/pointers/byte-offset-expressions.bas` covers sums, products, shifts,
negation, masks, native signed wrapping, declared fields and a call evaluated
once. Its record array has a negative lower bound: a valid element can require
a negative scaled index followed by a positive field displacement. The corrected
C combines those integer terms before taking its single pointer step, for reads
and writes, while retaining layout and byte-copy access.

The old producer passes two Win64 native controls and four Win32/Win64 object
profiles. The revised producer passes four native runs and eight object profiles
across this fixture and the previous byte-offset control. In the new fixture,
Clang buffer warnings fall from twenty to eleven, and both compilers' qualifier
warnings fall from eight to one. The previous fixture's counts remain unchanged.
The unproved call-result text retains its existing path and diagnostic. Three
source files pass strict non-semantic lint with zero findings. A separate native
Clang probe checks a negative signed shift under `-fwrapv` with shift traps.

The comparable compiler cohort completes all 378 profiles with 3,808 GCC and
9,754 Clang warnings. Qualifier warnings fall from 2,013 to 2,008 in each
compiler; Clang buffer warnings fall from 3,917 to 3,551. Other warning-family
counts remain unchanged. Seventy-eight changed C outputs are recompiled, 300
retain exact-C proofs, and 73 fresh objects reproduce their prior bytes.
Complete global-symbol sets match. Three serializer objects change in addition
to the two backend HLC objects; coherent-producer and full-model acceptance
of this extension remains pending. The original corpus/platform gates remain
open.

<!-- end of c-emitter-pointers.md -->
