<!--
Project: FreeBASIC compiler verification
File: c-emitter-pointer-equality.md
Purpose: Record object-address equality lowering and its qualification.
Responsibilities: Explain pointer qualifiers, evaluation and retained diagnostics.
Application build and execution receipts retain their individual project scope.
-->

# Object-pointer equality

FreeBASIC accepts object-address equality across distinct declared pointer types.
Nested const qualifiers can also differ between accepted operands. The C emitter
previously passed these operands directly to the native equality operator, where
GCC and Clang diagnosed incompatible pointer types. Comparing the addresses of
overlapping const and writable union members exposed this defect during the
preceding union-member qualification.

Both C producers now give differing object-pointer operands a `const void*`
conversion at equality and inequality emission. Each operand retains its
original expression and executes once. Explicit source casts remain inside the
operand. Procedure-address comparisons keep their established native unsigned
address handling. Ordered pointer comparisons keep the existing unsigned
conversion, with a redundant second lowering block removed.

The regression covers two and three pointer levels, const pointer slots, distinct
record types, element qualifiers, erased pointer types, union member addresses,
null and high-bit address patterns, procedure-address equality and operand side
effects. The independent native observer checks stored address values without
dereferencing constructed pointers.

Explicit const removal still produces FBC warning 40 under `-w constness` and
native `-Wcast-qual` diagnostics. Invalid pointer/floating comparisons still
produce FBC error 20. The public checker treats all native warnings as errors,
using options supported by older GCC releases as well as current GCC and Clang.

Run the public diagnostic and value checker with the compiler and SDK prefix
being qualified.

```text
python -B tests/compiler-support/c-pointer-equality-views.py --fbc /path/to/fbc --prefix /path/to/sdk --backend gcc
python -B tests/compiler-support/c-pointer-equality-views.py --fbc /path/to/fbc --prefix /path/to/sdk --backend clang
```

## Focused qualification

Candidate `4a477330b0c04b61a2c7786da22b7d9c04a1f3868e69d9f38e6b123939c2604f`
passes its two-round 224-module self-build with zero GCC warnings. Every emitted
C module is identical on the second round.

The Windows/Linux 32/64-bit matrix covers both C producers, GCC and Clang,
O0/O2 and both compiler revisions. All 64 value executions pass 37,120 cases
and 368,896 checks. The valid preimage produces the incompatible-pointer
diagnostic, and every repaired positive context passes strict native checks.
All 64 explicit const-removal controls retain their source warning and native
error. All 32 invalid scalar controls retain FBC error 20.

The complete original GCC/Clang backend suite passes in 484.957 seconds.
A second complete suite run passes in 521.436 seconds with the portable
warning-as-error options. Eight additional DOS-targeted GCC 12.2 compile
controls pass their expected outcomes, including the original comparison
failures and retained explicit const-removal diagnostics. The initial older-GCC
probe supplied Windows C to the DOS compiler and stopped at the Windows
calling-convention spelling. The subsequent probe emits the actual DOS target.

All 1,053 current Coreutils and SQLite modules retain their preceding complete
C identities. Their original native flags and tool identities remain bound to
the preceding native receipts. Coreutils retains GCC 159 / Clang 1,782 and
SQLite retains GCC 18 / Clang 7,696.

## Complete owning queues and selection

Development main now selects `4a477330` and retains `ba3b0a98` as its rollback.
Both ordinary GCC/Clang entries execute the pointer and union fixtures with
warnings treated as errors. All four invocations pass, and all ten protected
installed product hashes are verified.

The full retained replay passes all 6,419 profiles at GCC 14 / Clang 16,433.
Only the compiler's changed C-backend implementation module changes emitted C.
All 224 compiler modules and all 703 fbctests remain GCC-clean, with Clang
1,340 and 771 respectively. The existing PNG, provider, assembly and frontend
dispositions retain their recorded scope.

Templari passes all 677 client/server contexts under both producers at GCC 2 /
Clang 54,317. A later `matrix_backup_gui.bas` version receives an incremental
same-compiler replay, reusing the other current source contexts. The compiler
selection requires its own compiler and test inputs to stay current, while
later application versions retain explicit source-cut records.

All 443 additional standalone project contexts remain GCC-clean, covering
Midisoft 149, Rotator3d sixteen, FS4 125 and HART 153. HART's additional included
provider retains its reader checks. Midisoft's recipe update changes its
executable-version metadata expectation and preserves all compile contexts.
The rejected initial empty audit identity is retained with the successful retry.

The last complete disjoint GCC total remains 193. The two additional idle-time
range observations are resolved by their native execution contract. The current
runtime replay and existing DOS audit close their recorded source versions.
Seventeen other platform versions, later EOF and integer-128 inputs, the
conditional long-double parser, the Windows Unicode-header view and the
remaining formatter/source owners retain their follow-up. Platform source
versions repeated in the later-input records count once in the final ledger.
The source-only lint run reports zero errors and one success-marker `Print`
advisory. The marker supplies the public backend runner's expected result.

Receipts reside in `build/c-emitter-forward-views-20261105`.

- `pointer-compare-selfhost-v1-20261116.json.zlib`
- `pointer-compare-native-windows-v1-20261116.json.zlib`
- `pointer-compare-native-linux-v1-20261116.json.zlib`
- `pointer-compare-scalar-controls-v1-20261116.json.zlib`
- `pointer-compare-suite-v1-20261116.json.zlib`
- `pointer-compare-port-native-result-v1-20261116.json.zlib`
- `pointer-compare-port-producers-v1-20261116.json.zlib`
- `pointer-compare-suite-v2-20261116.json.zlib`
- `pointer-compare-templari-v2-20261116.sqlite3`
- `pointer-compare-gcc12-v2-20261116.json.zlib`
- `pointer-compare-selected-v1-20261116.json.zlib`
- `pointer-compare-qualification-v2-20261116.json.zlib`

<!-- end of c-emitter-pointer-equality.md -->
