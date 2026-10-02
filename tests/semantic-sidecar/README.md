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

`sidecar.py` independently validates schema 25. `test_sidecar.py` checks the
compiler's records against source locations and known types, layouts, values,
and target relationships. The BASIC fixtures own separate responsibilities:

| Fixture | Coverage |
| --- | --- |
| `bindings.bas` | declarations, qualified lookup, USING, scopes, WITH, labels, assembly |
| `types.bas` | packed/union/bitfield layouts, inheritance, visibility, arrays, qualifiers, literals |
| `procedures.bas` | overloads, canonical formals, defaults, variadics, by-reference results, pointer calls |
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

<!-- end of README.md -->
