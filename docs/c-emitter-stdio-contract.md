<!--
Project: FreeBASIC compiler verification
File: c-emitter-stdio-contract.md
Purpose: Record the stdio type binding and full current-source qualification.
Responsibilities: Explain C naming/alignment, diagnostic counts and test scope.
This file intentionally does NOT claim acceptance of unselected source contexts.
-->

# C stdio binding and current compiler, November 2, 2026

The C backend now gives Clang's stdio builtins the `FILE` typedef they require.
It derives the typedef from the stream pointee already used by the BASIC
declarations. Complete records keep the existing alignment-one pointer view;
unresolved stream pointees keep their existing `void*` projection. The underlying
record definitions, field offsets and linker names remain unchanged.

The binding is emitted before generated declarations. A synchronous symbol walk
requires one consistent global stream view. Inconsistent views retain their
original declarations and diagnostics. The scratch state is cleared for every
output. BASIC objects and procedures whose external name is `FILE` receive a
distinct C identifier and retain their original assembler name. Record fields
use their separate C namespace.

## Current source integration

The earlier compiler snapshots excluded concurrent frontend and semantic-tooling
changes. The current tree has now been built in full, without replacing installed
tools. All 224 selected compiler modules were freshly emitted; 40 changed C units
were compiled with expanded unsuppressed GCC diagnostics and Clang
`-Weverything -Wwrite-strings`. Inherited objects were checked by reproducing the
baseline link. Re-emission with the resulting compiler matches all 224 C hashes.

The current compiler is
`build/c-emitter-stdio-contract-20261102/current/fbc.exe`, SHA-256
`d1c4fd3b752ac2d58e52ec00722194d882a811a9098aedce19c44518e1be3ca1`.
Its receipt pins the complete current compiler/SDK source snapshot and its input
object graph. GCC remains at zero warnings. Clang reports 4,830: 1,639 padding,
3,146 buffer observations, 30 unused functions, 13 constructors and two reserved
intrinsic names. The newly integrated frontend code adds 45 padding/buffer
observations; the stdio repair removes five header warnings from this build.

The structure checker now accepts both exclusion-list headings and complete
exclusion sentences. The 13 previously failing files already stated their
boundaries; their semantic code and comments were preserved. The checker also
recognizes the optional stdio prelude before its fixed generated-C signature and
checks generated, handwritten and orphan cases. All 392 files and 21 host source
graphs pass, together with the typed x86 callback controls.

## Diagnostics and native checks

`current-audit/results.sqlite3` records all selected non-PNG profiles with this
compiler. Each C output was freshly emitted. Unchanged C retained its exact
accepted object checks; changed C was compiled with both original unsuppressed
profiles. All 2,059 noncompiler profiles pass, with 12 PNG profiles excluded.
The stdio repair removes 36 Clang header warnings across the four queues.

Enabling the correct stdio signature also exposed one format warning in
fbcunit's report writer. The subsequent
[report repair](c-emitter-fbcunit-report.md) resolves it and composes the latest
queue totals. The original stdio audit remains an immutable before-repair record.

`tests/compiler-support/stdio-contract.py` checks complete and opaque streams,
direct/indirect I/O, and object/procedure exports named `FILE`. The current-source
matrix passes 64 old/new native cases and 80 Win64/Win32 object checks under
GCC/Clang, GNU11/GNU23 and O0/O2 where execution applies. Object symbol checks
retain `FILE` and `_FILE` at their respective widths. A C companion includes the
platform's real stdio header and writes through the same native stream.

Mixed stream views preserve exact old/new C in three contexts. Both module orders
are compiled to check binding lifetime: 24 further object checks pass. The main
backend runner includes stdio and report cases. It now recognizes the legacy
`win64` target spelling when selecting native x86-64 arithmetic, layout and TLS
checks. Linux i386 assembly uses an explicit ELF assembler on Windows; its ELF32
object header is checked. Native GAS64 cases also pass. Foreign object acceptance
does not imply execution on that foreign target.

## Boundaries

All changed BASIC compiler files and the stdio fixtures pass strict source-only
lint without warnings. Compiler-backed semantic lint remains separate. The full
warning campaign still includes nonlocal ERROR ownership, descriptor/pointer
contracts, remaining diagnostics and source contexts outside the selected plans.

Installed compiler/runtime binaries were preserved. Work reused scratch outputs
and incremental objects, retained compressed diagnostics, and stayed below the
96 MiB stdio-stage cap. PNG work remains excluded.

<!-- end of c-emitter-stdio-contract.md -->
