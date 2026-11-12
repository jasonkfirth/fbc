<!--
Project: FreeBASIC C backend qualification
File: c-emitter-debug-parameters.md
Purpose: Explain ignored calling-contract parameters in debug output.
Responsibilities: Preserve formal storage, source diagnostics and native warning evidence.
This file does not claim complete corpus cleanup or installed promotion.
-->

# Ignored parameters in debug C

The retained cohort has 176 GCC unused-parameter observations across 43 debug
profiles. Removing formal names, as the non-debug C23 path does, would lose
debugger-visible arguments. FreeBASIC already reports unread source parameters
with warning 54 under `-w all`; C lowering must preserve that diagnostic.

The backend inspects the complete emitted body and retains each formal name.
For an unreferenced formal, it emits a local enum declaration whose value is
`sizeof(&parameter)`. This acknowledges the required ABI storage without reading
the argument, evaluating an address or creating runtime storage. The declaration
works in GNU11 and GNU23 and precedes the ordinary local declarations. It adds
no newline, so existing debug source-line mappings stay unchanged.

This proof is separate from dead-store removal. Debug locals and parameter
values retain their existing storage. Naked procedures and bodies containing
general opaque assembly retain their conservative behavior and C diagnostics.
The non-debug C23 unnamed-parameter path and GNU11 fallback are unchanged.

## Qualification

The candidate is `9d60dd5092770ecf583c586237b0ccf4c604f3dcfe58a327e6a6950fbc182b98`.
It reproduces 224 compiler C units at a fixed point with zero GCC warnings,
reusing 223 unchanged objects and then all 224 in the fixed-point round.
Installed release binaries, runtime archives and the executable pinned by the
running JRPG audit are unchanged.

The regression passes 16 native value runs across both C producers, GCC and
Clang, GNU11/GNU23 and O0/O2. Four GDB runs inspect all six ignored formal
arguments and complete the program. Removing only the enum declarations gives
exact prior debug C for both producers. All seven source unread-parameter
warnings remain, and four opaque-assembly controls retain their native warning.

All 43 affected retained profiles pass both native compilers under their
original aggressive unsuppressed flags. GCC warnings fall from 183 to 7 and
Clang warnings from 582 to 406: all 176 unused-parameter observations disappear
from each tool, and every other warning class is identical. All 165 existing
source warning 54 observations in those profiles are identical too. The
checkpoint working set peaks at 19,531,279 bytes; fixed scratch slots are reused.

Eight semantic models pass the independent reader across both C producers,
32/64-bit targets and text/binary transport. Export preserves eight exact C
identities. Four Win32 C objects pass the unused-parameter error gate; these
objects are not Win32 execution evidence.

Both complete C-backend suites pass in 218.531 seconds, including 462 source
warning checks. Compiler structure checks cover 409 documented files and 21
host graphs. Strict lint is clean for the three owned BASIC files.

The new debug fixture also exposed a pre-existing reader defect, reproduced
with both compiler generations: tabs inside literal source/debug text were
split as structured fields. The reader now splits that final free-form payload
once, retaining its exact text. Other union classes keep exact field arity and
discriminator checks. Four focused payload tests cover tab preservation, wrong
unions, extra structured fields, omitted records and both raw type encodings.

An intermediate statement acknowledgement introduced Clang's
`-Wdeclaration-after-statement` in the first original-flags corpus recheck and
was rejected. A static-assert experiment also introduced a compatibility
diagnostic and was not used. The final declaration-only form passes both
native C dialect controls without either diagnostic. Failed receipts remain
separate from accepted qualification.

Receipts reuse `build/c-emitter-forward-views-20261105/` with the
`debug-abi-selfhost-v3-`, `debug-abi-values-v3-`, `debug-abi-semantic-v3-`,
`debug-abi-gates-v3-` and `debug-abi-parameter-profiles-v3-` prefixes.
The completed 43-profile original-flags recheck remains separate from the completed
[retained census](c-emitter-current-status.md); it does not claim that every
remaining warning class is resolved.

<!-- end of c-emitter-debug-parameters.md -->
