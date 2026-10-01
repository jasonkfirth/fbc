<!--
Project: FreeBASIC compiler
File: compiler-semantic-model.md
Purpose: Define the semantic sidecar contract for compiler tooling.
Responsibilities: Describe records, identities, provenance, limits, and validation.
This file intentionally does NOT contain independent parsing or linter rules.
-->

# FreeBASIC compiler semantic model

`-semantic-model <file>` writes an opt-in, tab-separated semantic sidecar from
compiler-selected symbols, typed expressions, and procedure ASTs. It enables
source observation without enabling language debug options or changing
predefined configuration values such as `__FB_ERR__`. Consumers can compile to
a temporary object, or use normal emission-only compilation with `-r`.

`-semantic-model-expressions <file>` retains `M`, `E`, and dependency records,
with the same versioned header and completeness footer. It omits symbols,
bindings, signatures, values, implicit relationships, and serialized ASTs.
Its expression symbol/subtype IDs and detail count are zero. Both modes retain
completed binary precedence results, unary results, and member/index/call
prefixes. These are completed parser results, not a reconstructed source tree.

The exporter observes the compiler; it does not resolve names independently.
This is an invocation artifact, not an incremental workspace index or language
server. Tooling remains responsible for arguments, project state, and process
lifetime.

## Completion and transactions

A full model is complete only after a successful invocation writes a valid
`END`. Module records are staged and committed after successful parsing.
Parser restarts and failed modules discard their attempted records. A failed,
interrupted, oversized, or otherwise incomplete model must be rejected.
Successfully committed earlier modules can remain in an incomplete file.
The `-pp` option writes preprocessed text while retaining compiler language
validation, so its model follows the same completion rules.

Compact output supports an explicitly incomplete recovery form. A module that
recovered from compiler-reported source errors has an `R` record and the file
ends with `RECOVERY`, not `END`. This does not make its facts authoritative.
Consumers must reconcile physical ranges with their primary parser's invalid
or recovered regions, withhold overlapping facts, and mark retained facts
provisional. A full-model reader must reject recovery output. Interrupted
processes do not write a recovery footer.

## Schema version 20

Fields are separated by tabs. The wire representation is ASCII. A percent sign,
every byte below 32, and every byte at or above 127 is encoded as `%HH`, using
two uppercase hexadecimal digits. Printable ASCII otherwise remains literal.
This preserves compiler strings, accepted legacy source bytes, and filesystem
names without assuming they are all UTF-8. Decode percent escapes exactly once
to bytes, then decode valid UTF-8 or retain the original bytes. The Python
reader uses `surrogateescape` for reversible non-UTF-8 bytes. `%25E9` represents
the literal text `%E9`, not byte E9. Counts below include the record tag.

| Record | Fields after the tag | Count |
| --- | --- | --- |
| `FBCSEM` | schema version, compiler version | 3 |
| `M` | source path | 2 |
| `D` | source path | 2 |
| `S` | ID, name, symbol class, data type, subtype ID, scope, attributes, procedure attributes, length, offset, lexical parent ID | 12 |
| `B` | symbol ID, role, physical flag, source path, start line, start column, end line, end column | 9 |
| `I` | owner symbol ID, selected procedure ID, owner UDT ID, relationship kind, physical flag, source path, start line, start column, end line, end column, target signature | 12 |
| `P` | procedure ID, name, symbol class, data type, subtype ID, start line, end line, source path | 9 |
| `V` | symbol ID, procedure name, variable name, type kind | 5 |
| `N` | ID, parent ID, child edge, AST class, raw operator, operator code, operator kind, data type, symbol ID, subtype ID, source line, source path | 13 |
| `E` | ID, physical flag, source path, start line, start column, end line, end column, AST class, raw operator, operator code, operator kind, data type, symbol ID, subtype ID, source type spelling | 16 |
| `R` | schema version, expression count for the preceding module | 3 |
| `END` | schema version, module count, procedure count, symbol count, type-fact count, node count, expression count, binding count, implicit-call count, dependency count, dependency-complete flag, detail count | 13 |
| `RECOVERY` | schema version, module count, expression count, recovered-module count, binding count, dependency count, dependency-complete flag | 8 |
| `T` | symbol ID, current name, class name, type kind, type spelling, data type, subtype ID, lexical owner ID, namespace ID, attributes, procedure attributes, status, length, offset, alias, visibility, storage, origin | 19 |
| `A` | symbol ID, rank, dimension, bound kind, lower bound, upper bound | 7 |
| `F` | procedure ID, kind, calling convention, explicit parameter count, optional parameter count, returns-by-reference flag, ABI result data type, ABI result subtype ID, return method, operator code, overridden procedure ID, vtable index | 13 |
| `G` | procedure ID, parameter ID, source ordinal, passing mode, optional flag, array rank, body variable ID, instance flag | 9 |
| `U` | symbol ID, kind, base type ID, alignment, unpadded length, options, ABI result type, abstract method count, enum element count, scope start line, scope end line | 12 |
| `C` | identity domain, ID, value kind, encoded value | 5 |
| `H` | source domain, source ID, target domain, target ID, relationship kind, ordinal | 7 |
| `K` | identity domain, ID, property name, property value | 5 |
| `J` | AST node ID, ordinal, normalized unsigned case value, label ID | 5 |
| `O` | operator code, operator kind, selected procedure ID, physical flag, source path, start line, start column, end line, end column | 10 |
| `Q` | module source path, target ID, CPU ID, language, backend, pointer bytes, byte order, wchar bytes, default integer data type | 10 |
| `Y` | data type, name, class, storage bytes, natural alignment, signed flag | 7 |
| `Z` | define symbol ID, ordinal, token kind, token value | 5 |

The detail count totals `T`, `A`, `F`, `G`, `U`, `C`, `H`, `K`, `J`, `O`, `Q`,
`Y`, and `Z`, including repeated snapshots. Earlier schemas have a 12-field
`END`. Compact mode has no detail records and detail count zero. Unknown
schemas, tags, field counts, references, and inconsistent totals must be
rejected; readers must not silently ignore unfamiliar records.

## Identities and symbol metadata

Symbol, node, and expression IDs are positive and unique in their respective
file-wide domains. Nullable references use zero. Symbol allocation identities
are distinct from memory addresses: recycling a compiler pool node cannot
make a later local inherit an earlier local's ID.

`S` is an initial snapshot, emitted when an identity is first needed. Types can
still require completion or forward backpatching, and prototypes can still
lack their body's parameter names. Use the latest `T`, `F`, `G`, and `U`
snapshot for an ID. Locals are captured before flushing, and nested namespace,
type, enum, and scope tables are inventoried while alive. Cached formal-to-body
IDs survive flushing without dereferencing recycled variable pointers.
Unused private procedures still have semantic facts.

The lexical owner is the symbol-table owner; the namespace is the lookup-table
owner. They differ for locals and nested scopes. Anonymous aggregate containment
has an `anonymous-parent` relationship. A `canonical-symbol` relationship
connects temporary procedure headers and formal parameters to retained
prototypes, and forward types to their replacements. Follow these relationships
before comparing declarations with body references. Retired parser placeholders
can retain only their `S` snapshot.

`T` class names distinguish variables, constants, procedures, parameters,
defines, keywords, labels, namespaces, enums, types, unions, fields, typedefs,
forward types, scopes, and namespace imports. Raw classes and flags remain
compiler-specific diagnostic detail. Type kinds are `pointer`, `numeric`,
`dynamic-string`, `fixed-string`, `aggregate`, `procedure`, and `other`.
ZSTRING/WSTRING storage is classified as fixed string even though its code
units have the compiler's integer data class. A procedure-pointer variable is
a pointer; its subtype supplies the signature.

Visibility is `public`, `protected`, or `private`. Storage is `parameter`,
`field`, `constant`, `external`, `static`, `local`, `shared`, `global`, or `none`.
Origin is `source` for an observed declaration, `runtime` for a registered
runtime symbol, or `compiler` for other compiler-created symbols. This includes
generated storage and implicit declarations. Raw flags allow finer distinctions.
Field offsets and completed type sizes describe target layout. Local stack
offsets can remain zero because backend stack allocation follows export.
Type spellings are compiler-produced display text, not a substitute for type
and subtype identities.

`V` records describe variables. Their procedure name is empty for namespace
and global storage. `P` records describe parsed procedure bodies, including
compiler-generated procedures. Their lines and filenames are informational;
they are not verified edit ranges.

## Signatures, arrays, layouts, and relationships

`F` kinds are `sub`, `function`, `constructor`, `destructor`, `property-get`,
`property-set`, `operator`, and `procedure-pointer`. Calling conventions are
`cdecl`, `stdcall`, `stdcall-ms`, `pascal`, `thiscall`, and `fastcall`. Explicit
parameter counts exclude THIS and include a variadic marker. ABI result fields
retain the compiler's lowering decision alongside the source result in `T`.

`G` modes are `byval`, `byref`, `bydesc`, and `vararg`. Written parameters have
ordinals starting at zero; THIS has ordinal -1 and instance flag 1. Zero body
variable ID means no body storage was observed. Default initializer trees use
`default-initializer` relationships. Retained field, global, and static
initializers use `initializer`. Their AST roots belong to the owning symbol,
have source line zero, and carry no editable range.

`A` dimensions start at zero. A fixed array has a `fixed` record per dimension.
An unresolved ellipsis uses `unknown` with an empty upper bound. Dynamic arrays
have one `runtime` record, dimension -1, and empty bounds; rank -1 means unknown
rank. Runtime allocation bounds are not fixed declaration bounds. A variable's
length is its element size, not total array storage. Descriptor relationships
identify its compiler-generated storage and type.

`U` describes types, unions, enums, and scopes; inapplicable fields are zero.
The base ID identifies the actual base type, not a hidden base storage field.
Enum element values are named `C` constants. Scope lines are logical parser
lines, with zero for unavailable locations, and are not editor ranges.

`H` domains are `symbol` and `node`. Relationship kinds are:

- `canonical-symbol`, `imports`, `anonymous-parent`, `overload-next`, `overrides`,
  and `label-scope`;
- `initializer`, `default-initializer`, `array-descriptor`,
  `array-descriptor-type`, and `result-variable`;
- `default-constructor`, `copy-constructor`, `const-copy-constructor`,
  `destructor`, `deleting-destructor`, `copy-assignment`, and
  `const-copy-assignment`;
- `result-temporary`, `static-target`, `branch-target`, `default-target`, and
  `false-target`.

Missing targets do not produce a relationship. These records have no edit
range. Relationships observed before a table is cleared remain valid even
when a later snapshot no longer has live body storage or an import target.

`Q` and `Y` belong to their preceding `M`. CPU IDs use compiler option spelling,
for example `686` or `x86-64`. Primitive IDs and encoded data types remain
compiler-specific. The table supplies target sizes, signedness, and alignment
without assuming the compiler host's ABI. Empty size/alignment means unknown
or inapplicable. Fixed-string storage bytes describe a character unit; actual
declaration length is in `T`. User layouts are in `U` and field snapshots.
Retain context per module because an invocation can change language modes.

## Constants, AST payloads, and operations

`C` domains are `symbol`, `node`, and `expression`, with independent ID spaces.
Value kinds are:

- `signed` and `unsigned`: exact decimal integers;
- `float64-bits`: `0x` followed by 16 IEEE hexadecimal digits, using the
  compiler's DOUBLE value storage, including widened SINGLE values;
- `bytes`: two hexadecimal digits per decoded byte;
- `wide-units`: eight digits per decoded compiler wide unit.

Embedded NULs and negative floating zero are retained. Empty strings have
empty payloads. The host floating-bit helper handles host word order; declared
data type determines target representation. Ordinary variables and expressions
without a compiler-known constant do not acquire guessed values. Values for
adjacent duplicate expressions are staged until their final expression ID is
settled. Compact mode omits value records.

Every `N` has a `K` node `kind`. Names cover all current AST classes, including
assignment, binary/unary operator, conversion, call, construction, argument,
constant, variable, field, index, dereference, branch, jump-table, sequence,
memory, stack, initializer families, scope families, checks, runtime-macro,
unicode-index, literal-text, assembly, and debug. Unrecognized classes use
`unknown`. Payload properties include `vector-width`, `byte-offset`,
`index-scale`, `bytes`, `fill-byte`, `element-count`, `array-initializer`,
`result-edge`, `text`, `conversion`, `float-narrowing`, and `const-conversion`.
Conversion flags are 0/1 and are never misread as raw operator IDs.

`call-kind` is `direct`, `runtime`, `indirect`, or `virtual`. `argument-count`
is the lowered argument count. Virtual calls preserve the selected method in
`static-target`, even though their lowered AST calls a procedure-pointer
signature. This does not claim which runtime override executes. Ordinary
procedure-pointer calls have no guessed static target. Argument `passing-mode`
is an explicit override or `default`, which refers to its exact formal.
`branch-kind`, `memory-operation`, `stack-operation`, and `macro-operation`
describe their corresponding compiler operations; unsupported operations use
`unknown`.

Address-of nodes have no branch target. Their operator number is initialized,
but the unused extra-operand slot can retain a recycled AST node's data. The
exporter reads branch labels only from node classes that initialize that slot.

`J` preserves normalized unsigned jump-table value/label pairs. Interpret
`jump-bias` and `jump-span` in the selector's target integer representation to
recover source case values. Label IDs and default targets are compiler-resolved.

Symbol properties include `alignment`, `bit-position`, `bit-width`, and
`array-elements`. Define properties include `macro-kind` (`text`, `tokens`, or
`callback`), `macro-argument-count`, `macro-argless`, and `macro-flags`.
`Z` kinds are `parameter`, `parameter-reference`, `stringify-reference`, `text`,
and `wide-text`. Parameter ordinals and reference values use zero-based
parameter numbering. Replacement tokens have their own zero-based ordinals.
Wide text has eight hexadecimal digits per compiler wide unit. Replacement
text is retained preprocessor text, not an evaluated literal. Callbacks are
identified without invoking them. `#undef` history is retained while inactive
definitions are not invented. Macro declarations have `B` records; this is not
a complete macro invocation/expansion graph.

`N` parent IDs for `root` edges refer to procedure or initializer-owner symbols;
`left` and `right` parents refer to nodes. Raw node classes, operators, types,
attributes, and offsets are compiler-specific. Stable conceptual operator
codes provide a tooling vocabulary. Operator kind is `builtin`, `overloaded`,
or `none`; `none` has an empty code. Unknown operations remain `none` rather
than being guessed from numeric fields.

The vocabulary includes assignment and arithmetic (`add`, `subtract`,
`multiply`, `divide`, `integer-divide`, `modulo`, `power`), bit/logical operators
(`and`, `or`, `logical-and`, `logical-or`, `xor`, `equivalence`, `implication`,
`shift-left`, `shift-right`), their `-assign` forms, comparisons (`equal`,
`not-equal`, `greater-than`, `less-than`, `greater-or-equal`, `less-or-equal`,
`identity-test`), unary operations (`not`, `logical-not`, `unary-plus`, `negate`,
`address-of`, `dereference`), `index`, `member-access`, `cast`,
`convert-to-integer`, `convert-to-float`, `convert-to-boolean`,
`convert-to-signed`, `convert-to-unsigned`, `concatenate`, `concatenate-assign`,
`allocate`, `allocate-array`, `deallocate`, `deallocate-array`, `length`,
`iterator-initialize`, `iterator-step`, and `iterator-next`. Math codes include
`absolute-value`, `sign`, `sine`, `arcsine`, `cosine`, `arccosine`, `tangent`,
`arctangent`, `arctangent2`, `square-root`, `reciprocal-square-root`, `reciprocal`,
`logarithm`, `exponential`, `floor`, `truncate`, and `fractional-part`.

`O` preserves resolved binary, unary, assignment, math, and conversion
operations at their written operator tokens. A folded `1 + 2` can have an `E`
with `none` and a `C` value of 3, while `O` identifies its addition. Overloads
carry their exact procedure ID; builtins have target zero. Operator tokens are
not identifier bindings or editable spellings of generated callees. They must
not become procedure rename occurrences. `E` and `N` continue to describe
completed typed results and AST operations, respectively.

## Bindings, implicit calls, and source ranges

`B` roles are `declaration` and `reference`. Routes include variables, fields,
WITH members, named procedures and exact overload/address selections, types,
unions, enums and elements, typedefs, constants, namespaces and qualifiers,
USING targets, macro declarations, REDIM declarations/references, and labels.
Label targets include direct GOTO/GOSUB/RETURN, ON lists, numeric IF branches,
error-handler targets, and RESTORE. Assembly names resolved as variables,
constants, procedures, or labels have bindings. Assembly keywords and local
assembler names do not acquire guessed targets. A call through a procedure
pointer binds its written variable, not an invented concrete callee.

`I` kinds are `default-constructor`, `initializer-constructor`, `new-constructor`,
`destructor-call`, `argument-constructor`, `temporary-destructor`,
`scope-exit-destructor`, `delete-destructor`, and `return-constructor`.
They identify exact compiler-selected UDT procedures, not written callee tokens.
Their opaque `sig2` signature uses explicit formal mode, optional flag, array
rank, data type, and subtype identity, excluding the implicit instance parameter.

Default/initializer/destructor selections are anchored to variable, field, or
optional parameter declarations. NEW is anchored to its written type. Argument
construction belongs to the exact selected formal and actual expression;
retained argument locations survive overload selection and are released with
pending arguments. Conversion-created temporaries retain that verified range.
Return construction belongs to the result UDT and return expression. DELETE
uses its keyword and pointee type. Temporary cleanup uses a verified physical
completed expression range, including IIF and WITH lifetimes; scope-exit cleanup
uses the verified GOTO/RETURN/EXIT/CONTINUE keyword and exact departing local.
Macro/remapped or unverified temporary/scope-exit ranges are omitted. Non-UDT
cleanup and operations without a concrete UDT procedure are omitted.

All source ranges use one-based lines and zero-based UTF-16 columns, with an
exclusive end. A physical flag of 1 is required before editor projection.
Macro expansion, logical #line remapping, cross-file expressions, or failed
physical bounds make ranges informational. After #line, the logical filename
is kept separate from the opened physical file. Empty/reversed spans are
omitted. No I/H/O relationship implies an editable generated callee.

The bounded physical-line reader handles unmarked text and BOM-marked UTF-8,
UTF-16LE/BE, and UTF-32LE/BE. Supplementary characters use two editor columns.
Checks use complete physical lines rather than truncated diagnostic excerpts,
and a physical line cursor independent of logical #line reuse. Explicitly
malformed encoded lines and oversized lines fail closed.

Adjacent identical E facts are written once. Later parser-known operator
concepts can replace the same staged AST result without changing its ID.
Different ranges, types, identities, values, or intervening records stay
separate. Repeated binary chains retain each completed prefix before subsequent
folding. For cursor lookup, prefer the narrowest physical range; exact selections
must match exactly. Compiler-generated details are not a complete source graph.
Repeated includes can retain identical spans with contradictory types under
different contexts. Source-only consumers must withhold ambiguous facts.

## Dependencies, limits, and tests

`D` paths are unique normalized opened source paths in read order, including
root modules, successful includes, and preincludes. Unopened paths and inactive
includes are not dependencies. At 5,000 paths, dependency completeness becomes
zero; consumers validating source-backed facts must not treat omissions as
unrelated files. Root modules remain M records only when their stage commits.

Each module buffer is limited to 256 MiB, symbols to one million per module,
nodes/expressions/bindings/implicit calls to one million per model, and details
to 16 million per model. Allocation or record/buffer exhaustion invalidates the
model instead of writing END. Call signatures and literal unit counts also have
bounds. Dependency exhaustion alone writes the explicit incomplete-list flag.
Compiler modules are serial and exporter state is process-local, not thread-safe.
Parallel jobs require distinct output paths.

Run `make compiler-semantic-model-test` for the dedicated suite. Its independent
reader checks shapes, identities, references, ranges, escaping, and totals.
Fixtures exercise each record family and positive/negative semantics across
GCC, Clang, and LLVM emission. The suite covers layouts, arrays, signatures,
overloads, pointers, virtual dispatch, values, operators, macros, cleanup,
retries, rollback, recovery, encodings, dependency limits, and pool reuse. It
rejects guessed callees, fixed runtime bounds, invented values, editable macro
or remapped facts, inactive definitions, and false completion. The historical
`compiler-semantic-model-smoke` target remains available.

The runner accepts `--backend` and `--test` to select emission backends and
individual test methods. It creates private temporary artifacts and includes
a native behavior comparison with export disabled, full, and compact.

The reader also checks completed metadata independently of footer counts:
every symbol needs a type snapshot or a canonical replacement, procedures
need complete parameter signatures, types/scopes need layouts, and named
constants need values. Each module needs a target context and a primitive
type table with every current datatype slot. Canonical replacements must be
acyclic, and AST child edges must refer to an earlier parent without duplicating
its left or right child. Preliminary procedure headers and ordinary symbols both reset
their allocation identity when reusing a pool slot. A method's procedure-pointer
signature exposes THIS as an explicit parameter; only the method's own THIS
has ordinal -1 and the implicit-instance flag. XMMWORD is an internal register
type without a source scalar alignment, so its primitive alignment is empty.
Repeated procedure-pointer signatures can expose parameter bindings before
the compiler reuses an existing signature. The unused preliminary header and
its formals receive canonical-symbol relationships to that selected signature
and its corresponding formals. Their unfinished ABI metadata is not exported.

Run `make compiler-semantic-corpus-test` to export the maintained OMA native
programs with GCC, LLVM, and Clang. The corpus runner freezes the compiler,
headers, BASIC inputs, and independent validator, validates each full/compact model and physical range,
checks every translation unit and dependency, and compares full/compact expression
facts. It also requires byte-identical backend emission with export disabled,
full, and compact. This is an emission and semantic audit, not a gameplay test.
For retained artifacts, run `build_scripts/test-compiler-semantic-corpus.py`
with `--fbc bin/fbc --output <new-directory>`; `--backend` and `--program` select
smaller runs.

Run `make compiler-semantic-self-test` for the same audit against every module
selected by the native compiler build. This extended target freezes the
compiler sources, public headers, Make modules, compiler binary, and validator.
It reads the source list and flags from the frozen Make graph rather than
maintaining a separate module list. GCC, LLVM, and Clang each run with export
disabled, full export, and compact export. Cases use distinct backend source
trees so parallel jobs cannot overwrite emitted files.

For retained evidence:

```sh
python3 build_scripts/test-compiler-semantic-self.py --fbc bin/fbc \
    --output out/compiler-semantic-self --compress-artifacts
```

`--unit symbols/symb-proc.bas` selects a native module, `--backend` selects
backends, and `--jobs` bounds concurrent cases. `--target-triplet` selects other
host policies through Make, including modules omitted from the native graph.
The Make target accepts these options in `COMPILER_SEMANTIC_SELF_FLAGS`.
The runner retains exact
commands, input hashes, mode hashes, sidecars, and per-case results. Compressed
artifacts preserve their original bytes. This checks the native build's active
source branches; other host policies and inactive preprocessor branches need
separate target coverage. The audit emits code and does not link a new compiler.

## Consumer migration

Schema 19 adds metadata families and the detail count, corrects lexical parent
ownership, and gives full output the compact mode's expression-prefix coverage.
Readers must explicitly support this version. Earlier readers must reject it.
Raw encodings require knowledge of the corresponding compiler family; stable
labels, relationships, and target type facts reduce that dependency.

Fblint was the initial consumer for undeclared-name and variable-type checks.
Existing data-flow and safety checks are consumer policy, not sidecar behavior.
A consumer adopting schema 20 must validate it before using compiler facts as
authoritative, and must continue treating recovery output as provisional.

<!-- end of compiler-semantic-model.md -->
