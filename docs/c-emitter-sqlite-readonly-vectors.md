<!--
Project FreeBASIC SQLite qualification
File c-emitter-sqlite-readonly-vectors.md
Purpose Record the read-only callback-vector contracts and native evidence.
Responsibilities Explain qualifiers, legacy ABI preservation and source delivery.
Whole-engine SQL behavior retains application qualification.
-->

# Read-only SQLite argument vectors

Virtual-table constructors use `Const ZString Const Ptr Ptr`, matching the
existing public SQLite binding. The receiver borrows both the text and its
pointer slots. The preceding `Const ZString Ptr Ptr` declaration allowed a
receiver to replace an argument slot, producing an unsafe multilevel const
conversion when a mutable pointer vector reached that interface.

Schema initialization uses the same read-only view in `SqliteDbInitInfo.azInit`.
Its owning connection can still replace the view when parsing begins or ends.
The schema checker borrows that view and the database opener initializes it
through its correctly qualified address.

The direct schema-loader call now enters `sqlite3InitCallbackReadOnly` with its
constant text vector. `sqlite3InitCallback` retains the existing C callback
signature used by `sqlite3_exec` and forwards all arguments and the return value
to that implementation. The original schema implementation and error-reporting
body are recovered exactly after reversing the documented type and name edits.

## Native and frontend checks

Both C producers refresh all 555 SQLite profiles. Their C is identical for
every profile. The complete native replay covers 1,110 GCC/Clang profiles,
including ten exact contexts reused from the completed five-module check.
SQLite reaches GCC 2 / Clang 7,693, removing three observations from each tool.
The remaining GCC null observations belong to `sqlite3_btree.bas` and
`sqlite3_bulk_multi_select.bas`.

The forty before/after module profiles retain all other native diagnostics.
The constructor, database opener and schema-name checker have identical objects
under both native compilers and optimizations. Eight additional native object
comparisons preserve the immediately preceding VM-error, DROP COLUMN and
constraint repairs. Together with the original census and constructor control,
553 of 555 current GCC objects match their preceding qualified implementations.
The two changed objects own the schema implementation and its direct loader call.

Thirty-two public-runner executions cover GCC and Clang, O0/O2, both producers,
and native 32/64-bit Windows and Linux. They pass 9,024 cases and 325,696 checks,
including 7,680 legacy callbacks, 1,344 constructor callbacks and the original
initialization-record size and field offset. Pointer-vector identities, null
vectors, output pointers and the full C int range are checked. The layout
observer uses fixed-width 64-bit output cells across both pointer widths.

All 96 FreeBASIC assignment controls have their expected outcomes. Forty-eight
are accepted and forty-eight fail with error 119, `Cannot modify a constant`.
The controls preserve existing protection for text bytes and add protection for
constructor and schema-vector slots. Replacing the connection's whole view
remains valid.

Strict source-only lint covers six library files and the BASIC fixture. It
reports zero errors and 21 existing production advisories, with zero fixture
warnings. The first Windows observer staging check stopped before native
compilation because text output changed LF bytes to CRLF. The corrected runner
preserves the captured bytes and all final native checks pass.

An intervening Templari browser-transport edit receives a fresh paired warning
replay. Its 677 configured native profiles retain GCC 2 / Clang 54,317. Browser
behavior retains its application tests. The complete disjoint GCC census
reaches 177.

## Source delivery and reproduction

The repair covers six port library files and preserves the original callback
symbol. Its patch is `sqlite-readonly-vectors-port-v1-20261117.patch` under
`build/c-emitter-forward-views-20261105`. Forward application and reverse checking
pass from port HEAD `c7e29f0daf37a5880b63c7778a8f8b93055cdb5d` after applying
the preceding `sqlite-vtab-transaction-port-v1-20261116.patch`.

The public fixture is `tests/compiler-support/sqlite-readonly-vectors.bas` with
its native observer beside it. Supply `sqlite-readonly-contract.bi` containing
the actual `c2fb_callback_27` declaration from `sqlite3_bulk36_vtab.bas` and the
complete legacy `sqlite3InitCallback` adapter from `sqlite3_prepare.bas`. Emit
the fixture with the canonical port headers, matching target and pointer width,
MT context and both C producers. Its ordinary native entry point is
`build_scripts/test-sqlite-readonly-vectors.py`.

```sh
python3 -B build_scripts/test-sqlite-readonly-vectors.py \
    --module "$WORK/module.c" --cc gcc --bits 64 \
    --native-flags-json "$WORK/gcc-flags.json" --optimization 2 \
    --scratch-root "$WORK" --receipt "$WORK/receipt.json"
```

The flags file contains the native compiler's argument array. Qualification
uses the full GCC diagnostic recipe and Clang `-Weverything`, with all
observations retained. The runner rejects `-w` and `-Wno-*`. Each receipt path
must be fresh and the runner removes its temporary objects and executable.
The largest native scratch tree uses 1,452,846 bytes. Decoded transports and
superseded controls release 6,835,592 bytes. Installed products retain their
recorded hashes.

The aggregate receipt is `sqlite-readonly-qualification-v1-20261117.json.zlib`
under the audit directory. Port publication and whole-engine SQL testing remain
open, together with the compiler cleanup's other recorded queues and platforms.

<!-- end of c-emitter-sqlite-readonly-vectors.md -->
