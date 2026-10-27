<!--
Project: FreeBASIC compiler verification
File: c-emitter-alignment.md
Purpose: Record the raw-storage alignment contract and qualified emitter repair.
Responsibilities: Preserve failures, adoption evidence and remaining limits.
This file intentionally does NOT claim warning-free or full-corpus output.
-->

# Raw-storage alignment

Canonical source includes v54 unaligned scalar/record lowering and v55 pointer
field qualification without losing declared paths. The v52 negative controls
still trap with GCC and
Clang alignment sanitizers. The earlier v53 prototype was rejected because it
lost typed member structure and substantially increased Clang buffer diagnostics.
The following history preserves that rejected approach separately from v54.

The old offset query returned zero for expressions it did not recognize. An
unknown pointer or variable byte offset can therefore look aligned. A byte
buffer also has only byte alignment, even at offset zero. The alignment helper
distinguishes stored pointer values from addresses of declared storage, returns
unknown when low bits cannot be proved, and retains proven multiples through
arithmetic. Scalar transfers use byte copies when alignment is unknown. That
path removes outer typed pointer casts before copying. It also retires the
unused dereference wrapper rather than leaving its expression node allocated.

`tests/quirk/c-array-index-unaligned.bas` preserves the original reduction.
`tests/quirk/c-scalar-alignment.bas` adds opaque pointer parameters, a pointer
variable, floating-point bit patterns, packed/BYREF fields, side-effect counts
and aligned multidimensional controls. The first prototype passes 80 ordinary
objects and 16 additional sanitizer-object stages across both widths and C
backends. Forty Win64 executions retain eight expected old traps and 32 passes.
All 20 before/after source-model pairs match.

Nested byte-copy expressions exposed shadowing of the original fixed temporary
name. Revision r2 uses a counter owned by C emission, reset for each output. It
does not consume BASIC symbol identities. Forty ordinary objects match the
tested first prototype exactly. Six of eight sanitizer objects also match;
the other two are freshly linked and run successfully. Its 20 native proofs
therefore comprise two fresh executions and 18 retained exact-object proofs.
Four changed BASIC files pass strict non-semantic fblint with zero errors,
warnings and information messages.

All 378 stages in the retained 189-module audit graph pass, with identical
complete global symbol sets, including imports. Both full compiler producers
link and reproduce 40 exact C/source-model outputs from the bootstrap. This
retained graph includes the owned source overlays; it does not requalify other
concurrent changes in the canonical compiler checkout.

| Retained compiler-graph diagnostic | v52 | Private v53 r2 |
| --- | ---: | ---: |
| GCC total warnings | 6,820 | 6,573 |
| Clang total warnings | 20,593 | 36,531 |
| Cast alignment, either compiler | 1,450 | 1,163 |
| GCC cast qualifiers | 2,014 | 2,094 |
| Clang cast qualifiers | 2,027 | 2,094 |
| Clang unsafe buffer usage | 4,470 | 20,619 |
| Shadow warnings, either compiler | 0 | 0 |

The increase is retained, not suppressed. Treating all opaque pointer values as
unknown sends more previously typed member accesses through byte arithmetic.
That prototype required a replacement retaining declared field paths while
allowing unaligned access. It remains private; v54 uses the typed views below.

A separate host C capability probe tests `aligned(1)` typedef views of scalar
and record types. GCC and Clang pass O0/O2 sanitizer controls using these views;
all four ordinary-view negative controls trap. Static assertions preserve
record sizes, field offsets and C type compatibility. This is a promising way
to keep member syntax without promising natural alignment. The adopted v54
implementation uses this representation for compatible declared record paths.

Six focused Clang core-analyzer checks report zero findings before and after
the private repair. The old output still has the sanitizer traps. This contrast
is direct evidence that a clean path-analysis result does not establish the
alignment contract. Aggregate transfers and typed-pointer formation also need
their own review beyond these scalar controls.

Evidence is below `E:/fbc-c-audit-01a166ce/scalar-alignment-v53/`: the first
`qualification/results.json`, `r2/qualification/results.json`,
`r2/compiler-matrix/results.json`, `r2/full-contract/results.json`,
`r2/analysis/results.json` and `type-view-controls/results.json` retain their
separate scopes. Completed generated C is losslessly compressed. Duplicate
model gzip files refer to an exact retained copy with restoration metadata in
`deduplicated-models.json`; completed native controls have a separate lossless
compression journal. No installed compiler/runtime or canonical HLC changed.

## Typed alignment views, adopted v54

The adopted source is `typed-unaligned-views-v54/r4`. It remembers the
actual section where each C record was declared and emits an alignment-one
typedef beside that declaration when a view is needed. Member access keeps its
declared path and pointee constness. Original record declarations, public
parameter types and BASIC symbol identities remain unchanged. Local records,
pointer members and const record pointers pass the emitted-BASIC controls.

The first member-view prototype accepts both widths and C backends. Its 28
emissions preserve 20 earlier source-model hashes and four new before/after
model pairs; the native controls comprise 24 passes and four old traps. Three
Clang compiler-module samples improve beyond v52: `ast-node-bop` 254 to 197,
`parser-expr-variable` 98 to 96, and `symb-proc` 158 to 146 warnings. These are
bounded module observations, not a complete new warning total.

Further testing found a whole-record BYVAL load which still assumed natural
alignment. A direct alignment-one aggregate dereference passed GCC but trapped
with Clang. Revision r4 transfers aggregate values into an aligned C temporary
with byte copying, while preserving separate storage/address expressions for
BYREF use and assignments. It also covers nested aggregates and the two-word
32-bit C representation of INTEGER128.

The Clang LLVM trace exposed a second problem in `_emitMem`: passing a typed
record pointer directly to a byte-copy builtin attached a natural-alignment
check and an eight-byte LLVM alignment promise. Explicit byte-pointer arguments
produce `align 1` for the unaligned destination. Both copy and fill emission now
use byte views, preserving source pointee qualifiers. This changes the address
contract, not the diagnostic settings.

Latest r4 checks pass 24 unchanged source models and 24 fresh native fixture
executions across the two C backends and O0/O2. Separate whole-record and nested
aggregate controls pass another eight sanitizer executions. The nested control
exercises clearing, BYVAL copies, BYREF identity and stores. Nine scoped BASIC
files lint with zero errors, warnings or information messages. The emitted
wasm32 record test recompiles and links to exactly the earlier sanitizer-tested
module bytes; its Node execution proof is retained. This remains a fragment
test, not full FreeBASIC JS-runtime qualification.

The older member-only compiler graph stopped at the storage guard after 330
accepted stages. It remains an incomplete historical run. The completed r4 graph
qualifies all 189 current compiler sources under both C compilers, with matching
exported and imported global symbols. All retained BAS sources match the current
canonical preimage. Of 251 shared headers, two differed: `asm-names.bi` comments
and the current `c-identifiers.bi` name encoder factoring. Fresh HLC objects using
those exact current headers replace the older-header rows in `effective-graph.json`.

| Compiler-graph diagnostic | Canonical v52 | Adopted v54 |
| --- | ---: | ---: |
| GCC total warnings | 6,820 | 5,736 |
| Clang total warnings | 20,593 | 19,558 |

Both full compiler producers link and reproduce 80 exact C/source-model outputs
across ten fixtures, two widths and two C backends. Additional inputs cover the
whole-record and nested transfers, mutable/const copy overloads and allocator
initialization. The selected 26 project profiles pass 44 fresh C objects and 24
fresh native executions, with matching global symbols and four exact legacy
source models. These are selected project controls, not a complete corpus sweep.

`build_scripts/test-c-emitter-alignment.py` registers a reusable Win64 sanitizer
gate over five canonical BASIC fixtures. Each C backend runs O0/O2. The adopted
full GCC producer passes all 20 executions; the v52 full producer traps in all
20 corresponding negative controls. Both profiles retain their highest warning
settings. The five changed backend files and five fixtures have a strict
non-semantic fblint summary of ten files and zero findings. `source-adoption.json`
retains exact preimages; installed compiler/runtime providers are unchanged.

Clang core/unix/deadcode analysis of the latest pointer, whole-record and nested
fixtures reports zero findings. GCC's analyzer also completes all three, while
retaining an ordinary pointer/integer comparison warning in the pointer fixture.
That expression's C type bookkeeping is a follow-up repair. Shared descriptors,
empty dynamic copies, general typed-pointer formation and error-owner lifetimes
remain open, along with the remaining warning families and wider corpus checks.
The v54 byte-copy path also exposed six implicit pointer-qualifier discards per
C compiler in `hash.bas` and `symb.bas`: a `char*` temporary receives a
`const char*` initializer. The v55 repair below resolves that specific family.

Evidence is under `E:/fbc-c-audit-01a166ce/typed-unaligned-views-v54/`. The first
`qualification` retains the member controls, `representatives` the three module
samples and `wasm-alignment` the original wasm negative/positive controls.
`r2`/`r3` preserve rejected aggregate attempts; `r4/qualification`,
`r4/aggregate-probe`, `r4/nested-probe`, `r4/wasm-current` and `r4/lint-result.json`
record the focused checks. `r4/effective-graph.json`, `r4/full-contract/results.json.gz`,
`r4/projects/results.json.gz`, `r4/canonical-native-gate/results.json`,
`r4/canonical-lint.json` and `r4/structural-analysis/results.json` record the final
adoption gates and their separate scopes.

Completed E: receipt JSON and superseded private compilers may be stored as
`.gz`. `E:/fbc-c-audit-01a166ce/receipt-storage-v54.json`,
`resource-stop-storage.json` and `storage-final-v54.json` in the v54 directory
record exact original and compressed hashes and restoration paths. The final
compaction checks the global cap before each compressed output is written.
Existing C: evidence remains in place. Canonical HLC includes v55; no compiler
or runtime has been installed. Completed r4 executables also have exact gzip
restoration journals; the full GCC producer remains native for subsequent work.

## Pointer-field qualifiers, v55

`hEmitType()` omits qualifiers unless its caller requests them. Byte-copy
load/store temporaries used that default even though their IR value and emitted
initializer retained pointed-to constness. Their declarations now request the
full value type and remove only const on the local binding. This avoids an
implicit `const char*` to `char*` conversion without adding a cast or changing
the transferred representation.

The member matcher also used `typeGetDtOnly()` to reject raw fixed strings.
That discarded pointer depth and excluded CHAR/WCHAR pointers too. It now tests
`typeGetDtAndPtrOnly()`: scalar text pointers recover their declared field paths;
fixed character arrays keep the raw access path. This also recovers fixed-string
array descriptor `.DATA` and `.PTR` writes. Their bias arithmetic and ABI are
unchanged and remain separate open work.

| Compiler-graph diagnostic | v54 | Adopted v55 |
| --- | ---: | ---: |
| GCC total warnings | 5,736 | 5,709 |
| Clang total warnings | 19,558 | 19,235 |
| Implicit pointer qualifier discards, either compiler | 6 | 0 |
| Explicit const casts, either compiler | 2,082 | 2,117 |
| Clang unsafe buffer usage | 4,473 | 4,177 |

The extra explicit casts remain visible. Preserving readonly pointer values
reveals conversions previously obscured by unqualified temporaries. This repair
does not claim that those conversions or the remaining warning families are clean.

All 378 compiler graph stages pass with matching complete global symbol sets:
136 fresh objects, four retained representative objects and 238 exact-C reuse
proofs. A concurrent accepted lexer endpoint repair changed one canonical BAS
file. Two fresh objects from its exact canonical source, SHA-256
`533c4506a433940dd7fdebb88cef6afe46e1f0ae5edeb46770bbe6a135afeaad`,
replace the earlier lexer rows; `effective-graph.json` binds both cuts.
No other retained canonical source/shared header
changed, and the emitter patch preserves that lexer edit.

Both complete producers match 88 qualified C/source-model outputs across eleven
fixtures, two widths and two C backends. Eight existing fixture profiles changed
because descriptor writes now use field names; their 16 fresh ordinary objects
and eight native O0/O2 runs preserve symbols and stdout. The selected 26 project
profiles qualify eight fresh fbfrog objects and 36 exact-C retained objects,
with four matching legacy models and 24 retained native runs.

`tests/quirk/c-alignment-pointer-fields.bas` covers ordinary and packed text
pointers, a second pointer level, null stores, readonly access, fixed narrow/wide
strings, store sentinels and one address evaluation. Four private before/after
source-model pairs match. The prior emitter fails eight qualifier-error object
checks; the new private emitter passes four native alignment-sanitizer runs.
The registered Win64 gate now makes implicit qualifier discards errors for its
after compiler. Invoking it on the prior emitter confirms the expected rejection;
the adopted canonical case passes all four native executions. Three canonical
BASIC files lint strictly without semantic analysis, with zero findings.

Both emitted Win64 variants of the new fixture also pass Clang core/unix/deadcode
and GCC analyzer checks without diagnostics. These analyses and sanitizer flags
cover the emitted C; the pinned runtime archive was not instrumented or analyzed.
The wider corpus, other target runtimes and the three historical analyzer timeouts
remain outside this bounded proof.

Evidence is under `E:/fbc-c-audit-01a166ce/char-pointer-fields-v55/`:
`qualification/results.json`, `effective-graph.json`, `effective-warning-counts.json`,
`full-contract/results.json`, `projects/results.json`, `canonical-gate-verification.json`,
`canonical-lint.json`, `structural-analysis/results.json` and `source-adoption.json`.
The earlier producer attempt is retained; successful unchanged emissions were
reused when the descriptor cases needed new qualification. The lexer reconciliation
records its source owner, exact snapshot and rollback-preimage identity.

Changed graph C is stored as verified line-copy deltas over pinned v54 gzip C;
`c-delta.py` reconstructs exact bytes and checks their SHA-256. Replaced owned
objects and completed private providers have separate gzip restoration journals.
Existing C: evidence and installed providers remain unchanged. The overall goal
remains unfinished.

<!-- end of c-emitter-alignment.md -->
