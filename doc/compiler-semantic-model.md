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

`-semantic-bundle <prefix>` publishes the complete model as
`<prefix>.fbcsem` and the matching structured diagnostics as
`<prefix>.fbcdia` from one compiler invocation. The two files retain their
independent versioned formats and transactional completion markers. A consumer
must validate both files and the model's `FILE` hashes before reusing the pair;
the shared prefix is naming infrastructure, not a promise that stale sources
remain valid.

`-semantic-model-expressions <file>` retains `M`, `E`, and dependency records,
with the same versioned header and completeness footer. It omits symbols,
bindings, signatures, values, implicit relationships, and serialized ASTs.
Its expression symbol/subtype IDs are zero. Provenance records contribute to
the detail count in every mode. The full and
expression-only modes retain completed binary precedence results, unary
results, and member/index/call prefixes. These are completed parser results,
not a reconstructed source tree.

`-semantic-model-bindings <file>` retains compiler symbol identities, resolved
token bindings, selected implicit calls, dependency records, and source
provenance. It omits typed-expression records and the procedure AST dump, so
edit validation can keep complete identities without retaining unrelated AST
details. It is not an expression-typing substitute.

`-semantic-model-compact` can accompany any semantic-model mode to omit the
verbose macro-expansion graph (`MD` through `MR`). It does not change macro
evaluation or the selected symbols, bindings, calls, expressions, dependencies,
conditional facts, or ordinary source provenance. Use it when a consumer needs
those compiler facts but does not need per-expansion argument and replacement
segments. The default retains the complete macro graph.
Full compact models keep a bounded in-memory map of macro invocation locations
so generated typed expressions can still point to their call sites. The map is
not serialized and does not make macro-expansion or macro-reference capabilities
available.

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

## Schema version 27

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
| `NT` | subject domain, subject ID, role, primitive name, nominal symbol ID or zero, pointer depth, reference flag, const qualifier bits, storage bytes or empty, mangling modifier or empty | 11 |
| `PH` | phase ID, procedure symbol ID, phase name, emitted flag | 5 |
| `NP` | AST node ID, phase ID | 3 |
| `EV` | parent AST node ID, evaluated AST node ID, ordinal, condition | 5 |
| `CB` | control-flow block ID, phase ID, ordinal | 4 |
| `CN` | block ID, root AST node ID, ordinal | 4 |
| `CE` | phase ID, source block ID, target block ID or zero, transfer kind, label symbol ID or zero | 6 |
| `CL` | phase ID, label symbol ID, defining block ID | 4 |
| `DI` | diagnostic ID, severity, compiler code, message, detail, custom text, path, line, source context ID or zero, statement ID or zero, procedure symbol ID or zero | 12 |
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
| `ASM` | node ID, token ordinal, text/symbol kind, symbol ID or zero, byte-safe token text | 6 |
| `DCL` | occurrence ID, symbol ID, declaration role, written name, physical flag, source path, start/end range, module ordinal, statement ordinal | 13 |
| `CTX` | configuration identity, module ordinal | 3 |
| `OPT` | configuration identity, option domain, option name, integer value | 5 |
| `USE` | subject domain, subject identity, configuration identity | 4 |
| `FILE` | revision ID, opened source path, byte count, SHA-256, encoding, regular/stream kind | 7 |
| `SRC` | context ID, parent context ID, revision ID, module ordinal, context kind, include depth, requested path, physical flag, directive path, start/end range | 14 |
| `SRE` | context ID, revision verification result | 3 |
| `INC` | occurrence ID, parent context ID, requested path, resolved path, outcome, physical flag, directive path, start/end range | 12 |
| `MAP` | context ID, logical line, logical path, physical flag, directive path, start/end range, remap occurrence ID | 11 |
| `ORIG` | subject domain, subject identity, source context ID | 4 |
| `PPB` | branch ID, parent branch ID, group ID, source context ID, configuration ID or zero, branch ordinal, directive kind, physical flag, directive path, start/end range | 14 |
| `PPD` | branch ID, evaluation state, condition truth or empty, selected flag, physical flag, final token path, start/end range | 11 |
| `PPE` | group ID, source context ID, physical flag, closing directive path, start/end range | 9 |
| `PPT` | probe ID, branch ID or zero, source context ID, selected symbol ID or zero, first identifier spelling, found flag, physical flag, query path, start/end range | 13 |
| `PPS` | inactive region ID, branch ID, source context ID, physical flag, source path, start/end range | 10 |
| `MD` | definition ID, symbol ID or zero, module ordinal, configuration ID or zero, name, kind, formal count, flags, argless flag | 10 |
| `MT` | definition ID, ordinal, token kind, value, paste-before flag | 6 |
| `MI` | attempt ID, parent attempt ID, definition ID, source context ID, configuration ID or zero, conditional branch ID or zero, phase, parent relation, physical flag, name path, start/end range | 15 |
| `MA` | attempt ID, formal ordinal, unit kind, argument text, has-source flag, physical flag, argument path, start/end range | 12 |
| `MS` | attempt ID, piece ordinal, definition token ordinal or -1, formal ordinal or -1, piece kind, output offset, output length | 8 |
| `MC` | attempt ID, unit kind, callback error code, actual callback return text | 5 |
| `ME` | attempt ID, outcome, unit kind, output length, actual replacement text, actual argument count, physical flag, final input path, start/end range | 13 |
| `ML` | occurrence ID, definition ID or zero, action, source context ID, configuration ID or zero, spelling, physical flag, declaration/undef path, start/end range | 13 |
| `MR` | subject domain, subject ID, expansion attempt ID, observed role | 5 |
| `LOC` | subject domain, subject ID, role, source context ID, physical start/end UTF-16 coordinates, start/end byte offsets, mapping state | 12 |
| `ST` | statement ID, parent statement, compound ID, procedure owner, source context, configuration, module, source ordinal, token ID/class, physical flag, opening location | 17 |
| `STE` | statement ID, parser route, outcome, physical flag, closing location | 10 |
| `BLK` | compound ID, parent compound, opening statement, procedure owner, kind, source/configuration context, physical flag, opening location | 14 |
| `BEND` | compound ID, closing statement ID, physical flag, closing location | 9 |
| `OWN` | subject domain, subject identity, statement owner | 4 |
| `CAP` | module ordinal, semantic capability, available/partial/unavailable coverage | 4 |
| `ACC` | binding occurrence ordinal, compiler-observed source use role | 3 |
| `SOP` | statement identity, accepted builtin statement operation | 3 |
| `Z` | define symbol ID, ordinal, token kind, token value | 5 |

The detail count totals `T`, `A`, `F`, `G`, `U`, `C`, `H`, `K`, `J`, `O`, `Q`,
`Y`, `Z`, `ASM`, `DCL`, `CTX`, `OPT`, `USE`, `FILE`, `SRC`, `SRE`, `INC`, `MAP`,
`ORIG`, `PPB`, `PPD`, `PPE`, `PPT`, `PPS`, `MD`, `MT`, `MI`, `MA`, `MS`, `MC`,
`ME`, `ML`, `MR`, `LOC`, `ST`, `STE`, `BLK`, `BEND`, `OWN`, `CAP`, `ACC`, and
`SOP`, including repeated snapshots.

Array-bound query properties retain the selected array, the optional source
dimension expression, and the compiler-selected dimension expression. If the
source omits the optional dimension, `array-bound-dimension` is `0` and
`array-bound-dimension-explicit` is `0`; the compiler-selected dimension still
has a positive expression identity. An explicit source dimension has a
positive `array-bound-dimension` identity and an explicit marker of `1`.
Earlier schemas have a 12-field `END`. Expression and binding modes retain
source provenance details. Unknown
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

Schema 20 also retains initialized operator options, assignment initialization,
result-load and literal-suffix flags, and initializer scope offsets/byte counts.
Natural alignment and requested packing are separate symbol properties. Formal
descriptor types, argument register classification, finalized aggregate return
classification, vtable/RTTI ownership, procedure status/priority, and declared
label roles are preserved as properties and relationships.

Full models add optional `K symbol <id> declared-field-count <count>` for
completed TYPE/UNION layouts. The count includes source FIELD symbols whose
actual owner is that aggregate, including promoted anonymous members. It does
not include inherited fields, static variables, methods, hidden base storage
or compiler-created dynamic-array descriptors. An unfinished snapshot has no
count receipt; an absent receipt means unknown, not zero.

Each FIELD also has optional `K symbol <id> field-array-rank <rank>`: zero for
a scalar, -1 for an unspecified descriptor rank, or 1 through 8 for an array.
This distinguishes a scalar from missing `A` rows. Readers that require these
facts must compare counts with the complete source FIELD group and ranks with
the retained array records. Older readers may ignore these additive properties;
older producers may omit them. Bindings-only and expressions-only exports omit
both properties. No record layout, schema number, language behavior or emitted
instruction changes.

CALL auxiliary trees use `copyback`, `profile-begin`, and `profile-end` N edges.
Their parent is a node. `auxiliary-ordinal` records action order, and each
copyback destination has a `copyback-temporary` relationship to its source
temporary. `copyback-count` records the exact number, including zero. These
describe deferred compiler actions without fabricating source call nodes.

ASM rows preserve the compiler's ordered text and bound-symbol tokens. Registers
and assembler-local labels remain text. Assembly effects remain explicitly
unknown for memory, registers, and control; no instruction analysis is implied.

DCL occurrences preserve named prototype formals, procedure prototypes and
definitions including constructors/destructors/operators, and implicit source
variables. A name may be empty for an unnameable special procedure. Occurrence
identity is distinct from symbol allocation identity. A formal's preserved
written name can supply T's name when the compiler intentionally discarded its
internal prototype name.

CTX/OPT snapshots contain all current compiler option, language-default, and
literal/feature-policy fields. AST nodes retain the context at construction,
including through cloning. USE connects node and declaration identities to
those snapshots. A later snapshot never retroactively changes an earlier
declaration or node. Rolled-back parse attempts discard their context records.
Full models add optional `OPT <context> language-policy remkeyword-active <flag>`.
The flag is one while the native REM keyword remains installed, and zero after
an accepted `OPTION NOKEYWORD REM` removes that keyword from the lookup hash.
The removal invalidates the option snapshot even though `FBOPTION` is unchanged.
Keyword installation and this observation reset for each module. No symbol
lookup scratch, AST ownership or native pointers are retained by the observer.
Older models may omit the key; a consumer needing REM comment boundaries must
treat that absence as unknown. `language-default escapestr` separately supplies
the committed string escape mode. These facts do not parse documentation tags.
These records describe effective configuration. File, include, and expansion
contexts use the separate provenance records below. The exported compiler facts
do not claim to form a complete source expression graph.

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
For constructor syntax such as `WrittenType(...)`, the type or typedef token is
a reference to that exact symbol; the generated constructor remains an `I`
relationship rather than an invented callee-token binding.
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

## Source revisions and occurrences

`FILE` describes bytes from the actual CRT stream opened by the compiler. The
observer hashes from the beginning, including any BOM, restores the stream
position, and verifies metadata and bytes again before the compiler closes it.
SHA-256 is lowercase hexadecimal. Encoding is `unmarked-bytes`, `utf-8-bom`,
`utf-16le`, `utf-16be`, `utf-32le`, or `utf-32be`; it describes the compiler's
selected decoder, not a conversion of the captured bytes. Nonregular streams
have kind `stream`, an empty digest, and zero byte count.

Each successful source open has a distinct `SRC` occurrence, even when the
path was opened before. Kinds are `module`, `include`, and `preinclude`. Root
contexts have no parent; nested contexts identify the active includer and its
actual depth. The requested path preserves include spelling, while `FILE`
identifies the opened path. A directive range is absent when no source token
supplied the open, such as a command-line preinclude. Each context closes with
one `SRE`: `verified`, `unverified-stream`, or `changed-or-unreadable`. A changed
or unreadable revision prevents successful publication.

`INC` retains attempted include outcomes: `opened`, `include-once`,
`pragma-once`, `not-found`, `open-failed`, or `depth-limit`. Skipped and failed
attempts do not create successful source contexts or dependencies. `MAP`
records an observed `#line` directive's logical line and filename, together
with its directive range. It does not make synthesized locations editable.

`ORIG` associates a node, declaration occurrence, expression, or binding with
its source context. Binding identities are their file-wide record ordinals,
starting at one. Node and declaration origins are absent in expression mode,
which retains expression origins. A source context and a physical range do
not establish freshness: before editing, consumers must verify the current
file bytes against the captured digest and reject unverified streams. The
independent reader's `validate_source_revisions()` provides this check. Its
per-operation source cache is keyed by resolved path, byte count, and SHA-256,
so repeated occurrences do not cause repeated reads. The cache is discarded
after each validation operation; a later operation observes the filesystem
again rather than trusting retained bytes.

The reader checks that each origin belongs to its subject's module. Binding,
declaration, and expression origins must also agree with their recorded
statement occurrence, when present. Their physical attachments must retain
that same origin. Equal bytes or identical coordinates in another module or
repeated include do not establish ownership.

## Physical coordinate attachments

`LOC` links an observed source subject to the original opened source context,
independently of its logical filename and line number. Physical lines are
one-based, columns count UTF-16 units from zero, and the end is exclusive.
Byte offsets are zero-based and include the original BOM. `mapped` means both
boundaries were resolved against the compiler's open binary handle with the
captured encoding; `unverified` retains observed positions with byte offsets
-1. Malformed sequences, unprovable boundaries, unavailable streams, or source
line limits cannot become successful byte mappings.

Domains include `binding`, `declaration`, `expression`, `conditional`,
`conditional-end`, `defined-probe`, `macro-attempt`, `macro-argument`,
`macro-lifetime`, `source-context`, `include`, and `remap`. A `macro-argument`
uses role `formal-N` for its canonical formal. Other current roles are `range`,
`name`, `directive`, and `include`. Remap identities remain distinct when a
logical line and filename are reused.

The physical cursor advances when input characters are consumed, including
lookahead, rather than when the parser later consumes EOL tokens. CRLF, CR,
and LF each advance one line. Replacement and EVAL text do not advance the
original-file cursor, and their token subjects do not acquire invented physical
attachments. A physical enclosing source range can still contain written macro
invocations; that describes the original input extent, not expanded text.

Coordinate lookup temporarily reads the already-open handle, restores its
position, and supports unmarked UTF-8-compatible bytes plus all five BOM
encodings. It never replaces the observed revision with a pathname reopen.
Projection must still pass source freshness and existing edit-eligibility
checks. `LOC` preserves remapped origin without making the logical location
editable. The independent reader's `validate_physical_locations()` checks the
current revision, encoding boundaries, byte offsets, and UTF-16 coordinates.
It collects requested offsets by verified byte revision and encoding and
decodes them in ascending order, avoiding a full prefix decode for every
attachment. `plan_edits()` builds on
that gate for exact mapped binding, declaration, or expression subjects. It
rejects stale, missing, noneditable, empty,
overlapping, encoding-invalid, or oversized replacements and returns proposed
bytes and digests without writing a source file. A consumer must recheck the
expected digest immediately before its own atomic replacement.

Unmarked source can contain legacy bytes on lines without mapped attachments.
Malformed unqueried lines in wide encodings likewise do not invalidate later
mapped lines. Line endings are found in aligned encoded units, then each line
carrying mapped boundaries is strictly checked in full, including a malformed
suffix after its last requested boundary and the producer's line-size limit.
Repeated opens of the same byte revision share coordinate decoding while
retaining distinct source owners.

Edit planning requires an `END` transaction. Allowing `RECOVERY` during reading
does not authorize edits: provisional facts still require a consumer's primary
parser and error-region reconciliation. An unmatched statement with no consumed
endpoint retains its observed opening point without a physical range. Empty
physical spans remain invalid for ordinary written records.

Procedure declaration records can describe a complete header while a `LOC`
retains its observed opening token. That attachment must lie within the written
header; it is not permission to replace the entire header using token offsets.

A physical source line is indexed up to 16,777,216 bytes. A larger or malformed
line yields an unverified attachment, retaining explicit uncertainty instead
of a fabricated offset.

## Conditional preprocessing

`PPB` begins each observed `#IF`, `#IFDEF`, `#IFNDEF`, `#ELSEIF`,
`#ELSEIFDEF`, `#ELSEIFNDEF`, or `#ELSE` branch. The first branch's identity is
also the group identity. Ordinals start at zero; successors retain the group
and lexical parent branch. Source contexts distinguish repeated includes.
Configuration identities describe the state before evaluating the directive;
they are zero in expression and binding modes, which omit configuration records.

`PPD` separates evaluation from selection. `evaluated` has the compiler's
normalized Boolean result, including an evaluated but unselected `#ELSEIF`.
`unconditional` describes `#ELSE` without inventing a condition value.
`parent-inactive` describes directives scanned inside an inactive ancestor:
their expressions and names were not evaluated. `invalid` retains error
recovery without claiming a valid condition result. The observer never
reevaluates a condition or invokes its callbacks. `PPE` closes the group at
the actual `#ENDIF`; a normal complete model requires every group to close
and every branch to have exactly one decision. Recovery may retain an unfinished
group and remains provisional.

`PPT` records the selected symbol, or its absence, from an actual `DEFINED()`
or conditional defined-name query. The spelling is the first written identifier
token; the range can include qualifiers. A branch identity of zero describes a
query outside a conditional group, such as `#ASSERT DEFINED(...)`. Expression
mode has no symbol inventory and uses zero symbol IDs even for successful
queries. Inactive nested directives have no invented query records.

`PPS` marks source text skipped by the inactive scanner. Its start is the next
whole line after the condition, including whitespace and continuations, and
its exclusive end is the `#` of the directive which resumes processing. Nested
directives in that region still have branch events, without declarations or
overload selections for their inactive bodies. Empty or generated regions
without a source extent have no fabricated physical span. Generated directive
and condition locations can retain a logical point with equal start/end
coordinates; this is permitted only when their physical flag is zero.
The inactive scanner does not validate nested branch syntax. Its events may
therefore retain duplicate `#ELSE` directives or an `#ELSEIF` after `#ELSE`,
all marked `parent-inactive`; readers must not impose active-branch grammar on
that accepted skipped text.

Observation storage supports up to 65,536 nested conditional groups, including
inactive nesting beyond the parser's active-depth limit. Exceeding that export
limit fails publication without changing the parser's preprocessing decisions.

## Macro definitions and expansions

`MD` snapshots the selected definition while its data is alive. It has a
separate identity from the symbol inventory so expression mode can retain
complete macro provenance with zero symbol and configuration IDs. Kinds are
`text`, `tokens`, `define-callback`, and `macro-callback`. `MT` retains formal
names, replacement tokens, parameter/stringify references, and observed `##`
boundaries. Its token kinds match the `Z` vocabulary; parameter ordinals and
replacement ordinals are separate sequences starting at zero. A callback
definition has no fabricated replacement body. Snapshots are immutable;
undefining and defining the same name again creates a distinct definition.

`ML` observes `define`, `identical`, `undef`, `undef-missing`, or
`definition-rejected`. Successful identical definitions retain the selected
definition rather than inventing a replacement. A missing undef has no target.
Initial built-in or command-line definitions are snapshotted when selected;
that observation does not claim a source declaration location.

`MI` describes an attempt to load a selected macro. Parent relations distinguish
`root`, `replacement`, `argument`, and `callback`; phase distinguishes normal
source, inactive scanning, and preprocessor evaluation. The token's origin is
captured before consuming its identifier because that token can exhaust an
older replacement. Independent observation frames follow actual replacement
lengths; they never alter the compiler's recursion stack.

`MA` maps each effective argument to its canonical formal, including variadic
tails and empty/missing slots. Text is observed after the loader's trimming and
before callbacks or substitution. Source ranges describe observed argument
tokens, with an explicit absence flag for a missing slot. `MS` partitions the
actual replacement into ordered `text`, `parameter`, `stringify`, `callback`,
`definition-text`, or `restored-delimiter` pieces. Offsets and lengths use the
replacement's units. Parameter and stringify pieces identify the exact
definition token and formal. The paste-before marker and adjacent piece offsets
retain token-pasting provenance without resolving the resulting identifier again.

`MC` records the return value from the one callback invocation the compiler
performed. `ME` records the actual text prepended to the lexer, before mixing
in the caller's remaining input. Outcomes distinguish `expanded`, `not-invoked`,
`unsupported`, `recursive`, `failed`, and `recovered`. An empty successful
replacement remains an expansion; a macro name passed without required
parentheses remains unexpanded. Suppressed or failed attempts do not become
ordinary source calls. Recovery is provisional, and a successful completeness
footer cannot advertise a macro failure.

The `bytes` unit kind uses the normal byte-safe field encoding. `wide-units`
uses eight uppercase hexadecimal digits per code unit from the compiler's
wide lexer buffer, independently of the target's runtime wchar representation.
Readers must validate output lengths, piece ordering and coverage, formal
mapping, and actual parameter/stringify text, with no implicit byte-to-Unicode
conversion.

`MR` links actual expansion origins to bindings, declarations, expressions,
conditional directives, defined-name probes, and macro arguments. Expression
`token-N` roles come from consumed tokens, including expanded operators between
physical operands; they are not inferred from overlapping source ranges.
Generated declaration names without an extent can link their selected `symbol`
with a `declaration-ID` role while omitting a fabricated binding range. All
these links preserve nonphysical editing policy. Definition and token-history
storage are each bounded at 1,048,576 entries per module; overflow fails export
publication without changing macro evaluation.

## Dependencies, limits, and tests

Sidecars are staged in a private directory beside their destination. The
compiler checks writes, flush, close, and final replacement before publication.
An output path cannot alias a source, include, preinclude, object/library input,
or requested compiler artifact. File identities detect hardlinks; existing
destination symlinks, devices, directories, and FIFOs are rejected. Protection
continues when the public dependency list reaches its limit.

An interrupted invocation leaves the destination untouched. A transport or
publication failure reports a compiler error and preserves existing output.
Ordinary parser errors retain the previous incomplete/recovery output contract;
consumers still require the invocation result and the appropriate footer.

`D` paths are unique normalized opened source paths in read order, including
root modules, successful includes, and preincludes. Unopened paths and inactive
includes are not dependencies. At 5,000 paths, dependency completeness becomes
zero; consumers validating source-backed facts must not treat omissions as
unrelated files. Root modules remain M records only when their stage commits.

Each module buffer is limited to 512 MiB, symbols to one million per module,
nodes/expressions/bindings/implicit calls to one million per model, and details
to 16 million per model. Allocation or record/buffer exhaustion invalidates the
model instead of writing END. Call signatures and literal unit counts also have
bounds. Dependency exhaustion alone writes the explicit incomplete-list flag.
Compiler modules are serial and exporter state is process-local, not thread-safe.
Parallel jobs require distinct output paths.

The independent reader applies configurable limits to sidecar bytes, individual
record bytes, record count, source bytes per file, aggregate cached source
bytes, edit count, and planned output bytes.
`build_scripts/validate-compiler-semantic-model.py` provides the same bounded
validation as a command-line entry point, with optional source and physical
location verification and text or JSON status.

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

The runners also accept `--backend gas64` for x86-64 assembly and `--backend gas`
for x86 assembly. The compiler-source runner requires a matching Make target,
for example `--backend gas --target-triplet i686-linux-gnu`. The fixture and OMA
runners select the x86 target for gas emission without requiring 32-bit linking.
Frozen self and corpus audits copy the reader's complete Python module closure,
not only `sidecar.py`, so retained validation remains importable without the
live source tree.

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

Schema 27 is the current format. It includes the metadata and detail count
introduced by schema 19, stable operator concepts and configuration/source
provenance from later versions, compiler-selected preprocessor facts, and the
macro expansion graph, source constructs, source/AST associations, access roles,
argument default origins, explicit capability coverage, normalized types,
and procedure evaluation/control-flow phases. Readers must explicitly support the schema they accept
and reject unknown versions. Raw encodings require knowledge of the
corresponding compiler family; stable labels, relationships, and target type
facts reduce that dependency.

Fblint was the initial consumer for undeclared-name and variable-type checks.
Existing data-flow and safety checks are consumer policy, not sidecar behavior.
A linter's suppression directives, unused-suppression accounting, project rule
configuration, path policy, and JSON or SARIF rendering are also consumer
responsibilities. The compiler supplies exact source identity, configuration,
diagnostic, capability, and location facts needed to implement those policies;
it does not embed one consumer's rule language in FBCSEM.
A consumer adopting schema 27 must validate it before using compiler facts as
authoritative, and must continue treating recovery output as provisional.
The complete ownership audit is recorded in
`doc/compiler-semantic-foundation-audit.md`.

## Normalized types and procedure flow

`NT` is a full-model type snapshot for a `symbol`, `node`, or `expression`
subject. Its current role is `value`. Primitive names use the `Y` vocabulary;
the nominal symbol ID preserves distinct aggregate, enum, and procedure types.
Const bits run from the current value at position zero through each pointer
dereference. There are exactly pointer-depth plus one bits. Storage width is
empty when unknown, and pointer width follows the selected target. Symbol
snapshots may repeat as forward declarations become complete; the latest
snapshot applies, matching `T` metadata. Expression and node subjects have
one snapshot. All `NT` records contribute to the detail count.

`PH` observes a procedure's `pre-load` AST, with an emitted flag distinguishing
emitted and observed-only procedures. `NP` assigns each body node to that phase.
Symbol initializer and default-argument trees remain separate metadata trees,
identified by their `H` relationships. They are not procedure blocks.

`EV` records evaluation order under its condition: `always`, `argument`,
`profile-begin`, `call-target`, `profile-end`, `copyback`, `condition`, `true`,
`false`, or `result`. Assignment evaluates its right side before its destination
address. Conditional arms can share an ordinal because only one arm is taken.

Each `CB` currently owns one procedure root through `CN` ordinal zero. Block
ordinals follow the procedure list. `CL` maps a label to its defining block in
the same phase. `CE` either names the next block for `fallthrough`, or retains
a label for `label`, `conditional-label`, `case-label`, `default-label`, and
`subroutine-call`. Label transfers keep target block zero; consumers can resolve
them through `CL`. An indirect subroutine call may have label zero.
`unknown-indirect`, `unknown-assembly`, `subroutine-return`, and `procedure-exit`
keep both target and label zero. Assembly can retain a fallthrough edge while
also declaring unknown control effects. These observations are conservative
and do not claim that opaque or indirect targets are known.

`DI` retains an `error` or `warning` actually reported by the compiler in every
sidecar mode, including recovery. Its source and statement IDs are optional;
its path and line describe an informational point, not an editable range.
Procedure symbol IDs are absent in expression-only output. These records
contribute to the detail count in complete output and preserve the compiler's
message, detail, and custom text without parsing console output.

## Source statements and compounds

`ST` opens a parser dispatch observation and `STE` closes it at the last
consumed token. The parser route distinguishes declarations, compounds,
calls/assignments, intrinsic statements, assembly, pointer operations, labels,
aggregate members, enum lines, and assembly lines. Outcomes are `parsed`,
`recovered`, or `unmatched`; the last two must retain their provisional meaning.
Comments and line separators do not become accepted statements. Empty colon
separators can produce an unmatched dispatch with no valid combined range.

Parent statement identities describe nested grammar calls, including inline
IF bodies and aggregate members. `BLK`/`BEND` describe the active compound
stack, whose lifetime can span many statements. Kinds are `if`, `for`, `do`,
`while`, `select`, `with`, `scope`, `namespace`, `extern`, `procedure`, `type`,
`union`, `enum`, and `assembly`. One NEXT statement can close several FOR
compounds. An aggregate or single-line compound can close inside its opening
statement. This containment graph does not supply execution or branch selection.

Opening and closing locations preserve logical coordinates. `LOC statement`
and `LOC construct`, with role `range`, independently retain the original
physical span when both endpoints belong to the same source occurrence.
Continuations and colon-separated statements keep their actual boundaries.
Remapped locations retain their conservative editing policy. Macro replacement
statements retain macro origins without invented physical spans. Inactive
preprocessor bodies do not acquire statement or compound observations.

`OWN` attaches bindings, declaration occurrences, expressions, and AST nodes to
their observed statement. An AST node keeps the statement that created it;
cloning preserves that identity even when emission happens later. Generated
cleanup and other compiler nodes can therefore have a statement owner while
retaining no editable node range. Ownership alone does not prove a written
operation, runtime execution, or an original source token for a generated node.

Completed output closes all statement and compound stacks. Expression recovery
can retain unfinished observations with its RECOVERY footer. The reader checks
nesting, closure, source/configuration identities, and same-module fact
ownership independently of transport totals.

## Source analysis relationships in schema 25

`H node <id> expression <id> source-expression 0` connects a surviving AST
allocation to the typed source observations actually attached to it. Multiple
precedence results can share an allocation, and cloned initializer nodes can
share their source observations. The exporter retains an immutable association
chain until the module closes. It does not infer this link from overlapping
source ranges or recycled pool addresses.

These links describe the exported compiler phase. A typed call may disappear
when a constant short-circuit condition is lowered. Absence of a link means
that no associated node was exported; it does not by itself prove execution,
purity, reachability, or a particular reason for removal. The expression-link
capability is therefore `partial`.

`H node <id> binding <ordinal> source-binding 0` identifies an actual written
variable/member occurrence retained by that node. The ordinal counts B records
from one across the invocation. `ACC` gives its parser-selected use role:
`read`, `write`, `read-write`, `address`, `byref`, or `callee`. A field write
does not classify its containing pointer or index as written. Passing BYREF
does not assert that an unknown callee mutates the object. Uncovered binding
routes have no ACC and must retain an unknown use role. ACC remains available
in bindings-only exports.

Argument nodes now have the boolean `default-argument` property. It captures
whether the actual argument was absent before optional initializer cloning.
This is independent of `passing-mode=default`, which means the formal selects
the passing convention. An explicit value equal to the default therefore
remains distinguishable from an omitted argument.

SOP records identify `file-open`, `file-close`, `file-seek`, `file-get`,
`file-put`, `file-lock`, `file-unlock`, `file-rename`, and `line-input` after
their actual grammar route succeeds. Their ST/STE owner supplies the statement
extent, configuration, module, and source context; OWN links connect generated
operations with that owner. Identifier lookalikes and graphics GET/PUT routes
do not acquire a file-operation classification. These observations describe
accepted source operations, not runtime success or external effects.

The parser also supplies `goto`, `gosub`, `gosub-return`,
`gosub-return-label`, `procedure-return`, `on-goto`, `on-gosub`,
`on-error-set`, and `on-error-clear`. The `statement-control-operations`
capability describes this vocabulary in full, bindings and expression modes.
The actual dispatch determines the observation: RETURN follows its selected
language options, while the ON parser distinguishes error-handler changes and
computed GOTO/GOSUB. Bound label references retain the same ST owner and target
identity. Macro expansions retain their accepted operations even when no
physical reporting range exists. This classification does not itself prove
reachability, runtime return success or resource cleanup.

CAP records describe availability per module and export mode. `available`
means the named observation family is supplied under the documented contract;
`partial` means consumers must account for uncovered routes or phases;
`unavailable` means this artifact does not supply that analysis. In particular,
external-call effects and alias analysis remain unavailable. END only confirms
artifact completion and totals, and must not be used as a semantic capability
claim. Full AST/default-origin facts are unavailable in bindings/expression
modes, while source constructs and selected statement operations remain.

The Python reader and the editor decoder reject dangling expression/binding
links, invalid access roles, malformed capability values, and invalid statement
operation owners. The corpus reductions distinguish source typing from retained
calls and verify source access roles without enabling debug information.

## Committed OPTION occurrences and opening tokens

The full model advertises optional `option-base-occurrences` and
`statement-opening-tokens` capabilities. Bindings/expression-only modes mark
these unavailable. Existing schema-27 clients may ignore the new OPT keys,
CAP names and LOC role; the schema and AST-code version remain unchanged.

Each parsed OPTION route adds `OPT <committed CTX> language-default
option-statement-<ST id> <kind>`. Kind 1 means the BASE route; kind 0 means
another OPTION route. BASE adds `base-statement-<ST id>` with the actual
parser-consumed scalar. Its ordinary context `base` agrees. ST's own CTX is
the entry configuration, before the declaration changes its default. The
occurrence keys remain unique even when repeated directives change nothing.
Recovered statements are not accepted declarations and must retain their
STE outcome and unsuccessful artifact status.

These facts observe the existing numeric-token `CLng` text conversion. They
do not create expression nodes for OPTION operands or alter accepted syntax,
default bounds, diagnostics or generated code.

`LOC statement <ST id> opening-token ...` describes the independently saved
lexer opening token when it has a raw source location. It is serialized
after STE for existing streaming readers. A macro operand can invalidate the
whole range without invalidating the physical opening token. A generated
keyword has no invented token location: MR/MI ancestry retains its actual
physical invocation instead. Logical #Line remapping uses the existing
FILE/SRC physical-coordinate mapping. These token extents are not substitute
whole-statement ranges and do not claim source coverage for generated tokens.

`option-defaults.bas/.bi` exercise includes, option routes, repeats, macros,
conditionals, logical remaps and actual array lower bounds. The focused tests
also cover operand-macro tokens in both byte orders of UTF-16/UTF-32 and
byte-identical ordinary emission across the selected compiler backends.

## Original formal passing modes

Full schema-27 models advertise `formal-passing-modes`. The parameter parser
records `K symbol <original-id> formal-accepted-mode <1..4>`,
`formal-written-mode <0..2>`, `formal-declaration <DCL-id|0>` and
`formal-role <prototype|definition>`. Accepted values are FB_PARAMMODE:
BYVAL, BYREF, BYDESC and VARARG. Written zero means no mode word was consumed;
one/two mean an actual BYVAL/BYREF token, including macro expansion.

The original parameter exists before prototype merging or callback interning.
Its K receipts must not be moved to a canonical G replacement. Existing
formal-span-kind marks independent written membership; hidden receivers have
no such grammar. An unlocated generated name can have DCL zero and retain
`MR symbol <id> <expansion> formal-name` without a fabricated source range.
Physical names retain their original declaration LOC even when another part
of the formal is generated. Compact modes mark the capability unavailable
and omit these receipts. No grammar, defaults, warnings or generated code
are changed by these optional observations.

## Direct source writes and FOR counter initialization

Full schema-27 models advertise optional `for-counter-writes` and
`direct-source-writes` capabilities. Compact models mark them unavailable.
FOR's existing-variable counter occurrence now retains the initialization
write that this parser previously lowered directly without an access hook.
Bounds, steps and actual body uses keep their independent read observations.
The generated loop does not read the counter's incoming value. The existing
rejection of a BYREF/dereference counter remains unchanged.

`K symbol <variable-id> direct-source-write <0|1>` is a refreshable presence
flag, not an occurrence count or an edit location. Explicit zero is exported
for unmodified variables and live parameter slots. One records a parser-owned
direct destination: ordinary/compound assignment, FOR initialization, SWAP,
READ, INPUT, LINE INPUT, GET destination/byte-count outputs, MID or LSET/RSET.
Capturing before runtime lowering keeps this evidence even when a destination
subsequently becomes a BYREF argument. An arbitrary application BYREF call
does not establish a write.

The flag also covers generated identifiers with no physical binding extent.
No B, LOC or editable range is fabricated for a replacement token's logical
point. A parameter's compiler-inserted string/reference dereference is
distinguished from a pointer target by its actual declared/lvalue type and
parameter-variable attributes. Field, index and explicit pointer writes do
not mark the containing input variable. Existing ACC roles, alias and external
effect coverage remain partial rather than acquiring unsupported claims.

Direct-write storage belongs to the serial module observation lifecycle,
uses checked allocation under the identity limit and is released/reset with
the access arrays. Live G body-variable refresh publishes final zero/one
state before procedure storage is released. These optional observations
change no grammar, defaults, warning policy, runtime ABI or generated code.
Independent tests cover scalar/string/UDT copies, native destinations,
generated counters, shadowed locals, opaque calls, rejected BYREF counters
and byte-identical off/full/compact output on each selected backend.

## Accepted FOR counter identities

Full schema-27 output has optional `for-counter-identities` availability and
one `K symbol <variable-id> for-counter:<statement-id> local|existing` receipt
for each accepted FOR. The variable is the parser-selected counter before
initialization lowering; `local` describes a declaration made by that header.
Statement-specific keys preserve multiple loops using the same variable.
They are additive generic K properties, not new AST codes or record tags.

`LOC statement <statement-id> for-counter` identifies the captured counter
token where physical coordinates are supported. It is emitted after STE so
existing streaming readers have the required statement end. The corresponding
MR role records expansion ancestry for generated tokens without inventing a
physical binding range. Normal statement/body and G/H variable ownership
remain independent facts; a matching name alone is not a counter identity.
Compact bindings/expressions exports mark this capability unavailable.

Disabled/full/compact emission and scope/macro observations are verified on
GCC, GAS64 and LLVM. Existing BYREF-counter and explicit descriptor-modifier
grammar rejections remain unchanged. This observer neither changes loop
behavior nor adds alias or call-effect analysis.

## Parsed numeric suffix observations

Full models advertise `CAP module parsed-numeric-suffixes available`. Compact
models advertise it as unavailable and do not emit these observations. Schema
27 is unchanged. A module's global namespace owns optional K properties named
`parsed-numeric-suffix-ID`, where ID is a module-lifetime detail identity.

The escaped property value contains five tab-separated fields:

    source-context, configuration-context, macro-invocation-or-zero, native-dtype, exact-suffix

Only alphabetic suffixes consumed by the native numeric lexer are retained.
The parser emits a receipt when it constructs the numeric literal. Decimal
exponent letters and hexadecimal digits are not suffixes. Macro arguments
expanded before being discarded/stringified, inactive branches and inline
assembly operands do not become numeric-literal observations.

`LOC source-context source-ID property-name ...` attaches the original physical
token range when available. `MR symbol namespace-ID invocation-ID property-name`
attaches generated literal origins. Both use existing record domains so older
schema-27 clients can retain or ignore the properties without a format change.
Consumers must check the advertised coverage, identities and captured source
revision. A macro's replacement text alone does not prove parsed meaning.

## Numeric assignment destinations

Full models advertise `CAP module numeric-assignment-targets available`.
Compact models advertise it as unavailable. Accepted ordinary assignments,
numeric function-result assignments and scalar initializers attach two
properties to the original RHS expression:

    K expression expression-ID assignment-target-dtype raw-dtype
    K expression expression-ID assignment-kind assignment-or-initializer

The pair supplements the expression's own type. It must not replace `NT value`
or E's original dtype. The observer captures the RHS identity before assignment
conversion or TYPEINI lowering consumes its AST. `Y` supplies target-selected
widths, including native INTEGER. Primitive numeric scalar targets are covered;
pointers, enums, aggregates, user-defined LET, compound updates and generated
assignments are excluded. A valid reader requires both properties, an existing
expression and primitive target type, and consistent repeated values.

`CAP module numeric-function-result-inputs available` establishes that
`RETURN value` and `function-name = value` retain the parsed input before
result conversion. The actual result lvalue supplies the selected destination
type. BYREF returns store addresses and do not provide numeric conversion
receipts. The generated ABI result variable is never a named source destination,
including targets that implement aggregate returns through a hidden parameter.
Constructor calls and nonnumeric results do not acquire numeric properties.
Compact modes advertise this coverage as unavailable. A producer that omits
the capability provides unknown function-result coverage even when it supports
ordinary numeric assignments.

Schema 27 readers must accept expression-domain K properties to consume this
extension. Older readers that restrict K to symbols and nodes reject it. The
updated reference reader and linter reader validate the new domain explicitly.

## Named source assignment destinations

Full models advertise `CAP module source-assignment-targets available`.
Compact modes advertise it as unavailable. Accepted ordinary assignments,
compound updates and declaration initializers can retain the exact storage
symbol selected by source grammar:

    K expression expression-ID source-assignment-symbol symbol-ID
    K expression expression-ID source-assignment-kind assignment-or-initializer

This pair supplements the original RHS expression. It does not replace the
expression's own symbol, type or value. The target can be a source variable,
constant, parameter or field. A procedure-body parameter access can name its
compiler parameter variable; the corresponding `G` record provides the exact
source formal. Explicit pointer dereferences, property setters, temporaries and
other generated storage do not claim a named destination.

Readers require both properties, an existing target symbol in the same module,
and an accepted assignment kind. A compiler-origin variable is valid only when
it is the parameter variable of a retained source formal. Numeric assignments
can also carry the independent `assignment-target-dtype` pair described above.

## Original source assignment inputs

Full models advertise `CAP module source-assignment-inputs available`.
Compact modes advertise it as unavailable. Successful ordinary and compound
assignments retain both original typed `E` identities before the assignment
builder consumes, clones or converts their ASTs:

    K symbol procedure-ID assignment-input:statement-ID:ordinal left-E<TAB>right-E<TAB>kind<TAB>code<TAB>selection<TAB>target-S
    H symbol procedure-ID symbol procedure-ID assignment-input:ordinal statement-ID

The six payload fields are percent escaped in the serialized TSV. `kind` is
`copy` or `compound`. A copy uses `assign`; a compound update uses the normal
operation code, such as `add` or `concatenate`. `selection` is `builtin` with
target zero, or `overloaded` with the selected operator procedure identity.
For a compound update, this is the arithmetic operation selected before the
final store. A subsequent LET overload remains represented by the native call
graph and does not replace that arithmetic target in this input receipt.
These observations describe parser assignments. Declaration initializers,
function result assignments and destructuring destination slots retain their
existing independent receipts and are not included in this count yet.

Every observed statement closes with a count, including statements with no
ordinary or compound assignments:

    K symbol procedure-ID assignment-count:statement-ID count
    H symbol procedure-ID symbol procedure-ID assignment-count statement-ID

The count follows `STE` and all of that statement's input properties. Ordinals
are contiguous from one; zero-input statements have no input properties. The
compiler bounds each count at 2,097,152. Readers bound their own work and
storage by the actual row inventory rather than allocating from that count.

The operator token has `LOC statement statement-ID assignment-operator:ordinal`
or an `MR` attachment with the same role. Physical operator locations are
emitted while their statement is active; this specific role can precede `STE`.
The source context must belong to the statement's module. Macro operators use
their captured invocation instead of invented physical coordinates. A newly
observed destination `E` may use a logical operator anchor; its physical flag
is false, so consumers must use the separate operator or statement origin for
reporting.

Both readers require complete statement counts, a property/marker bijection,
the original operator origin, same-module procedure ownership and exact
`OWN expression` membership for both operands. An overloaded target must name
an operator signature in that module. These facts do not establish storage
equivalence, purity, adjacent execution or safe source edits by themselves.

## Original assignment storage trees

Full models advertise `CAP module assignment-storage-trees available` and
retain the two original typed trees of every accepted assignment input:

    K symbol procedure-ID assignment-storage:statement-ID:ordinal left-root<TAB>right-root<TAB>node-count
    H symbol procedure-ID symbol procedure-ID assignment-storage:ordinal statement-ID
    K symbol procedure-ID assignment-tree:statement-ID:ordinal:node-ID payload
    H symbol procedure-ID symbol procedure-ID assignment-tree:ordinal:node-ID statement-ID

Each node payload has 16 percent-escaped fields: raw class, raw dtype,
subtype symbol, selected symbol, conceptual operator, operator options, byte
offset, index scale, conversion flag, float-narrowing flag, constant-conversion
flag, constant kind, constant value, left child, right child and coverage.
Coverage is `complete` or `opaque`. Unsupported operations retain an opaque
typed leaf. Numeric power calls preserve their selected base and exponent
when the parser receipt identifies built-in arithmetic; unrelated calls stay
opaque. This distinguishes actual operations without interpreting callee names.

An `OFFSET` address is a leaf atom identified by its symbol and byte offset.
The executable AST retains an original operand for address optimizations,
but that operand is not a separate computation in the metadata forest. Both
child identities of an exported address atom are zero. Procedure pointers
and static scalar/array addresses use the same representation; exporting a
backend optimization child would violate the forest's atomic node contract.

Node identities are local to one assignment pair and contiguous from one.
Children follow their parent identities. The two roots own disjoint trees;
every node must be reachable exactly once. Each pair is bounded at 65,536
nodes, with opaque leaves at depth 64. These metadata trees are independent
of executable `N` roots, which optimization can remove for a self-copy.

`LOC` or `MR` with role `assignment-destination:ordinal` retains the destination
opening token. This role can precede `STE`; legacy LET starts at its operand,
and macro destinations retain their invocation origin. Both readers validate
the property/marker bijection, original typed roots, complete input coverage,
module ownership and destination origins. Compact modes publish none of these
trees and advertise the capability as unavailable.

Constant IDX offsets can absorb the entire index. Its metadata retains an
explicit zero index child in that case. This is an observation-only node;
the executable AST is unchanged.

Full models also advertise `assignment-initializers available`. Scalar DIM,
VAR and STATIC initializers use assignment-input kind `initializer`, captured
before type checking converts the original value. These require a declaration
statement and built-in assignment. Arrays, reference aliases and aggregate
initializers retain their separate contracts.

## Original LET destination inputs

Full models advertise `CAP module let-destination-inputs available`. Each
accepted modern LET write uses assignment-input kind `let` and the existing
original storage forest. The selected operation remains built-in or overloaded.
Its parser-owned slot is recorded as:

    K symbol procedure-ID let-slot:statement-ID:assignment-ordinal field-slot<TAB>field-symbol
    H symbol procedure-ID symbol procedure-ID let-slot:assignment-ordinal statement-ID

Field slots follow declaration order and count omitted destinations. Assignment
ordinals count actual writes only. Both readers require one slot property and
marker per LET input, increasing slots within the statement, the original RHS
field symbol and the same module. LET statements use the pointer-or-assignment
parser route. Each invocation bounds destination slots at 65,536. Compact modes
advertise this capability as unavailable and publish no LET inputs.

Equal destination trees do not prove equal addresses when an index or pointer
loads mutable storage between writes. Consumers must establish address stability
before treating repeated trees as the same destination.


## Original formal default inputs

Full models advertise `CAP module formal-default-inputs available`. Compact
modes advertise it as unavailable. When a source formal accepts an optional
default expression, its symbol retains the original parsed E identity before
the optional-value tree is cloned, converted or folded:

    K symbol parameter-ID formal-default-expression expression-ID

The parameter's `G` record remains authoritative for its procedure, ordinal,
passing mode and optional flag. Its existing `H ... default-initializer` edge
describes the retained AST node. The new K property describes the original
source input, which can differ after conversion. Variadic parameters and
omitted caller arguments do not produce this property. Readers require a
source parameter, an optional non-variadic G slot, the default-initializer
relationship and an expression from the same module.

Built-in conversion intrinsics also retain `EX cast` links to their original
operands before constant conversion destroys those nodes. Explicit CAST uses
the same operand relationship. These facts describe the selected operation;
they do not infer a programmer's intended rounding policy.
Full models advertise these original operand links through
`CAP module explicit-cast-inputs available`; compact modes mark it unavailable.

## Selected size-query inputs

Full models advertise `CAP module size-query-inputs available`. Compact modes
advertise it as unavailable. LEN and SIZEOF can discard an unevaluated operand
and fold the result into a constant. The parser preserves the selected input
in five expression K properties:

    size-query-kind       len or sizeof, as originally parsed
    size-query-dtype      original input raw dtype
    size-query-subtype    nominal symbol ID, or zero
    size-query-operand    original expression ID, or zero
    size-query-input      type, expression or array

The array kind distinguishes an indexless array's total storage from a scalar
pointer query. An indexed array element remains an expression. Type-only forms
have no operand expression. LEN overloads retain their selected nominal input;
the observer does not convert them into built-in storage queries. An operand
identity describes the parsed expression, not its runtime evaluation. SIZEOF
can observe a function call or dereference that it never executes.

Readers require the complete property set, an existing expression in the same
module, valid input type/subtype and operand identities, and consistent repeated
values. Expression K support is required, as for numeric assignment receipts.
The observer leaves normal code generation unchanged and excludes inline
assembly's size-query grammar. Existing AST expression association chains also
retain contributing query provenance through precedence unwinding and folding:

    H expression result-ID expression original-query-ID size-query-source 0

These links describe an original query contributing to the observed parser
result, not equal values. A folded operation can reuse an operand allocation.
The original query precedes its result and belongs to the same module. Readers
require its complete input receipt. Module-owned ID storage and a bounded
association scan preserve ownership and reject resource exhaustion explicitly.

## Unevaluated query parser intervals

Full models advertise `CAP module unevaluated-query-inputs available`.
Compact modes mark it unavailable. TYPEOF and SIZEOF discard parsed operands,
including folded casts and nested call arguments which the surviving AST can
no longer reach. The parser records the complete expression-ID interval issued
while accepting each query input:

    K expression input-ID unevaluated-query-input 1
    K symbol namespace-ID unevaluated-query-range-detail-ID first-ID%09last-ID%09typeof|sizeof

The second payload has three tab-separated fields, escaped as usual on the
wire. `typeof|sizeof` denotes one query kind. The owner is the module's global
namespace. Every ID from first through last must exist in that module and
retain its immutable input marker. Nested queries can repeat a marker and
have distinct range identities. Empty intervals produce no group.

Readers reject orphan markers, missing interval members, invalid bounds,
foreign owners, duplicate ranges and unsupported query kinds. The compiler's
one-million-expression limit bounds every interval; reader work limits also
bound overlapping nested intervals. These observations describe parsing roles
without changing AST nodes or generated code. Ordinary dynamic-string LEN
inputs remain evaluated and do not acquire these TYPEOF/SIZEOF markers.

## Selected target wide literal prefixes

Full GCC models advertise `c-target-wide-literal-prefixes` as available.
Full GCC, GAS and GAS64 models also advertise `target-wide-literal-prefixes`.
The original C capability remains available only for GCC, so older readers
retain its existing meaning. Compact modes and unsupported backends mark the
general capability unavailable. Each literal WSTRING variable retains three
symbol properties:

    literal-target-wide-unit-bytes  1, 2 or 4
    literal-target-wide-kind        terminated or unterminated
    literal-target-wide-prefix      zero or more eight-digit uppercase hex units

The prefix stops before the first null unit. GCC uses its literal decoder and
selected target width. GAS and GAS64 decode the actual octal byte stream
returned by their shared HESCAPEW emitter into target x86 WCHAR units. They
append a complete zero WCHAR after that stream. The original `C wide-units`
payload remains separate: host wide units do not establish the runtime target
representation.
For example, a supplementary scalar uses a surrogate pair on Win32 but one
unit on a target with four-byte WSTRING units. Invalid scalars retain the
backend's actual emitted representation rather than an assumed replacement.
Backend differences remain visible. For example, GAS retains both explicitly
escaped surrogate units on a UTF-32 target, while the C builder's stored
literal length can limit that same input to its first unit. The observations
do not normalize or change those emitted bytes.

Readers require all three properties on a literal variable, its original
wide value, the module capability and matching primitive WSTRING width.
Each prefix unit must be nonzero and fit the selected width. Payload and
cumulative work limits bound validation. Either available capability establishes
coverage. This observation leaves C and assembly emission unchanged and does
not infer the contents of mutable wide storage.

## Accepted SELECT CASE inputs

Full models advertise `select-case-inputs` as available. Compact models mark
it unavailable. The parser retains accepted selection and clause ownership
before lowering consumes the operand trees. Properties are attached to the
owning procedure, or the global namespace when no procedure owner exists:

    select-case-input:construct-ID
        header-statement-ID, original-selector-expression-ID, selected-storage-symbol-ID, normal|constant
    select-case-clause:statement-ID
        construct-ID, clause-ordinal, else-flag, alternative-count
    select-case-alternative:statement-ID:alternative-ordinal
        construct-ID, value|range|is, original-comparison-operation, first-expression-ID, last-expression-ID, last-flag
    select-case-end:construct-ID
        clause-count, else-flag

Each payload is tab-separated and escaped on the wire. Ordinals start at one.
Ordinary value and IS alternatives have no last-expression ID. Ranges retain
both original bounds. ELSE clauses have no alternatives and must be final.
The original comparison operation uses FBC's AST relation codes 45 through 50;
value and range alternatives use equality code 45. The last flag retains the
parser's branch-lowering choice, including its relevance to floating comparisons.
AS CONST preserves original inputs before integer conversion and jump-table bias.

The SELECT construct, accepted SELECT/CASE statements, original E identities,
their OWN relationships and selected storage must belong to the same module.
SELECT and CASE statements retain the procedure owner of their SELECT construct;
using another procedure's statement or repeating an identity under another owner
does not form a complete observation group. Wire property order is independent
of parser clause order, which is defined by the accepted statement IDs and ordinals.
End counts distinguish a complete selection from missing clauses or alternatives.
The ordinary parser's 1024-entry table is checked before indexing; AS CONST
retains its separate 8192-slot jump-table limit. These facts describe grammar
ownership and selected storage, not inferred side effects or constant coverage.

Full models additionally advertise `select-case-lowering-inputs`. Compact models
mark it unavailable. Every ordinary alternative then has one selected comparison
receipt, or two for a range. Every AS CONST alternative has a converted constant
receipt, and its SELECT has a final table receipt:

    select-case-comparison:statement-ID:alternative-ordinal:bound-ordinal
        construct-ID, branch-operation, jump|fallthrough, scalar|narrow|wide|unicode|unclassified,
        selected-left-dtype, selected-right-dtype, selected-left-expression-ID, selected-right-expression-ID
    select-case-constant:statement-ID:alternative-ordinal
        construct-ID, case-conversion-dtype, converted-first-value, converted-last-value, initial-bias
    select-case-table:construct-ID
        case-conversion-dtype, selected-storage-dtype, final-bias, span, occupied-slot-count

Selected comparison operands have logical E anchors and belong to the CASE
statement. Scalar observations occur after FBC's operand coercion. Text observations
occur before the comparison becomes a runtime call followed by an integer test.
The selected dtypes describe the actual operational types, which can differ from
the E snapshot type for character dereferences or bit fields. Generated operand
snapshots keep distinct identities even when two casts have the same visible E
shape. They do not introduce a parsed binary EX operator for CASE grammar.

`jump` means that the emitted relation accepts this alternative; `fallthrough`
means that it rejects the alternative. Range lower bounds use a less-than
failure branch. Upper bounds use greater-than failure for the last alternative,
or less-than-or-equal success otherwise. The same last-alternative inversion
applies to value and IS clauses. Consumers must preserve this disposition for
unordered floating inputs instead of assuming a negated relation has the same
meaning. Overloaded or recursively coerced UDT comparisons retain an explicit
`unclassified` receipt with zero types and operand IDs.

Converted AS CONST values and biases are canonical unsigned 64-bit decimal
bit patterns. Negative constants retain sign extension; these values are not
32-bit wire identities. Subtraction of a bias uses modulo 2^64 arithmetic.
The initial bias is the first converted value minus 8192, before final table
rebiasing. The final bias and span describe the actual selected table. Its
occupied-slot count excludes gaps. An ELSE-only table has zero bias, span and
slots. Complete readers reject missing bounds, mixed modes, foreign operand
owners, duplicate slots, out-of-range conversions and inconsistent table totals.

## Parsed addresses and storage families

Full models advertise `parsed-pointer-addresses` and `parsed-storage-families`
as available. Compact models advertise them as unavailable. These additive
schema-27 properties preserve parser selections before lowering consumes the
original operands or turns storage operations into runtime calls.

Accepted built-in `@`, `VarPtr` and `StrPtr` results retain five properties:

    K expression result-ID pointer-address-kind address-of|varptr|strptr
    K expression result-ID pointer-address-dtype original-input-dtype
    K expression result-ID pointer-address-subtype original-input-subtype-or-zero
    K expression result-ID pointer-address-operand original-expression-ID
    K expression result-ID pointer-address-temporary 0|1

The input precedes its result and belongs to the same module. The temporary
flag identifies storage backed by a compiler temporary or a constructed
temporary. Zero means that temporary storage was not established. It is not
a lifetime guarantee. Built-in addressing is observed after overload selection;
an overloaded address operator does not acquire these built-in properties.
Procedure addresses continue to use their ordinary typed expression facts.
As with size queries, an observed address in an unevaluated operand does not
prove runtime execution. The observer walks only the selected storage base,
with checked depth and cumulative work limits.

Each accepted `New` attaches seven properties to its canonical pointer temporary:

    K symbol temporary-ID memory-new-kind scalar|array
    K symbol temporary-ID memory-new-dtype selected-pointee-dtype
    K symbol temporary-ID memory-new-subtype selected-pointee-subtype-or-zero
    K symbol temporary-ID memory-new-elements unsigned-count|unknown
    K symbol temporary-ID memory-new-clear 0|1
    K symbol temporary-ID memory-new-placement 0|1
    K symbol temporary-ID memory-new-placement-operand expression-ID-or-zero

The count is the compiler-selected constant element count after conversion,
when available, bounded by unsigned 64-bit storage. It is not a guessed heap
extent. The pointee type must agree with the temporary's pointer type. Clear
records the selected initialization mode; placement records an explicit target
address. The original placement pointer expression is retained before lowering;
nonplacement construction has a zero operand identity. Its module and pointer
type must agree with the parsed placement flag. User-defined allocation operators retain this parsed selection without
acquiring a claim about their allocation behavior.

Accepted `Delete` retains `K expression original-operand-ID memory-release-kind
scalar|array` before destruction and release lowering. A generated runtime free
call alone cannot distinguish `Delete` from a parsed `Deallocate` call. Consumers
must follow original expression and statement identities rather than generated
procedure names to make that distinction.

Readers require complete property groups, valid domains and types, consistent
repeated values, and identities in the same module. Observers retain no AST
ownership and do not change generated code. Regression checks cover all six
source encodings on Win32, Win64 and Linux x64, byte-identical disabled/full
generation, compact availability and rejected incomplete receipts.

Full models also advertise `parsed-expression-origins`. An expression can
retain its immediate earlier parser observation using:

    H expression result-ID expression predecessor-ID parsed-expression-source 0

The predecessor is a contribution on the same surviving AST allocation,
including precedence unwinding and folding. It is not an equivalent value or
storage alias. A single checked predecessor per result keeps this extension
linear in the number of observations. Chaining these links lets a consumer
recover an original address operation inside an unevaluated size operand.
Readers reject self/forward links, foreign-module identities and nonexpression
domains. Compact models mark this capability unavailable.

Full models advertise `parsed-macro-expressions`. Replacement token positions
can run backwards relative to physical caller positions. Typed expression
observations therefore use the actual invocation name as a logical anchor when
available, with E physical zero. MR records retain the consumed macro origins;
EX retains its own operator token location. Distinct operations at the same
invocation are distinguished in adjacent-fact deduplication by parser observation
identity. This anchor never advertises editable expansion bytes.

Invocation snapshots contain fixed LEX_LOCATION storage, are module-owned and
released at reset. Monotonic IDs support binary lookup; ancestor traversal is
limited to 64 steps and all lookup work is bounded by 8,388,608 comparisons.
Allocation and work exhaustion fail the model rather than publishing guesses.

Full models also advertise `parsed-compound-operators`. The assignment parser
captures the original op= inputs. The AST builder observes its selected binary
result before assignment conversion and before tree optimization consumes it.
E uses a logical operator anchor, EX retains the actual written operator range,
and accepted primitive numeric targets use the existing assignment receipt.
The original astNewSelfBOP calling interface remains intact; an explicit parser
entry point supplies observations without hidden global parser state. Neither
observer retains AST ownership or changes generated target code. Compact models
mark both capabilities unavailable.

Full models advertise `selected-numeric-operands`. For accepted binary operations
on primitive numeric inputs, four expression K properties retain the operands
chosen by the compiler after coercion and before constant folding or shift-count
normalization:

- `numeric-selected-left-dtype` and `numeric-selected-right-dtype`
- `numeric-selected-left-expression` and `numeric-selected-right-expression`

The selected E observations retain the converted types and any compiler-known C
values. EX continues to reference the original operands. Each selected E belongs
to the same module and precedes the result; its range is a logical operator anchor,
not editable source. This distinguishes comparison operand types from the final
Boolean result without requiring consumers to reproduce integer promotions.
Readers require the complete group and reject foreign, forward, nominal or pointer
inputs and inconsistent selected types. Compact models mark the capability
unavailable. The observer borrows AST inputs without retaining ownership, and the
original astNewBOP interface remains available for existing callers.

Full models advertise `parsed-numeric-literals`. The numeric-atom parser records
four expression K properties: `numeric-literal-kind` (integer/float),
`numeric-literal-base` (decimal/hex/octal/binary), `numeric-literal-text` and
`numeric-literal-text-complete` (0/1). These belong to the observed typed E/C
literal. A named constant or folded calculation does not inherit literal origin.
Canonical token text excludes suffixes and may normalize leading zeroes or the
exponent marker. It does not advertise original spelling. Text at or above the
lexer's 64-character numeric limit is marked incomplete for exact-decimal proofs.
The wire bound is the existing 1024-character token buffer. Both readers enforce
complete, consistent groups, module ownership and radix character domains;
consumers can impose stricter mathematical parsing for a particular proof.
Parsed directive conditions are observable; skipped directive bodies are not.
The observer does not change AST ownership or generated code. Compact modes mark
this capability unavailable.

Selected numeric operands also cover the primitive Boolean domain. Its Y
entry describes byte storage while its semantic values are normalized to -1
and 0. Boolean storage is not an unrestricted signed-byte integer range.

Parsed-pointer-dereferences retains a complete expression K pair:
pointer-dereference-operand and pointer-dereference-count. Full exports with
`pointer-access-origins` also emit an independent `H` relation from the result
expression to its original operand, with role `pointer-dereference-input` and
the dereference count in its final field. Readers require matching properties,
source statement ownership and one origin for every group when this capability
is available. Earlier producers remain readable without promising this coverage.
The parser records
the typed pointer input before astBuildMultiDeref can cancel an address or
consume its AST. The result retains its own E range. The count is 1 through 8,
does not exceed the input's pointer depth, and agrees with the result type and
nominal subtype. Input identities are earlier and belong to the same module.
Implicit UDT conversions to pointers do not establish this observation.

Procedure-prototype-statements retains symbol K properties named
procedure-prototype-statement-<statement-id>, with that statement ID as value.
Each accepted prototype retains its canonical procedure owner and actual ST
identity independently of DCL coordinates. A wholly expanded signature may
have no editable DCL range; its statement's MR/MI records still identify the
real invocation. Repeated prototypes retain independent statement keys.
Both readers check domains, class, key/value agreement and module ownership.
Both new capabilities are unavailable in compact models.

Parsed-pointer-indexes retains pointer-index-operand and pointer-index-index
as a complete expression K pair. Both inputs are earlier E identities from the
same module. The first has pointer type; the second is the numeric index before
integer conversion and byte scaling. Only built-in pointer indexing supplies
this receipt. Array indexing, string indexing, overloaded [] and inactive
source do not. A subsequent field selection can change the result type, so the
receipt does not assert that the final E type equals the indexed pointee type.
Neither AST pointers nor borrowed source buffers survive the observation.
Full exports provide this capability; compact exports mark it unavailable.
With `pointer-access-origins`, the result also owns a `pointer-index-input`
relation to the original pointer expression. Its final field is the original
index expression ID. Both readers check agreement with the property pair,
backward identities and common accepted statement/module ownership. The origin
and property group must cover each other exactly. Compact exports advertise
`pointer-access-origins` as unavailable and emit neither origin family.

Selected-call-argument-inputs retains the original caller expression on each
accepted argument before formal conversion and later AST optimization. The
node K property call-argument-expression names its original E identity. Its
argument node retains the selected formal symbol and actual call ownership.
Omitted/default arguments have no caller input receipt. The argument stores an
observation identity without retaining or borrowing the original AST pointer.
Both readers check node class, non-default status, formal/call ownership, module
closure and consistency with expression links. Cached owners and traversal
budgets bound validation of linked argument lists. Full exports provide this
capability; compact exports mark it unavailable. This observes arguments and
does not assert that an external procedure has any particular runtime effect.

Full schema-27 exports advertise `scalar-for-step-inputs`. Each accepted
numerical FOR counter has `K symbol <id> for-step-explicit:<statement-id> 0|1`
and `for-step-expression:<statement-id> <expression-id>` observations. An
implicit step has expression zero. An explicit step identifies its original
parser expression before counter-width and signedness conversion, with the same
module and statement owner as the `for-counter` receipt. The same variable can
control several statements. Pointer counters and UDT iteration operators have
different contracts and do not use this numerical receipt. Compact exports mark
the capability unavailable. Independent compiler checks passed 20 cases on
Win32, Win64 and Linux x64 in six encodings, with byte-identical generated C.

Full schema-27 exports advertise `file-transfer-inputs`. An accepted selected
GET/PUT runtime call has three expression K properties: `file-transfer-kind`
(`get` or `put`), `file-transfer-storage` (`scalar` or `array`), and
`file-transfer-operand` (its earlier original E identity). The operand preserves
the typed object before BYREF AS ANY or array-descriptor lowering. Whole arrays
retain their NIDXARRAY wrapper; individual elements are scalar inputs. Scalar
String overloads transfer characters, so a receipt does not assert that a
descriptor or a pointed-to allocation was transferred. Call anchors use the
last accepted token when a statement intrinsic has closed its expression range;
the original operand has its own precise range.

Both readers validate complete groups, selected runtime families, input shape,
module/statement ownership and full coverage of surviving selected call nodes.
Source-expression links to executable N snapshots distinguish actual transfers
from calls parsed in unevaluated SIZEOF inputs. Compact exports mark this
capability unavailable. Compiler observation checks passed 20 cases across
Win32, Win64 and Linux x64 in six encodings with byte-identical generated C.

Full models additionally advertise `pointer-index-lvalues`. Built-in pointer
index receipts include accepted assignment targets and compound writes, even
when those enter variable parsing without an active cExpression range. The
parser supplies its observed prefix anchor for that path. Original pointer and
index identities are retained before scaling; source-expression associations
survive target cloning and assignment lowering. Ordinary arrays, string
indexing and overloaded indexing do not establish these receipts. Compact
models mark this coverage unavailable. The expanded pointer observation suite
passed 56 cases across Win32, Win64 and Linux x64 in six encodings, including
store-target links and byte-identical generated C.

Full models additionally advertise `scalar-for-step-sites` and
`enum-for-step-inputs`. Explicit scalar STEP clauses have a compiler-observed
`LOC statement <statement-id> for-step ...` location, or a macro reference
with that role when the clause is expanded. A macro invocation does not create
an editable STEP range. Enum counters use the same complete input receipts as
ordinary scalar counters. Omitted clauses have neither STEP location form.

`scalar-for-bound-inputs` retains symbol K observations named
`for-start-expression:<statement-id>` and `for-limit-expression:<statement-id>`.
They preserve the original typed input before conversion to counter storage.
Each belongs to the same module and FOR statement as its counter receipt.

`scalar-for-selected-steps` retains `for-step-selected-dtype:<statement-id>`
and `for-step-selected-expression:<statement-id>`. The latter names the selected
converted constant E/C value, or zero for a dynamic step. An omitted step
selects constant one. A known original step must have a selected constant.
The selected constant has a logical FOR anchor rather than an invented physical
expression range. Width and signedness reflect actual counter conversion:
for example, STEP 0.4 becomes integer zero and Byte STEP 128 becomes -128.
Compact models mark these capabilities unavailable. Independent compiler
checks passed 20 cases across Win32, Win64 and Linux x64 in six encodings,
with byte-identical generated C and converted enum/byte step coverage.

Full models advertise `parsed-string-intrinsics`. Parsed CHR, WCHR, UCHR,
TRIM, LTRIM and RTRIM results have complete expression K observations named
`string-intrinsic-kind`, `string-intrinsic-count`, `string-intrinsic-any` and
`string-intrinsic-argument-<ordinal>`. Ordinals start at one and identify the
original typed inputs before literal folding or runtime conversion. CHR inputs
therefore retain values outside the selected byte range. ANY identifies the
parser-selected character-set form, rather than an inferred pattern spelling.

Each result also has an H relation from its module namespace with role
`parsed-string-intrinsic`. This preserves occurrence coverage when folding
turns the entire operation into an ordinary literal. Both readers require the
complete group and marker, earlier input identities, common module and
statement ownership, and a compatible selected runtime or literal result.
Compact exports mark the capability unavailable. Independent compiler checks
passed 20 cases across Win32, Win64 and Linux x64 in six encodings, including
macro anchors, decoded escaped patterns and byte-identical generated C.

Full models advertise `string-initializer-targets`. An accepted fixed ZString,
WString or String element initializer retains expression K properties named
`string-initializer-symbol`, `string-initializer-dtype` and
`string-initializer-bytes`. The symbol is the selected variable or field, the
type is its original full dtype, and the byte count is one element's declared
storage. ZString and WString capacities include terminator storage; fixed String
has its own padding semantics. Array dimensions do not multiply the element
capacity. BYREF binding initializers and primitive type-name spelling do not
establish character-buffer initialization.

An H relation from the target symbol to the original expression, with role
`string-initializer`, retains occurrence coverage after TYPEINI lowering or
static emission consumes the original tree. Readers require the complete group
and marker, matching target storage, module closure and accepted statement
ownership. Compact models mark the capability unavailable. Compiler checks
passed 20 cases across Win32, Win64 and Linux x64 in six encodings, including
shared objects, record fields, array elements, macro literals and unchanged
generated C.

### Diagnostics for rejected modules

`-semantic-diagnostics <file>` publishes an independent invocation artifact.
The compiler reports its actual diagnostics through the same observer used
by the semantic model. A rejected module can complete this artifact without
publishing authoritative AST facts. Diagnostic output neither enables recovery
facts nor changes the compiler's exit status. The option can be used alongside
`-semantic-model`, with distinct output destinations.

The tab-separated format starts with `FBCDIA`, version `1`, and the compiler
version. Text fields use the semantic model's percent-escaped byte encoding.
Record tags remain literal ASCII. Record layouts are:

| Record | Fields after the tag |
| --- | --- |
| SRC | sequential source ID, source path |
| M | sequential module ID, root source path, target ID |
| CAP | current module ID, capability name, availability |
| DI | sequential diagnostic ID, current module ID, severity, compiler code, parser context, source path, displayed line, message, detail, custom text, physical-point flag, physical line, zero-based physical column |
| END | format version, module count, source count, diagnostic count, error count, compilation-succeeded flag |

Sources retain observation order and unique path spelling. Every module root
and diagnostic source must have a source record. Diagnostics belong to the
current module. Severity is `error` or `warning`; flags are `0` or `1`. A
complete artifact can end with compilation-succeeded `0`. Successful compilation
has no error diagnostics. A reader supplied the compiler exit code checks that
the outcome agrees. Readers reject incomplete records, invalid identities,
counts, escaped fields and inconsistent physical points before exposing facts.

Module capabilities use optional `CAP` records. The signature capability is
`procedure-signature-mismatches`, with availability `available`. A module can
advertise it once; its module ID must be the current module. Complete signature
classification requires coverage in every module. Older schema-1 artifacts
without this capability remain valid diagnostic transports. They cannot prove
the absence of signature conflicts. A `procedure-signature-mismatch` diagnostic
without its own module's capability is invalid.

The independent `variable-case-collisions` capability observes actual variable
rejections in the native duplicate checker. Each module advertises it once;
complete coverage requires all modules. A `variable-case-collision` DI kind
requires its own module's capability. Older artifacts without it remain valid
transports but do not establish the absence of these collisions.

The symbol allocator retains the raw spelling supplied to a live variable,
including variables lowered from procedure formals. The duplicate checker
selects the actual conflicting variable after applying scope and legacy suffix
rules. A different original spelling classifies that rejected binding only.
Accepted EXTERN reuse, REDIM, shadowing and suffix-distinct bindings remain
quiet. Constants, procedures, reserved symbols and fields do not supply this
variable proof. The parser scopes the kind around its actual redefinition
error and retains the current identifier point across continued declarations.
A written preceding token can anchor a generated name; wholly generated
declarations remain logical observations without editable coordinates.

The serial parser thread owns a hash index of live native identities. Each
payload owns its immutable borrowed hash key, spelling, canonical name and
symbol-table guard. Native removal unlinks the hash item before deleting the
payload; module and invocation completion release every remaining payload.
Temporary variables are excluded. Collection is bounded by 100,000 live
bindings, 64 MiB of retained name bytes and 20 million bucket comparisons per
module. Exhaustion prevents diagnostic publication and preserves the prior
destination. These observations leave duplicate acceptance and code emission
unchanged; the existing public duplicate-check signature is preserved.

Displayed lines can be affected by `#line`. Ordinary physical points use the
last consumed token only when its source and displayed line agree with the
reported diagnostic. A scoped declaration point instead retains the parsed
procedure name, including its original physical line in a continued header.
A written header-start token can anchor a generated name. Fully generated
headers remain logical observations when no physical token is available.
These are diagnostic points, not editable expression ranges. Missing physical
points have a zero flag, line and column; no range is reconstructed from text.

Parser contexts distinguish unknown LINE INPUT character-buffer capacity,
NEXT counter binding failures, NEXT variable mismatch and native procedure
signature conflicts. Consumers use these contexts rather than console text.
The signature classification covers rejected prototype additions and actual
prototype/body type, result, calling convention, parameter and BYREF-result
checks. Prototype comparison uses selected types and subtypes, formal modes,
descriptor ranks, result contracts, explicit aliases and optional defaults.
Legal overloads remain accepted. Exact duplicate declarations remain ordinary
redefinition diagnostics. Default-argument warnings are not error proofs.

Header parsing can repair an unknown type or malformed parameter. Such a
header cannot prove a signature conflict. The independent writer tracks native
identities of invalid retained procedure headers for the current module. A
valid newly allocated symbol at a reused address removes the obsolete entry.
The tracking array grows geometrically from 32 entries, is bounded by 100,000
identities and has a two-million-comparison work budget per module. Exhaustion
prevents publication of incomplete evidence. Module and invocation completion
release this state. This diagnostic observation does not change legacy repeated
prototype acceptance or any ordinary compiler check.

The writer bounds output to 64 MiB, 5,000 sources and 100,000 diagnostics. It
uses checked staging and protects source, backend and other semantic output
paths before publication. Invocation state is released on completion or
compiler restart. Both independent and native readers enforce those bounds;
the native reader grows retained diagnostics in blocks and uses a sorted
source index for repeated path lookups.

Compiler checks passed 75 cases across Win32, Win64 and Linux x64, including
six source encodings, valid and rejected modules, unchanged generated C,
coexisting sidecars and protected output collisions. Both readers agreed on
65 genuine, malformed and growth cases. Native linter selections passed
20 encoding, target, include and suppression cases.

### Original loop condition inputs

Full schema-27 models advertise `loop-condition-inputs`. Accepted WHILE and
DO/LOOP statements retain a paired property group on their owning procedure:

| Property | Value |
| --- | --- |
| `loop-condition-kind:<statement-id>` | `while`, `do`, `do-while`, `do-until`, `loop`, `loop-while`, or `loop-until` |
| `loop-condition-expression:<statement-id>` | original expression ID, or zero for bare DO/LOOP |

Conditional statements additionally retain `H symbol <procedure-id> expression
<expression-id> loop-condition <statement-id>`. The parser observes the accepted
predicate before branch construction can convert or fold it. Zero observations
for bare grammar distinguish unconditional statements from missing observations.
Readers require matching statement ownership, procedure and module closure,
complete property groups, condition markers and actual WHILE/DO construct links.
Compact models mark the capability unavailable and omit these observations.

Independent compiler checks passed 20 cases across Win32, Win64 and Linux x64
in six encodings, including macros, continuations and constant predicates.
Generated C remained byte-identical. The production native reader and independent
Python reader also rejected 18 malformed loop observation groups.

### Original IF and ELSEIF conditions

Full schema-27 models advertise `if-condition-inputs`. Every accepted IF or
ELSEIF header retains its original predicate before `astBuildBranch` can
convert it, invoke an overloaded conversion or fold the branch:

| Record | Value |
| --- | --- |
| `K symbol <procedure-id> if-condition-expression:<statement-id>` | Original predicate expression ID |
| `H symbol <procedure-id> expression <expression-id> if-condition <statement-id>` | Independent occurrence marker |

The statement's native opening token distinguishes IF from ELSEIF. Single-line,
nested and legacy GOTO forms use the same facts. Constant predicates keep their
original typed values even when the lowered branch no longer reads them.
Both readers require exact expression ownership, a parsed compound statement,
a procedure owner in the same module, and matching property and marker records.
Coverage includes every accepted IF and ELSEIF when the capability is available;
removing both records therefore cannot turn a missing predicate into valid data.
Compact models advertise unavailable coverage and omit these observations.

### Original and selected array subscripts

Full schema-27 models advertise `array-subscript-inputs`. Each accepted fixed
or dynamic array access retains these properties on its selected element E:

| Property | Value |
| --- | --- |
| `array-subscript-symbol` | resolved array variable or field symbol ID |
| `array-subscript-rank` | accepted rank, from one through eight |
| `array-subscript-index:<dimension>` | original input E before integer conversion |
| `array-subscript-selected-index:<dimension>` | converted integer input E before offset scaling |

Dimensions are zero based. A distinct selected-index E records the compiler's
integer conversion but has a nonphysical source range; consumers must use the
original index E for source queries. A no-op conversion can reuse the original E ID.
`H symbol <array-id> expression <element-id> array-subscript 0` closes the group.
Parser locations anchor both reads and assignment destinations independently of
expression frames. The observations precede descriptor indexing, multiplication
by element size and field selection; none must be recovered from byte offsets.

Readers require complete rank-sized groups, A/S rank agreement, selected element
type agreement, integer selected indices, statement and module closure, ordered
input IDs and one matching relation. Unknown descriptor bounds remain unknown.
Compact models mark the capability unavailable and omit the groups.

Compiler checks passed 20 cases across Win32, Win64 and Linux x64 in six source
encodings. Generated C remained byte-identical. Both readers rejected 26 malformed
groups with valid completion totals. Cases include array fields, macro accesses,
unknown-rank formals, nested indices, floating conversion and assignment targets.


The optional full-model `array-bound-query-inputs` capability retains native
LBOUND and UBOUND occurrences before fixed-array folding. Each query owns four
immutable `K expression` properties: `array-bound-kind` (`lower` or `upper`),
`array-bound-symbol`, `array-bound-dimension` and
`array-bound-selected-dimension`. `H symbol <array> expression <result>
array-bound-query 0` preserves occurrence membership independently of its value.
The original dimension is observed before the compiler converts it to INTEGER;
the selected dimension records that actual conversion. Their expression IDs
satisfy original <= selected < result and share accepted statement and module
ownership. An omitted dimension is the compiler's generated one. The result has
INTEGER type whether it is a constant or a runtime call. Array identity also
covers fields and descriptor formals; these facts do not claim allocation,
empty storage or execution. Compact exports mark the capability unavailable.
Both readers check group closure, marker membership and converted types.

Full exports with `array-storage-inputs` retain the storage selected before
array indexing and bound-query folding. Each result has three expression
properties: `array-storage-symbol`, `array-storage-kind` (`variable` or
`field`) and `array-storage-receiver`. Plain arrays use receiver zero; their
canonical symbol identifies the storage. Fields retain a positive native node
ID describing the original containing record address. The selected field alone
does not distinguish arrays belonging to two different objects.

`H symbol <array> expression <result> array-storage-selection 0` independently
records every group. Field inputs also have `H expression <result> node
<receiver> array-storage-input 0` and `H symbol <array> node <receiver>
array-storage-root 0`. A receiver root belongs to the array field and its
accepted statement. All descendants share that statement and module, and none
has an execution phase. These borrowed parser snapshots observe addresses;
they do not establish that a dereference or element access executes. Exporting
them neither clones nor retains compiler AST allocations.

Unindexed field prefixes keep the receiver ID in the module's expression
association chain until a bound query consumes the storage. The query and its
prefix can refer to the same snapshot. Both readers require exact agreement
between properties and independent relations, and complete storage coverage for
all subscript and bound groups when the capability is available. Unknown keys,
duplicate groups, missing roots, foreign ownership and phased snapshots are
rejected. Earlier producers remain readable without promising this capability.
Compact exports advertise it as unavailable and omit storage observations.

The optional full-model `procedure-exit-labels` capability exports
`H symbol <procedure> symbol <label> procedure-exit-label 0`. The label is
the procedure AST's actual shared exit target, including native cleanup paths.
It is not inferred from RETURN or EXIT spelling. Readers require a signature,
a label in the same module, unique membership and a CL receipt in the body's
phase. Compiler-generated module constructors may keep their label in the
module's symbol table; compiler origin and phase membership validate that case.
Compact exports mark this capability unavailable. It does not prove that an
exit executes, that OPEN succeeds or that a callee closes any resource.
Compiler checks passed eight cases on Win32 and Linux x64, with byte-identical
generated C. Both readers passed 439 schema cases, including nine exit-label
closure and compatibility cases.

The optional full-model `string-initializer-copy-contracts` capability adds
`K expression expression-ID string-initializer-copy kind` to each accepted
fixed-character initializer group. Actual TYPEINI runtime lowering and static
emission select `runtime-terminated`, `runtime-counted`, `runtime-wide`,
`static-bytes` or `static-wide`. Clones selecting different contracts remain
`ambiguous`; unused static storage and other unlowered initializers remain
`unselected`. The compiler owns one bounded byte per expression and retains no
AST pointers in this index. Binding-only and expression-only exports omit it.
Readers require complete copy coverage only when this capability is available;
legacy groups without the capability remain accepted. Unknown kinds and a copy
property under an unavailable capability are rejected.

Counted fixed String storage differs from zero-terminated character storage.
Static narrow literal emission preserves embedded zeros, while a runtime
literal input stops at its first zero. BOM-marked Unicode source selects wide
literals; narrow conversion of a static wide literal containing a zero needs a
separate payload proof. Eight compiler checks passed on Windows and Linux with
byte-identical generated C. Both readers passed 447 schema cases.

The optional full-model `sequence-tail-branch-contracts` capability adds
`K node root-ID sequence-tail-branch-node child-ID` and an actual
`CE conditional-label` edge for an operator at the final right-child tail of
an AST LINK sequence. Every LINK child on that path is evaluated unconditionally.
The target must be a root label in the same phase. A checked temporary bitmap
owns at most one MiB per phase and distinguishes internal IIF labels from CB
destinations. Internal expression flow, additional transfers and assembly stay
unknown. Binding-only and expression-only exports omit these node groups.
Readers require complete properties, evaluation receipts, phase membership and
the matching selected conditional edge when the capability is available.
Legacy models without this capability and its groups remain accepted.

This closes a CFG gap in string comparisons lowered through cleanup sequences.
All consumers see the actual edge; predecessor proofs cannot mistake its target
for a block with only one fallthrough predecessor. Eight compiler checks passed
with byte-identical generated C on Windows and Linux. Both readers passed 459
schema cases, including damaged groups and legacy capability handling.

## Original declaration typing inputs

Full models advertise `declaration-typing-inputs`. Variable declarations handled
by the DIM, STATIC, COMMON and EXTERN grammar, and CONST declarations, retain a
separate group for each accepted parser dispatch. REDIM operations, FOR counter
syntax, auto-typed VAR and ordinary instance fields are outside this group.
Each group uses the procedure identity, or the global namespace when there is
no procedure. Its opening detail position supplies the group identity:

```
K symbol owner declaration-type-group:group statement-ID<TAB>kind
K symbol owner declaration-type-input:group:ordinal symbol-ID<TAB>written-type<TAB>initializer-kind<TAB>source-context-ID
K symbol owner declaration-type-end:group count
```

The payload tabs are escaped by the normal K wire encoding. Ordinals start at
one and completion covers every accepted symbol in the original list. A leading
AS clause belongs to every symbol in that list. Per-symbol AS clauses and active
identifier suffixes remain separate parser choices. Variable choices are `as`,
`suffix` and `implicit`; constant choices are `as`, `suffix` and `inferred`.
The choices are captured before default typing or initializer conversion changes
the selected data type. An ignored suffix in modern FB does not become a type.

Initializer kinds are zero for no explicit initializer, one for ANY, two for
an accepted value or aggregate initializer, and three for an actual scalar
constant whose selected value is zero, one or signed minus one (including
Boolean true). Unsigned maxima and all-ones pointer addresses are value inputs. The third kind requires
one TYPEINI assignment with the same full type and subtype as its root and no
array dimensions. This preserves the simple-value style exception using native
values, including folded constants and casts. A one-field record retains its
aggregate initializer semantics. The inspection neither changes nor owns ASTs.
CONST inputs always have initializer kind two.

An identifier's actual location is attached to its source context with the
input key as its LOC role. SRC preserves the include spelling; FILE supplies
the captured canonical path for reporting. Available expansion origins use MR
on the group owner with the same role. A macro's final replacement token can
lack an individual physical location or expansion identity. The accepted group
statement remains an honest reporting anchor; it does not supply an editable
identifier range.

Each original parser formal carrying `formal-span-kind` also retains
`K symbol original-parameter formal-written-type as|suffix|implicit|vararg`.
These identities precede prototype merging and callback interning. Canonical G
records alone cannot recover the original grammar choice. Varargs retain their
own kind; compiler-created receivers have no written-formal observation.

Readers require complete and unique groups, bounded canonical identities,
contiguous ordinals, same-module statements, symbols and source contexts, valid
classes and initializer choices, and full written-formal coverage. Compact
exports advertise this capability as unavailable and omit these observations.
Older models without the capability remain valid and do not establish a type
spelling policy. All observation state belongs to the existing parser thread;
no AST allocation, parser choice, emitted instruction or ABI is changed.

## Original procedure typing inputs

Full models advertise `procedure-typing-inputs`. Each accepted named prototype
or definition retains its original result-type grammar before canonical
procedure reuse. Definitions also retain the written visibility before
`OPTION PRIVATE`, `OPTION PUBLIC` or member defaults select the final scope:

```
K symbol selected-procedure procedure-typing-input:identity statement-ID<TAB>role<TAB>kind<TAB>result-form<TAB>exported
K symbol selected-procedure procedure-written-visibility:statement-ID default|public|private
```

The first key uses a unique detail identity. Its payload tabs use normal K
escaping. Roles are `prototype` and `definition`; kinds are `sub`, `function`,
`property`, `operator`, `constructor` and `destructor`. A SUB has result form
`none`; a FUNCTION retains `as`, `suffix` or `implicit`. Other procedure kinds
use `special`. The export flag is zero or one at that accepted occurrence.
An inactive identifier suffix in modern FB does not become a written type.

These observations belong to individual accepted headers. A typed prototype
and an implicit definition can share the same selected symbol while retaining
different forms. Completed T/F metadata supplies the selected type, kind and
actual lexical owner; it cannot reconstruct the original grammar. In the same
way, a final private T visibility does not prove that PRIVATE was written.
Anonymous callback signatures do not create named-header observations.

Readers require unique bounded identities, accepted same-module statements,
selected procedure symbols and kinds, and visibility for every definition.
Prototypes have no written-visibility record. Named DCL/OWN receipts and accepted
procedure statement routes establish complete coverage. Compact exports mark
the capability unavailable and omit both properties. Older models without
the capability remain valid but cannot establish these spelling policies.
The existing parser thread owns the observations; no AST ownership, language
choice, emitted instruction or ABI changes.

## Scalar dynamic String declarations

Full models advertise `scalar-string-declarations`. A `K symbol` property
`scalar-string-declaration` stores four tab-separated fields: accepted statement
ID, original initializer expression ID, grammar kind (`dim`, `static`, `field`),
and source-context ID. Expression zero denotes the compiler's default
initialization. Nonzero expressions retain their original `OWN` statement and
typed `E`/`C` values before descriptor assignment conversion or field-constructor
cloning. Calls retain their expression identity without a fabricated value.

Only scalar, non-reference, non-const dynamic String storage is covered. VAR
inference, arrays, fixed strings, character buffers and pointer storage have
different initialization contracts. DIM/STATIC facts agree with the original
declaration-typing group. User field facts agree with the accepted aggregate
body and are complete for `B declaration` field history. The parser records
default fields as well as explicitly initialized fields. Local STATIC String
storage currently rejects an explicit initializer; failed compilation never
publishes an accepted model.

The identifier location is a `LOC source-context` role named
`scalar-string-declaration:<symbol ID>`. An expanded identifier instead retains
its `MR symbol` origin under `scalar-string-declaration`. Bindings-only and
expressions-only models advertise unavailable coverage and omit these facts.

## Aggregate bodies and written access sections

Full models advertise `aggregate-access-sections`. The accepted TYPE/UNION
body retains its opening statement and the complete sequence of explicit
PUBLIC, PRIVATE and PROTECTED sections from the member grammar:

```
K symbol aggregate aggregate-body-statement opening-statement-ID
K symbol aggregate aggregate-access-section:ordinal section-statement-ID<TAB>public|private|protected
K symbol aggregate aggregate-access-count count
```

Ordinals start at one within each body; counts include zero for bodies without
explicit sections. The section payload uses normal K tab escaping. Named and
anonymous aggregates retain separate owners. A UNION has no accepted access
sections. The body's opening ST owns its TYPE/UNION BLK; each section ST belongs
to that same BLK and uses the accepted `aggregate-member` route. Completed T/U
metadata supplies the selected aggregate class and type/union layout.

These observations preserve source order after macro expansion and active
conditional selection. A nested body does not change its parent's current
visibility. Default public access remains implicit: the first written section
is a separate policy choice from a repeated explicit section. Readers require
unique body ownership, one completion count, contiguous unique ordinals and
complete section coverage. Reordering K facts cannot change their logical order.

Compact exports advertise the capability as unavailable and omit these
properties. Older models without the capability remain valid but do not prove
an access-section policy. The existing parser thread owns the sequence counter;
the observations add no AST ownership, grammar choice, emitted code or ABI change.

## Accepted declaration repetitions

Full models advertise `declaration-repetition-inputs`. Each accepted EXTERN
variable, TYPE alias and named prototype has an occurrence-specific property:

```
K symbol selected-symbol-ID redeclaration-input:occurrence-ID statement-ID<TAB>extern|typedef|prototype<TAB>repeated<TAB>source-context-ID
```

The four payload fields use ordinary K percent escaping. The occurrence ID is
bounded by the detail stream and must be unique. The repetition flag is zero
or one. Owner classes are variable (1), typedef (13) and procedure (3),
respectively. Each occurrence belongs to an accepted statement in its source
context and module. This records the parser's decision before the earlier
declaration state is refined or an unused prototype header is discarded.

An EXTERN repetition has compatible selected types and exactly the earlier
array rank and fixed bounds. An unknown rank becoming known is a refinement,
even though both declarations resolve to the same final symbol. Aliases also
compare their selected storage extent and fixed-character flag. Accepted
legacy prototype repetitions use the compiler's existing result, convention,
formal mode/rank and optional-default matching. Discarded procedure and formal
symbols retain canonical relationships to the selected prototype; their
original DCL names remain observations of their own declarations.

Physical identifier sites use `LOC source-context` with the occurrence's
property key. Generated names use a distinct `MR symbol` origin under that
same key. The `redeclaration-` prefix avoids the macro recorder's deliberate
deduplication of ordinary `declaration-` symbol origins. Generated aliases can
lack an editable B range: their native declaration origin and occurrence
origin instead identify the accepted macro statement and real invocation.

Readers require complete EXTERN group and named prototype coverage, all
physical alias declarations, and generated alias declaration origins. A
repetition must have an earlier declaration of the same selected symbol.
Binding and expression exports advertise unavailable coverage and omit these
properties. Older models remain valid but do not prove declaration repetition.
The observations do not change accepted syntax, emitted code or ABI selection.

## Original callback convention inputs

Full schema-27 models advertise `procedure-callback-inputs`. Native parsing
records each anonymous FUNCTION/SUB type occurrence after selecting its
procedure-pointer symbol, including occurrences sharing one interned type:

```
K symbol selected-callback-ID callback-convention-input:identity statement-ID<TAB>explicit<TAB>source-context-ID
K symbol selected-procedure-ID procedure-abi-input:identity statement-ID<TAB>prototype|definition<TAB>LIB<TAB>ALIAS<TAB>source-context-ID<TAB>callback-count
```

Payload tabs use ordinary K percent escaping. Flags are zero or one. The
explicit flag records the active native calling-convention choice. In
particular, `-z no-fastcall` and `-z no-thiscall` can disable a written choice.
It does not describe the containing procedure's convention. Callback counts
include anonymous types parsed in that original header before canonical
procedure reuse; definitions have a zero LIB flag. Named type aliases parsed
in earlier statements retain their own occurrences.

The callback site is the consumed FUNCTION/SUB keyword. LOC identifies a
physical or remapped source point; generated tokens instead retain an MR
origin at the actual macro invocation. Procedure ABI sites use the original
procedure name. Readers require unique bounded occurrence IDs, selected
procedure-pointer owners, same-module accepted statement/source ownership,
and complete ABI receipts matching procedure-typing-inputs and callback counts.
Property order does not determine ownership or occurrence identity.

Named header receipts require a parsed statement ending. An indirect SUB cast
can consume and publish a void call while the statement dispatcher retains
its unmatched route. Callback type receipts on that route require a typed
indirect void-call AST in the same statement and a reachable selected callback
signature. This includes nested callback formal/result types. An unused receipt,
a different signature, or an unmatched named header cannot use this exception.

Compact models mark this capability unavailable and omit these properties.
Older full models remain valid but cannot establish callback explicitness.
The existing parser thread owns the module counter, reset at module start
and bounded by the 16000000-detail limit. These facts retain no native symbol
pointers or AST ownership and do not change calling-convention selection.

## Ordered aggregate source fields

Full models advertise `aggregate-field-order`. Every finalized aggregate with
`declared-field-count` also retains a complete one-based native member list:

```
K symbol aggregate-ID declared-field:ordinal field-ID
```

The list walks the accepted native member table, preserving positional field
order explicitly. Symbol identity allocation and byte offsets do not define
this order. It includes promoted anonymous source fields and excludes hidden
base storage, dynamic descriptors, Static variables and procedures, as does
the existing source-field count. Zero fields produce a count and no entries.
Only finalized snapshots establish completeness. Observations borrow the
native table during serialization; no symbol pointer survives the walk.

Both readers require complete bounded ordinals, unique source fields, final
layout, exact member counts and matching native owners and modules. Compact
modes advertise unavailable coverage and omit the list. Older models without
the capability remain valid; their field counts alone do not prove order.

FBL-DECL-024 consumes this list, declaration-typing groups, source routine
ownership and ordinary constant assignment receipts in the pre-load AST.
It reviews a single uninitialized DIM followed by one assignment per field
in the same parser parent, compound and procedure. Blank lines, comments,
inactive code, continuations and colon separators do not invent statements.
Labels, calls, declarations and control boundaries interrupt the sequence.
Aliases, namespaces and macro expansions keep their actual native identities.

The existing plain-integer-record policy excludes constructors, destructors,
methods, Static/nested members, default field initializers, arrays, bit fields,
unions, inheritance, overlap and non-integer field storage. RHS values must be
original compiler-valued constants; variable reads, calls and op= updates do
not establish this recommendation. Compiler-generated module procedures do
not turn module-level DIM into a user routine local. Findings are collected
before bounded publication, with declaration LOC or macro origin sites.
Style defaults and explicit selection retain the original activation policy.

## Native procedure object symbol observations

Full models advertise `procedure-object-symbol-observations`. Source procedure
snapshots, excluding procedure-pointer signatures, carry the optional properties
`procedure-object-symbol-state` and `procedure-object-symbol`. GAS/GAS64 report
`observed` with the exact cached object symbol, or `unobserved` with an empty name.
The exporter reads the name already selected by native emission. It does not call
the mangler, allocate identifiers, or retain a symbol pointer beyond its lifetime.
Consumers use the final snapshot for each symbol ID.

C and LLVM currently report `unsupported-backend` with an empty object name.
Their compiler identifiers can differ from the actual object symbols because of
assembler aliases, target decoration, or LLVM quoting. Compact models omit these
properties and advertise unavailable coverage. Older models remain readable;
missing properties mean unknown.

An observed object name identifies a procedure for later linker observations.
It does not prove a reference, an undefined symbol, a missing implementation,
or the absence of an implementation in another object or library. Actual link
resolution requires the complete native link inputs and linker decisions.

## Independent native link diagnostics

`-semantic-link-diagnostics <file>` requests a separate `FBCLNK` schema-2
artifact. It works without `-semantic-model`; a rejected compilation or failed
link does not make a recovered AST acceptable. Completion describes the native
invocation outcome, including an unattempted link under `-r` or `-c`.

Records are ASCII TSV with the same uppercase percent byte escapes as FBCSEM.
The header is `FBCLNK, 2, producer-version`. Readers also accept schema 1,
which has no native-access flag and cannot establish a declaration reference.
The remaining schema-2 record fields are:

| Record | Fields after the tag |
| --- | --- |
| `M` | module ID, source path, native target ID, backend enum |
| `FILE` | revision ID, module ID, opened source path, lowercase SHA-256, byte length |
| `SRE` | revision ID, `verified` |
| `P` | procedure ID, module ID, kind, first accepted header role, spelling, object-name state, object name, opened source path, physical-point flag, raw line, raw column, native-access flag |
| `ARGS` | native link arguments before adding the observation helper or generating a response file |
| `U` | callback ID, native failure kind, exact callback symbol/library argument |
| `TOTAL` | module count, source revision count, procedure count |
| `LINK` | attempted flag, completed flag, native tool exit, callback coverage, tool path |
| `END` | schema version, callback count, whole-invocation success flag |

Modules, source revisions and callbacks have serial IDs. Procedure IDs are
allocated at accepted native headers and emitted on native symbol release;
release order can differ from allocation order. Repeated includes retain
distinct revisions. Source hashes are observed and verified through FBC's
already-open source streams. `#line` never replaces that opened-file owner.
Generated headers and all remaps have no physical-point claim. Physical lines
are one-based and raw lexer columns are zero-based.

GAS/GAS64 copy only an already selected cached procedure object name, with
`observed` or `unobserved` state. C/GCC and C/Clang record the selected assembler
alias or target ABI name at native header emission. LLVM records the selected
global identifier after native quoting and target-prefix handling. These
observations do not change code emission or allocate extra native identifiers.
Procedure-pointer signatures are excluded. A prototype followed by a definition
retains its first accepted header role. A failed compilation can snapshot live
registrations before ordinary parser teardown, without using a recovered AST.
The native-access flag copies the procedure's ACCESSED state before release.
Calls and procedure addresses set that state on the selected symbol. An emitted
prototype alone does not establish a reference, even when another declaration
shares the same ALIAS. The flag can include uses later removed by optimization;
an unresolved finding still requires the actual native linker callback.

GNU ld's `--error-handling-script` interface supplies `undefined-symbol` and
`missing-lib` observations directly. FBC discovers interface availability from
native tool help and calls the same compiler executable as a private helper.
The helper runs before compiler initialization only with the exact callback
arguments and an invocation-owned environment capsule. It appends to an existing
private journal after checking the native open-file identity and journal header.
It never creates or truncates a public destination. FBC restores the preceding
environment after the link and validates the completed journal before publishing.

An existing caller-supplied error helper is preserved and makes callback
coverage `unavailable`. Unsupported tools also retain their ordinary behavior.
A native link failure without classified callbacks remains an unknown failure;
it does not prove that no unresolved symbols exist. An unused prototype is not
an unresolved import, and separate objects or archives may resolve declarations
without a source `LIB` or `ALIAS` clause. Consumers must use the complete native
link inputs and match callbacks to observed object names before claiming a
source declaration is unresolved.

The checked writer protects inputs, includes, emissions, program outputs and
other semantic artifacts. Publication is atomic, and private staging is removed
on success or failure. Limits are 64 MiB per artifact, 16 MiB per callback journal,
5000 source revisions, 100000 procedure/module/callback records and 4096 native
bytes per symbol. Collection and hash lookup also have explicit memory/work
bounds. Readers require source closure, complete totals, supported record
shapes, native exit agreement and a final completion marker before retaining
usable facts. Consumers must verify observed source revisions are still current.

FBL-DECL-013 consumes this independent artifact when fblint is given
`--compiler-link`. Repeatable `--compiler-object`, `--compiler-library` and
`--compiler-library-path` options supply the project's actual native link
inputs and also enable this mode. `--compiler-prefix` selects a native SDK
when the semantic compiler is staged separately from its toolchain. Existing
target, backend, include, define and multithread options apply to both passes.
The linter builds an owned temporary program and never executes that program.

The rule requires a used native prototype, an observed object name, an exact
`undefined-symbol` callback and accepted native source ownership. It verifies raw
source hashes, including BOM bytes, against disk and the scan's source cache
before publishing findings. Unused prototypes, resolved separate objects and
archives do not produce this finding. Missing link inputs, unavailable callback
coverage on failure, old providers and rejected source are analysis failures
when native linking is requested. Remapped/generated declarations without a
physical point use an unlocated finding at line and column zero.

Explicit selection without a link context reports the missing context instead
of recovering the former EXTERN/LIB/ALIAS source heuristic. Ordinary scans
without native link mode do not claim that declaration resolution was checked.

## Original constant and bound call inputs

Full models advertise `parsed-constant-symbols`, `bound-expression-symbols`
and `source-call-bindings`. Compact models mark these capabilities unavailable.
The optional schema-27 properties are:

| Record | Meaning |
| --- | --- |
| `K expression <E> constant-symbol <S>` | Selected CONST symbol at the original constant atom parser |
| `K expression <E> constant-atom-kind boolean-literal` | FBC's predefined Boolean literal symbol |
| `K expression <E> constant-atom-kind named-constant` | A source named constant, including a named string constant |
| `K expression <E> bound-value-symbol <S>` | Original scalar variable or field identity, including an implicit BYREF dereference |
| `H node <CALL> binding <B> source-binding 0` | The selected written callee occurrence before argument parsing |

Constant symbol and kind form a pair. The CONST symbol and original expression
belong to the same module. A Boolean literal has FBC's Boolean type and LITERAL
attribute, independently of its value. A named Boolean constant equal to True
does not acquire literal identity. String literal backing VAR storage does not
establish a bound variable name. Whole arrays, computed indices and explicit
pointer dereferences are excluded from the simple bound-value receipt.

These receipts describe original parser inputs, rather than every result later
associated with the same AST allocation. Consumers follow recorded
`parsed-expression-source` predecessors and inspect original `EX` operations
before accepting an atom. Grouping preserves identity; casts and folded
arithmetic or Boolean operations retain distinct operations. FBC also parses
a sole parenthesized SUB argument as a grouped expression.

Written direct CALL nodes use their native procedure symbol. A virtual CALL's
native symbol is an anonymous function-pointer signature; its `static-target`
relation owns the selected source procedure and interface. Source call bindings
must agree with that interface, rather than the function-pointer signature.
Macro calls can retain actual selected interfaces without a written callee
binding. Their statement ownership and macro invocation records provide native
reporting anchors. Indirect calls do not gain a fabricated written procedure.

Selected ARG nodes already name actual formal slots and retain original
`call-argument-expression` receipts. Their ABI chain can run in reverse source
order. Consumers map arguments by selected formal identity and ordinal, keeping
implicit receivers, variadic tails and omitted defaults distinct. These facts
do not establish call effects or require a change to emitted program code.

## Original enum declarations and members

Full models advertise `enum-declaration-inputs`. Bindings-only and
expressions-only models mark it unavailable and omit these properties:

| Record | Tab-separated property payload |
| --- | --- |
| `K symbol <enum S> enum-declaration-input` | Header statement identity, completed native member count |
| `K symbol <member S> enum-element-input` | Enum symbol identity, one-based member ordinal, member statement identity, explicit-initializer flag, original initializer expression identity |

The payload tabs use the ordinary `%09` field escaping. An implicit member
has flag zero and initializer identity zero. An explicit member has flag one
and the original constant expression identity observed before
`astConstFlushToInt()` consumes its AST. The selected member value remains
available in the member's ordinary `C symbol` record. The original expression
and the selected enum storage value serve different purposes.

These receipts describe the members accepted by `cEnumBody()`, including
anonymous declarations, comma-separated members, included declarations and
expanded macros. A CONST alias with the same enum subtype is not a member.
Non-Explicit enums inside Extern blocks place members in the surrounding
namespace, so a member's native namespace owner need not equal its enum.
The receipt supplies the actual enum independently of namespace placement.

Each declaration's native `U` element count closes its group. Ordinals must
cover that count exactly once, and every member subtype must match its enum.
The header has a native `BLK` enum construct and a parsed declaration or
aggregate-member route. Its first statement token can be PUBLIC or PRIVATE,
so that token alone does not establish the declaration kind. A member uses a
parsed enumerator statement whose parent and construct match that header.
Explicit initializer expressions use the native CONST
node class (16), have a compiler-valued constant, belong to the same module
and retain ownership by the member statement.

Both readers reject missing groups, repeated ordinals, inconsistent counts,
invalid initializer presence, foreign modules and conflicting receipts.
Counts are bounded before allocating ordinal tables. Receipt capture retains
identities in the existing export buffer; it introduces no AST ownership,
symbol-layout change or alteration to accepted grammar or generated code.

## Original IIf operands

Full models advertise `original-iif-inputs`. Bindings-only and expressions-only
models mark it unavailable and omit the following occurrence records:

| Record | Meaning |
| --- | --- |
| `K expression <result E> original-iif-inputs` | Original condition, true-arm and false-arm expression identities, in that order |
| `H symbol <module namespace S> expression <result E> parsed-iif 0` | Independent marker for the completed builtin IIf occurrence |

The property payload contains three decimal identities separated by escaped
`%09` tabs. The parser records the accepted operands before `astNewIIF()` can
convert the condition, lower the branches or discard an unselected constant
arm. Both original arms therefore remain available when the completed result
is a constant. The result receives a distinct expression occurrence even if
folding reuses the selected arm's AST allocation. Its ordinary `NT` and `C`
receipts describe the selected result, rather than the original input type.

Operand identities increase in parse order and precede the result identity.
All four expressions belong to the same parsed statement and module. The
namespace marker belongs to that module. Both readers require exactly one
property and one marker per occurrence, rejecting missing halves, duplicates,
invalid order and foreign statement or module references.

Later expression observations can link to the completed occurrence through
`parsed-expression-source`. A consumer following grouping or cast operands
stops at the original IIf occurrence before following the folded result into
its selected arm. Actual operators and ordinary selected calls remain distinct.
An ordinary function named IIf after `OPTION NOKEYWORD` receives no intrinsic
receipt. Macro and included occurrences retain their native statement and
module ownership; reporting coordinates have a separate role from operand
identity. These records add no AST ownership and do not change accepted source
or emitted program code.

## Original array initializer inputs

Full models advertise `original-array-initializer-inputs`. An accepted array
element retains these expression properties before assignment conversion or
implicit constructor lowering consumes its original input:

| Property | Value |
| --- | --- |
| `array-initializer-symbol` | source array variable or array field symbol ID |
| `array-initializer-dimension` | one-based innermost array dimension |
| `array-initializer-kind` | `assignment` or `constructor` |

The expression's native type remains its original input type. The source array
and its A records supply the selected destination and rank. Grouping remains an
ordinary original EX `group`, including its outer source range. Scalar fields in
an initialized array of records do not acquire an array role; only their actual
array fields qualify. Optional defaults, scalar constructor inputs and generated
padding have no element association. Compact models mark this capability
unavailable and omit the groups.

Readers require complete groups, source symbol origin, accepted statement and
module ownership, a valid array rank and an available capability. The compiler
records immutable identities rather than retaining initializer AST pointers.
The sidecar tests cover 18 original inputs and reject invalid groups across
GAS64, GAS and GCC, with unchanged emitted code and compiler diagnostics.

<!-- end of compiler-semantic-model.md -->
