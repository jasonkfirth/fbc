<!--
Project: FreeBASIC C backend warning cleanup
File: c-emitter-unused-parameters.md
Purpose: Record the final-body parameter repair and its GCC qualification.
Responsibilities: Bind warning reductions, ABI controls and promotion limits.
This file intentionally does NOT claim a new complete corpus census.
-->

# Unused C parameters after store removal

The main shared C helper now checks final emitted references even when the
original parameter access flag was set. Private store removal can remove the
last read. The check recognizes identifier boundaries, so a short name such as
`i`, `p` or `y` no longer matches part of an unrelated identifier. A fallback
search handles read-only ABI wrappers written after later procedure bodies.

C23 definitions omit only names proven absent from their complete bodies.
Argument types, calling conventions and caller-side effects remain intact.
Debug output keeps its names. GNU11 retains the original named definitions;
preprocessed ordinary GNU11 C is byte-identical to the baseline. Opaque assembly
retains the existing conservative policy. No diagnostic suppression was added.

The regression is `tests/compiler-support/c-unused-parameters-after-stores.bas`
and is registered in the existing backend runner. It covers value/reference
arguments, caller-side effects, static members, an addressable read-only wrapper,
short unused names and a short name which must remain available to the body.

## Qualification

- Repaired experimental compiler: `30eaf63f3d2170ebe4a24ef9d6871d98896565591045262d51e3c1cd039ddba8`.
  One fresh GCC compiler object and 223 exact qualified objects; compiler GCC
  warnings remain zero. Matching normal ST/MT archives are used.
- All 224 compiler C identities are qualified. The wrapper repair received the
  full 224-unit comparison and complete GCC backend suite. The final identifier
  rule receives a fresh HLC comparison, with 223 unchanged unit identities reused.
- Four native regression executions pass across ordinary/debug and GNU11/GNU23.
  Ordinary GNU23 has zero GCC warnings. Debug emitted C stays byte-identical to
  its baseline. Four Win32 definition checks and one GAS64 value control pass.
- All sixteen nondebug profiles with parameter-class GCC warnings were checked.
  The final boundary change needs four fresh GCC checks and reuses twelve exact
  C/tool/flag checks. Broad observations fall 18 to 4; Exampleageddon 7 to 0.
  Four retained observations are two unused assembly-unit parameters and two
  parameters assigned but never read in a calling-convention test.
- Strict source-only lint passes the helper and regression with zero findings.
  Compiler-backed semantic lint was not qualified because its required input
  closure was unavailable; it is not represented as a completed check.

This removes fourteen broad and seven Exampleageddon observations. The last
complete experimental queues remain 247 and 83 GCC observations, respectively;
this focused result does not claim a new full-body total. Valid examples and
installed compiler/runtime files remain unchanged. The experimental source
closure still needs merging with newer main parser/semantic and label work.

## Evidence and continuation

Controllers are under `build/c-emitter-forward-views-20261105/` and compressed
receipts under `E:/fbc-c-audit-20261105-forward-views-r2-qualification/crt-candidate/canonical/`:
`gcc-parameter-build-v3`, `gcc-parameter-values-v3`, `gcc-parameter-queues-v3`,
`gcc-parameter-final`, `gcc-parameter-gas-control` and their source-change records.
Earlier failed setup, entry-point, wrapper-order, source-name and working-directory
preflights remain separate. The working-directory retry repeated six successful
checks; its raw 22 rows describe sixteen unique profiles and are not added to
counts twice. The final queue receipt contains exactly sixteen unique profiles.

Priority remains GCC warning classes, then experimental-to-main promotion,
then Clang. The next pedantic class is 42 function-address initializers across
three valid examples. Promotion must preserve newer dirty main work and retain
explicit wider runtime/target limits. PNG and the user-closed investigations stay
outside this work. The complete campaign remains active.

<!-- end of c-emitter-unused-parameters.md -->
