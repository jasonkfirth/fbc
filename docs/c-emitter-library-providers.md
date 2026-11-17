<!--
Project: FreeBASIC C backend library qualification
File: c-emitter-library-providers.md
Purpose: Record provider handle repairs and complete library source accounting.
Responsibilities: Explain native types, build selection and diagnostic evidence.
External database execution has separate qualification.
-->

# Library providers and source coverage

## Opaque libpq handles

The PostgreSQL adapter stored `pg_conn` and `pg_result` as value fields. The
current `postgresql/libpq-fe.bi` binding declares opaque objects and exposes
them through `PGconn ptr` and `PGresult ptr`. The adapter now uses those pointer
types for its connection and result fields. Existing callbacks and public driver
declarations retain their signatures.

The original adapter produces FBC error 14 and pointer-conversion advisories.
Its failed single-threaded and multithreaded compilations remain recorded in
`current-fbext-support-context-v1-20261114.sqlite3`.

The database Makefile also selected its multithreaded PostgreSQL object through
`USE_POSTGRE`, while the compilation rule and single-threaded path use
`USE_POSTGRES`. Both variants now use `USE_POSTGRES`. Eight actual Make dry runs
cover the before/after recipes, ST/MT variants and enabled/disabled provider.
The original multithreaded recipe reproduces the missing provider selection.
The corrected recipe selects the corresponding object and archive in both modes.

The public recipe regression runs from the library root.

```text
python3 -B tests/c-backend-regression/provider-build-check.py --make make
```

## Native qualification

The two optional database adapters and six test-framework implementation files
receive sixteen actual ST/MT source profiles and thirty-two native object builds.
All pass with zero GCC warnings. Clang retains fourteen padding and eighteen
startup-registration observations under the original unsuppressed flags.

The repaired PostgreSQL adapter also passes eight source profiles across both C
producers, both Windows widths and ST/MT modes. Both native compilers accept all
sixteen objects with zero GCC warnings and eight Clang startup observations.
FBC warning 54 remains for the callback's unused `D` argument. The test framework
retains warning 55 for its existing unread `SUCCESS` binding.

Strict lexical lint of the edited BASIC file has zero findings. Current
compiler-backed lint retains the installed-consumer limitation described in
[current status](c-emitter-current-status.md). The source file keeps LF endings
and the Makefile keeps its existing CRLF endings. The Makefile whitespace check
recognizes CR as the line ending.

The provider implementation was an unused inventory entry in the previous
accepted queues. A fresh reconciliation verifies all 11,744 selection inputs
and checks 6,531 accepted parent profiles, finding zero readers of that file.
The selected compiler remains `987d6d9d`; installed release binaries and recorded
runtime archives retain their verified identities.

## Complete library BASIC-source accounting

The current `C:/fb_corpus/fb-ext-lib` tree contains 353 BASIC files. All 336
in-scope files have current owning C-object evidence. The other seventeen retain
the user's PNG exclusion. Eight regression fragments use their supplied runner
include and isolated visibility contracts. Their original standalone refusals
remain available.

This accounting covers source ownership and native compilation. Runtime database
behavior remains unverified.

Receipts use the existing `build/c-emitter-forward-views-20261105` work area.
The complete source ledger is `fbext-full-source-closure-v1-20261114.json.zlib`.
Publication and Make controls are `provider-source-published-v1-20261114.json.zlib`
and `provider-make-contract-v1-20261114.json.zlib`. Native results are
`current-fbext-support-context-v2-20261114.sqlite3` and
`current-postgre-abi-context-v1-20261114.sqlite3`. The selected-input reconciliation
is `provider-inventory-reconciliation-v1-20261114.json.zlib`.

<!-- end of c-emitter-library-providers.md -->
