<!--
Project: FreeBASIC C backend verification
File: c-emitter-stored-translations.md
Purpose: Describe native format contracts carried through saved translations.
Responsibilities: Explain pointer proofs, invalidation and native qualification.
Application execution retains its separately recorded scope.
-->

# Stored native translation formats

Both C emitters retain source-format contracts when a gettext-family result
passes through validated private pointer stores in a basic block. A saved
plural result carries both original literal formats. Native argument checking
continues to use the target C ABI and each original conversion specification.

The evaluated translation expression carries borrowed literal-symbol metadata.
The complete optimized procedure validates each formatter binding before AST
loading begins. A copied intermediate receives its own complete-body check,
allowing direct pointer copies into the validated destination. Address escapes,
BYREF exposure, writable aliases and integer-storage round trips retain their
existing diagnostic boundaries. Ordinary literal-copy propagation keeps its
previous scope.

At the native formatter, the existing `format_arg` identity helper receives
the actual stored pointer. Literal addresses describe its format contract.
Translation calls, source arguments and count expressions retain their original
evaluation. Direct translation expressions already carry that native attribute
and reuse their existing helper.

Ordinary calls expire saved translation metadata because another call can
change translation storage through an existing alias. Labels, branches,
saved-continuation calls and opaque writes also expire the tracked value.
Private bindings to immutable literal storage retain their previously qualified
ordinary-call behavior.

The registered `c-format-locals` regression covers translated output, pointer
identity and call counts through the normal backend entry. Native diagnostic
controls cover incorrect argument types, both plural contracts, dynamic source
formats, call barriers, alias writes and integer storage. The plural-format
regression also checks saved results for all three plural translation APIs.

An initial candidate lacked the intermediate-copy proof and failed the focused
regression. The corrected proof validates those copies before emission. The
first full suite then exposed its preceding saved-plural expectation, which
still required a nonliteral warning. That expectation now accepts the proven
saved contract and checks saved wrong-type arguments and call invalidation.

## Compiler and native qualification

Development main is `626a5305`, with full SHA256
`626a530549b6e5e150756d5cc63317d985d5f48cdd1de7d2f19718bc4db756b9`.
All 224 compiler units reach an exact emitted-C fixed point with zero GCC
warnings. The 703 fbctests profiles also retain GCC 0. Their Clang counts are
1,340 and 771. The complete normal backend suite passes in 724.973 seconds.
Selection runs the assertion-enabled public entry through both C producers and
preserves all ten protected installed products. The rollback image is `0be13d79`.

The Windows/Linux 32/64-bit matrix covers 64 native execution contexts and
704 diagnostic checks through both C producers, GCC, Clang and O0/O2. It
retains 160 wrong-type errors and 480 nonliteral errors, with 64 accepted
diagnostic controls. Native observers check 1,280 translated outputs, pointer
identities and translation-call counts. Linux evidence is reused after matching
all emitted modules, observer text, native options and current tool hashes.
Windows receives a fresh complete native replay with explicit tool pins.

The complete 1,053-profile port replay changes only the retained-mode format in
`chmod`. Both C producers emit identical C for that changed module, and its
GCC and Clang before/after native objects match byte for byte. Coreutils reaches
GCC 165 / Clang 1,784. SQLite retains GCC 21 / Clang 7,696. Every FBC warning
line is preserved. The remaining Coreutils nonliteral class has 129 observations.
Two finite-format sites need control-flow joins, and twelve dynamic-format
execution contracts retain their preceding follow-up scope.

## Current runtime source refresh

Runtime packet v44 qualifies seven concurrent runtime edits and the shared
Unicode-header edit. Both complete Linux Make-selected configurations remain
GCC-clean, with Clang 10,023 for MT and 10,021 for non-MT. The workers reuse
their freshly built archives for the supplementary suites, keeping native
builds serial and avoiding another archive rebuild for those tests.

Eight executions cover the existing narrow/wide PRINT USING and radix suites,
with 21,880 assertions. Sixteen direct production-source profiler controls cover
32/64-bit native code, O0/O2 and both runtime modes, checking minimum widths and
saturation near INT_MAX. The earlier 352 runtime contexts retain their current
rebuild and execution coverage. Owned peak scratch is 23,863,065 bytes for MT
and 16,897,318 bytes for non-MT, and both temporary trees are removed.

The subsequent packet-v47 replay closes the later Linux runtime source checks
through complete fresh MT/non-MT builds and all 376 existing execution contexts.
Both configurations remain GCC-clean. Sixteen additional 32/64-bit native
console-view and XTerm-focus executions check packed row results and saturated
reference state. Nine captured FreeBSD/OpenBSD source versions retain native
platform replay as an open check. Current receipts are
`runtime-current-linux-v40-20261116.json.zlib`,
`native-nonmt-controls-v7-20261116.json.zlib` and
`runtime-limits-v3-20261116.json.zlib`.

<!-- end of c-emitter-stored-translations.md -->
