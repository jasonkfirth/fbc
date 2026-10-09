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

Use `--toolchain-prefix <SDK>` when the source checkout has no native tools or
libraries. Source-tree declarations still take precedence through `-i inc`;
the SDK supplies executable tools and link libraries without another copy.

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

`sidecar.py` independently validates schema 27. `test_sidecar.py` checks the
compiler's records against source locations and known types, layouts, values,
and target relationships. The BASIC fixtures own separate responsibilities:

`call_atoms.py` checks original constant and bound expression identities,
direct and virtual source call bindings, compact exclusions and unchanged
emission. Its malformed groups include missing pairs, wrong symbol classes,
unavailable capabilities, virtual signature mismatches and foreign modules.

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

<!-- end of README.md -->
