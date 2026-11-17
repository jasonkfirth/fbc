<!--
Project: FreeBASIC C-emitter corpus qualification
File: c-emitter-sqlite-insertion-phases.md
Purpose: Record SQLite insertion and recursive-query phase repairs.
Responsibilities: Bind source changes, native diagnostics and executable checks.
Whole-engine SQL and publication retain their independent qualification.
-->

# SQLite insertion phases, November 17

SQLite's 555 measured Linux LP64 source profiles reach GCC zero and retain
Clang 7,693. The disjoint GCC census reaches 175, comprising Coreutils 159,
the retained corpus 14 and Templari 2. Compiler sources, fbctests and measured
additional-project and runtime queues retain GCC zero on their recorded cuts.

## Source changes

`sqlite3BtreeInsert` returns the existing overwrite result directly and ends
cursor preparation on error. The remaining insertion phases capture and check
one page before using it. This removes the `didOverwrite` flag and repeated
overwrite predicates while preserving cell construction, replacement, balancing
and cursor-position handling.

The original GCC optimized function contains a null-page trap behind an error
funnel that reloads the address-taken result variable. Its incoming edges
already require errors. The revised phase boundaries remove this funnel.
Before/after helper-error executions check the returned errors and phase order.
The revised complete GCC optimized function contains no trap.

`generateWithRecursiveQuery` retains the checked previous SELECT while searching
for the anchor. A missing predecessor reports an error through the existing
cleanup path. The caller's `hasAnchor` check remains in place. Aggregate-query
rejection, temporary link changes and borrowed ORDER BY/LIMIT restoration keep
their existing behavior.

Changes are confined to these two function bodies in the FreeBASIC SQLite port
at HEAD `c7e29f0daf37a5880b63c7778a8f8b93055cdb5d`. The scoped patch applies
to that HEAD, produces the exact repaired source and passes reverse checking.

## Diagnostic coverage

Both C producers emit both source revisions. All eight emissions succeed.
GCC 14.2 and Clang 19.1.7 compile the two complete modules before and after at
`-O0` and `-O2`, giving sixteen complete native profiles. Both original GCC
`-O2` null observations disappear. Original and repaired Clang diagnostic maps
match at each optimization.

The 555-profile census reuses 553 exact unchanged C identities and their 1,106
native results. Four freshly compiled current native contexts replace the two
changed modules. All 666 current SQLite source/dependency pins are checked.
The complete-module commands retain the aggressive unsuppressed warning flags.

## Executable coverage

The public runner extracts each complete function body, canonical type
declarations, layout assertions and required helper prototypes from its emitted
module. Native observers supply deterministic dependencies using those types.
Both producers have exact C identities for each tested source revision.

Sixteen fresh Linux LP64 executions cover GCC and Clang at both optimizations.
They execute 784 cases. The 56 insertion scenarios cover overwrite adapters,
cursor preparation, failed helpers, invalid pages, free-space computation,
replacement, insertion, balancing, saved positions and preformatted overflow
mapping. Forty recursive-query scenarios cover finite chains up to 32 terms,
ORDER BY, limits, distinct queues, authorization, aggregates, window rejection,
allocation failure and setup failure. Their before/after transcripts match.
Four additional incomplete-chain scenarios run against each repaired native
configuration and verify the error and restoration path.

The observer uses host C headers, so its runner removes the production recipe's
`-nostdinc` option. All diagnostic switches remain enabled. Initial observer
staging retained that option and failed before execution. The final runner also
corrects an observer-only mixed-operator parentheses advisory. The original
whole-module measurements retain their complete original flags.

Strict source-only fblint passes both changed FreeBASIC files with zero errors
and eight existing advisories. The preimages have the same advisory counts.
The largest execution scratch footprint is 1,481,766 bytes. The complete-module
GCC dump check peaks at 4,211,627 bytes. Temporary source,
object, executable and delivery directories are automatically removed.

## Reproduction and receipts

The runner accepts a complete emitted module, its observer, a native compiler
and the recorded flag list. Its `--help` output lists the invocation arguments.
Use `--missing-anchor` with the repaired recursive helper to include its four
additional cleanup scenarios.

- `build_scripts/test-sqlite-insertion-phases.py`
- `tests/compiler-support/sqlite-btree-insertion-phases.c`
- `tests/compiler-support/sqlite-recursive-query-phases.c`

Receipts under `build/c-emitter-forward-views-20261105` record exact source and
tool identities, commands, diagnostics and execution transcripts.

- `sqlite-phase-packet-v1-20261117.json.zlib`
- `sqlite-phase-native-v1-20261117.json.zlib`
- `sqlite-phase-census-v1-20261117.json.zlib`
- `sqlite-phase-execution-v4-20261117.json.zlib`
- `sqlite-phase-dumps-v2-20261117.json.zlib`
- `sqlite-phase-lint-v1-20261117.json.zlib`
- `sqlite-phase-delivery-v1-20261117.json.zlib`
- `sqlite-insertion-phases-port-v1-20261117.patch`
- `sqlite-phase-qualification-v1-20261117.json.zlib`

Whole-engine SQL execution and publication of the accumulated port patches
remain open. The function-level observers use controlled dependencies.
The compiler cleanup goal retains its other warning queues and platform checks.

<!-- end of c-emitter-sqlite-insertion-phases.md -->
