<!--
Project: FreeBASIC C backend qualification
File: c-emitter-padding-contract.md
Purpose: Record the disposition of natural ABI padding diagnostics.
Responsibilities: Preserve unsuppressed warnings and rejected representation evidence.
This file intentionally does NOT change record ABI or suppress diagnostics.
-->

# Natural record padding, November 3, 2026

The selected queues retain 4,702 Clang padding warnings. Whole-record sizes and
named field offsets pass the accepted compiler and project checks. Natural C
record declarations preserve their existing cross-module representation.

Explicit padding members were rejected: forty native mixed-representation value
checks passed, but GCC LTO reported 64 type mismatches in eight mixed-declaration
programs. Eight same-layout controls produced none. Matching sizes and values
alone did not establish declaration compatibility.

A separate small control checks ordinary, field-attribute aligned, standard
aligned and packed/aligned declarations. Every record remains eight bytes with
its second member at offset four. GCC accepts all four without warnings; Clang
still reports all four padding observations under the accepted unsuppressed
profile. Its other ten compatibility/attribute observations are also retained.

Natural padding therefore remains an explicit ABI diagnostic disposition. This
is not a zero-warning claim, a blanket waiver of field bounds or a proof of
every target ABI. No declaration or warning flag was changed by these controls.
Receipts are `build/c-emitter-padding-contract-20261103/summary.json` and
`build/c-emitter-explicit-padding-20261103/summary.json`.

<!-- end of c-emitter-padding-contract.md -->
