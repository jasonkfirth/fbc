<!--
Project: FreeBASIC C backend qualification
File: c-emitter-c-resume-lowering.md
Purpose: Record compiler-emitted ERROR continuation lowering and its promotion limits.
Responsibilities: Bind warning-class measurements to the qualified private compiler.
This file intentionally does NOT claim whole-corpus adoption or general unwinding.
-->

# Compiler-emitted ERROR continuations

The November 3 private compiler now emits the continuation tokens, failure
checkpoints and native handoff itself. Generated C is unedited in the native
qualification. The candidate remains private because activation cleanup and
broader native ABI contracts are incomplete.

The sealed checkpoint is
`build/c-emitter-c-resume-checks-20261103/summary.json`. Its compiler SHA-256 is
`75ee838bddb0d0469c448321493c24169f838fdfa5af862a0d0a0fcb7b7aff03`.
The full Make-selected compiler graph passes 224 module checks and 224 emitted-C
fixed points with 0 GCC and 1,661 Clang warnings. Exact C permits reuse of 223
existing object/diagnostic records; the changed HLC module is checked afresh.
No installed compiler or runtime archive is replaced.

## Largest-class admission

Three existing high-warning project profiles use their accepted commands,
current source pins and aggressive unsuppressed GCC/Clang flags. The comparison
is against the accepted C-GOSUB queue, not an intermediate private candidate.

| Profile | GCC before | GCC after | Clang before | Clang after |
| --- | ---: | ---: | ---: | ---: |
| Scorched Earth, additional 156 | 237 | 0 | 321 | 107 |
| fb-VNC UI, production 6 | 89 | 0 | 131 | 81 |
| Stunts data, additional 11 | 90 | 4 | 95 | 21 |
| Total | 416 | 4 | 547 | 209 |

The GNU continuation class falls from 350 to 4 in each tool. Neither
`-Wjump-misses-init`, `-Wclobbered` nor `-Wdeclaration-after-statement` is
introduced in these profiles. Project emission and C compilation pass; these
checks do not execute the three applications.

The final input audit matches the selected project source hashes to the
baseline and verifies the archived diagnosed C. An optional emission-identity
repeat matches Scorched Earth but differs for fb-VNC; Stunts is not repeated
after that difference. The differing C is retained without investigation or
rerun, respecting the user-closed emission investigation. This checkpoint
does not claim repeated-C identity for all three projects. The detailed
disposition is `project-input-verification.json` beside the summary.

The remaining private Clang diagnostics are 141 padding, 51 missing-noreturn,
7 unreachable-code, 4 global-constructor, 4 GNU-label and 2 reserved-identifier
warnings. Padding includes new capture records. Missing-noreturn warnings
identify private cleanup barriers that terminate instead of handling unsupported
resources. These are visible debt, not grounds to suppress diagnostics.

Accepted totals remain **2,139 GCC / 8,408 Clang across 2,251 profiles**.
No reduction from this private admission is credited to those totals.

## Transfer and scope ownership

Each ordinary ERROR failure block compares its actual retry/next tokens and
returns directly to labels in the live source C scope. Checkpoints are created
on failure, and the constant buffer pointer is unchanged across `_setjmp`.
The executable root does not create a failure checkpoint for an outer native
handler. C handler-owner checkpoints remain a separate contract.

Array bounds, array dimensions and null-pointer checks have a different runtime
contract: they return NULL on success and a handler on failure, with no supplied
retry/next labels. They now use the native handoff without fabricating a
resumable checkpoint. Matching requires a compiler-owned RTL symbol, so a user
procedure with the same spelling does not acquire this behavior.

Two rejected approaches remain recorded. A shared function-header dispatcher
left and re-entered live C scopes, producing initialization and clobber warnings.
An initial local dispatcher failed to classify check helpers; its fallback
restored GNU addresses throughout affected procedures and multiplied warnings.
The current helper classification removes that regression. Historical private
receipts are retained separately and are not treated as accepted results.

## Qualification and remaining boundaries

- Eighty native programs cover unchanged GAS owners, recursive/conditional
  owners, C/GAS combinations and escaped worker values changed by a handler.
- Sixteen four-thread programs handle 8,192 errors, with thread-local metadata
  accounting returning to zero after the tested resume cycles.
- Forty helper programs cover success, unsigned and signed bounds, an
  undimensioned array and a null pointer. Their native handlers check ERR and
  owner data, then end the program. They do not prove abandoned-frame cleanup.
- Both C producers and GCC/Clang participate. The resume/thread matrices include
  GNU11/GNU23 and O0/O2; helper executions include O0/O2.
- The matching-prefix backend gate, seven procedure-state tests and structural
  gate pass. Strict source-only lint covers all 23 current compiler overrides
  and BASIC fixtures with zero errors and zero warnings.

Promotion requires retirement of resume records and captures on one-way
C/native abandonment; pending state through nested native errors; native
outgoing stack-argument capacity and broader calling conventions; complete
managed/partial resource cleanup; capture-allocation cost measurements; and
wider target/runtime integration. The complete stable corpus/project sweep
follows those contracts. The 32 changing Optical profiles remain pending;
the previously excluded work remains excluded.

The candidate and project scratch each have a 32 MiB cap and a combined
64 MiB cap. Scratch C, objects and executables are reused, then removed from
their verified private directories after successful checks. Compressed C and
diagnostic receipts are retained. At most two heavy workers run concurrently,
at below-normal priority, with bounded subprocess timeouts.

## C activation retirement

The next private checkpoint fixes checkpoint retirement when a C owner transfers
past a failing C worker. Previously, the runtime retained the worker's resume
record after freeing its frame. Callback-created records could also share the
surviving owner's frame and escape a frame-only retirement rule.

Frame removal now unlinks its anchored resume records before cleanup can free
the containing capture allocation. Each ready owner also remembers the older
resume boundary. A one-way transfer retires newer records, including records
created by the abandoned callback, while preserving older live activations.
Popping a boundary record invalidates the dependent owner checkpoint before
freeing its storage, preventing allocation-address reuse from reviving it.

The receipt is `build/c-emitter-resume-retirement-20261103/summary.json`.
Its compiler SHA-256 is
`0a39f09b94bf40fba3b9c3c0108ce768b4fdbcc535a25e8a35359bcec7d661df`.
The current compiler still passes all 224 module checks and 224 C fixed points,
with 0 GCC / 1,661 Clang warnings and exact reuse of all 224 prior C objects.
The private runtime is rebuilt into both normal and thread-safe archives.
Public `FB_ERRORCTX` and `FB_ERROR_FRAME` layouts and installed binaries stay
unchanged. No project emission is repeated for this runtime-only repair.

- Two baseline programs reproduce the stale-record failure before the repair.
- Twenty-four replacement-runtime programs check selective frame retirement,
  stale-boundary refusal, preservation of older records and repeated C-owner
  transfers before and after worker string initialization.
- Sixteen capture-observer programs execute 4,096 transfers and check 8,192
  watched capture frees. Allocation identities distinguish reused addresses;
  each worker allocation is freed before its owner's allocation, exactly once.
  The original scalar-string destruction assertions remain active.
- Eight additional programs execute 1,024 real older-checkpoint resumptions
  after nested owner transfers, with metadata returning to zero each cycle.
- Eighty mixed-backend and sixteen four-thread programs pass with the new
  runtime. The latter handle 8,192 errors. Backend, seven procedure-state and
  structural gates pass. Strict source-only lint of both new BASIC fixtures
  reports zero errors and warnings.

Both C producers, GCC/Clang, GNU11/GNU23 and O0/O2 participate in the generated
transfer matrices. Generated application C is unedited. Native API controls
use both C compilers, standards and optimization levels. All diagnostic flags
remain unsuppressed; the additional private owner field introduces one visible
Clang padding diagnostic in the runtime comparison.

This closes the tested C-frame/owner-boundary retirement gap. Native handlers
that abandon C workers, pending native nested error state, outgoing native
stack arguments, broader targets, managed/partial lifetimes and capture
allocation costs remain open. Promotion and the full stable queue refresh
remain pending; accepted warning totals are unchanged. The previously closed
emission investigation and other excluded work are not reopened.

This stage retains about 10 MiB under a 24 MiB cap, reuses the prior compiler
objects, limits heavy work to two below-normal workers and retains bounded
subprocess timeouts. It does not modify any installed compiler or archive.

## Native handler outgoing stack arguments

The next private checkpoint replaces emitted C's inline native handoff with a
typed call to a Win64 runtime trampoline. The trampoline reserves a separate,
aligned outgoing argument area below the live C stack and probes its pages
before entering the handler. C checkpoints restore the original worker stack.
No C `alloca` or warning suppression is introduced.

New GAS procedures use a private one-argument leaf thunk to pass their exact
outgoing argument size internally. Older GAS objects keep the public setter;
its leaf wrapper captures their complete frame extent as a conservative bound.
Restored opaque handler identities retain their original frame and size.

The receipt is `build/c-emitter-native-stack-20261103/summary.json`; compiler
SHA-256 is `8710ceba7d827b0a84d10e334558fa432c6de96fda695fafebaf94b4fa7ea3a0`.
All 224 compiler modules and emitted-C fixed points pass with 0 GCC / 1,661
Clang warnings. The bootstrap and final generation are recorded separately.

The old C worker passes a twelve-argument control but exits with illegal
resume on a valid 48-argument handler call. The replacement passes 128 programs
and 68,608 handled errors using both frozen older GAS objects and current GAS
owners. Cases include 12, 48 and 768 arguments, a 6,144-byte argument area,
and 64 resumptions within each worker call. Both C producers, GCC/Clang,
GNU11/GNU23 and O0/O2 participate; generated application C is unedited.

The same compiler/runtime pair also passes 80 existing interoperability
programs, 16 threaded programs, 24 C-retirement programs, 16 capture programs,
eight preserved-checkpoint programs, backend and structural gates, and seven
procedure-state checks. Strict source-only lint covers all twelve BASIC
overrides and fixtures with zero errors or warnings. Installed binaries and
public error-context/frame layouts are unchanged. Retained work stays below
the 32 MiB stage cap with at most two heavy workers.

A separate GAS-to-GAS owner-local control fails identically with the accepted
and prior private native compilers. It is recorded as an existing native frame
issue, not as a C-worker stack result. Native abandonment, pending nested state,
wider ABI/target integration, managed/partial lifetimes and allocation costs
remain open. The candidate remains private. Exampleageddon has been added as
an additional warning queue to supply small cases for the largest classes.

## Small-example initialization and emitter maintenance

The complete Exampleageddon queue identifies 54 checked examples containing all
84 GCC possible-uninitialized and 184 Clang conditional-uninitialized warnings.
The current private lowering removes all of those observations and 1,120 GNU
continuation observations per C tool without changing an example. This supplies
small real inputs for the shared control-flow cause already seen in larger
games; blanket source initialization is not the repair.

The next private maintenance cut removes the retired, unreachable header
dispatcher, its obsolete inline native handoff and unused API binding. Active
transfers keep their local choices and typed runtime handoff. The accepted
bitfield member spelling is rebased into this cut. One changed compiler object
passes GCC/Clang and its own emitted-C fixed point, reusing 223 exact objects.
The 54 example outputs and six native matrix emissions are unchanged except for
that accepted member name. All 80 native interoperability programs pass again.

The subset now reports 20 GCC / 77 Clang observations, including seven unused
cleanup helpers introduced by private planning in four multithreading examples.
Those findings remain visible. Both staged BASIC files pass strict source-only
lint. No installed binary, runtime archive, example or canonical compiler source
changes in this maintenance cut. The receipt is
`build/c-emitter-continuation-cleanup-20261103/summary.json`; the stage stays
under 24 MiB and at most two below-normal heavy workers. Native abandonment,
pending nested error state, wider targets and lifetime/allocation contracts
remain open. No private warning reduction is credited to accepted queue totals.

## Generated helper dependency ownership

The seven new cleanup-helper warnings in the small-example comparison exposed
a shared graph boundary defect. Ordinary private procedure bodies could be
pruned while their generated cleanup bodies remained outside the graph. Calls
inside generated callbacks could also be treated as output-wide roots.

Each callback and cleanup body now enters and leaves its own private graph
node. Generated prototypes have matching ordered spans. Raw destructor calls
from cleanup bodies record the same dependency edges as ordinary typed calls.
The existing source-owner references retain live helpers; unreachable owners
can retire their helpers and dependencies together.

The current compiler passes the affected module's C fixed point using one fresh
object and 223 exact reused objects. All 54 example C outputs differ only by
whole-line omissions. Seven helpers disappear across four examples, leaving
13 GCC / 70 Clang observations with no replacement warning. All 80 mixed native
programs, 16 capture-retirement programs (4,096 transfers and 8,192 watched
capture frees), 28 default scalar-object lifetime runs and seven procedure-state
checks pass. A small array-owning object reproducer drops its helper warning
from one to zero under both C producers/tools at O0/O2; all 16 old/new native
value runs pass. Three changed/new BASIC files pass strict source-only lint.

The receipt is `build/c-emitter-helper-ownership-20261103/summary.json`; compiler
SHA-256 is `084d073f3bde773de44b109cf7fa7f884c0e48af11e717f2e67723e3c420e9d8`.
The stage reuses unchanged private runtime archives, stays under 24 MiB and
uses at most two below-normal heavy workers. Canonical sources, examples and
installed binaries remain unchanged. Native abandonment, pending nested state,
wider targets and lifetime/allocation contracts still prevent promotion. The
accepted corpus totals remain separate from these private measurements.

## Native pending error state

The next private runtime cut puts native handler activations on the same ordered
dispatch chain as C callbacks. Native records use owned storage because the
handoff never returns through its C call site. A separate owned chain lets TLS
teardown release them without traversing borrowed C stack records. RESUME pops
the current native activation before restoring the surviving outer error state;
ordinary C callback restoration does not consume an outer native handler.

The original runtime exits at the new nested-handler fixture's outer ERR
assertion (31). A C worker resumed through a second native handler had lost the
first error's ERR/ERL and continuation state. The initial unified chain exposes
a separate normal-retirement refusal (1) with alternating C/native/C handlers.
That check retained a saved handler slot which RestoreState deliberately does
not restore. Retirement now protects actual native pending activations and C
owner restoration handles, while ignoring that unused snapshot slot.

With the unchanged compiler
`084d073f3bde773de44b109cf7fa7f884c0e48af11e717f2e67723e3c420e9d8`,
16 nested-native and 16 alternating C/native/C programs pass. They handle 5,120
errors and check owner/worker values, outer ERR/ERL and zero owned metadata after
each cycle. Both C producers, GCC/Clang, GNU11/GNU23 and O0/O2 participate.
The generated application C remains byte-identical between runtime revisions.

The same runtime sources also pass 80 existing mixed programs and 16 four-thread
programs handling 8,192 errors. Thirty-two API control programs check ordered
retirement, refusal to retire a still-pending native owner, invalid-resume
refusal, allocation refusal and owned-only TLS teardown. Runtime warning-class
counts match the original units under both aggressive unsuppressed C profiles.
The alternating source fixture still exposes the existing original-RESUME-tail
GNU fallback diagnostics; those remain visible. Four BASIC fixture files pass
strict source-only lint with zero findings.

The receipt is `build/c-emitter-native-pending-20261103/summary.json`. Its
derived mixed-case error count is corrected by an explicit journal: that case
has three errors per cycle, rather than the shared driver's two-error count.
No compiler objects or runtime archives are copied or rebuilt. Fresh runtime
objects are linked explicitly with the unchanged private archive and ASM
wrapper. Public error layouts, canonical sources, examples and installed
binaries remain unchanged. The stage retains about 2.3 MiB under a 24 MiB cap
and uses at most two below-normal heavy workers with bounded timeouts.

This qualifies pending-state restoration for the tested Win64 resumption paths.
Native one-way abandonment, managed/partial lifetimes, allocation costs, wider
targets, matching canonical integration and a complete stable corpus refresh
remain open. Private runtime results do not change accepted queue totals.

## Ordinary owner entry reachability

The next private compiler cut traces the ordinary owner entry separately from
the outlined callback entries. Re-emitting an unreachable original RESUME tail
had disabled the local token representation for its entire C procedure. The
alternating C/native/C fixture therefore retained 18 GNU observations per C
tool despite using outlined handlers.

The new ordinary-entry graph follows direct branches, conditional branches,
GOSUB callee/return sites, jump-table destinations, escaped label addresses and
ERROR retry/next operands. Only the rewritten handler setter's source label
does not seed an ordinary entry. The owner checkpoint explicitly retains final
cleanup. Opaque assembly keeps the old representation, and a reachable source
RESUME or unknown indirect transfer conservatively retains the complete owner.

The existing statement flush consumes an ordered mask without rewriting the
source AST or its exported semantic facts. Lexical declarations, balanced
scopes and annotations remain; unreachable initializers and executable tails
are discarded. Discarded jump tables release their owned value/label arrays.
The alternating fixture loses all 18 GNU, two unused-label and two Clang
unreachable observations, retaining its existing single unused-variable warning.

Real-example comparison exposed another compiler defect: modern-dialect ERROR
owners requested fb_SetJmp even though the GOSUB module registered it only for
dialects permitting GOSUB. Internal setjmp/frame-address registration now also
serves the C backends. Language GOSUB services and syntax keep their original
dialect gating. Both C producers still reject the existing OPTION NOGOSUB test,
and modern-owner GAS64 assembly remains byte-identical to the prior compiler.
No conflicting fb_SetJmp/fb_FrameAddress declarations were found in the corpus
and games outside compiler, bootstrap and build directories during this cut.

All 22 profiles of the 11 source-handler Exampleageddon programs compile under
both aggressive unsuppressed C profiles. Among the 20 profiles that compiled
with both private providers, GCC observations fall from 78 to 17 and Clang from
128 to 39. The remaining two profiles are the unchanged modern-dialect
proguide/errors/on-local.bas, whose prior private frontend failed on fb_SetJmp.
They now compile with one unused-label observation per C tool/profile. Their
newly available diagnostics are kept separate from the paired reduction.

Sixteen old/new native value programs preserve direct, fallthrough, GOSUB and
opaque owner bodies byte for byte while checking SELECT CASE AS CONST and
retry/next values. Sixteen alternating native programs pass. A separate modern
owner passes eight programs and 512 handler transfers with a live string.
Sixteen capture programs check 4,096 transfers and 8,192 watched capture frees;
28 scalar-object lifetime runs and seven procedure-state checks pass. The final
compiler re-emits the four already-executed value/alternating C inputs exactly,
so the registration follow-up reuses those native receipts explicitly.

The compiler SHA-256 is
`2edf8baf4194fe30bad124202995c31fb9a96dd154ea437bed5c4ae45f23b7f8`.
Three affected module C fixed points pass, reusing 221 base objects. Strict
source-only lint covers eight changed/new BASIC files with zero findings. The
receipt is `build/c-emitter-owner-flow-20261103/summary.json`; retained work is
about 11 MiB under a 24 MiB cap with at most two below-normal heavy workers.
Canonical sources, examples, archives and installed binaries remain unchanged.

The real-example comparison still retains reachable indirect transfers, two
Clang noreturn observations, unused declarations/labels and natural padding.
Those require separate correctness or ABI dispositions. Native abandonment,
managed/partial lifetimes, allocation cost, wider targets, canonical integration
and the complete stable corpus refresh remain open. Accepted totals are not
changed by these private measurements.

## Resume-only owner transfer contracts

The next private emitter cut distinguishes callbacks that reach owner cleanup
from callbacks that only resume or terminate. Previously every outlined owner
received a synthetic checkpoint jump to cleanup. That created a phantom C
return edge in genuinely non-returning procedures. With a live string, the
same old path can reference a cleanup label which was not emitted at all.

The compiler now derives cleanup-transfer capability from the existing handler
graphs. Only a handler that reaches the cleanup boundary gives the ordinary
owner graph that hidden entry, emits the checkpoint-to-cleanup jump and marks
its label as referenced. The checkpoint still owns the existing registration
metadata. Genuine cleanup transfers retain their previous C unchanged.

The two retained Clang noreturn diagnostics in erfn.bas and ermn.bas occur in
the default, non-resumable profile; those examples document -exx. The source
return classification is valid in that profile. This repair preserves every
noreturn declaration/definition across all 22 real-example profiles rather
than weakening the attribute. Their counts change from 19 GCC / 41 Clang to
19 / 39 with no new diagnostic family or count.

A separate resume-only string fixture reproduces the missing-label defect in
16 old C compile profiles. All corresponding replacement C profiles compile.
The C execution matrix passes 48 old/new programs: 16 repaired non-resumable
cases still exit with illegal-resume status 5, and 32 resumable cases continue
and observe 2,048 exact string destructions. Four GAS controls produce identical
old/new assembly, exit 5 without resumption and succeed with -exx. These are
matrix controls, not sixteen distinct corpus program failures.

Sixteen capture-retirement programs check 4,096 transfers and 8,192 watched
capture frees. Eight modern-dialect string-owner programs and seven
procedure-state checks pass. The capture and modern transfer C outputs remain
byte-identical to the prior provider. Seven staged/new BASIC files pass strict
source-only lint with zero findings. One fresh HLC object and its emitted-C
fixed point reuse the other 223 current compiler objects.

The compiler SHA-256 is
`cdc51444a350b6311adb790844f9e815b04ec37cf34d0fcd1d1f8378a6994873`;
receipt: `build/c-emitter-owner-noreturn-20261103/summary.json`. The superseded,
uninstalled first prototype executable is removed after its replacement passes;
its object response, source, compressed C and retirement receipt remain. This
releases 4,088,832 bytes and leaves about 7.4 MiB under a 24 MiB stage cap. No
runtime archive is copied, and at most two below-normal heavy workers run.
Canonical sources, examples and installed binaries remain unchanged.

The initial incomplete value run is retained as the baseline missing-label
witness, superseded by the explicit old/new matrix above. Native abandonment,
managed/partial lifetimes, allocation costs, wider targets, remaining reachable
indirect transfers and unused/padding observations remain open, along with
canonical integration and the complete stable corpus refresh. Accepted queue
totals remain separate from these private measurements.

## Native handler return cleanup

The private native-return cut repairs a lifetime defect when a native Win64
handler abandons a C worker by returning from its owning procedure. The prior
final owner hook runs after native local destruction. Retiring the worker only
there would destroy the caller's string before its abandoned callee's string.
The retained baseline reports destruction-order exit 84 for a constructed
worker and retirement exit 1 when the error precedes worker construction.

The compiler now emits a separate pre-destruction hook on native fallthrough
and explicit procedure exits. Runtime native-owner metadata saves its C frame,
resume and dispatch entry boundaries. The hook validates those boundaries,
discards newer pending continuations and retires abandoned C workers in order.
The native handler remains registered through its own local destruction; the
existing final hook restores the surviving state and retires its metadata.
The direct owner-leave API retains its refusal contract. Public error context
and C frame layouts, existing RTL indices and the zero-argument native leaf ABI
remain unchanged.

The 96 new execution profiles pass under both C producers and GCC/Clang,
GNU11/GNU23 and O0/O2. They cover EXIT SUB, a jump to function completion and
fallthrough, with errors before and after worker-string construction. They
observe 6,144 handled errors and 9,216 exact worker/caller string destructions.
Another 160 executions cover ordinary mixed continuations, nested/alternating
handlers, four-thread isolation, API refusal, allocation failure and TLS
teardown. Seven procedure-state checks pass. All 22 Exampleageddon handler
profiles produce byte-identical C and retain 19 GCC / 39 Clang observations.
Old/new runtime warning class counts also remain identical. Six staged BASIC
files pass strict source-only lint with zero findings.

This component fixes a structural promotion blocker; it claims no additional
corpus warning reduction. Only three compiler objects are rebuilt, 221 are
reused, and all three affected C fixed points pass. Compiler SHA-256:
`fc33609747b10d7bdc2e1da3a8222ab5dbfde02821ccaf2daa156df89d5d5703`.
Receipts are in `build/c-emitter-native-return-20261103/summary.json`.
About 7 MiB is retained under a 32 MiB cap with at most two below-normal heavy
workers. Canonical sources, examples, installed binaries and runtime archives
remain unchanged.

Recovery into a continuing native loop or through arbitrary native GOTO,
abandonment across newer native owners, managed/partial lifetimes, allocation
costs, wider targets, canonical integration and the full stable corpus refresh
remain open. An attempt to cross a newer native owner is explicitly refused
by this component. The return matrix does not qualify those other paths.

## Rebase onto the current canonical compiler

The ERROR emitter now includes the accepted unsigned pointer-order repair and
is rebound to the complete current canonical source/SDK snapshot. All 224
private-provider C fixed points pass. Relative to the current canonical graph,
128 changed outputs are recompiled and 96 exact objects are reused. The private
compiler has zero GCC warnings and 1,661 Clang observations: the canonical
1,639 padding/13 global-initialization observations plus nine generated cleanup
helpers missing a valid nonreturning annotation. Those helper observations are
not suppressed or treated as accepted canonical cleanup.

The private compiler SHA-256 is
a5f91a647ec65cfb123ec03e94aaeec690cf9db9acc1e48dd7f5bf4225b845ee.
Its complete ordinary backend suite passes with a matching single/MT runtime
prefix. The prefix uses thin archive references, with four replaced old
ERROR/TLS members removed. It copies zero archive member contents. A direct
four-thread probe completes 256 resumptions through an ordinary FBC MT link.
Both original runtime archives and installed binaries remain unchanged.

The rebased producer passes 96 native-return cleanup programs and 16 shared-code
resumption programs, plus seven procedure-state checks. Four exact C and two
GAS assembly identities bind the pointer-order matrices, with two fresh GAS
executions. The selected 54 checked examples are re-emitted and compiled under
current before/after compilers and unsuppressed GCC/Clang flags. GCC changes
from 1,208 to 1 and Clang from 1,331 to 70. All possibly-uninitialized
diagnostics disappear. The one remaining GCC observation is lesson32.bas's
qsort callback cast, separate from ERROR continuation lowering. Examples are
unchanged and are not executed. Accepted full-queue totals are not changed.

## Ordinary recovery remains a promotion blocker

A new eight-error local recovery loop passes native GAS and correctly paired
canonical C controls, but the private provider exits with status 1. The seed
and complete rebased compiler produce identical failing loop C. This is a real
private regression, not a completed lifetime repair. It is retained in
build/c-emitter-error-rebase-20261103/loop-current/receipt.json.

An ordinary recovery edge must return to the owner checkpoint rather than
execute the next error from inside an active callback. Its boundary must also
precede owner branch destructors so abandoned child cleanup keeps its order.
The shared-code control demonstrates why an arbitrary GOTO cannot by itself
discard a pending worker: its native handler passes through an ordinary label
and later RESUME NEXT. Sixteen mixed programs pass with the current private
provider. The separate all-native cross-procedure control fails and remains
distinct from this supported C-worker path. It is not repeatedly retried.

An initial canonical loop probe linked the private archive and is excluded as
a runtime-pairing error. The correctly matched canonical probe passes. The
inherited shared driver's standalone 16 MiB guard stopped before a compiler
child; its final driver uses the previously declared 32 MiB rebase cap. A
qualification INPUT-script prefix could not be rescanned inside ld's archive
group. The thin-archive prefix resolves that limitation and passes single/MT
and complete backend links. An initial MT fixture used a reserved identifier
and duplicate legacy loop declaration; the corrected fixture passes. These
stopped harness observations are retained separately from compiler defects.

Receipts: build/c-emitter-error-rebase-20261103/summary.json and verification.json.
The next repair and the destructor/resumption constraints are captured in that
stage's NEXT.md. Work remains bounded by 32 MiB, two heavy workers and checked
subprocess deadlines. Canonical code, examples, runtime archives and installed
binaries remain unchanged by this private batch. Ordinary recovery, generated
helper annotations, managed/partial lifetimes, allocation costs, wider targets,
canonical integration and the full stable corpus refresh remain open.

## Ordinary recovery and explicit exits, November 4

The local recovery-loop regression is closed in the private compiler. Admission
tracing found that an explicit EXIT SUB reached the procedure-end label, which
the outlined graph rejected as falling off its body. The earlier failing loop
C did not yet contain an outlined callback. That old failure receipt remains
valid; callback re-entry alone was not its complete cause.

The C backend now owns hidden recovery boundaries before source GOTO and
explicit-exit destructors. Proved edges return to the original owner checkpoint;
the runtime retires abandoned child C activations before source edge cleanup.
The callback keeps paths that can still RESUME. Ordinary destinations and edge
destructors with unknown source/foreign callees remain conservative because
those callees may RESUME. The proof currently admits a small explicit set of
callback-free string/ERROR runtime primitives. It does not infer a general
interprocedural effect contract.

Source RESUME uses the existing typed runtime handoff, with C tokens restored
inside the runtime call. The original-entry analysis treats it as a known
terminal transfer rather than abandoning its entire graph. Callback emission
applies the ordinary unused-label rule, and does not emit a callback label for
an owner-only recovery boundary. Primitive callback temporary storage is emitted
above the entry jump; producing calls, pointer typedefs, enum types and source
initializers retain their original scope and order.

The ordinary -e profile exposed a separate Win64 owner-checkpoint crash, status
0xC0000028. A controlled C probe isolated the checkpoint frame argument; it is
investigation evidence only. The actual emitter now passes a null second frame
argument, matching the existing statement-checkpoint convention and leaving
resource retirement with the runtime. The final tests use unmodified emitted C.
Generic SETJMP lowering and the runtime ABI are unchanged.

The current private compiler SHA-256 is 96fa83ea05bee355b949c1738069ef56b478cd4904f1d0843bff29b38b5da410. All 224 current provider
C fixed points pass. GCC reports zero compiler warnings. The nine generated
cleanup-helper noreturn warnings are gone; Clang retains 1,639 natural-padding
and 13 global-initialization observations. Only generated helpers that always
terminate receive matching declaration/definition attributes.

The checked matrix passes 112 executions across both C producers, GCC/Clang,
GNU11/GNU23 and O0/O2. It covers direct/conditional recovery, handler replacement,
forward/backward paths, worker-before-scope-before-owner string destruction,
failures before worker construction, shared-code RESUME and mixed exit/RESUME.
A further 28 ordinary-profile executions pass their expectations: 20 recover
successfully and eight reject RESUME without statement-resume metadata with the
documented status 5. All ordinary-profile generated C has zero GCC/Clang
warnings. The checked fixtures retain only 48 global-initialization observations
across repeated C compilations. The permanent backend suite, seven procedure-state
checks and lint of all 22 created/changed BASIC provider/fixture files pass;
lint reports zero errors and warnings.

The selected 54 checked Exampleageddon profiles are freshly emitted. Their C is
byte-identical to the previous qualified private cohort, retaining its unsuppressed
1 GCC / 70 Clang observations versus 1,208 / 1,331 for the current canonical
control. These are cohort counts, not new accepted queue totals. No example is
edited or executed. Installed compilers, base runtime archives and unrelated
canonical code are unchanged. Thin runtime archives continue to reference existing
members rather than copy them.

Current receipts and the provider map are in
build/c-emitter-owner-recovery-20261104/summary.json and verification.json.
The failed fixture dialect/observer-alias runs, source-RESUME warning admission,
and ordinary-profile checkpoint failure remain separately recorded. Superseded
private executables are hash-recorded and retired; shared objects are reused.
The batch retains one private executable under a 32 MiB cap with at most two
heavy workers and bounded subprocess deadlines.

ERROR promotion remains open for broader managed/partial lifetimes, allocation
costs, unproved callee/native recovery crossings and wider targets. The next
warning measurement is a complete current private Exampleageddon census, followed
by the stable corpus/project queues and canonical integration decisions. Natural
ABI padding must remain observable. PNG and the user-closed optional emission
investigation remain excluded.

## Complete private Exampleageddon measurement

The current recovery compiler completed all 3,548 Exampleageddon profiles with
unchanged source/status membership. It records 139 GCC and 7,996 Clang warnings;
7,542 Clang observations are padding. The [complete census](c-emitter-exampleageddon.md#complete-private-census-november-4)
retains raw checks and ranks procedure-pointer conversions next. Six new unused
variables and seven new unused labels per tool remain promotion gates, alongside
the previously documented lifetime/platform boundaries. No example is edited.
This full private census supersedes the selected-cohort measurement for queue
ranking, and does not change accepted corpus/project totals.

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

<!-- end of c-emitter-c-resume-lowering.md -->
