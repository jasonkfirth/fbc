# FreeBASIC compiler semantic model

`-semantic-model <file>` asks the compiler to write a tab-separated semantic
sidecar for external tooling. The option is opt-in and does not change ordinary
compiler invocations. It enables source locations during analysis; tools should
still compile to a temporary object when they only need the model. It does not
enable language debug options or change predefined compiler configuration
values such as `__FB_ERR__`.

`-semantic-model-expressions <file>` writes the same versioned header and
completeness footer, but limits successful module output to `M` and `E` records.
This mode avoids retaining the symbol table, procedure summaries, variable
type facts, and serialized AST when a consumer only needs compiler-typed source
ranges.
Its `E` symbol and subtype IDs are zero by design; the numeric dtype and
compiler-formatted type spelling remain available. The full
`-semantic-model` mode is unchanged and continues to include those identities.
The expressions-only mode also records completed results from the binary
precedence parser tiers, unary operators, and the highest-precedence parser
route. This includes useful intermediate ranges such as a typed product nested
inside an addition, plus complete unary expressions and source-visible
member/index/call prefixes. The parser exports the current receiver before
consuming a suffix and each completed prefix after resolving its member or
index, so chains such as `record.child.items(0).value` retain the intermediate
compiler types. Adjacent expression records with identical fields after their
unique ID are written once when several parser tiers return the same result.
The exporter does not merge different ranges, types, identities, or facts
separated by another record. Expression IDs and footer totals count the records
actually written. For
repeated operators at one precedence, each completed left-associated prefix is
recorded before the next operation can fold or replace its AST. For example,
`1 + 2` keeps its own compiler type inside `1 + 2 + value`. These facts are
completed parser results, not a complete graph of compiler-generated
conversions, calls, or other lowering operations.

The format is tool-neutral. Fblint is its first current consumer, and editor
integrations can use the same symbol, type, AST, and source-location facts. The
sidecar is output from a compiler invocation, not a language-server protocol or
an incremental workspace index; an editor integration must manage compiler
arguments, project state, and invocation lifetime itself.

The full semantic model is complete only when it ends with a valid `END`
record. A failed or interrupted compile may leave an incomplete file, which
full-model consumers must reject. The compiler stages each module's records
and commits them only after parsing that module succeeds. This prevents failed
parser retries from leaving stale symbols or AST nodes in the final model.

Expression-only output has a separate, explicitly incomplete recovery form for
compiler-reported parse errors. A recovered module is followed by an `R` record
and the file ends with a `RECOVERY` footer instead of `END`. The footer counts
all committed expression records; it does not claim that a failed module is
complete. Consumers using this form must reconcile each physical range with
their primary parser's invalid or recovered source regions, withhold overlapping
facts, and mark retained facts provisional. The full-model reader must never
treat a `RECOVERY` footer as a valid semantic model. Interrupted processes do
not write this footer and remain invalid.

## Schema version 8

Fields are separated by tabs. Literal percent signs, tabs, carriage returns,
and line feeds inside names and paths are escaped as `%25`, `%09`, `%0D`, and
`%0A`, respectively. Record and field counts include the record tag.

| Record | Fields after the tag | Meaning |
| --- | --- | --- |
| `FBCSEM` | schema version, compiler version | Model header |
| `M` | source path | Successfully parsed module |
| `D` | source path | Unique source file opened by the compiler invocation, including root modules and includes |
| `S` | ID, name, symbol class, data type, subtype ID, scope, attributes, parameter attributes, length, offset, parent ID | Resolved compiler symbol |
| `B` | symbol ID, role (`declaration` or `reference`), physical-range flag, source path, start line, start column, end line, end column | Compiler-resolved identifier occurrence |
| `P` | symbol ID, name, symbol class, data type, subtype ID, start line, end line, source path | Procedure metadata |
| `V` | symbol ID, procedure name (empty for global scope), variable name, stable type kind | Resolved variable type fact (`pointer`, `numeric`, `dynamic-string`, `fixed-string`, `aggregate`, `procedure`, or `other`) |
| `N` | ID, parent ID, child edge, AST class, raw operator, operator code, operator kind, data type, symbol ID, subtype ID, source line, source path | Typed AST node |
| `E` | ID, physical-range flag, source path, start line, start column, end line, end column, AST class, raw operator, operator code, operator kind, data type, symbol ID, subtype ID, source type spelling | Type and resolved identity of one completed parser expression |
| `R` | schema version, expression count for the preceding module | Explicitly recovered module in expression-only output |
| `END` | schema version, module count, procedure count, symbol count, type-fact count, AST-node count, expression count, binding count, dependency count, dependency-complete flag | Completeness marker and record totals |
| `RECOVERY` | schema version, module count, expression count, recovered-module count, binding count, dependency count, dependency-complete flag | Incomplete expression-only recovery marker and record totals |

Binding occurrences preserve the compiler's resolved symbol identity rather
than asking tooling to infer a target from spelling. Coordinates are one-based
lines and zero-based UTF-16 columns with an exclusive end. A physical flag of
`1` is required before an editor may treat the span as editable; macro-expanded
or otherwise nonphysical occurrences remain informational. The parser hooks
cover variable declarations, direct variable/member references, named
procedure declarations and calls, namespace declarations and namespace
prefixes resolved in qualified identifiers, named type/union/enum declarations
and references, typedef declarations and references, constant/enum-element
declarations and references, and field declarations. Compiler-resolved implicit
member references written as `.field` inside `WITH` blocks share the field
declaration's symbol ID. Label declarations and compiler-resolved label
targets are also exported, including numeric labels, direct
`GOTO`/`GOSUB`/`RETURN`, `ON ... GOTO`/`GOSUB` lists, single-line numeric `IF`
branches, error-handler targets, and `RESTORE` labels. An overloaded call is
associated with the exact procedure selected by argument resolution, not
merely the shared overload head. Procedure-address expressions written with
`@` or `ProcPtr` are also associated with the exact overload selected by the
expected or explicit signature. Calls through a procedure pointer and
implicit/generated calls are not represented as direct procedure references.
The record remains an extension point for further compiler-resolved
occurrence kinds, not a claim that every FreeBASIC binding route is exported
yet. The compiler tracks the opened include path
separately from its logical filename; after a `#line` remap, `B` and `E` ranges
are marked nonphysical until that source context ends. Expressions-only output
deliberately omits `B` records.

Dependency records are emitted once per normalized source path in compiler
read order. The root module is included, as are successfully opened include and
preinclude files. At most 5,000 paths are retained. The final `dependency
complete` flag is `1` only when the list covers every source opened by the
invocation; consumers that need to validate source-backed ranges must reject
an incomplete list rather than treating omitted files as unrelated.

The `operator code` and `operator kind` fields provide stable conceptual
operator semantics for external tools. The vocabulary is `assign`, arithmetic
codes (`add`, `subtract`, `multiply`, `divide`, `integer-divide`, `modulo`,
`power`), their `-assign` forms, logical/bitwise codes (`and`, `or`,
`logical-and`, `logical-or`, `xor`, `equivalence`, `implication`,
`shift-left`, `shift-right`, and their `-assign` forms), comparisons (`equal`,
`greater-than`, `less-than`, `not-equal`, `greater-or-equal`,
`less-or-equal`, `identity-test`), unary codes (`not`, `logical-not`,
`unary-plus`, `negate`, `address-of`, `dereference`), and `index`, `cast`,
`convert-to-integer`, `convert-to-float`, `concatenate`, `concatenate-assign`,
`allocate`, `allocate-array`, `deallocate`, and `deallocate-array`.
`operator kind` is `builtin` for a compiler built-in, `overloaded` when the
resolved call target is a user-defined operator procedure, or `none` when the
node is not a recognized operator. A `none` kind is paired with an empty code.
An operator that this compiler does not recognize for export is reported as
`none`; consumers must not infer its meaning from the raw numeric field.
This vocabulary is part of schema version 8 and can grow only through an
explicit schema update.
These concepts describe the operator represented by the exported AST node;
they do not reconstruct source operators removed by optimization. A
constant-folded expression can therefore have an empty code and `none` kind.

Symbol and AST node IDs are unique within one output file. The exporter keys a
compiler symbol by its allocation lifetime rather than its memory address,
because the compiler recycles symbol-table nodes after scopes end. Reusing a
node therefore cannot make a later binding inherit an earlier symbol's ID. A
zero subtype, parent, or symbol ID means that the AST or symbol has no
corresponding reference. Raw AST class, operator, symbol class, type, scope,
attribute, offset, and length values use compiler-internal numeric encodings.
Consumers must not assume that those numbers are a public binary interface.
Schema version 8's conceptual operator fields are the cross-version tooling
interface; the raw AST fields remain optional diagnostic detail and must be
interpreted only for an inspected compiler family. The VS Code consumer keeps
the raw values opaque.

Expression line numbers are one-based and columns are zero-based UTF-16 code
unit offsets, with the end position exclusive. The source type spelling is
produced by the compiler's normal type formatter and is intended for display;
consumers should still use the numeric data type and subtype IDs for semantic
comparisons. A `physical-range` value of `1`
means every consumed token in that parser expression came directly from the
same physical source file; the range can be used for editor navigation. A
value of `0` means macro expansion or another non-physical token participated,
or the reported range failed a physical-line bound check. In either case,
coordinates are informational only and must not be used as an editable source
range. The exporter also checks single-line ranges against the complete
physical line for unmarked source and BOM-marked UTF-8, UTF-16LE/BE, and
UTF-32LE/BE. It reads bounded chunks aligned to the input encoding and counts
editor UTF-16 columns, including two columns for supplementary-plane
characters. A parser cursor that extends beyond the physical line is marked
nonphysical. The check does not use the shorter diagnostic excerpt and fails
closed when the source line is oversized, an explicitly encoded line is
malformed, or the input cannot be read. Its cache uses a physical line count
independent of `#line` remapping, so repeated logical locations do not share a
stale bound. Empty or reversed parser spans are omitted. Full-model `E` records
are emitted at the common `cExpression()` parser
boundary. Expressions-only output additionally records completed binary
precedence results, unary results, and completed results from the
highest-precedence parser route. This includes complete primary, member, index,
and call expressions. Special forms such as `SIZEOF()` and `IIF()` retain their
completed result range, while operands parsed through the common expression
parser also retain their own typed ranges. Nested parser calls can therefore
produce overlapping ranges, for example an arithmetic subexpression inside a
larger expression or a member expression inside a compile-time `TYPEOF()`
assertion.
For cursor queries, consumers should
prefer the narrowest physical expression range containing the position; an
exact selection should use an exact-range match so an operand does not inherit
the enclosing comparison's type. A special parser route may expose only its
completed result and subexpressions that re-enter the common expression
parser. Repeated binary parser chains are captured prefix-by-prefix, but other
constant-folding and lowering paths can still remove intermediate AST
structure before it can be exported. Compiler-generated implementation
details are not source ranges. The record therefore supplements the typed AST;
it is not a complete source-expression graph.

The schema is intended to grow through an explicit version change when record
meaning or layout changes. Consumers should reject unknown schema versions,
unknown record types, incorrect field counts, or mismatched footer totals.

Physical ranges can occur more than once when a header is intentionally
included and parsed under different namespace or macro contexts. If identical
source spans carry contradictory types, a source-only consumer cannot choose
the applicable include instance from the range alone and should withhold that
type rather than rely on record order.

## Current use in Fblint

Fblint validates the header, record shapes, and footer before treating a
successful compiler run as authoritative for undeclared-name analysis. On a
source error, compiler-resolved names inform `FBL310`; Fblint still applies its
existing policy for known external SDK identifiers when the corresponding
header is intentionally absent. Resolved variable type facts also let
`FBL503` distinguish dynamic `String` appends from numeric `+=` operations,
reducing false positives while retaining source-based fallback for expressions
that cannot be matched to a simple variable. Other local checks remain active.
Fblint removes the sidecar and object after processing.

The initial consumer does not yet derive all data-flow or safety checks from
the AST or expression records. Existing rules continue to use Fblint's source
analysis until each rule can be migrated and tested against representative
programs. The symbol, AST, and expression records provide a compiler-owned
basis for static analysis and editor features without changing FreeBASIC source
syntax or existing compiler output by default.
