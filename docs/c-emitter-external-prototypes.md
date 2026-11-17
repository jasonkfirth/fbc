<!--
Project: FreeBASIC C backend verification
File: c-emitter-external-prototypes.md
Purpose: Record emitted external declaration reachability and its qualification.
Responsibilities: Explain retained roots, native controls and selected queue impact.
This file intentionally does NOT rename required external ABI symbols.
-->

# External object declarations, November 14, 2026

Multiple BASIC names can bind the same external C object. The emitter now
checks the complete generated extern declaration against its existing owned
declaration cache. An identical declaration is emitted once per translation
unit. Types, bounds, qualifiers and attributes participate in the comparison.
Definitions and their initializers continue through the normal allocation pass.

Object cache keys use a prefix containing a space, keeping them distinct from
valid C procedure identifiers. The existing per-output initialization and
cleanup own the records. Semantic name observations run before the duplicate
check, preserving each source binding's export metadata.
Independent model-reader checks verify all eight aliased source bindings with
both C producers. Each retains its C token and object name. Enabling the full
semantic export preserves the exact emitted C bytes.

The compiler image is `acc4a8921db861ccd1b2856f164c2033d78ae28ce069b1cf0b52f5f573c57f76`.
Its 224-unit self-host reaches an exact emitted-C fixed point with zero GCC
warnings. Only the HLC compiler unit changes. All 703 fbctests outputs match
their preceding C identities and preserve GCC zero / Clang 771. Their replay
also records actual opened BASIC inputs for each owning profile.

All 1,053 coreutils/SQLite production units are reemitted. Exactly 135 remove
one repeated extern declaration each. Both Linux GCC and Clang compile every
before/after pair with the original unsuppressed flags. All 270 complete native
objects match byte for byte. The remaining 918 C outputs match their parents
exactly. Every original FBC warning line is preserved. Coreutils falls from
911 to 776 GCC warnings, with 2,412 Clang observations. SQLite retains GCC 111
and Clang 7,784 across its 555 Linux units.

The permanent external-prototype fixture now checks aliased scalars, arrays,
pointer objects and initialized BASIC data. An independent C observer compares
addresses and reads the final values. Sixteen native value executions and four
Win32 object profiles pass. Eight negative controls retain native errors for
conflicting scalar types and array bounds. The ordinary backend runner enables
`-Werror=redundant-decls` for this fixture.

Two Win64 full-program runs compile two BASIC modules in one invocation, checking
cache reset between outputs. Four further Win32 executions observe two BASIC
modules through a native C startup. Both C producers and both native observer
compilers are covered. The selected SDK has Win64 runtime files. Its Win32
FreeBASIC runtime files are absent, so the Win32 probe records its C-startup
scope explicitly.

The Windows intrinsic profiles retain two Clang reserved-identifier observations
per object for the required `_InterlockedExchangeAdd` spellings. The preceding
compiler reproduces the same class and count through the exact saved recipes.
Source-only lint and compiler structure receipts accompany the value controls.
The installed semantic-lint consumer retains its existing capability mismatch.

Receipts use the `extern-aliases` prefix dated November 14 under
`build/c-emitter-forward-views-20261105`. The source cut preserves eighteen dirty
compiler inputs and separately records three subsequent test-input corrections.
The earlier standalone helper's module-assembly expectation is corrected against
the preceding compiler's actual reachability behavior. Failed controls and the
input-invalidated suite attempts retain their logs. The final complete GCC/Clang
backend suite passes in 621.517 seconds. Development-path selection passes
twenty-four actual executions, including both producers of the extended fixture.
The source cut occupies 310,599 compressed bytes. Strict source-only lint reports
zero findings and structure checks pass 413 files across 21 host graphs.
Immediate rollback is `2ebfd87d`. Installed release binaries and runtime archives
pass their preservation checks.
The supplemental source-delivery receipt contains forty-six dirty repository
qualification inputs in 266,822 compressed bytes, with relative paths, hashes
and the Git base revision for replay elsewhere.

# Historical external prototype reachability, November 3, 2026

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
