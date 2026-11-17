<!--
Project FreeBASIC SQLite qualification
File c-emitter-sqlite-constraints.md
Purpose Record checked WHERE-term consumers and their native regressions.
Responsibilities Explain cache, cleanup, diagnostics and reproduction.
Whole-engine SQL behavior retains application qualification.
-->

# Virtual-table constraint lookups

Both file-local `termFromWhereClause` helpers reject negative offsets, negative
term counts and missing selected term arrays. Empty clauses still advance to
their outer clause. Subtraction occurs only after the nonnegative offset has
exhausted a nonnegative term count.

`sqlite3_vtab_rhs_value` checks its output and index-info arguments. On a cache
miss it retains the parser, connection, term and expression before requesting
the RHS value. Missing internal records return `SQLITE_CORRUPT` with a null
result. Cached values retain their identity and bypass parser-state access.
The existing misuse and `SQLITE_NOTFOUND` results keep their meanings. A null
right-hand expression still reaches the value-conversion provider.

`whereLoopAddVirtual` retains the term returned for each flattened constraint
offset. A failed lookup follows the existing index-info cleanup policy and
returns `SQLITE_CORRUPT`. The adjacent `whereLoopAddVirtualOne` already uses
that error for a missing term. Cached RHS values, owned index strings and the
index-info allocation are released once through the original helpers.

## Measured results

Sixteen complete-module GCC/Clang O0/O2 profiles reproduce both original GCC
warnings in each owner. Both repaired modules are GCC-clean, removing four
observations. Their Clang maps retain 35 and 53 padding observations. Both
C producers emit identical C for each source revision, and the original FBC
unused-variable and unused-parameter messages remain present.

Thirty-two native executions pass 919,616 cases and 7,846,592 checks. Both
revisions produce the same defined-path operation transcripts under both native
compilers and optimizations. Candidate-only cases cover exhausted or negative
offsets, absent record storage, missing parser views, invalid public arguments,
cached values, conversion failures and planner cleanup. Sixteen additional
executions through the public runner repeat the current producer fixtures,
passing 794,848 cases and 6,619,936 checks.

Strict source-only lint covers both library files and the public BASIC fixture.
It reports zero errors, fourteen existing production style advisories and zero
fixture warnings. Each remaining long line occurs in the saved preimage.

SQLite reaches GCC 5 / Clang 7,696 across its 555 captured profiles. The two
remaining null observations belong to `sqlite3_btree.bas` and
`sqlite3_bulk_multi_select.bas`. Three cast-qualifier observations remain.
The complete disjoint GCC census reaches 180. The other 1,051 port profiles
retain their exact source/dependency cuts and existing native receipts.

An intervening Templari `matrix_account.bas` source edit receives a fresh paired
emission and native replay. All 677 current profiles retain GCC 2 / Clang
54,317. The other 676 profiles retain their exact dependency and recipe pins.

## Reproduction

The public BASIC fixture uses the port's canonical headers. Generate
`sqlite-vtab-constraints-body.bi` from complete procedure bodies in the source
revision being tested. For `rhs`, copy `termFromWhereClause` and
`sqlite3_vtab_rhs_value` from `sqlite3_bulk16_vtab.bas`. For `where`, copy
`termFromWhereClause`, `freeIdxStr`, `freeIndexInfo` and `whereLoopAddVirtual`
from `sqlite3_bulk36_where.bas`, and define `SQLITE_CONSTRAINT_WHERE=1` when
emitting the BASIC fixture.

The following extraction runs from the compiler checkout, with `PORT`, `WORK`
and `KIND` set to the port checkout, an owned scratch directory and either
`rhs` or `where`.

```sh
python3 - "$PORT/fb_src" "$WORK" "$KIND" <<'PY'
from pathlib import Path
import re
import sys
source, work, kind = Path(sys.argv[1]), Path(sys.argv[2]), sys.argv[3]
names = ['termFromWhereClause', 'sqlite3_vtab_rhs_value']
filename = 'sqlite3_bulk16_vtab.bas'
if kind == 'where':
    names = ['termFromWhereClause', 'freeIdxStr', 'freeIndexInfo', 'whereLoopAddVirtual']
    filename = 'sqlite3_bulk36_where.bas'
text = (source / filename).read_text(encoding='utf-8-sig')
bodies = []
for name in names:
    match = re.search(r'(?ms)^(?:Private )?(Function|Sub) ' + name + r'\b.*?^End \1[ \t]*$', text)
    assert match, name
    bodies.append(match[0])
(work / 'sqlite-vtab-constraints-body.bi').write_text('\n\n'.join(bodies) + '\n')
PY
```

Emit `tests/compiler-support/sqlite-vtab-constraints.bas` with the matching
source include directory, the body include directory, Linux target, C producer,
MT context, `SQLITE_THREADSAFE=1`, `SQLITE_STRICT_SUBTYPE=0` and `-w all`.
Then run its standalone emitted C under the public entry point.

```sh
python3 -B build_scripts/test-sqlite-vtab-constraints.py \
    --module "$WORK/module.c" --kind "$KIND" --cc gcc \
    --native-flags-json "$WORK/gcc-flags.json" --optimization 2 \
    --scratch-root "$WORK" --receipt "$WORK/receipt.json"
```

The flags file contains a JSON array of native compiler arguments. Use the
audit's full GCC diagnostics or Clang `-Weverything`, with the applicable C
standard and arithmetic options. The runner rejects `-w` and `-Wno-*` flags.
Use `--before` with the original source revision to select its defined-path
cases. Each receipt path must be fresh. All temporary native objects are
removed after the run.

The portable two-file repair is
`build/c-emitter-forward-views-20261105/sqlite-vtab-constraints-port-v1-20261117.patch`.
Its preimages match port HEAD `c7e29f0daf37a5880b63c7778a8f8b93055cdb5d`.
Forward application, resulting content and reverse checking pass in an owned
temporary checkout. SQLite's largest scratch tree is 364,610 bytes. The
Templari refresh uses 824,663 bytes. Both scratch trees are removed.
The aggregate receipt is `sqlite-constraints-qualification-v1-20261117.json.zlib`
under the same audit directory. Port publication and whole-engine SQL testing
remain open.

<!-- end of c-emitter-sqlite-constraints.md -->
