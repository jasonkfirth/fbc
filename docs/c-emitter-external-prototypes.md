<!--
Project: FreeBASIC C backend verification
File: c-emitter-external-prototypes.md
Purpose: Record emitted external declaration reachability and its qualification.
Responsibilities: Explain retained roots, native controls and selected queue impact.
This file intentionally does NOT rename required external ABI symbols.
-->

# External prototype reachability, November 3, 2026

Discarded private helpers can leave an external procedure's AST access flag set.
The C backend previously retained those declarations even when every emitted
caller had been removed. The existing per-output dependency graph now tracks
external declaration nodes as well as private bodies. Public bodies and global
initializers remain roots. Callback adapters retain their targets; debug bodies,
exports, virtual entries and opaque assembly retain their existing behavior.
Function-pointer typedefs and parsed procedure definitions are excluded from the
new external-declaration path. Graph nodes own C names rather than borrowed
procedure symbols, which can expire during IR flushing.

The permanent regression passes 16 native programs and four Win32 objects. It
checks dead recursive helpers, live calls, C calls into a public BASIC procedure,
global callback addresses, debug retention and assembly fallback. Two historical
controls reproduce stale prototypes. Actual Windows InterlockedAdd and
InterlockedAdd64 calls verify that required intrinsic declarations remain usable.
The updated full backend suite passes, along with eight semantic tests on the
staged provider. The canonical compiler closes all 224 C fixed points under the
original unsuppressed GCC and Clang profiles: zero GCC and 2,527 Clang warnings.

Across all 2,283 selected non-PNG profiles, the repair removes 276 Clang
reserved-identifier observations, from 450 to 174.
GCC remains at 2,141; Clang falls from 18,362 to 18,086. No new warning
class or per-profile warning increase is accepted. Unchanged emitted C reuses
only exact prior C/tool/profile receipts. Changed C is checked with both tools.
Twelve PNG profiles remain excluded. Required external spellings are preserved.

One intermittent native emission stop is preserved with its failed command and
output. All 32 isolated old/new controls pass, and the unchanged controller then
completes the selected audit. The cause remains unresolved; further investigation
is set aside per user instruction. This does not erase the failed attempt.

Strict source lint checks both changed BASIC sources without errors. The fixed
success marker remains the runner's execution result contract, without a lint
suppression. Its final diagnostic output is preserved in the summary.

Receipts are under `build/c-emitter-prototype-reachability-20261103/`:
`summary.json`, `canonical/receipt.json`, `native-canonical/receipt.json`,
`canonical-backend-gate.json`, `semantic-gate.json` and `audit/results.sqlite3`.
The stage has a 64 MiB cap and reuses native scratch outputs. Installed binaries
and unrelated dirty source bytes remain unchanged. The broader warning cleanup,
including continuation interoperability and access contracts, remains unfinished.

<!-- end of c-emitter-external-prototypes.md -->
