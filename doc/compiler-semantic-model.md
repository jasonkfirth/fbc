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

## Schema version 25

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
These records describe effective configuration; file/include/expansion source
contexts and a complete source expression graph are separate work in progress.

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
independent reader's `validate_source_revisions()` provides this check.

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

The runners also accept `--backend gas64` for x86-64 assembly and `--backend gas`
for x86 assembly. The compiler-source runner requires a matching Make target,
for example `--backend gas --target-triplet i686-linux-gnu`. The fixture and OMA
runners select the x86 target for gas emission without requiring 32-bit linking.

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

Schema 25 is the current format. It includes the metadata and detail count
introduced by schema 19, stable operator concepts and configuration/source
provenance from later versions, compiler-selected preprocessor facts, and the
macro expansion graph, source constructs, source/AST associations, access roles,
argument default origins, and explicit capability coverage. Readers must explicitly support the schema they accept
and reject unknown versions. Raw encodings require knowledge of the
corresponding compiler family; stable labels, relationships, and target type
facts reduce that dependency.

Fblint was the initial consumer for undeclared-name and variable-type checks.
Existing data-flow and safety checks are consumer policy, not sidecar behavior.
A consumer adopting schema 25 must validate it before using compiler facts as
authoritative, and must continue treating recovery output as provisional.

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

<!-- end of compiler-semantic-model.md -->
