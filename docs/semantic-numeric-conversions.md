<!--
Project: FreeBASIC compiler tooling
File: semantic-numeric-conversions.md
Purpose: Document compiler-owned numeric conversion inputs and their limits.
Responsibilities: Observation ownership, transport closure, consumer policy and constant safety.
This file intentionally does NOT define linter diagnostics or claim a complete analyzer.
-->

# Source numeric conversion inputs

The optional `numeric-conversions` request retains scalar conversion inputs
before folding and runtime lowering. It joins the existing
`-semantic-extra-facts` list and `all`; `none` leaves capture disabled. The full
model advertises `source-numeric-conversions` and `source-numeric-truth-inputs`
only when this request is enabled. Consumers must check availability rather
than treating an absent stream as a source file with no conversions.

`tooling/semantic-numeric-conversions.bi` declares the internal hooks.
`astNewCONV` supplies ordinary scalar conversions. Floating stores handled
directly by a backend and floating truth tests supply their original inputs
before those operations fold or lower. Integer truth tests and user-defined
types do not become floating inputs merely because a later carrier uses one.

The observer borrows the live AST during the compiler's synchronous module
export. It owns no AST storage. It snapshots the actual selected input type
and value, then restores every borrowed expression association before normal
compilation continues. The older parser expression identifies the written
site; it cannot establish the selected value after earlier coercions.

## Observation layout and closure

A `K` property named `numeric-conversion-input` belongs to the fresh selected
expression. Its escaped payload contains three tab-separated fields:

| Field | Meaning |
| --- | --- |
| 0 | Original parser expression identity, used for source provenance |
| 1 | Selected destination dtype in the stable transport vocabulary |
| 2 | Explicit conversion bit, either 0 or 1 |

An `H` edge named `parsed-numeric-conversion` relates the original expression
to this selected occurrence. A module-owned `numeric-conversion-count:<id>`
property closes the stream, including an empty stream. Consumers validate the
property, edge, statement ownership and module count together. A lost pair
must reject the model rather than silently remove a diagnostic.

The stable wire type vocabulary is distinct from native compiler enum values.
Ordinary compiler builds default to native export identities. Consumers of the
stable contract select `-semantic-type-abi stable-v1`; the linter requests this
explicitly when the compiler advertises it. The dedicated semantic product is
also built with `FBC_SEMANTIC_STABLE_TYPE_ABI=1` for its selector module, giving
it the stable-v1 default while preserving an explicit native override. The
profile changes export numbering and does not change generated program code.
Exact integer magnitudes in the present consumer remain bounded to 64 bits.
A 128-bit type domain can describe possible conversion loss without supplying
an exact 128-bit constant. No full-width constant evaluation is established by
these observations.

`AST_CONVOPT_SEMANTIC_TRANSPORT` marks compiler-created representation
conversions, such as address offsets and low-word runtime helper arguments.
It prevents these internal carriers from becoming source narrowing sites. It
is observation metadata and does not select a different runtime conversion.

## Consumer policy and constant safety

The shared linter conversion policy consumes these inputs across assignments,
arguments, defaults, loop bounds, indices and returns. Source spelling and
lowered runtime helper names do not establish the conversion domain. Lint and
analyzer ownership is independent of the compiler facts a check requires.
Explicitly selecting one of the 20 reviewed numeric diagnostic IDs requires an
accepted model while semantic analysis is enabled. An explicit
`--no-semantic` retains its opt-out behavior. The reviewed input catalogue in
fblint owns that policy; the compiler does not select warnings.

The declaration parser also checks that conversion of a CONST initializer
actually returns a constant node before reading its value union. A lowered
runtime call has no compile-time FBVALUE payload. Currently unsupported wide
CONST conversions are diagnosed with `Expected constant`, rather than
silently reading another node's union as an integer. Ordinary wide variables
are outside this rejection. The two `constant-*-no-value.bas` tests use the
normal compile-fail harness.

## Qualification boundary

The integration rebuilds the current Make-selected Win64 compiler source
graph and reuses the previously qualified runtime archive. Artifact identities
and focused compiler/linter receipts are under
`C:/fblint/build/backlog-20261021/stage33-canonical-integration`.
Source integration is not public installation, hosted CI, native Linux
execution, general interprocedural analysis or a speed comparison with Clang.

<!-- end of semantic-numeric-conversions.md -->
