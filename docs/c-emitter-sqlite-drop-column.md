<!--
Project FreeBASIC C-backend qualification
File c-emitter-sqlite-drop-column.md
Purpose Record the SQLite DROP COLUMN boundary and argument-type repair.
Responsibilities Describe failure handling, native diagnostics and reproduction.
Whole-engine SQL execution retains the port's application qualification.
-->

# SQLite DROP COLUMN boundaries

The audited port's `dropColumnFunc` checks its requested column index and both
token-lookup results before reading their SQL splice boundaries. A missing
token returns `SQLITE_CORRUPT` through the existing common cleanup path.
That path retires the parse context, restores the saved authorizer and reports
the result code. Negative column indexes also take the existing corruption path.

The output call now passes its `%.*s` precision as `SQLITE_C_INT`. The previous
expression was a 64-bit pointer difference on the measured Linux LP64 target.
SQLite's variadic formatter consumes a C `int` for a star precision, as its
`sqlite3_printf.bas` argument traversal specifies. The explicit conversion
preserves the intended precision and its native variadic type.

Both revisions of the complete owning module emit identical C through the
GCC and Clang C producers of development compiler `4a477330`. The complete
before emission matches the preceding port census byte-for-byte. Under the
original unsuppressed GCC 14.2 and Clang 19.1.7 recipes at O0 and O2, the five
GCC null-dereference observations are removed. The module is GCC-clean.
Clang retains 61 padding and 21 strict function-pointer cast observations.
The existing FBC warning messages are preserved.

## Regression scope

The permanent fixtures are `tests/compiler-support/sqlite-drop-column.bas`
and `tests/compiler-support/sqlite-drop-column.c`. They use canonical database,
parse, column, result-context and rename-token records. The production
`dropColumnFunc`, `renameTokenFind` and `getConstraintToken` bodies are copied
verbatim into a generated include. Each native run also links the complete
current production tokenizer module.

Controlled parse providers supply valid token mappings, missing mappings,
absent tables and parse failures. The observer checks exact SQL splice output,
formatting argument identities, native precision type, result-text ownership,
free ordering, one parse cleanup, authorizer restoration and final error codes.
Inputs include quoted identifiers, comments, whitespace, commas inside strings
and nested expressions. Token lists run in both orders. Column counts and
indexes exercise valid and invalid bounds, both schema indexes and both saved
authorizer states.

Sixteen runs cover both revisions, both C producers, GCC/Clang and O0/O2.
They pass 270,336 cases and 4,139,088 checks. The repaired revision covers
245,760 cases and 3,758,120 checks, including the missing-token and negative-index
paths. The original revision runs its defined paths. Its observer reads the
actual old int64 precision, avoiding an incompatible C int extraction.
The repaired observer reads the required C int precision directly.

Eight native type controls add a standard printf-format contract to the
fixture's `%.*s%s`-only formatter declaration. All four original precision
calls report a type warning. All four repaired calls pass that type check.
This metadata applies to the controlled fixture. SQLite's production custom
formatter declarations retain their existing contracts and diagnostics.

Strict source-only lint reports two files, zero errors and 23 existing long-line
advisories in the production module. The fixture has no lint findings. The
serial native worker runs at reduced priority. Its peak scratch set was
834,190 bytes, and its temporary directory was removed after completion.
Whole-engine parsing, formatting and SQL execution retain separate project
qualification.

## Source delivery and current counts

The repair is applied in the audited local SQLite checkout.
`sqlite-drop-column-port-v1-20261116.patch` reproduces the complete source file
byte-for-byte when applied to its captured preimage. Publication to the port's
GitHub repository remains pending.

The complete 1,053-profile port census replaces the owning module's result and
retains the other 1,052 unchanged native contexts. SQLite reaches GCC 9 and
retains Clang 7,696. Coreutils remains GCC 159 and Clang 1,782. The disjoint
GCC total reaches 184. The remaining SQLite GCC classes contain six null
observations and three qualifier observations.

## Reproduction

Supply `sqlite-drop-column-body.bi` with the three verbatim production bodies
listed above. Emit the permanent BASIC fixture and the complete
`sqlite3_tokenize.bas` module with the port's canonical includes, target,
threading defines and ABI options. Compile those C modules and the permanent
C observer with the matching native compiler and link them together. Repeat
for both C producers and both native compilers at O0 and O2.

For the original revision, compile the observer with `AUDIT_DROP_BEFORE=1`.
That run covers the original helper's defined paths and observes its actual
precision type. The repaired run exercises the complete matrix. Successful
runs print `SQLITE_DROP_PASS` with their recorded counters.

Receipts under `build/c-emitter-forward-views-20261105` include
`sqlite-drop-packet-v1-20261116.json.zlib`,
`sqlite-drop-native-v1-20261116.json.zlib` and
`sqlite-drop-qualification-v1-20261116.json.zlib`. Their capture and native
controllers retain the exact source, emission, build, link and execution
commands, together with original diagnostics and input hashes.

<!-- end of c-emitter-sqlite-drop-column.md -->
