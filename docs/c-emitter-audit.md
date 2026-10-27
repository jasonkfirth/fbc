<!--
Project: FreeBASIC compiler verification
File: c-emitter-audit.md
Purpose: Describe unsuppressed generated-C audits and their evidence boundaries.
Responsibilities: Document reproducible inputs, warning profiles and finding triage.
This file intentionally does NOT equate successful compilation with correct code.
-->

# C emitter diagnostic audit

The [ranked findings](c-emitter-findings.md) summarize open compiler/runtime work,
scoped repairs and the evidence needed for follow-up.

The diagnostic inventory is complete, but warning cleanup and structural repairs
remain active. Accepted C, a completed inventory, or a documented open defect
does not satisfy the goal of fixing the emitter.

`build_scripts/audit-c-emitter.py` checks generated C independently of the
compiler driver's warning suppressions. It accepts an explicit JSON plan and
records BASIC emission, GCC compilation, Clang compilation and optional
Cppcheck results separately. Original BASIC sources stay in their project
directories; `-r -o` places intermediate C under the audit output directory.

## Running an audit

Each plan contains a `units` array. A unit provides a unique safe directory
`id`, absolute `source` and `cwd` paths, `group`, `project`, and a `flags` array
containing the project's BASIC dialect, definitions, include roots and main
module selection. Paths and flags are passed as arguments, without a shell.
An optional `input_identity` field can pin a wider source inventory.
Select standalone entrypoints with `-m MODULE` as required by their normal
build. Emission through `-r` otherwise uses a library role; main-only builtins
such as `__FB_ARGC__` can then appear undefined in an otherwise valid program.

For example, with native tools on PATH:

```text
python build_scripts/audit-c-emitter.py --plan build/c-audit/plan.json \
  --output build/c-audit/results --fbc bin/fbc.exe \
  --gcc /path/to/gcc --clang /path/to/clang
```

Use explicit executable paths. Output must be below the checkout's `build/`
directory. The runner permits one or two workers, defaults to a 30-second
limit per stage and a 2-GiB output budget, and refuses new work below 2 GiB
of free storage. Large diagnostics are gzip-compressed. It emits no object
or assembly artifacts during C checks: optimized assembly goes to the null
device. Completed receipts are reusable only while their pinned source,
tools, runner, options and configuration identity agree.

`--structural --cppcheck /path/to/cppcheck` adds a Cppcheck pass configured
for Win64. Other target ABIs require a corresponding analyzer configuration;
changing only the compiler executable is insufficient.

The runner regression check is:

```text
python build_scripts/test-c-emitter-audit.py --fbc /path/to/fbc \
  --gcc /path/to/gcc --clang /path/to/clang
```

It verifies isolated public-header emission, real C compilation, unsuppressed
warning flags, failed-emission handling, source preservation, reuse of completed
receipts, rejection of changed input, timeout receipts and project-header
precedence over installed headers. The public nine-case integration passes with
the composed compiler candidate; `public-audit-integration-v14.json` records it.

Focused emitter regressions accept the same explicit compiler paths:

```text
python build_scripts/test-c-emitter-gotos.py --fbc /path/to/fbc \
  --gcc /path/to/gcc --clang /path/to/clang --output build/c-goto-check
python build_scripts/test-c-emitter-qualifiers.py --fbc /path/to/fbc \
  --gcc /path/to/gcc --clang /path/to/clang --output build/c-qualifier-check
python build_scripts/test-c-emitter-segments.py --fbc /path/to/fbc \
  --gcc /path/to/gcc --clang /path/to/clang --output build/c-segment-check
python build_scripts/test-c-emitter-error-flow.py --fbc /path/to/fbc \
  --gcc /path/to/gcc --clang /path/to/clang --output build/c-error-flow-check
python build_scripts/test-c-emitter-null-comparisons.py --fbc /path/to/fbc \
  --gcc /path/to/gcc --clang /path/to/clang --output build/c-null-check \
  --runtime-dir /path/to/win64-runtime --baseline /path/to/old-fbc
python build_scripts/test-c-emitter-warning-policy.py --fbc /path/to/fbc \
  --gcc /path/to/gcc --clang /path/to/clang --output build/c-warning-policy \
  --runtime-dir /path/to/win64-runtime --asm intel
python build_scripts/test-c-emitter-data-payload.py --fbc /path/to/fbc \
  --gcc /path/to/gcc --clang /path/to/clang --output build/c-data-payload \
  --runtime-dir /path/to/win64-runtime --baseline /path/to/old-fbc
python build_scripts/test-c-emitter-termination.py --fbc /path/to/fbc \
  --gcc /path/to/gcc --clang /path/to/clang --output build/c-termination \
  --runtime-dir /path/to/win64-runtime
python build_scripts/test-c-emitter-termination.py --forward --fbc /path/to/fbc \
  --gcc /path/to/gcc --clang /path/to/clang --output build/c-termination-forward \
  --runtime-dir /path/to/win64-runtime
python build_scripts/test-c-emitter-prototypes.py --fbc /path/to/fbc \
  --gcc /path/to/gcc --clang /path/to/clang --output build/c-prototypes \
  --runtime-dir /path/to/win64-runtime --baseline /path/to/old-fbc
python build_scripts/test-c-emitter-labels.py --fbc /path/to/fbc \
  --gcc /path/to/gcc --clang /path/to/clang --output build/c-labels \
  --runtime-dir /path/to/win64-runtime --baseline /path/to/old-fbc
```

These cover Win32/Win64 C emission and both C backend selections. Each output
directory must be new. `--expect-rejection` verifies the preserved compiler's
specific diagnostic in the qualifier/control-flow checks. The segment check
instead accepts `--baseline /path/to/old-fbc`, verifies four old Clang object
rejections, and compares GCC output and quoted assembly data byte for byte.
The checked-array test uses both `-e` and `-exx`, two procedures and a nested
scope. It builds 32 real objects across both widths, C backends, C compilers
and O0/O2. `--expect-uninitialized` verifies that the preserved compiler
reproduces the specific temporary-initialization diagnostic.

## Warning profiles and language contracts

Clang runs with `-Weverything`. GCC has no equivalent switch; the runner
combines `-Wall -Wextra -Wpedantic` with conversion, qualifier, alignment,
prototype, format, bounds, string-operation, shift, initialization, logical,
allocation and related diagnostics. The complete profile is in the runner
and in each stage's saved command. This is an aggressive C11/GNU C audit,
not an attempt to enable mutually incompatible historical C dialect checks.

Neither compiler receives `-w` or a `-Wno-*` warning suppression. Both run
through `-O2` code generation because some GCC diagnostics require optimized
dataflow analysis. `-fwrapv`, `-fno-strict-aliasing`, `-frounding-math` and
`-fno-math-errno` preserve the C backend's language contract. Removing those
options would test different arithmetic, aliasing or floating-point semantics.

The ordinary driver suppresses unused constructs, some GCC indentation checks,
and Clang uninitialized analysis. This audit removes its blanket incompatible
pointer-type suppression after repairing the generated casts and null comparisons.
Compiler build flags add missing-initializer, builtin-declaration,
maybe-uninitialized and type-limit suppressions. The audit bypasses those
command builders rather than overriding a subset of their flags.

## Receipts and interpretation

Each unit retains `unit.c` and `result.json`. A stage records its exact command,
working directory, native exit status, elapsed time, compressed raw log,
compressed structured diagnostics, counts and representative locations.
`tools.json` pins executable bytes and versions. `summary.json` is published
only after all selected units finish.

The public generator now appends the compiler include fallback after explicit
project flags. Its frozen-C continuation retains the exact previously imported
script bytes. `public-helper-include-order-change.json` records the source
boundary: only the BASIC-emission function changes, and the continuation calls
the unchanged digest, invocation and receipt functions. Stored C, tools and
warning profiles remain independently verified.

An `emitted` unit means BASIC emission succeeded. It does not imply that GCC,
Clang or an analyzer accepted the C. Likewise, Cppcheck can return zero while
reporting syntax or internal-AST errors. A timeout is incomplete analysis.
Inspect each stage's diagnostics as well as its process status.

Classify findings before proposing changes:

| Finding class | Required follow-up |
| --- | --- |
| C emitter or ABI defect | Reduce to valid BASIC and reproduce in raw emitted C. |
| Missing runtime contract | Verify the real runtime behavior before adding C attributes. |
| BASIC source or binding defect | Trace the warning back to its source declaration or control flow. |
| Intentional generated construct | Retain the diagnostic and explain the lowering requirement. |
| Analyzer limitation | Record unsupported syntax, incomplete paths or exhausted budgets. |
| Emission blocker | Record missing includes, dialect, target, definitions or dependencies separately. |

Labels, `$` identifiers, computed jumps, generated constructors and temporary
stores dominate some warning totals. Their counts do not measure the number
of compiler bugs. Cppcheck 2.21 rejects some GNU label-address constructs that
GCC and Clang accept, so those files need an analyzer that understands the
emitted dialect. Clang's path analyzer accepts those constructs but can still
exhaust a bounded analysis budget on large procedures.

Tool references: [GCC warning options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html),
[Clang diagnostics](https://clang.llvm.org/docs/DiagnosticsReference.html), and
[Clang static analyzer](https://clang.llvm.org/docs/ClangStaticAnalyzer.html).

## October 22, 2026 audit

The audit and its completed continuations are under
`build/c-emitter-audit-20261022/`. Its frozen
Win64 compiler executable was built from the current Make-selected 189-module
source graph. Compiler input hashes agreed before and after that build.
The executable identifies itself as FreeBASIC 1.20.5-2 and has SHA-256
`c6d35dd2efc8ebd3b1c612e9a7bd3f26c9c897094977cdd348fa946876ccdc71`.
Installed compiler and runtime files were preserved.

An early post-build comparison found three parser/semantic source changes.
`compiler-input-drift.json` records those first differences. Independent source
work has continued since that comparison. The audit executable's hash is
unchanged, so the baseline still measures one fixed emitter. Isolated repair
candidates retain their own source, object and tool manifests.

The inventory contains 19,034 BASIC files. The emission plan selects 9,254
candidate roots: 189 compiler modules, 1,763 compiler tests, 1,774 examples,
2,773 corpus roots and 2,755 local game/project roots. Intentional negative
compiler fixtures and separate compiler forks are recorded as exclusions.
Generated build trees and asset-only files are excluded from discovery.
Discovered files are candidates, not a claim that every file is a standalone
program or that its normal project build has been reconstructed.

A later discovery review checks extension casing explicitly. It finds 269
additional uppercase/mixed-case source names and 81 files added since the
original snapshot. `additional-inventory.json` pins all 350 observations.
It records 157 historical Visual Basic input assets outside FreeBASIC's
grammar, nine timestamped backups, two deliberate failing tests and one BBK
Basic source as exclusions. The remaining 181 candidates form a separate
repaired-compiler batch, leaving the frozen 9,254-entry baseline unchanged.
The combined candidate scope is therefore 9,435, with compiler versions and
coverage reported by cohort. QBX/VBDOS profiles are explicit compatibility
probes; they do not claim original VBDOS platform or API qualification.

A later review of the original failures finds 182 unchanged standalone roots in
`EXTRACTED FROM FBC/Legacy` that need a compatibility dialect. All lack explicit
language, command-line and include directives. An explicit `-lang qb` probe
emits all 182 and passes 364 full-profile GCC/Clang checks; the eight-source
pilot and disjoint 174-source remainder are sealed in
`legacy-primary-context-pilot/qualification.json` and
`legacy-primary-context-rest/qualification.json`. Their generated C contains no
inline assembly. These are additional configurations of the original inventory,
with a separately pinned compiler epoch. No source edit, application execution
or original-platform qualification is claimed. Modern included fragments with
missing declarations retain their own context failures.

The recorded tools are GCC 16.2.0, Clang 22.1.7 and Cppcheck 2.21.0, all
native x86-64 Windows tools. The priority plan uses maintained corpus flags,
TurboTrek's production source manifest, explicit game entry points and the
compiler's Make source list. The wider plan also attempts discovered roots
and retains their dependency, target and dialect failures.

The original 9,254-entry first pass is complete across five disjoint cohorts:
6,466 emit C, 2,787 fail BASIC emission, and one input is missing. The separate
181-candidate original-profile sweep is also complete. This accounts for every
selected inventory attempt; failed emission is not successful C coverage.
`primary-inventory-completion.json` verifies primary ID membership and excludes
overlap. Extended primary C checks are terminal. Documented project-context
recoveries are terminal, with accepted output, frontend failures, changed-source
boundaries and deferrals recorded separately. This document does not claim
completed project or platform qualification.

The completed primary C error ledger contains 116 failing compiler stages across
115 inputs. The qualifier repair covers 100 stages; 14 are resolved by matching
backend/ABI contexts or identify the GCC-specific builtin fixture. The segment
repair resolves another stage, leaving duplicated assembly labels as the one
unresolved primary compile-error case. `c-error-disposition.json` links every
stage to its evidence.
Supplementary native findings and timeouts retain their separate receipts.

The terminal primary overview includes all completed continuations and actual
assembly-object amendments. Each tool has one effective result for each of the
6,466 baseline C outputs:

| C compiler | Accepted | Compile errors | Incomplete (timeout) |
| --- | ---: | ---: | ---: |
| GCC | 6,359 | 101 | 6 |
| Clang | 6,415 | 15 | 36 |

These are frozen-baseline results. The repair and corrected-context proofs above
explain their dispositions without overwriting rejected original C. The remaining
42 timeout stages retain their exact commands, C hashes and diagnostic receipts
in `final-accounting.json`; a finished controller does not make them accepted C.
That receipt also pins inventory, repair, analyzer and context-coverage summaries.
All 9,435 selected attempts are accounted for. Warnings remain in accepted C.

Those 42 historical timeouts now have accepted exact-C counterparts in
`frozen-C-effective-qualification.json`. Forty-one pass the serial replay and
one passes the extended retry, for 43 attempts. Effective frozen primary totals
are GCC 6,365 accepted / 101 errors and Clang 6,451 accepted / fifteen errors,
with no unfinished C stages. Original receipts remain unchanged.

An additional lexical scan verifies all 6,466 immutable C archives and ignores
assembly keywords inside C strings and comments. Thirty-seven translation units
contain actual assembly statements. Their 74 object stages include 54 fresh
checks and 20 reused checks with identical C, tools and argument profiles.
Sixty original stages pass; fourteen reject. Corrected backend/ABI contexts and
the qualified DOS segment repair account for thirteen rejections. Named-label
duplication remains unresolved in the last case. The resulting source-context
coverage is 36 qualified and one unresolved. The rejected original C and every
corrected proof remain separate in `primary-assembly-objects/qualification.json`.

A separate scan covers 1,023 completed supplemental source profiles. Fifteen
contain assembly, across nine distinct BASIC files. Their 30 original object
checks have 16 acceptances and 14 rejections. Six rejected stages qualify after
the pointer repair or matching backend selection; six retain optimized label
failures, and two belong to the Linux-only DSP plugin outside the Windows graph.
The resulting source coverage is six qualified, two unresolved and one outside
the native graph. `supplemental-assembly-objects/qualification.json` links every
original rejection to its disposition.

Fresh matching-backend checks with candidate-v6b reproduce the fbsound CPU and
DSP failures: all four O0 objects pass, while three of four O2 objects reject
duplicate labels. GCC rejects both modules; Clang rejects DSP. These results
are retained in `fbsound-assembly-v6b/qualification.json`.

The include continuation has now finished all 414 planned attempts: 345 emit
C, 64 fail during BASIC emission and five defer changed inputs. GCC accepts
all 345 outputs. Clang accepts 338, rejects four assembly-bearing outputs and
times out on three. `include-continuation/completion.json` verifies retained
C and records later source changes separately.

All five additional assembly profiles have real object checks. The recovered
MP3 stream example passes four matching GCC/Clang O0/O2 objects with the current
candidate. The callback example has its own four accepted matching objects;
CPU/DSP retain the optimized-label finding. ALSA host objects pass, but its
Makefile selects that source only for Linux. It remains outside the Windows
native graph. `include-continuation-assembly-objects/qualification.json` links
these results without counting host object acceptance as Linux qualification.

New recipe replays retain small source, recipe and selected-header snapshots
compressed by content hash. Source and selected-header hashes are checked again
after BASIC emission. Recipe and selector scripts document the frozen command;
the replay does not execute them. Qualification verifies those exact retained
documentation bytes even if the live helper later changes. It also verifies
the exact historical compiler inputs when
the working project later changes; it explicitly reports whether current
files still match. A small control passes both C compilers, retains valid
historical qualification after fixture edits, and rejects a corrupt snapshot.
This adds no project-tree or asset copies. Earlier cohorts without snapshots
keep their original current-file checks; missing old recipe bytes are not
reconstructed or silently waived.

The refreshed Optical pilot passes eight C checks with captured inputs. The
next batch emits eighteen C files: all pass GCC, seventeen pass Clang, and one
Clang check times out. A further helper edit deferred its remaining eighty
inputs. Their completed replay emits another seventy-nine C files, all accepted
by both compilers. One newer graphics fixture fails during BASIC parsing at its
four-argument PAINT forms. The composed 106-profile plan therefore has 105
emissions: GCC accepts all 105; Clang initially accepts 104 and times out on one.
`optical-header-context-coverage.json` verifies unique source membership across
the four sealed epochs. One subsequently changed source and one selected header
are explicitly refreshed. These checks retain the previous compiler epoch
so exact C/tool/command matches can reuse completed checks; the current emitter
candidate is qualified separately below.

The independent `context-warning-600/qualification.json` continuation verifies
nineteen retained C files against their original warning profiles, including
that Optical timeout. All nineteen Clang checks pass. Their C hashes are
disjoint from every frozen primary retry group; no successful check is repeated.

The original eight emitter repairs are qualified in isolated candidates:

| Repair | Result |
| --- | --- |
| Preserve nested pointer qualifiers and shared libc argument types | All 100 baseline GCC incompatible-pointer failures have accepted GCC/Clang counterparts with unchanged BASIC bytes. |
| Use the promoted C type for arithmetic aliases | The Win64 CLONG division reduction produces the correct result in all four native compiler/optimization checks. |
| Emit the computed-goto compatibility label only where needed | Removes 7,818 generated unused-variable warnings per C compiler across the same 227 configurations. |
| Keep LOCAL error-handler state alive for the whole invocation | Early exits restore only a registered handler; native regressions cover the original failing path and repeated/nested activation. |
| Normalize Clang Intel segment operands | Focused Win32/Win64 objects pass, with the original Clang failures retained as controls. |
| Reuse a procedure entry address when computed jumps need a fallback | The checked-array gate passes 32 objects against eight expected baseline rejections; native controls cover both check modes, scope boundaries, error handling and dispatch. |
| Emit plain integer zero for null equality comparisons | Sixteen Win32/Win64 objects and eight native runs pass, with four old Clang distinct-pointer failures retained as controls. |
| Preserve field/static-initializer pointee qualifiers and model DATA through a const-qualified payload view | Actual output removes 72 GCC qualifier warnings from unchanged legacy source; the maintained DATA gate passes sixteen objects/eight native runs against eight expected old rejections. |

Named assembly labels under optimization, cross-procedure error transfers and
array descriptors with a virtual origin remain open. The later termination and
prototype repairs have scoped results below. Successful C compilation still
contains other warnings; the complete audit is not yet a warning-free build gate.

The checked-array reduction identifies another emitter warning source. An
artificial address-taken label between a temporary's initialization and use
admits a path that Clang diagnoses as bypassing the initialization. The scoped
`candidate-v8b` records real addresses while emitting expressions and jump
tables. When a computed jump still needs an address expression, it references
the procedure's existing entry label. It creates no artificial target inside
the statement. The borrowed entry symbol is cleared when procedure emission
ends. Plain ERROR support and the JavaScript exclusion remain in place.

The new Win32/Win64 gate passes 32 objects; the preserved compiler has eight
expected uninitialized-diagnostic rejections. Additional controls pass 72
native runs, including four expected fatal exits, error-handler restoration,
RESUME, dense SELECT and normal checked-array values. Strict source-only lint
reports two files and zero findings. `candidate-v8b/qualification.json` pins
the one rebuilt backend object, 188 reused objects, compiler and receipts.

Twelve unchanged source contexts have identical C after removing only the
compatibility lines. Their Clang uninitialized diagnostics fall from 27 to 8.
One additional GCC `PROJECT_COUNT` diagnostic remains open: its C contains an
explicit zero initialization, and deleting every fallback reference in a
C-only control still reproduces the warning. No source variable initialization
or diagnostic suppression was added. Three other contexts changed through GUI
dependencies and are checked separately. All three pass GCC; two pass Clang,
while the updated picture test remains incomplete at a 300-second Clang limit.
The native compiler rejects the JS cross-target CLI probe, so this audit does
not claim JavaScript runtime qualification. Original reductions and C-only
controls remain under `error-check-flow`.

The composed `candidate-v12` also repairs null equality comparisons. The IR's
typed null cast can lose nested pointer qualifiers: a valid comparison then
becomes `const char *const * != (char **)0u`, which Clang rejects. Equality and
inequality now use plain integer `0`, a C null pointer constant. Argument casts
and ordered comparisons retain their existing paths. An intermediate `0u`
spelling still triggered GCC's normal-driver pointer diagnostic, so the retained
failed gate is not reported as a successful repair.

The normal driver no longer passes `-Wno-incompatible-pointer-types`. Its new
gate checks Intel and AT&T syntax, both Windows pointer widths and both C backend
selections. Eight valid real objects have zero warnings, eight injected pointer
type probes produce visible diagnostics, and four Win64 native runs pass. The
null comparison gate separately passes sixteen objects and eight native runs
against four old Clang rejections. Existing qualifier, computed-goto, checked
error-flow and segment gates also pass. Together these current-candidate checks
cover 68 objects, 40 C syntax checks and sixteen native runs; strict source-only
lint reports three files and zero diagnostics. `candidate-v12/qualification.json`
pins the composed driver/backend sources and all receipts.

Normal Win32 Clang Intel builds now compile C directly into their object. Clang
22's printed Intel assembly can contain symbol-immediate pushes that its own
assembler rejects. Explicit `-rr`/`-RR` requests retain the assembly-output path
and its known limitation. The normal-driver gate verifies actual COFF machine
types rather than accepting host-width objects as target qualification.

Four error-handler reductions re-emitted with the composed candidate are byte
identical to their preceding qualified C. Their retained passing LOCAL and
failing global native results therefore still apply; this change does not repair
the global frame contract. See `cross-procedure-error-v12-comparison/qualification.json`.

The legacy compatibility review also reduces a large qualifier-warning family.
Current-candidate Win64 GCC-backend output for the unchanged Math and Tic Tac Toe
source emits DATA literals through `(void*)` into the flattened descriptor's
mutable `NODE` field. GCC's full profile, including `-Wwrite-strings`, reports
72 `-Wcast-qual` diagnostics. Changing only that C field and its initializers to
a readonly payload view reduces the count to zero. Both GCC and Clang accept
both variants, with the generated descriptor-size assertions preserved.
`data-payload-const-control/finding.json` retains exact C/source/compiler hashes.
That initial control is retained separately from the implementation. Its mixed
READ/RESTORE reduction subsequently passes 32 objects and sixteen native runs
for original/readonly C views. The readonly view is byte-identical to direct
output from `candidate-v14`: structure fields and static integer/address
initializers retain their pointee qualifiers while their lvalues remain writable
for BASIC initialization. DATA keeps its existing packing and pointer values.

The maintained DATA gate passes sixteen real objects and eight Win64 native runs
against eight preserved-compiler qualifier rejections. Its BASIC emissions have
zero diagnostics. Tests cover narrow/wide strings, numbers, empty text, linked
blocks, RESTORE and an address of writable storage. Actual unchanged legacy
source now passes both C compilers with its GCC qualifier count reduced from 72
to zero; `data-math-v14/qualification.json` records that result. The game is not
executed.

The composed candidate passes 84 maintained objects, 40 C syntax checks and 24
native runs. Both changed compiler modules also pass four fresh optimized C
checks, and strict source-only lint reports three files and zero diagnostics.
Only two compiler objects differ from v12; unchanged-object controls are byte
identical, and the other 187 objects are verified by hash. Runtime archives and
installed compilers remain unchanged. `candidate-v14/qualification.json` seals
the composed result.

A broader current-source check queries the actual Make composition again,
selecting the same 189 compiler modules and pinning 502 compiler/runtime headers.
The repaired emitter generates C for every module; GCC and Clang accept all 378
optimized checks with the full audit warning profiles. Source/header hashes
remain unchanged before and after the pass. The Make `-Wc` settings are retained
as emission context, but `-r` stops before they can invoke a C compiler; the
independent C commands contain no warning suppressions. The lexical scan finds
no inline assembly requiring a separate assembler check. These are additional
configurations of the original compiler roots, with warnings retained rather
than a claim of warning-free compiler source. Evidence is sealed in
`current-compiler-v14/qualification.json`.

Four DATA GAS/GAS64/LLVM profiles are byte-identical to the preceding compiler's
output. Four LOCAL/global error-frame C reductions also remain byte-identical,
preserving their passing and failing native evidence. These scoped comparisons
are retained in `data-payload-contexts-v14/qualification.json`.

These reductions qualify the named changes within their recorded scope.
Global error handlers that transfer between procedures remain unqualified.
The historical frame-transfer tests below include a Clang O2 failure with v8b
where the preserved compiler passes; the latest v19 result is recorded next.

### Termination contracts and emitted-definition prototypes

The `candidate-v19` stage composes two later repairs over the retained v14
compiler objects. Runtime termination calls and bodies proved unable to return
receive C `noreturn` declarations. The AST pass removes unreachable executable
tails while keeping scope/declaration bookkeeping and all referenced label
entries. A fixed-point pass establishes callee contracts before C bodies flush,
including callers written before their terminating callees. Other backends keep
their existing flush behavior.

The core Win32/Win64 gate passes forty O0/O2 objects and 64 native executions.
The forward-call gate adds eight objects and sixteen executions. They cover
END/SYSTEM/STOP, explicit and implicit main endings, ordinary DLL initializer
returns, forward/backward labels and jump tables. Selected unreachable-code,
missing-noreturn, null-dereference and return-type diagnostics are treated as
errors; the full warning profiles remain enabled. Focused Clang path analysis
of the same null-guard function reproduces one old null dereference and none
with v19. See `termination-flow-v19`, `termination-forward-v19-qualified` and
`termination-path-analysis-v19`.

Native-target v18 C is separately qualified on Midcom Linux x86_64, FreeBSD
amd64, Haiku x86_64 and the ARMhf Linux laptop. GCC runs on all four; available
Clang installations run on Midcom and FreeBSD. All 48 objects and 192 native
returning/terminating executions pass, with native object machine/width checks
and pinned host runtime archives. The installed host compilers only select
native link libraries; they do not regenerate the candidate's C. Each transferred
bundle is approximately 6-7 KiB, with no source-tree copies or installed changes.
`native-termination-v18-qualification.json` retains the exact scope.

Public definitions without BASIC callers now receive prototypes. Previously,
the prototype pass skipped those bodies because their symbols were not marked
accessed. The new gate also admits emitted bodies, retaining the existing
exclusion for unused external declarations and procedure-pointer types. A
separate C caller exercises scalar, pointer, UDT, namespace and calling-convention
exports, including a terminating export. Its sixteen object checks and sixteen
native runs pass; all eight baseline/candidate object pairs are byte-identical.
`prototypes-v19-abi` retains that comparison.

The current Make graph is checked again with 503 unchanged header revisions and
188 unchanged source modules. All 378 optimized GCC/Clang checks pass, and each
compiler's missing-prototype count falls from 1,359 to zero. The new declarations
also expose 1,729 additional Clang dollar-identifier diagnostics; those and all
other warnings remain in `current-compiler-v19-prototypes/qualification.json`.
`prototype-compiler-warning-delta.json` records the exact changes. The isolated
build replaces one HLC object over v18 and retains 188 objects by hash; the prior
termination build replaces HLC/ASTPROC after unchanged-object and header-extension
controls. No compiler structure layout or installed runtime archive is changed.

Twelve full semantic models validate through the independent current reader,
with procedure identities, result types and physical source ranges preserved
against v14. The changed sources and three maintained fixtures have zero strict
source-only lint findings across the v18/v19 receipts. The large current JRPG
probe also passes matching GCC/Clang O0 object checks, with its 417 includes
pinned and prototype warnings treated as errors. Sixteen GUI dependencies have
changed since the historical graph, so none of its old C checks are borrowed.
Owned compiler emission peaks near 519 MiB of working memory; this is not a
measurement of C optimizer memory or a game runtime qualification.

Global error transfers remain broken: v19 Clang O0/O2 both exit with access
violation 3221225477. Four LOCAL controls pass, and global GCC O0/O2 happen to
pass. The cross-function computed goto remains outside its supported C contract.
`cross-procedure-error-v19/results.json` records all eight executions. The
repository compiler-structure guard also reports missing header/footer markers
in seven concurrent semantic-tooling files; that separate failure is recorded
without modifying those files. These limits keep the overall cleanup active.

### Call-result temporary storage

```sh
python build_scripts/test-c-emitter-temporaries.py --fbc /path/to/fbc \
  --gcc /path/to/gcc --clang /path/to/clang --output build/c-temporaries \
  --baseline /path/to/baseline-fbc --runtime-dir /path/to/lib/win64
```

The v24 emitter separates scalar/pointer scratch declarations from call-result
assignments. The v25 follow-up extends this to ordinary aggregates: C field
bindings are already made writable by `hEmitStructWithFields()`, while pointed-to
const qualifiers survive. Builtin `va_list` arrays retain initialization because
C forbids assigning an array. Calls remain at their original evaluation points;
skipped paths do not acquire a substitute result.

The expanded aggregate gate passes 32 real Windows objects and sixteen native
runs across both widths, C backends, O0/O2 and normal/debug configurations. Its
v24 baseline reproduces sixteen warnings per compiler; the candidate has none.
The test also verifies a BASIC read-only field and a const-qualified pointee.
Six of eight normal object pairs match exactly, including all four optimized
pairs. Clang O0 retains extra return-value copies and a larger stack frame; the
four targeted assembly checks are retained in `aggregate-temp-v25/assembly-review`.
See `aggregate-temp-v25/results.json` and `controls.json`.

All 378 current-compiler C checks pass with zero jump-initialization diagnostics,
down from 20,361 GCC / 3,574 Clang before the temporary changes. Uninitialized
warning counts remain GCC 41 and Clang 51 conditional/five sometimes-uninitialized.
The delta retains every other change, including identifier and alignment warnings.
One HLC module changes, with 188 others and all 503 headers unchanged from v24.
See `aggregate-compiler-warning-delta-v25.json`.

The varargs companion checks independent `cva_copy` state and scalar, string,
double and structure-pointer arguments. Its Windows gate passes 32 objects and
sixteen native runs. A 7,338-byte C/helper bundle also qualifies both fixtures on
Midcom with native GCC 15.2 / Clang 21.1.8: sixteen ELF objects and sixteen runs
pass at O0/O2 against unchanged installed runtime archives. Native assertions
prove that the SysV argument list is an array of one 24-byte element. No source
tree is copied and no installed tool is replaced. See `native-aggregate-v25-midcom`.

Thirty-two full semantic models validate against v24 with procedure identities
and source ranges preserved. Eight focused Clang analyses report no core, unix
or deadcode findings. Label, GOSUB, termination, forward, checked-array, DATA and
C-export controls also pass. Global Clang error-handler runs still access-violate.
The overall cleanup remains active, and the v23 JRPG probe remains coverage from
before these temporary changes.

### Private C type identifiers

```sh
python build_scripts/test-c-emitter-type-names.py --fbc /path/to/fbc \
  --gcc /path/to/gcc --clang /path/to/clang --output build/c-type-names \
  --baseline /path/to/baseline-fbc --runtime-dir /path/to/lib/win64
```

The v29 emitter prefixes private type identifiers with `fb_type_`, then preserves
ASCII letters/digits and encodes other bytes as `_HH`. Escaping underscores as
well as dollar signs makes the mapping injective. This only spells C type names:
assembler symbols, RTTI strings and builtin `va_list` aliases keep their original
names. The aliases test deliberately pairs equal-length `fb$tag` and `fb_tag`
with different layouts. The inheritance test also checks virtual dispatch and
runtime type identity.

Across the current 189-module compiler graph, Clang dollar-identifier warnings
fall from 561,547 to 419,906. Exact C token comparison restores the old private
type spelling and reproduces all 188 unchanged modules byte for byte, preserving
strings and comments. The HLC source itself changes. All 378 C counterparts
accept: 372 reuse verified identical C with the same compiler and full arguments;
six are new checks. The original executed commands/logs remain intact. Two new
Clang declaration-order warnings and one buffer-usage warning are recorded too.
See `type-name-compiler-warning-delta-v29.json`.

The fresh Windows alias/aggregate gate passes 64 objects and 32 native runs,
including sixteen identical normal object pairs. Forty-four current regression
emissions match earlier v27 C bytes without repeating their native controls.
Forty-four full semantic models validate; all 22 before/after pairs match exactly.
Three changed BASIC files pass strict non-semantic lint. The public type-name
controller supports `--case aliases`, `temporaries`, `inheritance` and `varargs`
for focused runs, and can resume its exact pinned configuration.

Native Linux GCC/Clang controls pass 32 ELF objects and 32 runs across four
fixtures, O0/O2 and old/new emitters. All sixteen object pairs match exactly.
The transferred C/helper bundle is 12,450 bytes; runtime archives and installed
tools remain unchanged. See `native-type-names-v29-midcom`.

The HLC module is also compiled to real objects using each C compiler's full
warning profile, then privately linked with the 188 preserved objects. Both
resulting compilers reproduce 24 exact C emissions across Win32, Win64 and
Linux x86_64. This checks the C-built emitter module's behavior without claiming
a full compiler bootstrap from C. See `C-built-emitter-v29/results.json`.
Rejected pilot candidates remain recorded. Wider current-source qualification
and the open error-frame, assembly-label and array-origin defects remain work.

### Unused label definitions

The later `candidate-v22` omits unreferenced C AST labels and retains referenced
targets, including references from assembly tokens and jump tables. Procedure
entries remain available during lowering because a later computed jump can need
their address. The backend records the entry definition's exact section span and
removes it only when still unused at procedure end. It validates the span before
moving overlapping bytes with `memmove`, reuses the existing buffer and retains
any preceding debug `#line` directive. Other backends and naked procedures keep
their previous paths.

The full compiler graph passes all 378 optimized C checks. Unused-label warnings
fall from 20,643 to 193 per compiler, with 503 unchanged header revisions and
187 unchanged BASIC modules. Dollar-identifier warnings also fall as those label
definitions disappear. Additional diagnostics in the changed emitter source
remain visible in `label-compiler-warning-delta-v22.json`. The remaining 193
unused labels are listed individually in `label-residual-v22.json`; examples are
structured branch exits referenced only by jumps discarded as dead code.

The v23 follow-up resolves those 193 retained labels by deriving references
from live control flow. Address expressions and ASM operands remain conservative
roots. A growing set of reachable branch targets converges across backward
edges; unreachable cycles cannot introduce their own entries. The existing
emission-time scan refreshes the result after callee termination inference.
All 378 full-profile compiler checks pass with zero unused-label warnings and
zero Clang unreachable-code warnings. One BASIC module changes, with 188 others
and all 503 headers unchanged from v22. No symbol layout changes are required.
`label-compiler-warning-delta-v23.json` retains every other warning change,
including additional declaration and jump-over-initialization diagnostics in
the changed AST implementation.

The maintained label gate covers returning IF/SELECT arms, dead branch cycles,
backward entries, a BASIC target reached only through inline ASM, and
direct/nested/computed GOSUB returns with invalid selectors. Its v22/v23 matrix
passes 96 real objects and 48 native runs across both Windows widths, matching
C backends and O0/O2, in normal and debug configurations. Candidate object
commands treat unused-label warnings as errors. Debug objects retain line tables
naming the BASIC file, allowing Windows filename case differences. Baseline
objects reproduce sixteen unused-label warnings per C compiler; the candidate
has none. Eight C-body controls also confirm that the dead cycle's two jumps
are absent while native results and backward entries remain correct.
See `label-targets-v23`.

Fresh v23 controls also pass 48 termination objects/80 native runs, 32 checked
array object compilations, sixteen DATA objects/eight native runs and 32 direct
or error-flow syntax checks. All 28 full semantic models from seven focused
sources validate against v22 with procedure identities and source ranges
preserved, including checked-array fallback with `-e`. The large JRPG translation
unit passes matching GCC/Clang O0 objects with unused-label and prototype
warnings treated as errors, with the same 417-input membership as v19 but 3 changed input revisions.
The latest global error-frame test still fails both Clang executions while its
four LOCAL controls pass. No installed compiler or runtime is replaced, and the
overall goal remains active.

### Structural analysis and cross-procedure error transfers

Clang path analysis of fifteen retained v8b candidate contexts completes
twelve checks and times out on three. It reports ten diagnostics across six
contexts: five uninitialized call arguments, three uninitialized comparisons,
one null pointer arithmetic and one null memory-set argument. Their traces
pass through indirect runtime-error jumps. The runtime handler's possible
targets and termination behavior need reduction before these paths can be
classified as reachable source defects. The complete traces and C excerpts
are retained in `entry-label-structural-v8b/qualification.json`.

A separate small Win64 native reduction confirms a frame-transfer problem. A worker
raises ERROR, the caller's global handler checks its own local array, and
RESUME NEXT should continue in the worker before returning to the caller.
GCC O0/O2 and GAS64 read the wrong caller frame. The current candidate's Clang
O0 run passes, while O2 returns without the expected worker successor.
The preserved compiler also fails the GCC and GAS64 checks; its Clang runs
pass, as do the immediately preceding candidate's Clang runs. All five
same-procedure LOCAL-handler controls pass. Seventeen native runs and strict
source lint with zero findings are recorded in
`cross-procedure-error-finding.json`.

The C lowering passes a handler label address through the runtime and uses
computed goto to enter it from another C function. [GCC's label-value contract](https://gcc.gnu.org/onlinedocs/gcc/Labels-as-Values.html)
excludes this cross-function transfer. Successful individual executions do
not establish a valid C contract. A repair needs explicit frame and continuation
handling across the compiler and runtime; adding artificial address-taken
labels would leave that contract unresolved. No production change for this
finding is applied.

### Reduced findings already verified

1. **Nested pointer qualifiers are lost in emitted argument casts.** The
   OpenGL image viewer and slideshow fail unsuppressed GCC compilation at
   `glShaderSource`. A small `nested-const.bas` fixture reproduces the issue
   without OpenGL: BASIC emission succeeds, GCC rejects the emitted callback
   call, and the preserved driver's `-Wno-incompatible-pointer-types` permits
   it. The prototype retains `const char *const *`, while the argument cast
   becomes `char **`. `hDoCall()` requests the parameter type, but expression
   cast emission calls `hEmitType()` without const emission. Correcting that
   one cast in a retained C copy makes compilation succeed; the linked
   callback fixture then runs with exit zero. Both reduced BASIC fixtures
   passed strict source-only fblint with zero errors and warnings.

2. **The original termination call lacked a non-returning contract.** A small
   `end-path.bas` fixture guards a null pointer with `END`, yet Clang's
   analyzer follows a return from `fb_End()` and diagnoses a null dereference.
   Adding a `noreturn` attribute to the retained C prototype removes that
   diagnostic. The Win64 runtime terminates the process; any compiler change
   must first check the runtime contract on every affected target.

   A later isolated prototype experiment checks all six runtime definitions;
   each terminates through `exit()`. END, SYSTEM and STOP pass 48 real object
   builds and 48 native returning/terminating runs at `-O0` and `-O2`.
   The attribute also exposes 24 Clang warnings for generated code after
   termination: a duplicate main-epilogue `fb_End()` and its default return.
   That initial experiment deferred the implementation until the contract and
   generated tails could be repaired together. The later v18/v19 work above
   performs that combined repair within its recorded scope. The original
   experiment and source hashes remain under `termination-experiment`; its
   comparison is `termination-v7b/results.json`.

3. **Some warnings belong to the source.** Araxia declares the source argument
   of `memcpy` as mutable, producing a builtin-signature warning. The slideshow
   also has an early `GOTO cleanup` path crossing its shader object's
   initialization, followed by a destructor access. These require separate
   source/control-flow review, not blanket suppression or an assumed C-emitter
   fix. Unchecked allocation paths and BASIC `ANY` storage require the same
   distinction.

   A further null-dereference representative comes from `fb-imageproc`'s
   `Bitmap.clear()`: the library reads `_image->height` without checking the
   image pointer, and the retained C preserves that access. The top-level BASIC
   test still matches its baseline hash. `imageproc-null-source-review.json`
   records the source method, C excerpt and current header hashes. This is a
   source-level valid-image precondition, not a classification of every null
   warning. Source-only strict lint is clean for the one test. Its two unchanged
   headers have 52 existing whitespace findings and one macro-parentheses
   recommendation on a literal definition; no source edits were made.

The raw and modified reduced C, commands, logs and runtime receipt are in
`reduced-findings/results.json`. `verify-findings.py` reproduces the comparison.

### Array descriptors and nonzero lower bounds

Clang's `-Warray-bounds-pointer-arithmetic` also identifies a structural C/ABI
issue. An array declared `(1 to 2)` stores a virtual zero-index address before
its allocation in the descriptor's `data` field. A sufficiently negative lower
bound puts that address beyond the allocation instead. The descriptor's `ptr`
field retains the physical allocation. This is the existing shared contract
in `src/rtlib/fb_array.h`, not an out-of-bounds BASIC subscript.

`array-bias.bas` reduces both cases and checks static/dynamic array bounds and
values through a BYDESC call. Both C backends pass eight object builds and eight
Win64 native runs across GCC/Clang and O0/O2; all four Clang checks reproduce
the pointer-arithmetic warning. Strict source-only lint reports zero findings.
`array-bias-check/qualification.json` pins the proof and descriptor consumers.

C11 restricts pointer arithmetic to the array and its one-past position.
The virtual-origin calculation therefore needs a compiler/runtime review;
successful native runs do not establish a portable C representation. See
[N1570, section 6.5.6 paragraph 8](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf#page=111).
No descriptor layout or array-lowering change is applied in this audit batch.

### Additional native-profile findings

The native Make and SCons profiles exposed two further correctness problems
and a reproducible GCC failure. These remain separate from the qualifier repair.

| Finding | Evidence | Status |
| --- | --- | --- |
| A promoted C `long` alias retains its narrower C spelling | Win64 `clong` subtraction has an eight-byte BASIC result, but the generated temporary and casts remain C `long`, which is four bytes. Division lowering then shifts that temporary by 63. | Repaired in the separate candidate described below. |
| Named inline-assembly labels are duplicated by C inlining | A private procedure called twice emits the literal `stored_value:` label. Object generation at `-O2` fails with duplicate symbols for both matching backend/compiler combinations. | Confirmed, unresolved. Assembly labels need a representation that remains valid when C duplicates code. |
| Clang rejects a segment override inside Intel operand brackets | The DOS low-memory example emits `[gs:0xB8000+2*ebx]` with `-gen clang -target dos`. Clang rejects the two operands at O0 and O2. | Repaired in candidate-v6b; the unchanged example passes both matching Clang object checks. |
| GCC 16.2 crashes in its `uninit` pass | OHRRPGCE's original generated `LOAD_HERO_FROM_RELOAD` reproduces an internal compiler error with `-O2 -Wmaybe-uninitialized` alone. | The retained preprocessed input still crashes. Both loading configurations generated by candidate-v5 pass GCC and Clang with unchanged tools and warning commands. |

The first reduction is `long-division.bas`. With a `clong` numerator of
`-2147483648` and offset of one, the promoted subtraction divided by 64 must
produce `-33554432`. GCC at `-O0` and `-O2`, and Clang at `-O0`, instead produce
`33554431`. Optimized Clang prints a different incorrect value and terminates
abnormally. Explicit INTEGER conversions in the BASIC source make all four
native checks pass. `long-division-check/results.json` records commands and
runtime logs. The original fixture and explicit-conversion control passed
strict source-only fblint.

The assembly reduction is `asm-labels.bas`. The unchanged emitted C builds and
runs at `-O0` and at `-O2 -fno-inline` with both matching backend/compiler
combinations. The normal `-O2` object checks reject duplicate labels.
`asm-findings/label-controls.json` records the four successful controls.
GCC documents that optimization can duplicate inline assembly and its labels;
see [Extended Asm](https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html).
This is also why assembly output alone is insufficient for these checks.

The current parser creates an assembly AST node for each source line, and the
C backend emits each node as a separate extended-assembly statement. GCC's
documented contract does not preserve consecutive volatile statements or jumps
between separate assembly statements. Its `%=` substitution identifies one
assembly statement instance, so applying it separately to a label and a later
branch would give different names. A repair needs to preserve the relationships
within the original BASIC assembly block, including register use and branches.
Changing label spelling alone does not establish that contract. See
[GCC's extended-assembly rules](https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html).

An additional matching-backend assembly matrix covers seven current tests and
examples at Win32/Win64, plus the DOS example in its 32-bit target context.
All 30 profiles emit C. Of 60 real object checks, 56 pass; two optimized Clang
checks reproduce `bindings.bas`'s duplicate `asm_local` label, and two DOS
Clang checks expose the segment-override spelling. The two corrected-C syntax
controls pass. `assembly-case-check/qualification.json` preserves that split.
These checks execute no programs. The DOS C is checked by a host x86 object
compiler; this does not qualify a linked DOS executable or its runtime behavior.

The GCC crash was reduced from about 3.1 MB of emitted C to one function with
its declarations, then preprocessed to 212,658 bytes. Both `-O2 -Wall -Wextra`
and `-O2 -Wmaybe-uninitialized` reproduce it. GCC at `-O0` with the aggressive
warnings and Clang at `-O2 -Weverything` accept the reduced C. The standalone
input, tool commands and logs are in `gcc-uninit-crash/`. No external bug report
has been submitted.

### First emitter repair

The working tree now preserves pointee qualifiers when rendering expression
casts. Prototype emission and call argument emission also share the existing
C-library parameter adjustments, so the repair preserves FreeBASIC's accepted
const-pointer arguments to `DEALLOCATE` and `REALLOCATE` while emitting the
mutable `void *` parameters required by C's `free` and `realloc`.

`tests/pointers/const-nested-arg-casts.bas` exercises ordinary calls, callbacks,
const record reads and const-pointer resizing/release. The focused checker is:

```text
python build_scripts/test-c-emitter-qualifiers.py --fbc /path/to/candidate/fbc \
  --gcc /path/to/gcc --clang /path/to/clang --output build/qualifiers \
  --runtime-dir lib/win64
```

The checker emits through both C backend selections at Win32 and Win64 widths,
then checks each output with both C compilers. It promotes pointer-type and
discarded-qualifier diagnostics to errors, while retaining other warnings.
The native runtime option checks all four backend/C-compiler combinations on
Win64. Omit that option for frontend-only qualification with compatible cross
tools. `--expect-rejection` supplies the negative control using the preserved
compiler.

The repaired candidate passes all eight C frontend checks and four native
executions. The original compiler fails all eight C frontend checks on the
same fixture. Strict source-only fblint reports two files, zero errors, zero
warnings and zero informational findings. These receipts are under
`qualifiers-final`, `qualifiers-final-baseline` and `qualifiers-final-lint.log`.
The candidate was linked by replacing only the C backend object in the
189-object baseline. The baseline compiler and the installed compiler remain
unchanged.

Wider qualification caught a needed distinction: a local constant array's
initializer still writes its storage. The final repair removes only the stored
value's const qualification when building a C lvalue; reads and pointed-to
qualifiers remain intact. The regression now also checks constant arrays,
constant records and arrays of constant pointers.

The final candidate re-emitted all 189 compiler modules and 29 reproduced
corpus pointer cases. All 218 outputs pass both C compilers. Exact C byte and
tool comparisons reused 428 completed checks and required eight new C checks;
include-dependent or location-sensitive C is not eligible for that reuse.
The final expanded fixture also passes eight frontend checks, four native
executions and strict source-only lint. These final receipts are under
`candidate-v2-audit`, `qualifiers-v2` and `qualifiers-v2-lint.log`.

Five additional native Make profiles reproduce the original pointer-type
failure: fb-ext-lib's JSON object, regular expression, SQLite and sprite
modules, and fbsound's OpenMPT call. All five outputs pass both C compilers
with the repaired candidate; fbsound also passes both corrected assembly/object
checks. A further `fbrtLib/time_parsedate.bas` root passes both candidate checks.
These receipts are under `make-candidate` and `new-pointer-candidate`.

### Arithmetic promotion repair

`hConvOperand()` now replaces the source's C type-mangling modifier with the
destination's modifier after joining the numeric type. Const bits remain
preserved. Declarations and prototypes still retain their C aliases; widened
arithmetic values use the destination's width and spelling.

`tests/crt/clong-promotion.bas` checks signed subtraction, addition and division,
plus unsigned addition, multiplication and shifts. The separate `candidate-v3`
compiler was linked by compiling only the changed AST object and reusing the
previously qualified backend object and remaining frozen compiler objects.

The repaired fixture passes eight C frontend checks across both C backend
selections and Win32/Win64 widths, and four native Win64 runs. The original
compiler is rejected by all four Win64 frontend checks for the reproduced
shift-count diagnostic. The prior qualifier fixture still passes its eight
frontend checks and four native runs. Strict source-only lint reports no
errors, warnings or informational findings for the changed AST source and
new fixture.

The new candidate re-emitted 227 compiler/project configurations, including
the real FreeType font module and the existing C-long prototype regression.
All 454 C checks pass: 420 reused verified identical self-contained C checks,
and 34 ran again. Receipts are in `promotion-check`, `qualifiers-v3`,
`candidate-v3-audit` and `promotion-lint.log`. This remains local qualification;
the installed compiler is unchanged and the broader inventory is unfinished.

### Computed-goto warning cleanup

The C backend previously emitted an unused `_llvmbug18658` pointer and a label
address in every ordinary function. Clang needs a label address in functions
containing computed jumps, including an otherwise plain `ERROR` statement.
Functions without such jumps do not need that compatibility code.

The separate `candidate-v4` emits one compatibility label at the first
indirect jump and discards its address through a void expression. Ordinary
functions emit neither the label nor the unused pointer. Dense SELECT jump
tables already contain their real destination addresses. The JavaScript
target continues to omit the workaround.

Focused qualification passes 40 C frontend checks and 20 native Win64 runs
across ordinary calls, fatal ERROR, dense SELECT, QB ON ERROR GOTO and FBLite
RESUME. Four fatal-error runs return the expected error 24; the other 16
return zero. The saved runtime archive predates RESUME's current ERR reset,
so that existing test links a separately compiled current `errors/error.c`
object before the unchanged archive. The preserved compiler and both
candidates fail identically with the older archive; the focused test passes
with the current object.

At this stage, the maintained `test-c-emitter-gotos.py` check passed 24 frontend
checks with unused variables promoted to errors. Its preserved-compiler
control is rejected for the unused-variable family. Both earlier repairs
still pass their eight frontend checks and four native executions each.
Strict source-only lint reports zero findings for the changed backend and
three focused BASIC fixtures. Receipts are in `goto-check`, `resume-controls`,
`error-runtime`, `gotos-maintained-v4`, `gotos-maintained-baseline`,
`promotion-v4`, `qualifiers-v4` and `goto-lint.log`. The frozen 227-root
qualification under `candidate-v4-audit` now passes all 454 C checks and both
corrected assembly/object checks. Across those same 227 configurations, GCC
unused-variable warnings fall from 8,002 to 184 and Clang's from 7,977 to 159.
Each compiler loses 7,818 unused variables generated by the old workaround.
Other warning families remain. `candidate-v4/qualification.json` pins this
result separately from later compiler changes.

### Local error-handler restoration

Clang's `-Wsometimes-uninitialized` warning in
`WSR5_3/src/raider.bas::RaiderWriteRotatingAutosave` exposes a shared AST
lowering bug. An early exit before ON LOCAL ERROR bypasses the assignment
that saves the previous handler, yet procedure cleanup still restores that
uninitialized pointer. A second registration in another branch can have the
same problem, and repeating the first registration overwrites the caller's
saved handler with the procedure's own handler.

The reduced `local-error-early-exit.bas` should return zero. Its original
generated C crashes with access violation or exits with error 24 across
GCC/Clang at O0/O2. Moving registration before the guard makes all four
control runs return zero. The source warning therefore identifies a real
compiler defect, rather than an uninitialized user variable.

`candidate-v5` gives each invocation a saved-handler value and activation
flag. Both have procedure lifetime and explicit entry declarations and
initialization. Every local registration can save the previous handler, but
only its first execution does so; cleanup restores it only after activation.
All 189 native compiler objects were rebuilt for the changed internal
procedure-error structure. No old-layout object is reused.

`tests/fblite/local-error-early-exit.bas` covers early exit, either conditional
registration, loops, repeated clearing and recursion. The same source also
checks scoped FB through `LOCAL_ERROR_TEST_FB`. Both dialects, both C backend
selections and Win32/Win64 pass 16 optimized C checks and 16 native Win64
O0/O2 runs. Repeating that matrix with `-exx` also passes. The small reduction
and its registration-before-guard control pass another four frontend checks
and eight native runs. Source-only strict lint reports zero findings across
the three changed compiler files and two BASIC fixtures.

The qualifier, arithmetic and computed-goto regressions still pass with this
full compiler build. The maintained flow check now includes the local-handler
case, promotes uninitialized reads to errors and passes 32 C checks; all 32
preserved-compiler controls reject the original generated-variable warnings.
Its broader 230-configuration audit includes the prior
227 roots, both local-handler dialect profiles and the original WSR5_3 root.
All 460 C checks and both corrected assembly/object checks pass. The original
WSR5_3 warning count falls from one to zero with unchanged game source. The
control-dispatch observational test also passes six cases across GCC, Clang,
LLVM, GAS64 and GAS, comparing disabled, full, binding and expression exports
for 120 emissions. `candidate-v5/qualification.json` seals these results.
Evidence is under `local-error-check`,
`local-error-v5b`, `local-error-regression-v5b`, `local-error-regression-exx`,
`candidate-v5` and `candidate-v5-audit`.

### Clang segment-operand repair

`candidate-v6b` moves Intel segment prefixes outside address brackets before
removing redundant brackets around generated `%N` memory operands. It accepts
spaces and horizontal tabs around those operands and copies quoted assembler
data, including escaped quotes, unchanged. The conversion applies to Clang C
emission. GCC output remains unchanged.

The maintained `c-asm-segment-prefix.bas` fixture covers all six x86 segment
registers, absolute/register addresses, C memory operands, whitespace and quoted
data. The common register expression uses GNU as's accepted constant-first
ordering; a Clang-only branch also checks register-first addresses. Win32 and
Win64 at O0/O2 pass eight candidate object checks. The same fixture rejects four
old Clang checks while all four old GCC controls pass. GCC C output and quoted
assembly lines compare byte for byte across both compiler versions.

An unchanged-source control rebuilds the prior C backend to a byte-identical
object under the current headers. The repair then rebuilds only that object
and reuses the other 188 qualified compiler objects by hash. It preserves the
separate candidate-v5 binary and receipts. The real DOS low-memory example now
passes both Clang object checks with unchanged BASIC bytes. A 30-profile replay
retains 29 identical C outputs and the repaired DOS Clang output; all 60 matching
C/tool checks reuse verified identical C, leaving 58 acceptances and the two
known optimized duplicate-label rejections.

The maintained control-flow checks pass another 32 C checks; the qualifier
checks pass eight C checks and four native runs. No segment-dependent test
instructions execute. `candidate-v6b/qualification.json` seals the build,
negative controls and matrix receipts. Strict source-only lint reports zero
findings for the changed backend and new BASIC fixture.

### Coverage and resource handling

The priority pass completed all 443 entries: 419 emitted C, 23 failed BASIC
emission and one inferred editor filename did not exist. A supplemental pass
uses the actual `newjrpg/jrpg.bas` editor and the current fb-VNC Makefile's
seven production modules. All seven VNC module outputs pass GCC and Clang.
The editor emitted about 17 MiB of C but exceeded both initial 180-second
warning budgets. Its later 600-second checks accept the exact retained C with
both GCC and Clang, taking 252.401 and 378.268 seconds respectively. Current
BASIC source bytes still match that emission. Prospector emits C with both `FB_NO_SFXLIB` and `FB_NO_USTRING`;
Clang completes its warning check; GCC also completes with an extended budget.
Those outcomes remain distinct from emitter errors.

A later JRPG include trace produces different C through changed dependencies,
with the top-level BASIC unchanged. Repeating that context verifies 417 stable
dependency hashes and identical new C. Its 17,285,969-byte output passes the
same unsuppressed GCC and Clang profiles in 353.510 and 351.858 seconds.
`jrpg-current-include-graph/qualification.json` retains those checks separately
from the older C. The graph opens 49 included primary BASIC files; each already
has qualification through another accepted source or owner, so this adds editor
context evidence without increasing the original-input coverage count.

Additional native Make profiles select 152 fb-ext-lib modules, 24 fbfrog
modules and six fbsound modules. Of these 182 profiles, 181 emit C. The remaining
fb-ext-lib JPEG module fails BASIC checks around its CRT declarations.
The Windows Midisoft editor's actual PowerShell source list supplies another
43 modules; all 86 C compiler checks complete successfully with warnings.
These are supplemental configurations, not extra primary-inventory roots.

OHRRPGCE's current native SCons graph selects 94 configurations for Game and
Custom. Its generated ReloadBasic sources and version/backend headers are
prepared under this audit directory. Signatures, caches and packaging outputs
are isolated from the original project. All 94 emit C. GCC completes 92 checks
and crashes on the two configurations containing the same reduced loading
function. Clang completes 93 checks; `scriptcommands.bas` exceeds its extended
180-second budget, and remains incomplete after a 600-second Clang retry.
`supplemental-warning-600/qualification.json` preserves that timeout separately
from the two accepted JRPG stages. A targeted candidate-v5 repeat of both loading configurations
passes all four C checks; the original preprocessed control still crashes the
same GCC. `ohr-loading-v5/comparison.json` pins unchanged BASIC source and C
warning commands for that comparison. `native-profiles.json` records supplemental coverage
separately. Structural analysis is limited to compiler correctness checks.

The current checkout also contains six BASIC utilities outside the compiler
graph: the graphics-data generator, the DOS export-table generator and two
copies each of the fbportio generator and tester. All six now emit C through
both C backends. The graphics-data generator previously used reserved OUTPUT
as a parameter name; renaming it to `output_buffer` removes that BASIC
emission blocker. Its Win32 and Win64 object checks pass all eight backend/tool
combinations, and strict source-only lint has zero findings.
The exact original generator bytes are also rejected by both the frozen and
repaired compilers, confirming the reserved name was a pre-existing source
problem; see `gfxdata-blocker-control/results.json`.

The utility matrix has 14 backend/width profiles and 28 object checks. All 14
matching backend/compiler combinations pass. Twelve cross-compiler checks
also pass; the two Clang rejections read GCC's Intel operand-bracket syntax
from the x86 testers. Their matching Clang outputs pass. Both tester copies
use their documented 32-bit x86 profile. `current-utilities/qualification.json`
records these supplemental checks separately from the primary inventory.

Extended checks reuse exact saved C with unchanged warning commands.
Repeated identical C is grouped by content hash and normalized compiler command.
Successful checks can be shared only after each member's bytes are verified;
files containing external includes or location/time-dependent constructs are
checked separately. Timeouts receive a larger bounded budget, never a clean
result inferred from another unfinished check.
An opt-in buffered-capture experiment preserves merged diagnostic bytes and
timeout behavior in its control tests. A 4.8-MB GUI translation unit then
finishes under a 180-second budget, retaining about 43 MB of raw diagnostics
and qualifying four exact-C copies. The observed 97-second total establishes
a workable budget; that experiment does not isolate an I/O speed improvement.
The active primary and 60-second retry implementations remain unchanged.
An additional filename experiment retains the warning flags and C bytes but
passes `unit.c` from its own working directory. On the calibrated GUI input,
Clang's complete diagnostic stream matches the full-path run byte for byte
after filename normalization, with identical warning counts. The raw log falls
from 43,315,473 to 20,298,801 bytes. The elapsed measurements do not establish a
speed improvement. Relative-path use is restricted to C without external
includes or location-dependent constructs; the actual command is retained.
The subsequent buffered retry freezes 122 completed content/profile groups
covering 413 primary units. It processes only published groups from the earlier
retry, so it does not compete for an active group's C input. Checks still
exceeding 180 seconds retain timeout status and their partial diagnostics.
TurboTrek, VTREK and the JRPG player completed both GCC and Clang checks under
the extended budgets. Inline-assembly C additionally needs the driver's
assembly dialect and an object-generation check. The priority outputs contain
no inline assembly; five such outputs from the wider sweep pass both compilers
with the native Intel dialect. Assembly-profile results remain separate from
the original receipts.

The wider sweep preserves its first 445 completed receipts. Its next two-worker
batch completed another 3,702 entries before reaching the 1.5 GiB per-run
storage budget. The following batch completes 1,002 of the remaining 4,664
entries before reaching its own cap. Only the final 3,662 pending IDs enter
`sweep-final`, with tests and examples before larger applications. That batch
archives completed C immediately and shares checks only for exact C bytes and
the same compiler/warning profile. Its output cap is 1 GiB.
`overview.py` combines only disjoint primary cohorts and
rejects duplicate or unplanned unit IDs. Supplemental profiles and candidate
qualification are reported separately.

The completed 60-second retry covers 256 content/profile groups. Longer
180-second and 300-second retries retain the same C and warning arguments;
unfinished groups owned by the earlier worker are excluded from the later
plan. Some larger inputs still timed out at that point. The later exact-C
serial replay and one extended retry now resolve all 42 final unfinished stages
in `frozen-C-effective-qualification.json`; the earlier timeout receipts remain
as history. Acceptance by one compiler was never substituted for the other.

After the complete baseline pass, another 339 pending content/profile groups
covering 404 units enter a two-worker, 300-second retry with a 1-GiB output cap.
Its frozen selection excludes every group in the earlier 256-group retry plan,
including those still owned by active workers. `final-warning-groups.json`
records both the new selection and excluded pending groups. No warning flags
are removed; publication still requires verified identical C and tool profiles.

That continuation is now terminal. Its original controller stops after 338
groups when retained diagnostics reach 1,040.48 MiB. A bounded 1,152-MiB resume
finishes only the remaining group, accepted by both C compilers. All 338 earlier
receipt hashes and modification times are unchanged. The resume proof is
`final-warning-300-resume-1152-qualification.json`; the complete continuation is
sealed in `final-warning-300/qualification.json`. No warning flags are relaxed
and no completed group is compiled again for this resume.

After the older workers terminate, sharing verified successful checks resolves
another 201 timeout stages across the primary cohorts. Only nine remaining
content/profile groups need a 300-second retry, covering 36 units. Two larger
4.8-MB GUI outputs pass their separate 600-second Clang budget. These plans
exclude the active 339-group selection and retain the original warning flags.

Another 63 newly reached baseline qualifier failures pass all 126 GCC/Clang
checks with candidate-v5 and unchanged BASIC source. Their separate evidence
is sealed in `new-const-candidate/qualification.json`; it supplements the
earlier 230-configuration qualification rather than replacing its receipts.
Seven later PCRE, Expat, libopenmpt and Stunts failures pass another fourteen
checks, sealed separately in `remaining-pointer-candidate/qualification.json`.
Together with the broader candidate qualification, these receipts cover all
100 baseline units reporting GCC incompatible-pointer errors. Each has the
same BASIC source hash and accepted GCC/Clang C from candidate-v5: 200 accepted
checks. `pointer-failure-coverage.json` verifies the complete correspondence
and retained C bytes. Other warnings and assembly findings remain separate.

Vendor-specific builtin and inline-assembly tests also require the matching
C backend. GCC output referring directly to `__builtin_memcpy` cannot be read
as interchangeable Clang input. The existing `-gen clang` path supplies the
appropriate libc alias. The shared builtin test and backend ABI test pass
eight real object builds and twelve native runs across GCC/Clang and O0/O2,
with all warning options retained. Evidence is sealed in
`matching-backend-check/qualification.json`. These cross-vendor failures do
not require another emitter repair.

Uninitialized diagnostics also need a source-flow review. In parameter parsing,
`dotpos` is assigned when a name is read and inspected only for a non-prototype
parameter; that earlier path requires a name. This is a correlated-branch
warning, unlike the repaired local-handler restore. The print/input lowering
helpers leave their selected procedure uninitialized outside eight integer
size cases. Those callers rely on the datatype-to-size mapping; the missing
fallback deserves an explicit invariant review before being classified as a
reachable bug. Initializing every `ANY` local would hide that distinction.

The additional-source pilot checks five profiles. The Optical error-context
module and arithmetic regression emit C and pass all four C checks. Three
copies of the DOS editor fail BASIC emission. A separate replay with the DOS
target removes the platform-header rejection but still rejects `DEF SEG` and
prefix `STATIC SUB/FUNCTION` syntax. Any partial C left by that failed frontend
is unqualified; `additional-dos-pilot/interpretation.json` states that boundary.
The full 181-profile supplementary run uses one worker and a 256-MiB cap.
Its first cohort stops on changed input after 61 profiles: 14 emitted C and all
28 C checks pass. The 120 remaining profiles receive refreshed input hashes in
a separate continuation. Later edits are deferred explicitly while independent
inputs continue. The original runner bytes and receipts are preserved.
That continuation completes 119 profiles and defers one changed source. A
separate refreshed attempt completes that last source. All 181 candidates now
have original-profile receipts: 81 emit C and pass all 162 C checks, while 100
fail BASIC emission. `additional-summary.json` verifies the disjoint cohorts,
source identities, retained C and unsuppressed warning profiles.

Corrected Optical working-directory/include profiles cover 46 completed inputs.
Of these, 35 emit C and pass 70 C checks; 68 reuse exact saved C and compiler
profiles. The standalone source-audit CLI additionally needs its main role,
which recovers one original emission failure. The other 11 failures are Visual
Basic fixture inputs containing `Attribute VB_Name`, outside the FB grammar.
Fourteen old Microsoft BASIC games also emit in an explicit QB compatibility
profile and pass all 28 C checks with unchanged source bytes. Across these
corrected profiles, 96 of the 181 supplementary candidates have accepted C;
85 remain frontend-blocked. Original-platform execution is a separate matter.

The earlier failure index contains ten more main-only argument-builtin cases.
Explicit standalone main selection recovers all ten and passes all 20 C checks,
including six shared checks for byte-identical C. The existing argument-only
probe also passes four native GCC/Clang O0/O2 runs, checking spaces, an embedded
quote, a trailing backslash and an empty final argument. Evidence is sealed in
`entrypoint-context/qualification.json` and `argv-main-native/results.json`.

A failure review snapshot contains 2,439 BASIC emission failures, including
1,419 missing-include errors. Of those missing-include cases, 155 already have
successful native-profile receipts for the same source path and bytes. Another
705 have local include settings documented by their own omaGUI README,
fb-ext-lib Makefile or fbsound Makefile. Their corrected attempts run separately
under `include-recovery`; the other 559 remain unresolved in that snapshot.
The four representative corrected profiles emit C and pass all eight GCC/Clang
checks. Original failed attempts stay in the primary coverage totals.

The final failure index contains 1,542 missing-include cases. Of the 123 added
after that first snapshot, 122 now emit C and pass 244 C checks with unchanged
BASIC bytes. These include 58 Make-selected variant-library modules, 53 SDL3
entrypoints and 11 test contexts. The variant query explicitly selects the
Makefile's non-debug branch; SDL3 uses its runner's shared helper include path,
thread setting and main role. The remaining source is RISC OS-specific fbcunit
code whose APCS/platform contract is outside the host profile. Evidence is in
`final-context-recovery/qualification.json` and
`late-test-context-recovery/qualification.json`; no programs were executed.

The remaining corpus assembly controls pass another 20 object checks across
ten matching backend/width profiles. The CPUID source belongs to the runtime's
x86 directory, and both naked-return fixtures address a 32-bit stack argument
at `ESP+4`. Their earlier 64-bit checks used the wrong ABI. Windows runtime
initialization additionally passes at both Win32 and Win64. Evidence is sealed
in `native-asm-contexts/qualification.json`.

The recovered fbsound MP3 test also needs matching backend and Intel object
settings for its inline FPU helpers. Both GCC and Clang pass O0/O2 object checks
under that context. `fbsound-test-asm/qualification.json` preserves the four
accepted controls and original rejected cross-vendor receipt.
Its OGG sibling passes four more matching backend/object checks,
sealed in `extra-fbsound-contexts/qualification.json`.
The discovered `plug-dsp.bas` is outside the Windows Make source graph. Its
header uses Linux i386 `int 0x80` wrappers and 32-bit registers. The speculative
Win64 replay fails real GCC object generation with pointer-width operand
mismatches; Clang accepting those objects does not qualify that target ABI.
That case remains outside native target coverage.

The recovered `fbs_set_mp3streamcallback2.bas` test also passes four matching
Win64 Intel O0/O2 objects. Its original cross-vendor Clang rejection is retained
in the include continuation; `stream-callback-assembly/qualification.json`
records the corrected profile. BASIC source bytes are unchanged. Source-only
lint reports one existing trailing-whitespace warning, and no audio callback
was executed.

`missing-include-coverage.json` maps each original missing-include failure to
accepted source bytes, an actual owning translation unit, a pending context,
or an explicit remaining gap. It distinguishes a rejected C check from a
timeout and does not count a failed replay as accepted C.
Changed-input deferrals have their own status rather than remaining labeled as
pending work. The known CPU/DSP optimized-label defects also retain their
matching object evidence as explicit failures, even when a raw cross-vendor
profile first rejected different assembly syntax.
`emission-coverage.json` extends that accounting to all 2,787 BASIC failures
and the missing input. Its terminal ledger links 1,461 failed standalone attempts
to accepted source profiles and another 330 to actual qualified owning units.
The remaining cases are 885 unqualified contexts, 79 failed context replays,
23 changed-source contexts, two outside the native target graph, five deferred
inputs, two confirmed optimized-assembly defects and one missing input. No
documented corrected-context C stage remains incomplete. The supplementary
181-candidate ledger qualifies 96 source candidates and retains 85 frontend
blockers; these do not enlarge the primary counts.

Further native-context recovery reads compiler arrays from project PowerShell
ASTs without executing the build scripts. Optical's main script provides 77
previously failed source profiles; its separate VBWIN1 script provides 18 more,
which pass all 36 C checks.
The plans retain owning cwd, include roots, defines, debug settings and executable
main roles. Concurrent source revisions have separate hashes. All 77 Optical
profiles pass 154 C checks across the pilot and remainder. Two BASIC sources
have changed since the original baseline; their new versions are explicit.

TurboTrek's regression manifest provides another 89 profiles. Eighty-eight pass
176 GCC/Clang checks directly. The last source exposes an audit include-order
mistake: the explicitly inserted compiler include path shadows the project's
`fbnetwire.bi`. The project build selects its own header first. Include traces
prove the selection, and the corrected unchanged source passes four real
GCC/Clang O0/O2 object checks in `header-order-qualified`.

The public runner now moves compiler includes after project roots. Its isolated
regression retains the old runner's expected rejection and two accepted C checks
with the corrected order; all three generated fixture files pass strict
source-only lint. The composed candidate passes the public nine-case integration.
The frozen-C continuation's imported helper bytes and unchanged function boundary
are independently verified. This context correction remains separate from the
eight emitter repairs.
`native-recipe-coverage.json` seals the combined 184 distinct source profiles
and 370 accepted C checks. Of these, 181 preserve the original BASIC bytes and
three explicitly qualify refreshed versions. All 184 C outputs are checked for
inline assembly and contain none; no application execution is claimed.

Seven more TurboTrek entrypoints use their documented include, main and graphics
settings. All seven emit C and pass 14 GCC/Clang checks with unchanged BASIC
bytes, sealed in `turbotrek-entrypoints/qualification.json`. The graphics helper
selects `-gfx3` from the pinned compiler's help output. No gallery, audio test,
multiplayer process or map tool was executed.

The 705-profile recovery's first cohort stops after publishing 291 receipts
because an Optical source changed. A separate continuation freezes only the
414 remaining IDs, recording twelve current Optical source versions alongside
their original hashes. It verifies source bytes before and after emission and
defers later edited inputs individually. Completed checks can share exact C
after full command/tool comparison; each completed C file is archived promptly.
Per-unit storage accounting avoids rescanning all prior files after every batch.

Included implementation files need their owning translation unit. For example,
omaGUI's public header assembles its implementation when an application defines
`OMAGUI_IMPLEMENTATION`; compiling individual widget files omits required theme
declarations. `include-graphs` captures the compiler's actual `-showincludes`
tree twice, requires unchanged dependency bytes and identical qualified C,
then records coverage through that owning root. Its first graph opens 129 files
and covers 49 additional primary BASIC candidates through a successful test
translation unit. It does not turn their failed standalone attempts into
successful independent programs. Conditional files absent from the observed
include tree remain outside that graph's coverage.
The second graph covers the current JRPG GUI through its new keyboard test,
opening 130 stable dependencies. Together the two graphs cover 98 primary
BASIC candidates through successful owning translation units.
The recovered raster-image entrypoints supply five more complete GUI roots.
The current seven-graph snapshot covers 343 primary BASIC candidates. Graph
capture distinguishes a complete umbrella from a component-only root and counts
only actual opened source files. Reuse also rejects location-dependent C.

Two further TurboTrek graphs each verify 137 stable dependencies and identical
qualified C on repeated emission. The gallery covers 49 included BASIC files.
The existing navigation smoke test explicitly enables
`OMAGUI_NAVIGATION_EXTENSIONS`; its graph covers 53 files, including all four
navigation/layout fragments omitted by the gallery's feature settings.
`turbotrek-navigation-graph` reuses the exact original C and its completed
extended GCC/Clang checks. These are owning-unit coverage proofs, not standalone
compilation or execution of the included fragments.
Together the source-profile and include-graph proofs account for all 141 original
failed TurboTrek inputs: 97 have accepted C profiles and 44 appear in qualified
owning units, with all original BASIC bytes unchanged. Evidence is retained in
`turbotrek-failure-coverage.json`.

The Midisoft corpus vendor snapshot has 86 tests and one HTML-view demo that directly include
its existing `omaGUI.bi`, but no retained test build scripts or README. An
explicit include-context replay adds that owning root and the standalone test's
main role. Four representative tests pass all eight C checks with unchanged
BASIC bytes; seven checks reuse identical C and matching warning commands from
completed contexts, while the eighth is fresh. The remaining 83 entrypoints
complete their separate cohort with all 166 C checks accepted. Together the
87 entrypoints pass 174 checks with unchanged BASIC bytes. These are
source-context corrections, not reconstructed
native build recipes. Their accepted listbox test also supplies a stable
129-dependency graph covering 49 included primary BASIC files, with repeated
emission matching the already qualified C exactly.
The accepted navigation smoke test supplies a second, 137-dependency graph
covering 53 files. It explicitly enables the optional navigation profile and
includes all four navigation/layout fragments. The two graphs are deduplicated
by source path and bytes when reporting coverage.

A further Optical include review finds 106 test/tool sources with direct
includes of unique existing project headers. Their explicit profiles use the
native helper's `src`, GUI and project include roots, plus its runtime checks
and BASIC warning level. Four representative profiles pass eight C checks;
one BASIC source has changed since the baseline and is recorded separately.
The remaining cohort completes 50 emissions and 100 accepted C checks before
the helper script changes. Its guards defer the last 52 inputs. The completed
receipts remain intact, while recipe-version revalidation stays explicit in
`optical-header-context-rest/completion.json`. Across the plan, eleven current
source versions differ from the original baseline. Twenty-four nested-include
cases remain separate rather than being treated as direct include proofs.
These profiles establish C acceptance, without claiming a reconstructed native
source list, linked application or runtime test.

Corrected include profiles also share successful C checks only after matching
the compiler bytes, full normalized warning command and exact C bytes. Shared
receipts point to the actual diagnostic log; they do not duplicate large logs
or infer acceptance from an unfinished check. Include/location-dependent C is
checked separately. Completed generated C is archived after hash verification.

Summary readers scan each unit directory once to discover completed receipts,
amendments and archives. The consolidated snapshot also records pointer-error
unit IDs, avoiding a second full receipt pass during native checkpoints.

Completed raw C is stored as `unit.c.gz` after a full SHA-256 round-trip check.
`c-archive.json` records the original and archive hashes and sizes; original
diagnostic receipts remain unchanged. This reduced about 388 MB of completed
C to about 69 MB in the first archival pass. The live archiver follows the
specific writer process handle and never removes active compiler input.
Inspection and repeat analysis materialize exact C under a per-unit lock.
For a permanent local restoration:

```text
python build/c-emitter-audit-20261022/c_artifacts.py build/c-emitter-audit-20261022/priority-v2/UNIT_ID
```

Archive verification and restoration were tested on real emitted C. A separate
changed-content check confirms that edited C is retained rather than removed.

This repair does not claim that every qualifier-related diagnostic is gone.
Typed null-pointer comparisons and low-level pointer loads can still generate
warnings, and the termination-contract finding remains separate work.

<!-- end of c-emitter-audit.md -->
