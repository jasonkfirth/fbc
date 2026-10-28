<!--
Project: FreeBASIC compiler tests
File: README.md
Purpose: Describe the semantic sidecar's contract and regression tests.
Responsibilities: Explain execution, fixtures, and positive/negative coverage.
This file intentionally does NOT contain the sidecar implementation.
-->

# Semantic sidecar tests

Run `make compiler-semantic-model-test` from the repository root. To use an
already built compiler:

```sh
python3 build_scripts/test-compiler-semantic-model.py --fbc ./bin/fbc
```

The default emission checks use GCC, Clang, and LLVM. These checks stop at
compiler emission and do not require the corresponding external compiler.
The behavior comparison builds and runs a small native program with the
normal C toolchain. `--backend gas64` selects native assembly emission on a
supported host. Repeat `--backend` to select several backends. `--test` selects
an individual `test_...` method.

The compiler defaults to the self-describing binary transport. Existing row
mutation fixtures select `-semantic-model-text` explicitly. Run the matching
producer/consumer transport and incremental-cache checks with:

```sh
python3 tests/semantic-sidecar/test_binary_transport.py --fbc ./bin/fbc --linter /path/to/fb_linter
```

`array_mutation_inputs.py` checks direct ERASE/REDIM identities and counted
module closure. It covers fixed/dynamic variables, preservation, expanded uses,
excluded fields, zero and multiple modules. GCC, Clang and LLVM emission remains
byte-identical with capture off, full text, binary and reduced/compact modes.
These tests do not classify collection usefulness or execute their subjects.

`macro_access_inputs.py` checks surviving variable/member access tags under
`macro-source-access-node-bindings`. It preserves exact selected MR identities,
separate source/node counts and unlinked generated cleanup. Its GCC, Clang and
LLVM controls compare unchanged emitted code, exact decoded text/binary rows,
and omitted observations in none, reduced and compact captures. Reduced-capture
controls inspect this family's emitted rows; they do not certify unrelated
compact assignment-location contracts.

`macro_call_inputs.py` checks `source-call-macro-references` (capability 118).
Full models with macro provenance retain the exact original `MR` reference
identity on surviving direct, virtual and source-selected runtime CALL nodes.
Enabled `ASSERT` and `ASSERTWARN` macros select runtime procedures through the
ordinary source-call parser. Runtime links also require a runtime-origin target;
generated helpers without an original MR reference remain unlinked. The immutable node
property `source-call-macro-reference` selects that reference; the selected
procedure must agree with the call target and its original macro invocation.
Physical calls continue to use their `H` source-binding relation. These are
distinct identity domains and never share a binding ordinal.

Each available module has one namespace-owned
`source-call-macro-count:<module-id>` closure, including zero. Counts describe
surviving CALL associations, so cloned nodes can refer to the same original
reference. This family does not assert complete implicit-call coverage.
Reduced models and compact macro provenance mark the capability unavailable
and omit its observations. The focused tests compare unchanged GCC, Clang and
LLVM output, exact text/binary rows and malformed-reference refusal.

This gate compares exact decoded facts, checks native codec rollback and bounds,
rejects malformed headers, and exercises source/include invalidation and concurrent
cache reuse, private C-stream ownership and cache lease release after a crash.
`C:/fblint/test_semantic_io.bas` is also a standalone native storage regression
for Windows, POSIX and DOS builds. `semantic_binary.read_rows()` streams either transport for dependency
receipts without retaining a second expanded model. Frozen graph readers must
include `semantic_binary.py` with their other imported modules.

Use `--toolchain-prefix <SDK>` when the source checkout has no native tools or
libraries. Source-tree declarations still take precedence through `-i inc`;
the SDK supplies executable tools and link libraries without another copy.

Under MSYS/Cygwin Python, the reader uses the bundled `cygpath` to access
native Windows filenames recorded by the compiler. Exported filenames retain
their original spelling; source hashes, byte limits and stale-source checks
apply to the resolved filesystem path before an edit plan is returned.

`pointer_access_origins.py` verifies independent dereference and pointer-index
origins before AST lowering. Its reviewed inputs include multiple dereferences,
fields, explicit index conversion, canceled addresses, macros and unevaluated
queries. Full and compact modes preserve emitted output and diagnostics.
Malformed origin/group tests keep record totals unchanged. Earlier producers
remain readable, while foreign module observations are rejected. This contract
describes the original inputs; it does not claim that every access executes.

`array_initializer_inputs.py` verifies original array elements before assignment
conversion and implicit constructor lowering. Its 18 reviewed inputs include
multidimensional and inferred bounds, array fields, static storage and grouped
and generated expressions. Scalar initializers and optional defaults stay
outside this contract. The `original-array-initializer-inputs` capability is
available only in full models. Complete expression property groups identify the
source array, its one-based innermost dimension, and the accepted assignment or
constructor path. Rejection tests cover incomplete groups, invalid targets and
dimensions and unavailable capabilities. Backend output and diagnostics remain
identical when the model is disabled.

The optional `initializers` request adds counted closure over those original
elements without changing the existing facts. Request controls cover each of
the eleven groups alone, every pair, all-but-one sets and all groups together;
default, repeated, reset, invalid and reduced-mode requests remain separate
checks. This bounds compiler launches as the optional surface grows. Every
successful request must preserve generated C, and the initializer witness must
retain its exact element count and target edge. The `type-shapes` and
`native-bytes` groups independently request written type roots and selected
pack/unpack operations. The request fixture has a native MKL call; checks require
its exact count and edge only when requested. The maintained type observation
regression requests both groups explicitly and retains reduced-mode exclusions.

`sidecar.py` independently validates schema 27. `test_sidecar.py` checks the
compiler's records against source locations and known types, layouts, values,
and target relationships. The BASIC fixtures own separate responsibilities:

`call_atoms.py` checks original constant and bound expression identities,
direct and virtual source call bindings, compact exclusions and unchanged
emission. Its malformed groups include missing pairs, wrong symbol classes,
unavailable capabilities, virtual signature mismatches and foreign modules.

`assignment_inputs.py` also checks virtual LET and compound operator selection.
Assignment tickets and written-operation observations name the statically
selected source operator, matching the lowered CALL's `static-target` relation.
The generated dispatch variable is not an operator identity, and this does not
predict which runtime override executes. Macro tickets retain their selected
targets even when the older O stream lacks a separate physical operator range.
The virtual regression covers remapping, same-line assignments, direct-call
fallback and identical binary/text facts across GCC, Clang and LLVM emission.

`function_result_inputs.py` checks original RETURN and function-name assignment
inputs and accepted numeric destination types. Enum inputs retain their own
types before Boolean normalization. BYREF results, pointers, enum destinations,
strings and aggregates do not acquire numeric assignment properties. Hidden
ABI result parameters never become named source destinations. Full and compact
observations preserve backend emission and compiler diagnostics.
`numeric-function-result-inputs` distinguishes this coverage from older
producers that only retained ordinary numeric assignment destinations.

`if_conditions.py` checks original IF and ELSEIF predicates, including folded
constants, nested and single-line forms, legacy GOTO grammar, macros, includes,
remapped source and overloaded conversions. `semantic_if.py` validates complete
accepted-header coverage independently of expression spans. Missing property and
marker pairs, wrong owners and cross-module inputs are rejected. Full and compact
observations must leave the generated backend output unchanged.

`enum_inputs.py` checks actual enum membership, explicit initializer presence
and original constant expressions. It covers namespace placement inside
Extern, ordinary CONST aliases, anonymous and nested enums, comma-separated
members, includes, macros, inactive branches and the deprecated/fblite
dialects. Full and compact models preserve unchanged backend emission.
`semantic_enums.py` validates native counts, complete ordinals, statement
roles, expression ownership and module closure. Damaged-group checks include
oversized counts, unavailable capabilities and foreign initializer modules.

The extensible `K symbol <id> written-override <0|1>` property records whether
the procedure parser consumed the contextual `OVERRIDE` marker inside a TYPE
declaration, after macro expansion. Both presence and absence are exported in
full models, independently of the resolved base-method identity in `F`.
Constructors, generated members and body headers do not provide this receipt;
bindings-only and expressions-only models omit it. Older producers may omit
the property, which means unknown, not a missing keyword. No schema layout,
procedure attribute, generated instruction or compilation diagnostic changes.

Full models also advertise `CAP <module> formal-parameter-spans available`.
The parameter parser emits `K symbol <id> formal-span-kind physical|generated`
and a separate `LOC declaration <occurrence> formal ...` for complete physical
formals, preserving the existing declaration-name `range` locations. The span
includes mode, type, descriptor and default-expression tokens. Unnamed and
variadic formals have explicit nameless declaration occurrences; the implicit
method receiver has no fabricated written span. Macro-expanded formals remain
generated, with at most a `formal-generated` first-token observation rather
than an editable full extent. Compact models mark this capability unavailable.
Anonymous callback interning may replace the type parameter while the written
declaration still belongs to its original parameter symbol. Readers validate
that original symbol and occurrence, not the canonical parameter's name.
These optional records extend schema 27 without changing its layout.

`semantic_flow.py` validates recorded procedure phases, evaluation edges, block
membership and conservative transfers. Declaration-owned default/variable
initializer trees remain unphased; they must not become actions in a procedure
body merely because local symbol details are exported beside that body. The
tests reject foreign procedure owners rather than guessing a replacement edge.

| Fixture | Coverage |
| --- | --- |
| `bindings.bas` | declarations, qualified lookup, USING, scopes, WITH, labels, assembly |
| `types.bas` | packed/union/bitfield layouts, inheritance, visibility, arrays, qualifiers, literals |
| `procedures.bas` | overloads, canonical formals, defaults, variadics, by-reference results, pointer calls |
| `parameter-spans.bas` | complete physical formals, unchanged names, unnamed/variadic declarations, nested callbacks, defaults, implicit receivers and macro exclusions |
| `lifetimes.bas` | each implicit construction/destruction relationship and nonphysical omissions |
| `control-flow.bas` | conversions, calls, branches, normalized jump tables, and memory access |
| `constructs.bas` | all compound families, nested/inline/colon statements, members, labels, assembly |

Generated cases cover the operator vocabulary, constant folding, repeated
includes, all five BOM encodings, long physical lines, language restarts,
multi-module transactions, error recovery, interruption, dependency limits,
and recycled symbol pools. Negative assertions check that unrelated identifiers,
inactive code, unresolved assembler names, unknown values, runtime array bounds,
and generated callee spellings never become fabricated facts. Mutated sidecars
exercise rejection of bad shapes, references, flags, ranges, and totals.

Conditional tests cover all seven directive kinds, selection independently of
evaluation, successful/missing defined-name queries, precise inactive spans,
deep inactive nesting, repeated includes, remaps, and module resets. Stateful
callbacks produce identical emissions with export off, full, bindings, and
expressions modes. Inactive bodies retain no invented declarations, queries,
include attempts, or overload targets. Generated narrow/wide directives remain
noneditable; invalid and unfinished recovery conditions remain provisional.

Macro tests check narrow and wide definition snapshots, nested argument and
replacement parents, parameter/stringify/token-paste pieces, actual callback
returns, empty replacements, name-only uses, optional parentheses and restored
caller delimiters. Retired definitions remain immutable. Consumed-token links
cover expanded operators between physical operands and generated declaration
symbols without inventing editor ranges. Corrupted parents, definitions,
formals, substitutions, output lengths, origins, and closure are rejected.
Stateful callback and repeated argument-storage fixtures verify execution and
emission parity with export disabled and in all three output modes.

Physical coordinate tests independently verify byte spans and UTF-16 columns
for unmarked UTF-8, UTF-8/16/32 BOMs in both byte orders, CR/LF/CRLF, and non-BMP
text. Repeated includes retain separate source occurrences; repeated logical
remaps retain original physical positions and unchanged noneditable policy.
Generated and malformed locations do not acquire fabricated byte mappings.
The reader rejects forged subjects, revisions, offsets, coordinate pairs, and
stale current files before source projection.

Construct tests cover all fourteen compound kinds and all accepted statement
dispatch routes. They check colon boundaries, continuations, single-line IF
parents, multiple FOR closures in one NEXT, aggregate/enum bodies, procedure
ownership, repeated includes, module resets, numeric labels, generated/inactive
omissions, and provisional recovery. Fact ownership and physical projections
remain separate: a generated AST node does not acquire an editable range from
its creating statement. Corrupted parents, configurations, source occurrences,
owners, and missing closures are rejected, including corrected-total models.
GCC, LLVM, and Clang emissions and diagnostics match export-disabled controls.

The callback regression checks canonical replacements for reused anonymous
procedure headers and their formal parameters. It distinguishes different
default arguments and preserves BYVAL, BYREF, array, and variadic signatures.
The LLVM literal regression checks decoded wide characters and zero padding
in all three export modes. On glibc it poisons fresh allocations to expose
reads beyond initialized text. It also checks a static wide-string initializer.
`make compiler-semantic-self-test` applies the export checks to the native
compiler modules selected by Make; `make compiler-semantic-corpus-test` applies
them to the maintained OMA programs.

Corpus regressions check thousands of sibling procedure prototypes, metadata
for temporary scopes used by default UDT arguments, and source columns after
legacy bytes or malformed UTF-8. Valid UTF-8 still uses UTF-16 editor columns.
`make compiler-semantic-projects-test` captures GCC build controls for registered
external projects before comparing disabled, full, and compact export. Supply
its corpus/compiler/output arguments through `COMPILER_SEMANTIC_PROJECTS_FLAGS`,
or replay an existing `--snapshot`; see the
[external corpus audit](../../docs/fb-corpus-semantic-audit.md).

The sufficiency fixture distinguishes compiler-selected formals from omitted
default arguments and source-typed calls from calls retained after constant
short-circuit folding. Its regression preserves the existing facts without
claiming missing source/effect relationships. The
[source-to-export review](../../docs/fb-corpus-semantic-content-review.md)
records concrete questions, supporting records, and remaining information gaps
from working corpus programs.

All compiler outputs and dependency-limit fixtures use private temporary
directories. Failed tests return a failing process status and report the
fixture, backend, or invalid contract.

Publication tests preserve inputs under source/include/preinclude, hardlink,
and symlink aliases. They reject generated-code and executable collisions,
verify interrupted output stays unpublished, and replace an ordinary existing
model successfully. The BASIC writer fixture injects partial writes, flush,
close, and replacement failures and checks old output, stream ownership, and
staging cleanup. The BASIC revision fixture compares SHA-256 against Python's
independent hash at padding and read boundaries, preserves the open stream's
position, and detects changed source bytes.

Optional schema-27 source-use capabilities preserve selected fields and procedures
when a macro-generated name has no physical reference-token range. Full models
publish `macro-reference-origins` and MR `reference-<detail-id>` or
`construction-<detail-id>` roles against the selected symbol and real expansion.
These facts identify the use without inventing an editable callee token; compact
models and disabled provenance do not claim this capability.
Full compact models retain only the in-memory invocation locations needed to
+anchor generated typed expressions. They do not serialize macro graph records.

`implicit-call-coordinates` adds K facts alongside the unchanged I columns.
`implicit-call-coordinate-<I-ordinal>` contains eight escaped tab-separated fields:
source occurrence, physical start line/UTF-16 column, physical end line/column,
start byte, end byte and mapping status. `implicit-call-origin-<I-ordinal>` carries
the expansion identity. Ordinals count published I rows across the model, rather
than restarting per procedure. Generated construction ranges remain unverified;
their expansion receipt supplies a separate physical invocation anchor.
The coordinate helper is shared with LOC emission, preserving its existing
byte format and source mapping. Tests cover nested/repeated macro references,
selected overloads, compact capability boundaries, source constructors, Unicode,
five BOM encodings and physical positions under logical `#Line` remapping.

The optional full-model `procedure-linkage` capability accompanies
`K symbol <identity> procedure-linkage <value>` snapshots. Values are `basic`,
`c`, `windows`, `windows-ms`, `c++`, `pascal` and `rtlib`, taken from the accepted
symbol's declaration mangling. F's calling convention remains separate: an
ordinary BASIC CDECL procedure or explicit alias is not an EXTERN "C" contract.
A foreign prototype retains its linkage when its later body is written outside
EXTERN. Bindings-only and expressions-only exports mark this capability unavailable
and do not emit the K property. The independent fixture covers prototypes, later
bodies, aliases and fixed signatures on GCC, GAS64 and LLVM. No existing field,
signature, runtime ABI or language behavior is changed.

`field-groups.bas` verifies optional finalized `declared-field-count` and
explicit `field-array-rank` symbol properties. Multi-name declarations, fixed
and dynamic arrays, callback fields, nested types and promoted anonymous
members participate through their actual FIELD owners. Inherited members,
static variables, methods, hidden base storage and array descriptors do not
inflate source counts. The independent reader rejects inconsistent receipts
but accepts older models without them. Full, bindings-only, expressions-only
and disabled exports produce byte-identical ordinary output on each selected
backend; only full models add the field properties.

The direct-write tests verify optional `for-counter-writes` and
`direct-source-writes` capabilities plus explicit zero/one variable flags.
They preserve counter initialization independently of generated loop reads,
native input/SWAP/string destinations independently of runtime BYREF lowering,
and generated identifiers without invented binding ranges. Pointer, field,
shadowed-local and opaque-call controls stay distinct. BYREF FOR counters
remain rejected by the original grammar. Scalar/string/UDT parameter slots
and off/full/compact emission are checked across the selected backends.

Full output also records parser-selected FOR variable identities with a
separate statement-specific generic K property for each loop. Tests separate
existing counters from scoped FOR AS declarations, preserve repeated and
macro-generated loops, and require counter locations after STE. They compare
ordinary emission with export disabled, full and compact across the chosen
backends. No new record tag, AST code or accepted grammar is introduced.

The parsed numeric suffix tests retain native spelling and physical coordinates
across all six encodings and #LINE remapping. They exclude discarded/stringified
macro arguments, inactive macro branches and assembler operands inside mixed
macro bodies, while retaining parsed neighboring literals. Full and compact
export modes are compared with disabled export for unchanged emitted code.
Parser receipts reuse existing K, LOC source-context and MR symbol domains.

`build_scripts/test-compiler-loop-conditions.py` verifies original WHILE and
DO/LOOP predicates, including unconditional statements, macros and continuations.
It checks six encodings on Win32, Win64 and Linux x64, unchanged emitted C,
compact capability exclusion and malformed group rejection by both readers.

`build_scripts/test-compiler-array-subscripts.py` exercises original and selected
integer array indices before offset lowering. Its fixed/dynamic arrays, fields,
formals, floating conversion and assignment targets run through the same target
and encoding matrix. Missing, conflicting, foreign and incorrectly typed index
groups are rejected independently without relying on invalid footer totals.

`build_scripts/test-compiler-array-bound-queries.py` checks folded and runtime bound queries, original and converted dimensions, fields, formals and macros. It verifies three targets, six encodings, unchanged generated C, compact exclusion and malformed groups rejected by both readers.

The Windows linker tests use the runtime built by the current CI row. An installed
SDK also exercises its additional x86 runtime when the matching tools and library
are present. Clang and llvm-nm can come from that SDK or the configured toolchain.
The ARM64 row checks native C objects and LLD's explicit unavailable callback
coverage; GNU ld rows require its advertised callback protocol and exact symbols.
LLVM object checks retain their independent Windows, Linux and macOS target matrix.

`header_policy_inputs.py` checks accepted per-source pragma-once, library and
namespace-import receipts across GCC, Clang and LLVM. Literal and macro-generated
library values preserve escapes and origin ownership. Full and compact models
leave emission unchanged, and damaged receipt cases must be rejected.
The reader also requires complete occurrence counts and same-module ownership.
Recipient facts have their own capability; older producers that only retained
operation counts remain readable. Tests distinguish global imports inside a
procedure from imports into a named namespace.

`namespace_inputs.py` checks parser-selected named and anonymous namespace
openings, including qualified, reopened, aliased and macro-generated forms.
It compares disabled, full and compact emission across GCC, Clang, LLVM and gas64, rejects
damaged identities and ownership, and accepts older producers without this
capability.

`numeric_macro_payloads.py` checks original decimal numeric observations carried
alongside canonical macro replacement buffers. With `macro-numeric-payloads`
available, each original floating literal also requires
`original-float-provenance`: an escaped three-field value containing its kind,
macro attempt identity and replacement-unit offset. `source` uses zero identity
and offset; `transported` retains a captured definition or argument payload;
`generated` describes newly re-lexed output without that source payload. Macro
identities and offsets must agree with the expression's existing `MR` start and
`macro-input-unit` receipts. A missing receipt cannot silently become source text.

Payloads retain the existing 1024-character observation bound and honest
completeness flag. Physical adjacency allows PP's split leading-dot tokens to
share an original payload, while token pastes retain generated provenance.
Character lookahead counts toward logical replacement positions, independently
of physical coordinates. These observations do not change numeric parsing,
canonical clipping, suffix selection, warnings or emitted code. Consumers must
keep the original spelling separate from the current typed constant value.

The focused tests cover definition/argument storage, nested aliases, repeated
parameters, trimming, leading dots, omitted exponents, undef/redefinition, six
encodings, compact exclusion, incomplete payloads, growing storage and module
reset. Damaged provenance and capabilities are rejected independently. Older
models without the new capability retain their original six-field contract.

`parsed_expression_coverage.py` checks statement-owned `parsed-operand-count`,
`parsed-expression-inputs`, and `parsed-iif-count` receipts. The independent
reader matches the operation count to owned EX rows and the IIf count to parsed
IIf H groups. It rejects missing, duplicated, mismatched or foreign-owner
receipts, and accepts explicit unavailability and legacy models without the
new capabilities. This keeps incomplete parser inputs from appearing complete
just because some expression rows were emitted.

Both readers validate each native CALL's `argument-count` against its right-edge
ARG chain. Hidden receivers and omitted/default arguments count as actual ARG
nodes; an ARG's left subtree is its operand and can contain a separate nested
CALL. Non-ARG chain children and ARG nodes outside a CALL/ARG right-edge chain
are malformed facts. Validation is linear in the observed AST and does not
infer source order, formal arity or external-call behavior. Existing producers
already export these counts and relationships; no new capability is needed.
`test_native_call_argument_counts_and_chains` covers zero/fixed/variadic calls,
defaults, nested calls, 201 arguments and damaged counts or chain ownership.

`redim_construction_inputs.py` checks the exact array-runtime constructor
selection for source REDIM, runtime declarations, STATIC storage, PRESERVE,
empty bounds, fields, BYDESC parameters, macros and remapped same-line sites.
Scalar, destructor-only and generated-field operations are negative controls.
GCC, Clang and LLVM preserve full/off/compact emission and exact binary/text
facts. Recounted damage controls check ownership, selection, statement/source,
macro origins, ordinals and closure, including loss hidden by a reduced count.
The tests compile these sources without executing their constructors.

`virtual_call_inputs.py` checks virtual selections independently of written
source bindings. A generated LET CALL still requires its static source target.
Recounted models damage unbound/bound targets, duplicates, procedure identity,
node ownership and dispatch kind. The direct CALL control keeps the original
virtual selection, so an extra static-target relation cannot hide behind an
unrelated missing relation. No method or override is executed by these tests.

`driver_inputs.py` checks actual module roles, main selection, restarts, search
order, predefines, preincludes, requested libraries, and explicit versus planned
objects. Reduced models omit the family; malformed groups and older producers
exercise rejection and compatibility separately. Native archive and XPM cases
cover generated modules, noneditable generated source and preservation of the
original resource under a rejected sidecar alias. Export retains unchanged
GCC, LLVM and Clang output.

`parser_observations.py` checks the native parser phase across full and reduced
models, independently of full-only input/selected snapshots. Ordered closures,
actual AST/type fields and independently computed SHA-256 value fingerprints
are validated. Corrupt digests, missing events, mismatched results and false
capability claims are rejected with corrected transport totals. Reduced models
retain no constant value payloads. Macro expression tests cover ordinary
operands on both sides of expansions and hidden provenance without changing
backend output.

`receiver_contexts.py` checks field receiver observations inside preprocessing
TYPEOF/SIZEOF queries and ordinary statements, including zero statement owners.
Independent context markers, nominal types and selected-field relationships
are checked; forged contexts and missing markers are rejected. The case also
requires unchanged backend emission and explicit reduced-mode omissions.

`data_streams.py` checks ordinary and QB DATA values, exact NUL/wide payloads,
address offsets, source order, actual forward descriptor links, synthetic empty
descriptors, forward-label and blank RESTORE selections, and READ destinations.
Inactive and raw QB text cannot invent source operations. Empty QB fields have
no fabricated range. Module resets, binary/text parity, reduced-mode omissions
and unchanged GCC/Clang/LLVM emissions are required. Recounted corrupt models
exercise missing items/payloads/runtime links, reversed chains, wrong targets,
foreign sources, excessive counts and false capability claims.

`emission_names.py` checks actual C/LLVM/assembler identifiers separately from
object symbols, including aliases, C++ linkage, builtin mappings and explicit
unknown states for private symbols, local statics and backend adaptation.
Unused prototypes remain unobserved; procedure-pointer types have no object
names. Native objects must be identical with export off and on, and selected
object names must agree with `nm`. Windows x86, Darwin x86-64 and ELF x86 cases
check prefixes and calling-convention decoration; LLVM tools additionally
validate COFF and Mach-O objects when available. Transport parity, module
resets, reduced omissions and recount-corrected corruptions are covered.

`array_forward_types.py` checks array element snapshots before and after
forward type and alias resolution. Each access uses the live `T` snapshot
written at that access, while the original `S` remains immutable. A forged
same-module nominal type is rejected, and backend emission remains identical.

`namespace_visibility.py` checks source USING order and the native import
relationships created by each request, including transitive imports, duplicate
and parent/self no-ops, namespace reopening and propagation. Procedure/block
imports retain lexical lifetime owners separately from namespace destinations.
After removal, the same written identifier binds to its actual outer variable.
Repeated includes, expanded/inactive operands, exact source ranges, module
resets, transport parity, reduced omissions and diagnostics parity are covered.
Binding contexts cover lexical tables, lookup namespaces/hash owners, scoped
enums, anonymous aggregates, out-of-line methods and reopened occurrences.
Recounted corruptions exercise false owners/targets, invented request results,
missing links/closure, forged lifetime states and incomplete binding contexts.

`named_flags.py` checks every bit, zero and all bits across all native symbol
classes, plus invalid classes, groups and dictionary bounds. The executable
`flag-vocabulary.bas` links the BASIC mapper directly. Independent controls
distinguish reused status bits and jump-table storage from procedure status.
Real symbol snapshots, all nine const positions on eight-level pointers,
address temporaries, fixed-string expression/node widths, nominal identities,
32/64-bit targets, WCHAR policy and QB promotions are checked. Full and reduced
GCC/Clang/LLVM output must match export-disabled output. Binary/text equality,
module reset and recount-corrected dictionary, payload and closure corruption
are covered.
`semantic_normalized_types.py` pairs NT with each live T/N/E snapshot. Its
controls remove or contradict valid native observations while keeping their
transport totals consistent, and retain the older no-closure contract.
Type controls also distinguish aliases from separate nominal types, pointer
signatures and variadic markers, Z/WSTRING storage, and actual C mangling
classifiers from the datatype slots used to encode them.

`layout_observations.py` checks original FIELD requests, coercion, inheritance
and QB defaults separately from native alignment decisions. The fixture covers
managed strings, dynamic descriptor fields and logical aliases, fixed arrays,
GCC/MS bitfield containers, hidden base storage, repeated anonymous relocation,
forward types and incomplete self-type queries. Linux/Windows 32/64-bit metadata,
all three high-level backends, exact off/full/reduced output, native execution,
stateful macro callbacks, repeated includes, transport and module reset are
covered. Corrupt packets and recount-corrected missing fields remain invalid.
Architecture controls additionally cover m68k word alignment, RISC OS double
alignment, Windows CE's alignment ceiling and AArch64 metadata. The reader
distinguishes pointer storage width from pointer/descriptor alignment. This
driver's unsupported JS target produces diagnostics without a complete model;
the separate JS compiler's QB exception remains a separate acceptance case.

`frontend_abi.py` checks source modes against live frontend ABI types,
nontrivial BYVAL indirection, BYREF qualifiers, descriptor pointers, variadic
markers, instance formals and result kinds. C/Clang/LLVM and native assembler
results remain distinct frontend policies. Target slot widths, transport,
module reset, reduced omissions and corrected-total corruption are covered.

`backend_abi.py` checks selected C/LLVM signature components against actual
header text, including i386 integer128 word pairs/sret, large aggregate byval,
va_list decay, C argv/callback/libc adaptations, PASCAL order and function-pointer
types. It checks explicit reduced/assembler omissions, unused prototypes,
transport/reset and damaged component or signature closure.

`call_abi.py` checks selected C/LLVM call arguments, actual variadic promotions,
hidden results, split integer128 values, aggregate memory arguments, va_list
decay, PASCAL order, source context and indirect/virtual target roles. It also
checks LLVM's separate builtin lowering receipts, exact call text, transport,
module reset, reduced/assembler omissions and damaged observations. Standalone
production-observer controls cover rejected state and copied context lifetime.
Native execution checks all export modes with working scalar/aggregate,
BYREF/BYDESC/wide-string, optional and virtual operations.

`raw_abi_consumer.py` exercises the complete reader with both native and
stable-v1 encodings across fourteen fixture families, three backends, export
modes and 32/64-bit targets. It compares normalized meaning and emitted code,
proves raw rows retain their selected encoding, and rejects mismatched type
vocabularies, encoding capabilities and semantic families. The written-type
observer tests also pass stable models through the complete reader.

`object_model.py` checks actual vtable slots and inherited payload origins,
abstract declarations, concrete overrides, empty/plain types, virtual
destructors and deferred/activated RTTI. Target and optimization partitions
compare exact emission; namespace controls keep equal short names distinct.
Runtime checks exercise virtual dispatch and IS across export modes. Source
snapshot, table/member, RTTI and closure corruptions include recounted removals.
`object-model-guards.bas` links the production observer to checked boundaries
for invalid classes/initializers, readonly capture, rollback and inactive hooks.

`startup_lifecycle.py` checks constructor/destructor registration and accepted
priorities, forward global initialization, reverse global cleanup, generated
wrappers and one-time static paths. Dynamic array allocation keeps its own
guarded path. Target/backend cases retain actual encoded priorities and NetBSD
or shared-library placement. Runtime controls compare all export modes; legacy
controls check native declaration counters and GOSUB status. Transport/reset,
unavailable modes, damaged ownership/order and production guards are covered.

`native_blocks.py` exercises native procedure/scope wrappers, original scope
exits, surviving branch links and selected cleanup order. Nested returns and
loop exits retain actual targets. Plain storage has no invented cleanup.
Runtime controls compare destruction/return behavior across export modes;
corruption controls reject invalid parents, targets, intervals, cleanup order
and recounted missing cleanup. `native-block-guards.bas` exercises the production
observer with invalid inputs, rollback, reset and disabled export.
Provenance controls retain each block's construction statement/source/configuration,
distinguish repeated includes and link inserted cleanup roots to their original
exiting edges. Corrected-count removals contradict the separate block snapshots
or actual cleanup insertion choices. The production guard also checks metadata
copying and clearing when an AST node is reused.

`lexical_lifetimes.py` checks normal reverse-order scope/procedure teardown,
empty groups, selected managed objects/strings/arrays and borrowed/static/plain
storage exclusions. Tests compare emitted code and runtime destruction across
export modes, both raw type encodings and binary/text transport. Corruptions
include recounted missing groups/actions/roots. The production observer guard
checks bad classes, bounds, inserted/elided roots, copying, reset and rollback.

`temporary_lifetimes.py` exercises native temporary registrations, reference and
cookie transitions, cancellation, recycled cookies, selected flush actions and
IIf branch ownership. Runtime controls check selected-arm behavior across all
export modes. Transport/reset controls cover both raw datatype encodings;
negative controls reject impossible transitions and corrected-count omissions.
`temporary-lifetime-guards.bas` drives the production observer through reference
release, cookie reuse, empty/branch flushes, invalid inputs and rollback.
Projection controls associate retained cleanup nodes with their selected native
flush actions, close zero/multiple projections, and reject missing links or
swapped IIf branch ownership against the actual EV arm trees.

`load_actions.py` checks actual recursive loading intervals, snapshot/inherited
origins, emitted runtime calls and conditional check-handler transfers. Unicode
scalar reads exercise a call created after the pre-load snapshot. Tests retain
unknown indirect handlers, omit checks when disabled, compare exact emission
and runtime behavior, and reject malformed or recounted missing intervals.
`load-action-guards.bas` drives the production observer through nested origins,
late callee metadata, invalid classes/roles, rollback and inactive hooks.

`load_payloads.py` checks actual active union payloads and typed call arguments,
both native/stable datatype encodings, sparse jump tables and assembly tokens.
Assembly controls distinguish entry tokens from the actual emitted stream
after native C/LLVM line joining. Corrected-count omissions, foreign targets,
wrong class unions, malformed arguments and emitted-token ownership are rejected.


`accepted_declarations.py` checks shared acceptance order across published source
bindings and DCL receipts. It covers procedure/formal/type/field/namespace and
variable routes, repeated legacy declarations, macro-generated names and unnamed
formals, unchanged native emission, zero counts, modes, binary/text parity and
module reset. Mutations reject wrong symbols/classes/provenance, swapped receipts
and omissions even when the declaration and transport counts are repaired.


The wide literal regression in `accepted_declarations.py` checks repeated live
symbol snapshots under C and GAS64, single publication of immutable target
prefixes, reader rejection of duplicated prefix records, exact unchanged emitted
bytes and independent prefix publication across module resets.


`object_operations.py` checks all 21 native object-operation kinds across C,
LLVM and Clang, including C++ deleting wrappers, anonymous union ownership,
actual base storage fields, generated bodies, constructor chaining, result
assignment versus RETURN, excluded ANY/fake-array fields, and plain storage
without invented constructors. It compares emitted bytes, executes all four
export modes, checks both type ABIs and binary/text parity, and mutates native
choices, selected calls, roots, base relationships and repaired-count omissions.
The production guard executable checks nesting, shared/cloned roots, reset,
invalid inputs, disabled capture and discarded lexer probes.


`ir_flow.py` verifies actual IR block/action membership and transfers across
C, LLVM, Clang, GAS and GAS64. Fixtures cover nested and late check labels,
loops, switches, GOSUB/computed dispatch, error handlers and RESUME NEXT,
assembly, result returns, and runtime versus arbitrary aliased call targets.
Tests compare emitted bytes, run all four export modes, check both type ABIs
and binary/text parity, mutate edges/reachability/targets/repaired counts, and
exercise production graph guards. Runtime error recovery is compiled with
`-exx`, which supplies the resume state required by this compiler.


`effect_contracts.py` checks runtime-domain/effect provenance, allocation and
release, string mutation, I/O, sound/graphics, synchronization, error handling,
profiling and computation. Lookalike externals and indirect calls remain
unknown; cloneable checks remain effectful. It compares emitted code and
runtime behavior, checks ABI/transport/reset/omissions, mutates purity/aliases/
masks/counts, and links the production selector and serializer into guard tests.


Native intrinsic dispatch tests exercise statement/function, graphics, sound and
varargs grammar selections, folded and nested results, declined fallback routes,
rejected inputs, inactive/PP/probe omissions, lookalike names, native/stable ABI,
all three model modes and text/binary transport. The production collector guard
checks copied entry state, unchanged AST payload, impossible arguments, the
100,000-event bound and module reset without reparsing a source fixture.




The reported diagnostic families exercise native syntax, lookup, overload,
access, invalid layout, include, argument and error-limit reports; warning
suppression and promotion; model/diagnostic parity in each export mode; native
backend open failures; and invocation warnings after parser teardown. A
preinclude warning followed by a language restart proves discarded attempt
ownership. Native reporter and collector harnesses check nested frame order,
all active symbol kinds, copied byte strings, lexer lifetime, count and byte
bounds, previous-output preservation, and reset. Corruption controls remove
reports while repairing totals, change native counters and attempt outcomes,
and omit required contexts. Text/binary model parity and older diagnostic
transports are checked separately.


Native expression recovery tests cover missing operands at binary precedence
levels, invalid unary/binary operands, typed cast replacement, legitimate zero
and abstract-procedure pointers, module and restart reset, corrected-count
corruption, datatype ABI and binary parity, and byte-exact POSIX filenames.
The production observer harness checks pointer preservation, probes, disabled
capture, null roots, bounds and reset. The capability remains partial while
other native recovery routes are reviewed.


The broader recovery-family matrix ties rejected constant/parameter/type
specifications, packing and initializer failures, array bounds, calls and
byref results, graphics/memory/file/string/math intrinsics, and missing
conditions/loop/select operands to their actual native recovery routes.
Valid omitted OPEN lengths, FOR steps, constant IIF arms and abstract-method
null pointers provide controls against classifying ordinary constructors as
recovery. These route tests supplement the partial coverage declaration.


Shared token-skip tests exercise real parser calls and the exact production
helper body. Independent expectations cover nested parentheses/braces,
requested comma and comma-stop policy, statement/line/EOF boundaries, exact
fetch/skip counts, endpoint consumption, probe/off capture, structured source
ownership, repaired-count corruption and compact/binary transport. A skip
observation records native parser control flow; it does not invent a binding
or by itself declare an expression invalid.


Declaration recovery tests exercise explicit primitive/pointer fallback,
discarded constant types, fixed-string parameters, missing callback results,
cast/PEEK repair, generated variable/typedef/field/enum names and real anonymous
mock parameters. Legal named-enum quirk members remain a negative control.
Native/stable binary parity, module/restart boundaries, copied snapshot lifetime
and count/class/ownership corruption are checked independently.


Native source-validity tests prove parsed/recovered/parsed boundaries around
an invalid middle statement, unknown endings for unfinished source compounds,
report-only loci for missing includes, native counter/status agreement,
compact validity records, and corrected-total corruption rejection. Physical
coordinates remain source observations; rejected-module expressions stay
provisional even when surrounding syntax parsed successfully.

Native resolution tests exercise native screening and winner confirmation, nested
constructor/CAST searches, zero-argument and omitted defaults, descriptor rank and
mode, constant compatibility, property accessors, imports, ambiguity, no-match,
access denial and binary-operator eligibility. Precision selections keep unknown
scores explicit. Reduced bindings, inactive and preprocessor paths, rejected
variadic/AS ANY declarations and ordinary variadic calls are exclusion controls.
Native/stable ABI, binary/text, module/restart/remap, policy/count corruption and
runtime evaluation checks accompany a production collector harness with copied
snapshots, unchanged returns, a million-event limit and 1,024-level nesting.
The CAST score-band regression is tested from the production transform and by
an actual conversion through an overloaded call in every export mode.

Resolution context tests require real procedure/formal/type identities and actual
argument E associations. Same-spelled nested namespace types, overloaded CAST
return/target subtypes, actual callee B records, reduced graphs, omitted operands,
failed choices and corrected-total corruption are checked separately from names.
Physical call bindings receive their actual callee access role. Macro procedure
references retain their separate existing reference identities and source-call
node counts; they never become B ordinals. These links do not turn considered
candidates into executed calls or edit ranges.

Native lookup tests compare compiler diagnostics and emission with tracing off,
then inspect actual lexical/import/type/enum/keyword probes and returned chains.
Ambiguous results retain every native alternative. Full/reduced graphs, both type
ABIs, text/binary transport, module reset and corrected-total corruption have
separate checks. The production collector guard has no linked name-search or
chain-pool implementation; it checks unchanged returns, copied names, inactive
gates, cyclic results and the 1,024-depth/million-event boundaries.

Symbol-filter tests compile the actual four native helper bodies against a
counting access hook and controlled symbols. They check matching and rejected
classes/types, first-match access denial, all native string suffix categories,
full-type versus masked-type behavior, unsuffixed DEF behavior and unchanged
return values. Source fixtures cover all four filter kinds and seven predicate
reasons; mode/type ABI/transport/reset and repaired-total corruption are separate
families. These receipts do not turn considered symbols into executed calls.

<!-- end of README.md -->
