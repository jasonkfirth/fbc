<!--
Project: FreeBASIC compiler verification
File: c-emitter-findings.md
Purpose: Keep the generated-C audit's actionable findings easy to review.
Responsibilities: Rank open defects, identify scoped repairs and link evidence.
This file intentionally does NOT replace raw diagnostics or claim a clean sweep.
-->

# C emitter findings

Warning cleanup remains unfinished. Completing the diagnostic inventory did not
complete the requested emitter repairs. The results below retain failing checks,
timeouts and residual warnings as unfinished work.

The audit emits C without the driver's diagnostic suppressions and checks it
with aggressive GCC warnings and Clang `-Weverything -Wwrite-strings`. The original 9,254-input
pass produced 6,466 C files, 2,787 BASIC emission failures and one missing input.
A separate 181-candidate inventory extends the total scope to 9,435. Compiler
checks and corrected project contexts remain separate from attempted coverage.
Explicit QB compatibility probes now qualify 182 unchanged legacy roots from
the original inventory, adding 364 accepted C checks without enlarging that scope.
The corrected-context warning continuation has completed all nineteen checks.
The final primary continuation has also completed all 339 groups covering 404
inputs; its original failures and timeouts remain in the receipts.
The v57/v58 array-copy repair now guards absent storage before loop arithmetic,
resets the destination descriptor after allocation failure and gives C runtime
constructor/destructor callbacks their actual `void(void*)` signature through
typed adapters. The final increment compiles eight changed compiler C profiles
and reuses 370 byte-identical profiles; all 378 complete global-symbol sets match.
The complete GCC and Clang compiler producers reproduce all twelve array-copy C
profiles. Eight affected native executions and four failing old-runtime controls
are fresh; four unaffected native executions and four controls retain exact-C
proofs. Both widths pass ordinary object checks; native execution covers Win64.
Twenty-four affected source models and four selected-project models preserve all
bytes except the two verified compiler-build-date macro values. All 26 selected
project C profiles remain byte-identical, retaining 44 object and 24 native proofs.
That v58 compiler graph reported 5,681 GCC warnings and 19,255 Clang warnings.
The v63 static-initializer repair reduces the same 189-module cohort to 3,919 GCC
warnings and 17,485 Clang warnings. Incomplete-record, missing-brace and braced-scalar
initializer warnings are zero. The final increment compiles sixteen changed C
profiles and retains 362 byte-identical profiles; fourteen changed profiles also
produce byte-identical objects. The Clang HLC object exactly reproduces the linked
emitter component. Six Win64 native runs and twelve Win32/Win64 object profiles
cover ordinary, wide, anonymous-member and descriptor initialization. Source errors
retain their diagnostic failure without an added internal error. The
[initializer contract](c-emitter-initializers.md) records these boundaries.
The following block-declaration and packed-address repair removes the implementation's
introduced warnings. The same cohort remains at 3,919 GCC warnings and falls to
10,324 Clang warnings, with complete global-symbol sets unchanged. Declaration-order
warnings fall from 6,987 to 37 and buffer warnings from 4,184 to 3,973. The initializer
families remain zero. This increment compiles 374 changed profiles, retains four
exact-C proofs and reproduces 325 previous objects. The
[block and packed-copy contract](c-emitter-blocks.md) records evaluation-order,
scope and byte-copy boundaries. Both complete compiler producers are rebuilt from
the qualified objects and each reproduces all 378 C outputs byte for byte.
Other warning families remain open. No warning option was disabled. The
[array-copy contract](c-emitter-pointers.md#empty-dynamic-array-copies-and-typed-callbacks-v57v58)
records the failure reductions, source ownership, registered gate and limits.
These scoped repairs do not finish the broader warning and structural cleanup.

The subsequent static-local repair removes the final 37 declaration-order
warnings in this compiler cohort. Its totals are 3,919 GCC and 10,287 Clang
warnings; all other warning-family counts remain unchanged. Fifty-eight C
profiles are recompiled, 320 retain exact-C proofs, and 53 fresh compilations
produce identical objects. Complete global-symbol sets still match. Persistent
scalar/array state, descriptor bounds, sibling locals and call order pass the
focused Win64 native and Win32 object controls. Broader warning and platform
acceptance remains unfinished.

The subsequent read-only access repair reduces qualifier diagnostics from 2,119
to 2,013 per compiler. Raw loads preserve a read-only storage view, and byte-copy
sources retain their constness. The same cohort now reports 3,813 GCC and 10,181
Clang warnings, with all other warning-family counts and global-symbol sets
unchanged. It recompiles 160 changed profiles, retains 218 exact-C proofs and
reproduces 158 previous objects. Four focused native runs, eight ordinary
Win32/Win64 objects and a Clang Linux argument-list object cover the changed
access roles. The [read-only view contract](c-emitter-pointers.md#read-only-load-and-copy-views-v65)
records the remaining qualifier and platform boundaries.

The subsequent byte-offset repair combines a proven native signed index and
constant correction before taking one byte-pointer step. The same cohort has
3,813 GCC and 10,125 Clang warnings; only Clang's buffer family changes, from
3,973 to 3,917. Sixteen C profiles are recompiled, 362 retain exact-C proofs,
and thirteen fresh compilations reproduce their previous object bytes.
Complete global-symbol sets match. The
[byte-offset contract](c-emitter-pointers.md#native-byte-offset-corrections-v66)
records pointer-width, conversion, evaluation and focused-test boundaries.
Both complete producers reproduce all 378 compiler C profiles exactly without
further C compilation. Thirty-six full-model validations pass, with 24 exact
before/after model comparisons and no metadata normalization.
Qualifier warnings, other buffer patterns, structural analysis and the broader
corpus/project/platform checks remain unfinished.

The earlier v56 checkpoint qualified the 189-module compiler graph under both C
compilers, with all 378 complete global-symbol sets matching, including imports.
There are 346 exact-C reused profiles and 32 fresh compilations; 26 fresh objects
are byte-identical to their prior objects. Both complete producers reproduce
96 qualified C/full-model outputs, preserving the accepted current lexer repair.
Eight changed array-index/pointer-field profiles pass sixteen ordinary objects
and eight native sanitizer runs. Twenty-six selected project profiles qualify
two fresh fbfrog objects and retain 42 exact-C objects, 24 native proofs and four
legacy models. The registered pointer-expression gate rejects the prior
comparison constraint and passes sixteen current objects/eight native controls.
Three changed BASIC files pass strict non-semantic lint with zero findings.
That graph reports 5,709 GCC warnings. Clang moves from v55's 19,235 to
19,236, with two added unsafe-buffer observations and one fewer declaration-order
observation. No diagnostic suppression was added. Eight focused before/after
full models match; four new-fixture models also pass independent source/location
validation. All eight canonical fixture C outputs match their qualified private
reductions. The [pointer-expression contract](c-emitter-pointers.md#pointer-expression-result-types-v56)
records the source changes, registered gate and ABI limits. Broader current
corpus qualification and other emitter/structural repairs remain unfinished.
The earlier v52 indexing repair increased Clang's unsafe-buffer observations
from 3,267 to 4,470; those observations were retained in its diagnostics.
The v51 explicit literal-type policy and its 1,717 added const-cast observations
remain part of that profile. Uninitialized-value and dollar-sign identifier
warnings are zero on this compiler graph. The five earlier implicit copy-source
qualifier defects retain their separate v49 proof. The six byte-copy temporary
qualifier discards are now zero in both compiler profiles. Keeping pointer-target
qualifiers exposes more explicit const casts: that family rises from 2,082 to
2,117 per compiler and remains visible. Other warning families remain open.
The [pointer/copy repair](c-emitter-pointers.md),
[typed member repair](c-emitter-members.md),
[identifier repair](c-emitter-identifiers.md),
[constant-dispatch repair](c-emitter-dispatch.md) and
[defined-path repair](c-emitter-defined-paths.md) retain their separate source
epochs and native/ABI limits. These counts do not replace the frozen corpus
results; other emitter repairs remain required.

The initial frozen-baseline results had 42 timed-out C stages. Their exact
archived C, warning commands and working directories now all pass: 41 checks
completed in the serial replay, and one passed its extended retry. The effective
baseline is GCC 6,365 accepted / 101 errors and Clang 6,451 accepted / fifteen
errors, with no unfinished C stages. `frozen-C-effective-qualification.json`
records all 42 resolved stages and retains the failed first attempt. Original
receipts remain unchanged; these results do not requalify current-source emission.
Separate repair proofs classify the 116 original error stages, leaving one
original assembly-label case open. Accepted C still has warnings.

The commands, warning profiles, source-version boundaries and detailed receipts
are documented in [C emitter diagnostic audit](c-emitter-audit.md). Evidence
paths below are relative to `build/c-emitter-audit-20261022/` in this workspace.

## Open work, in order

| Finding | Evidence | Required change |
| --- | --- | --- |
| Cross-procedure error handlers do not preserve the caller's frame. | Canonical lowering retains the Clang global access violations. The v37 runtime bridge is fragment-qualified on Windows and Linux. A private v42 outliner passes eight original, four ordinary-owner and four recursive-owner native checks. The fresh v43 current-source compiler and complete matching Win64 runtime repeat all sixteen native passes, with 189 compiler modules accepted by both GCC and Clang under unsuppressed O2 profiles. Warnings remain. It observes string destruction before frame retirement. Six model deltas preserve source graphs and payloads after identity resolution; generated helper metadata is classified separately. GOSUB and owner transfers remain open. The outliner remains private; canonical adoption and general owner-lifetime repair remain open. | Complete callable handler outlining, capture ownership and same-activation resume using the qualified bridge. [The implementation contract](c-error-frame-lowering.md) records the private proof, preserved negative controls, warnings and unsupported cases. Canonical lowering still crosses functions with computed goto, outside [GCC's supported label-value contract](https://gcc.gnu.org/onlinedocs/gcc/Labels-as-Values.html). |
| Optimized inline assembly can duplicate named labels. | Region-local labels are repaired in canonical source at v44. The current baseline reproduces three fbsound O2 failures; the repair qualifies all eight matching GCC/Clang CPU/DSP O0/O2 objects and sixteen native ASM cases. Four source-model pairs are byte-identical. `checkpoint-asm-regions-v44.json` | C Intel instructions share a template with deduplicated memory operands and unique region-local names. BASIC-label jumps retain their separate output/memory contract. Explicit global/macro names and cross-region raw control flow remain unqualified. The overall warning cleanup is open. |
| Array descriptors construct a virtual zero-index pointer outside the allocation. | Nonzero lower-bound reductions pass eight native checks but reproduce four Clang pointer-arithmetic diagnostics. `array-bias-check/qualification.json` | Review the shared FBARRAY representation and compiler/runtime indexing together, including BYDESC, static/dynamic arrays and both pointer widths. |
| General C pointer views remain incompletely qualified. | v56 repairs the confirmed integer-metadata comparison reduction and preserves its before/after controls. Array-decay aliases and other opaque pointer representations remain outside that proof. | Continue checking C expression type contracts alongside descriptor-origin arithmetic; do not infer that every typed-pointer formation is qualified from the scoped v56 controls. |

These are separate problems. Reintroducing artificial labels, adding integer
pointer casts or initializing every BASIC `ANY` local would leave their
underlying contracts unresolved.

## Changes with scoped verification

| Change | Verification |
| --- | --- |
| Preserve actual C pointer results and record subtypes in expression bookkeeping. | v56 qualifies all 378 compiler profiles with matching complete globals, 96 full-producer C/model outputs, sixteen changed-fixture objects/eight native controls and 26 selected project profiles. The registered canonical gate rejects the prior comparison constraint and passes sixteen objects/eight native runs. [The source contract and limits](c-emitter-pointers.md#pointer-expression-result-types-v56) retain exact evidence and the still-visible warning totals. |
| Preserve pointer qualifiers in byte-copy temporaries and recover text-pointer fields. | v55 removes all six implicit pointer-qualifier discards per C compiler. The complete 378-stage graph retains identical global symbols; two fresh current-lexer objects replace its earlier-source rows. Both full producers match 88 qualified C/source-model outputs. Eight descriptor fixture profiles pass 16 fresh objects/eight native runs; 26 project profiles retain 24 native proofs and qualify eight changed fbfrog objects. The registered gate rejects the prior emitter on the new case and passes four current native checks. [The source contract and limits](c-emitter-alignment.md#pointer-field-qualifiers-v55) retain the exact evidence. |
| Preserve unaligned scalar/record transfers and typed member paths. | v54 qualifies all 378 compiler stages, both full producers and 80 exact C/source-model outputs. Twenty-six project profiles pass 44 fresh objects and 24 native runs with unchanged global symbols and four legacy source models. The registered Win64 gate passes 20 current executions and preserves 20 v52 traps. Ten BASIC files have strict non-semantic lint with zero findings. [The alignment evidence](c-emitter-alignment.md) retains the rejected v53 prototype, aggregate/copy reductions, source preimages and target limits. |
| Recover typed fixed-array elements and member paths while preserving virtual-address formation. | v52 qualifies 378 compiler objects with matching complete global-symbol sets, 80 focused objects/40 native controls/20 source-model pairs, 40 full-producer C/model matches and 44 selected project objects. A focused analyzer finding falls from one to zero. Dynamic wasm32 exports pass Node before/after; full emcc remains blocked by its frozen cache. Rejected virtual-address controls and the still-failing unaligned byte-view control remain separate. See [the source contract and limits](c-emitter-pointers.md#typed-fixed-array-access-v52). |
| Qualify Clang's literal-const policy explicitly. | v51 adds `-Wwrite-strings` beside `-Weverything` in the auditor. All 189 retained compiler C modules compile to byte-identical objects while exposing 1,717 additional const-cast observations. Eleven auditor checks pass, including a real diagnostic probe and external artifact/source isolation. `E:/fbc-c-audit-01a166ce/checkpoint-clang-literals-v51.json` records the accepted profile and source preimages. The warning debt remains open. |
| Guard fixed-string array initialization after allocation succeeds. | v50 preserves four failing old NULL-fill controls and passes four guarded 16-case native controls with GCC/Clang at O0/O2. Four Win32/Win64 public-symbol pairs match. Eight separate BASIC REDIM executions pass against ordinary old/new allocator objects; both full v49 producers emit exact C/source-model pairs. `E:/fbc-c-audit-01a166ce/checkpoint-array-allocation-v50.json` records exact source pins. Installed providers are unchanged; shared descriptor/empty-copy arithmetic remains open. |
| Preserve nested pointer qualifiers and shared libc argument types. | All 100 primary GCC incompatible-pointer failures have accepted GCC/Clang counterparts with unchanged BASIC bytes. `pointer-failure-coverage.json` |
| Use the promoted C type for arithmetic aliases. | The Win64 CLONG division reduction produces the expected result with both C compilers at O0/O2. `promotion-check/results.json` |
| Emit computed-goto compatibility support only where needed. | Removes 7,818 generated unused-variable diagnostics per C compiler across the same 227 configurations. `candidate-v4/qualification.json` |
| Keep LOCAL handler state alive for the whole invocation. | Native checks cover early exits, conditional/repeated registration and nested restoration. `candidate-v5/qualification.json` |
| Normalize Clang Intel segment operands. | Eight candidate object checks pass; four old Clang rejections remain as negative controls. `candidate-v6b/qualification.json` |
| Reuse real label addresses or the procedure entry for computed-goto compatibility. | Thirty-two maintained object checks pass against eight old uninitialized-diagnostic rejections; additional controls pass 72 native runs. `candidate-v8b/qualification.json` |
| Emit plain integer zero for null equality comparisons. | Sixteen Win32/Win64 objects and eight native runs pass; four old Clang distinct-pointer rejections remain as controls. `null-comparisons-v12/results.json` |
| Preserve pointee qualifiers in fields/static initializers and give DATA payloads a const-qualified view. | Actual compiler output removes 72 GCC qualifier warnings from unchanged legacy source. The maintained DATA gate passes sixteen objects/eight native runs against eight old rejections; the composed candidate passes 84 objects, 40 C syntax checks and 24 native runs. `candidate-v14/qualification.json` |
| Declare termination contracts and remove unreachable generated tails while retaining referenced label entries. | The v19 core and forward-call gates pass 48 objects and 80 native runs. Focused path analysis changes the original null-dereference finding from one to zero. The v18 native-target checks pass 48 objects and 192 executions on Midcom, FreeBSD, Haiku and ARMhf. `termination-flow-v19/results.json`, `termination-forward-v19-qualified/results.json`, `termination-path-analysis-v19/results.json`, `native-termination-v18-qualification.json` |
| Emit prototypes for public definitions without BASIC callers. | Sixteen objects and sixteen native C-caller runs pass; eight before/after object pairs are byte-identical. Both C compilers' current-compiler missing-prototype counts fall from 1,359 to zero. `prototypes-v19-abi/results.json`, `prototype-compiler-warning-delta.json` |
| Follow live branches from procedure entries and address/ASM roots, then omit unreferenced AST labels and unused procedure entries. | Current-compiler unused-label warnings fall from 20,643 to zero per compiler; all fourteen Clang unreachable-code warnings also disappear. Before/after structured-exit, dead-cycle, backward-entry, ASM and GOSUB controls pass 96 objects and 48 native executions, including debug source tables. `label-targets-v23/results.json`, `label-compiler-warning-delta-v23.json` |
| Separate call-result scratch storage from evaluation, including ordinary aggregates. | Current-compiler jump-initialization warnings fall from 20,361 GCC / 3,574 Clang to zero. The v24/v25 aggregate fixture passes 32 objects and sixteen native runs; all four optimized pairs are byte-identical. A second 32-object/sixteen-run Windows varargs gate and sixteen native Linux object/run controls pass. `aggregate-temp-v25/controls.json`, `aggregate-compiler-warning-delta-v25.json`, `native-aggregate-v25-midcom/results.json` |
| Spell private C types with a standard identifier prefix and injective byte escapes. | Clang identifier warnings fall by 141,641. All 188 unchanged compiler modules differ only in private C type tokens. Windows alias/aggregate controls pass 64 objects and 32 native runs; Linux passes 32 objects/32 runs with sixteen identical object pairs. GCC-built and Clang-built HLC modules reproduce 48 exact C outputs. `type-name-compiler-warning-delta-v29.json`, `native-type-names-v29-midcom/results.json`, `C-built-emitter-v29/results.json` |
| Remove the normal driver's incompatible-pointer suppression. | Eight valid objects compile without warnings across both widths, C backends and assembly syntaxes; eight injected incompatible-pointer probes produce visible diagnostics. `candidate-v12/qualification.json` |
| Build normal Win32 Clang Intel objects directly. | Avoids Clang's rejected printed assembly relocation; real target-width objects pass the normal-driver gate. Explicit assembly-output requests retain their existing path and limitation. `driver-warning-policy-v12/results.json` |

The computed-goto entry-label change has the global-handler limit described above.
Its LOCAL and same-procedure controls do not establish full error-transfer
qualification. The composed candidate preserves the exact C bytes of four
LOCAL/global reductions; `data-payload-contexts-v14/qualification.json`
links their existing results and identical DATA output for four GAS/GAS64/LLVM
profiles. Installed compiler/runtime files have not been replaced. The later
termination and prototype work retains the global-handler failure above.

## Historical v29 warning cohort

All 378 v29 C checks accept their input, but warnings remain. The
following counts come from `current-compiler-v29-labels/qualification.json`; each row
counts diagnostics and affected modules, without assuming one warning is one bug.

| Warning | Diagnostics / modules | Follow-up |
| --- | --- | --- |
| GCC / Clang `-Wcast-qual` | 3,197 / 184; 1,470 / 184 | Trace remaining explicit casts to their BASIC types and mutation contracts. The qualified DATA and nested-argument repairs cover specific causes. |
| GCC `-Wmaybe-uninitialized` | 41 / 21 | Separate intentional `ANY` storage and correlated branches from missing initialization. |
| Clang conditional / sometimes uninitialized | 51 / 26; 5 / 3 | Reduce generated paths, especially runtime-call selection in `rtl-print`, `rtl-file` and `rtl-string`. |
| GCC `-Wnull-dereference` | 10 / 3 | Review `symb-struct`, `ast-misc` and `ast` against actual source preconditions. No blanket null-check repair is applied. |
| Clang `-Warray-bounds-pointer-arithmetic` | 2 / 1 | Follow the shared array representation finding above. |

Generated constructs still account for 419,906 Clang dollar-identifier warnings
and 22,823 cast-alignment warnings per compiler. Unused-label, jump-initialization and Clang
unreachable-code warnings are zero in this compiler graph. Their scale makes a categorized warning ledger necessary; these totals
are retained rather than hidden by the audit.

The private-type repair uses `fb_type_` and `_HH` byte escapes. Underscores are
escaped too, so dollar and underscore aliases cannot collapse to the same name.
Builtin `va_list` aliases keep their exact compiler-required spelling; assembler
names and RTTI strings retain their original mangling. The v29 compiler graph
has 378 accepted C counterparts: 372 reuse exact v27 C bytes, compiler identities
and warning arguments; six run again where the HLC source changes or a
conservative path-dependent-preprocessing check prevents reuse. Original actual
commands, directories and logs remain attached to every reused stage.

Forty-four fresh regression emissions also match full v27 C bytes. Their earlier
object/native receipts are retained as earlier executions. Forty-four full
semantic models validate, with all 22 before/after pairs byte-identical. Three
changed BASIC files lint clean. The new type-name test covers alias collisions,
aggregate storage, inheritance/RTTI and varargs. Its fresh alias/aggregate Windows
gate has sixteen identical normal object pairs; the Linux bundle is 12,450 bytes.
The GCC/Clang C-built-emitter controls replace only HLC and reuse 188 existing
objects, so they do not claim a compiler built wholly from C.

The complete warning delta is retained: two additional Clang declaration-order
warnings and one buffer-usage warning come from the changed HLC implementation.
No qualifier, alignment or uninitialized family is removed by this change.
`checkpoint-warning-cleanup-v29.json` seals the scoped evidence. The full current
corpus/project inventory still needs qualification with this candidate, and the
structural defects above remain open.

The prototype repair exposes another 1,729 dollar-identifier diagnostics through
the new declarations. The comparison also retains one additional alignment
warning per compiler and one Clang buffer warning from the changed HLC source.
`prototype-compiler-warning-delta.json` pins the same 503 header revisions and
188 unchanged source modules. Removing one warning family has not made the
remaining families disappear.

The later label cleanup omits definitions without AST references, while keeping
the procedure entry available until computed jumps have been lowered. If that
entry is still unused, the backend removes only its definition from the owned
procedure buffer. Preceding `#line` directives remain intact. No diagnostic is
suppressed. All 378 current-source C checks accept the result, with the same 503
header revisions and 187 unchanged source modules. The comparison also records
additional casts and buffer diagnostics in the changed emitter implementation;
`label-compiler-warning-delta-v22.json` retains every warning-family change.

The v23 refinement resolves the 193 labels retained by jumps that were later
removed. It clears the defined labels' old reference flags, keeps address-taken
and ASM targets as conservative roots, then follows reachable branches to a
fixed point. Backward entries survive; dead branch cycles do not create their
own entries. The existing emission-time scan refreshes references after late
callee inference. This changes one BASIC module, with 188 other modules and
all 503 headers unchanged from v22. No symbol layout changes are needed.
`label-compiler-warning-delta-v23.json` also retains three extra Clang declaration
warnings and nine GCC/five Clang jump-over-initialization warnings from the
changed AST implementation. These remain work to do.

The v24 temporary change separated scalar/pointer declarations from call
assignments while retaining their evaluation position. Its follow-up confirms
that `hEmitStructWithFields()` already makes C field bindings writable while
preserving pointed-to const qualifiers. The v25 change therefore uses assignment
for ordinary aggregates too. Compiler-provided `va_list` array types keep the
initializer path because C does not permit array assignment. Skipped calls leave
their scratch storage uninitialized; no default result is supplied.

The expanded aggregate fixture reproduces sixteen warnings per compiler with
v24 and zero with v25, while checking skipped/active calls, returned structures
and const-qualified pointed-to data. Six of eight normal object pairs are
byte-identical, including all four optimized pairs. Clang O0 adds return-value
copies and increases the inspected Win64 main frame by 32 bytes. Those differences
are retained in `aggregate-temp-v25/assembly-review`, rather than reported as
identical code. All sixteen native runs pass.

Eight focused Clang path analyses across old/new, Win32/Win64 aggregate and
varargs profiles report no core, unix or deadcode findings. Compiler warnings
remain in their logs. `aggregate-path-analysis-v25/results.json` records the
bounded check. The full compiler graph still has GCC 41 maybe-uninitialized
warnings and Clang 51 conditional/five sometimes-uninitialized warnings.
`aggregate-compiler-warning-delta-v25.json` retains other changes, including
another 286 dollar-identifier diagnostics. One HLC module changes, with 188 other
modules and all 503 headers unchanged from v24.

## Additional checks and limits

The v23 JRPG probe (before the temporary change) pins all 417 includes before and after emission,
with 3 changed inputs relative to the v19 probe. This is a fresh
configuration. Matching GCC and Clang O0
objects pass with unused-label and prototype diagnostics treated as errors.
Each emitted C file is 17,007,414 bytes. The owned BASIC compiler peaks near
519 MiB of working memory, below its 1 GiB private-memory cap. The game is not
run and its historical O2 checks remain separate.
`jrpg-labels-v23-current/result.json` retains memory counters and C hashes.

Twelve full semantic models from three focused sources and both C backends
validate with the existing independent reader. Procedure names, result types,
source files and line ranges match v14 despite deferred C procedure flushing.
`termination-semantic-v19/results.json` records that bounded compatibility check.

Label cleanup adds 28 validated before/after models from seven sources, including
ASM, GOSUB and checked arrays with `-e`, preserving procedure identity and source
ranges. `label-semantic-v23/results.json` records the producer paths and reader
hashes. Its current-candidate controls also pass 48 termination objects/80 native
runs, 32 checked-array object compilations, sixteen DATA objects/eight native runs
and 32 control-flow syntax checks. Four LOCAL error-frame controls pass; both
global Clang controls still access-violate in `cross-procedure-error-v25`.

Fresh v25 checks pass 32 full semantic models from eight sources against v24,
48 termination objects/80 native runs, 48 label-target objects/24 native runs,
32 checked-array compilations, sixteen DATA objects/eight native runs and 32
control-flow syntax checks. Sixteen C-export objects/sixteen native runs retain
eight byte-identical before/after object pairs. The latest four LOCAL handler
controls pass; both global Clang controls still access-violate. The large JRPG
object probe remains v23 coverage, before this temporary change.

The standalone varargs fixture checks independent `cva_copy` state, integer,
double, string and structure-pointer arguments, and a skipped/active extra read.
Its Windows gate passes 32 before/after objects and sixteen native runs. Midcom's
GCC 15.2 / Clang 21.1.8 pass sixteen native ELF objects and sixteen executions
across both fixtures, v24/v25 and O0/O2. Separate compiler assertions prove that
its `__builtin_va_list` is an array of one 24-byte element. Runtime archives and
tools remain unchanged. See `native-aggregate-v25-midcom`; the supplied compressed
C/helper bundle is 7,338 bytes, with no source-tree copy or installation.

The repository compiler-structure guard exits with eleven header/footer marker
failures in seven semantic-tooling files from the concurrent work. Those files
were preserved. `compiler-structure-v19-observation.json` records the separate
source-format check; the complete repository guard is not reported as passing.

## Keep these categories separate

| Category | Treatment |
| --- | --- |
| BASIC source and binding issues | Inspect the declaration or source flow. Examples include Araxia's mutable `memcpy` source parameter, a slideshow cleanup jump across object initialization and `fb-imageproc`'s unguarded image-height access in `Bitmap.clear()`. The last example is recorded in `imageproc-null-source-review.json`. |
| Deliberately unsafe compiler/linter fixtures | Preserve their diagnostics as expected test inputs; do not count each warning as a new emitter defect. |
| Runtime assumptions and analyzer target models | The retained v8b fifteen-context path-analysis pass completes twelve checks, times out on three and reports ten diagnostics through indirect error dispatch. Reduce those paths before classifying them as reachable source failures. |
| Compiler failures and resource limits | Preserve the original GCC internal error and every timeout. A different generated C file passing does not repair GCC or complete an unfinished check. |
| Generated GNU C constructs | Labels, computed jumps, temporaries and constructors still generate warnings. Historical output also contains dollar-sign identifiers; v45 eliminates that class on the current compiler graph. Retain counts and review the lowering contract rather than treating totals as defect counts. |

<!-- end of c-emitter-findings.md -->
