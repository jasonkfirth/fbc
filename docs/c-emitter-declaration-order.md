<!--
Project: FreeBASIC C backend verification
File: c-emitter-declaration-order.md
Purpose: Record the November 2 declaration repairs and their qualification.
Responsibilities: State source changes, warning deltas and remaining boundaries.
This file intentionally does NOT claim full corpus or error-frame acceptance.
-->

# Declaration-order repairs, November 2, 2026

PNG work is excluded by user request. The compiler remains noninstalled at
`build/c-emitter-warnings-20261102/pointer-asserts/fbc.exe`, SHA-256
`2a9f2106e070bf0edef5b02964fe33b605e03f48afa549ebbcf8df6714d167fc`.

`c-integer-abs.bi` emits the signed result declaration before its conditional
magnitude adjustment. `ir-hlc.bas` places pointer-representation size assertions
beside their already-hoisted carriers. Record-layout assertions remain beside
their completed types, including types in nested scopes. Evaluation stays at
the original expression site, and the assertions remain active.

The two repairs remove 621 Clang warnings. Other warning groups, including GCC
groups, are unchanged. All selected C outputs were regenerated. Changed C was
compiled with the saved aggressive, unsuppressed GCC/Clang profiles; exact C
byte identity permits reuse of the previous object diagnostics. Source, recipe,
tool and installed-binary identities were checked. Twelve PNG profiles were
excluded from both sides of the production comparison.

| Cohort | Profiles | GCC, before and after | Clang before | Clang after |
| --- | ---: | ---: | ---: | ---: |
| compiler | 224 | 0 | 5022 | 4975 |
| unit | 703 | 19 | 14377 | 14356 |
| production | 1117 | 502 | 11517 | 11254 |
| additional | 239 | 1810 | 41431 | 41141 |

The baseline includes the qualified UDP handle repair. Profiles can repeat a
source with different options and are not distinct-source counts.

The standalone regressions compile 384 before/after objects with GCC/Clang,
GNU11/GNU23 and O0/O2. All 192 object pairs are byte-identical. All 192 native
runs pass. Pointer checks include debug emission, qualified storage, nulls,
lazy expressions, callback calling conventions and Win32 objects. Sixteen
deliberately failed assertions still fail compilation. Win32 execution is not
claimed. The native backend regression suite passes, as do the 375-file,
21-host structure guard and strict source-only lint on both changed sources.
Compiler-backed semantic lint remains outside that source-only result.

Receipts and compressed diagnostics are under
`build/c-emitter-warnings-20261102`, with the composed result in
`declaration-summary.json`. Building each compiler required one new emitter
object. Each reused object graph first reproduced its preceding compiler's
exact binary hash. Audit scratch C and objects are reused and retired; installed
compiler/runtime binaries and unrelated dirty sources remain unchanged.

The campaign remains incomplete. Retained diagnostics include GNU label
addresses and indirect jumps, native layout padding, raw pointer operations,
unused declarations and external ABI contracts. The previous global ERROR
frame failures, unqualified source contexts and general pointer/descriptor
work remain open. The existing coverage ledger remains a historical baseline,
not evidence that every corpus source now has a valid build context.

<!-- end of c-emitter-declaration-order.md -->
