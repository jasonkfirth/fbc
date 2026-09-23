# FreeBASIC compiler semantic model

`-semantic-model <file>` asks the compiler to write a tab-separated semantic
sidecar for external tooling. The option is opt-in and does not change ordinary
compiler invocations. It enables source locations during analysis; tools should
still compile to a temporary object when they only need the model.

The format is tool-neutral. Fblint is its first current consumer, and editor
integrations can use the same symbol, type, AST, and source-location facts. The
sidecar is output from a compiler invocation, not a language-server protocol or
an incremental workspace index; an editor integration must manage compiler
arguments, project state, and invocation lifetime itself.

The sidecar is complete only when it ends with a valid `END` record. A failed
or interrupted compile may leave a partial file, which consumers must reject.
The compiler stages each module's records and commits them only after parsing
that module succeeds. This prevents failed parser retries from leaving stale
symbols or AST nodes in the final model.

## Schema version 4

Fields are separated by tabs. Literal percent signs, tabs, carriage returns,
and line feeds inside names and paths are escaped as `%25`, `%09`, `%0D`, and
`%0A`, respectively. Record and field counts include the record tag.

| Record | Fields after the tag | Meaning |
| --- | --- | --- |
| `FBCSEM` | schema version, compiler version | Model header |
| `M` | source path | Successfully parsed module |
| `S` | ID, name, symbol class, data type, subtype ID, scope, attributes, parameter attributes, length, offset, parent ID | Resolved compiler symbol |
| `P` | symbol ID, name, symbol class, data type, subtype ID, start line, end line, source path | Procedure metadata |
| `V` | symbol ID, procedure name (empty for global scope), variable name, stable type kind | Resolved variable type fact (`pointer`, `numeric`, `dynamic-string`, `fixed-string`, `aggregate`, `procedure`, or `other`) |
| `N` | ID, parent ID, child edge, AST class, operator, data type, symbol ID, subtype ID, source line, source path | Typed AST node |
| `E` | ID, physical-range flag, source path, start line, start column, end line, end column, AST class, operator, data type, symbol ID, subtype ID, source type spelling | Type and resolved identity of one completed parser expression |
| `END` | schema version, module count, procedure count, symbol count, type-fact count, AST-node count, expression count | Completeness marker and record totals |

Symbol and AST node IDs are unique within one output file. A zero subtype,
parent, or symbol ID means that the AST or symbol has no corresponding
reference. AST class, operator, symbol class, type, scope, attribute, offset,
and length values use compiler internal numeric encodings. Consumers must use
the schema version and must not assume that those numbers are a public binary
interface.

Expression line numbers are one-based and columns are zero-based UTF-16 code
unit offsets, with the end position exclusive. The source type spelling is
produced by the compiler's normal type formatter and is intended for display;
consumers should still use the numeric data type and subtype IDs for semantic
comparisons. A `physical-range` value of `1`
means every consumed token in that parser expression came directly from the
same physical source file; the range can be used for editor navigation. A
value of `0` means macro expansion or another non-physical token participated,
so the reported coordinates are informational only and must not be used as an
editable source range. Empty or reversed parser spans are omitted. `E` records
are emitted at the common `cExpression()` parser boundary. Special built-in
parser routes that do not pass through this boundary are not represented yet,
and constant folding can still remove subexpressions before their type is
exported. The record therefore supplements the typed AST; it is not a complete
source-expression graph.

The schema is intended to grow through an explicit version change when record
meaning or layout changes. Consumers should reject unknown schema versions,
unknown record types, incorrect field counts, or mismatched footer totals.

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
