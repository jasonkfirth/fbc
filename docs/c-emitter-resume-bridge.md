<!--
Project: FreeBASIC C backend qualification
File: c-emitter-resume-bridge.md
Purpose: Record the native continuation bridge experiment and its remaining work.
Responsibilities: Distinguish exact frozen-C evidence from compiler implementation.
This file intentionally does NOT claim promotion or general resource unwinding.
-->

# Native continuation bridge, November 3, 2026

Continuation addresses and indirect jumps account for 1,820 warnings in each
C compiler. The previous object-token candidate broke mixed GAS/C handling:
GAS expected executable resume addresses, and a C dispatcher could not consume
the native handler address. This experiment establishes the missing runtime
contracts before another compiler integration.

The accepted compiler is the GOSUB checkpoint,
`f2ee42945411cd7c72ec0eb5be74d1479cb90800c3b69ff4e7e27a4342a598d6`.
Its actual worker C is retained exactly, with explicit frozen edits. The
handler owner remains the byte-identical compiler-generated GAS object.
An additional GAS-visible counter proves the handler executes once. The
handler also reads its own 32-element local array; the worker verifies its
own array after RESUME NEXT and reports its continuation to the owner.

The bridge uses distinct data objects for the C retry/next identities. Runtime
resume hooks recognize those objects and return through a live C checkpoint.
The legacy native handoff is separate from local C dispatch. A two-instruction
Win64 assembly wrapper retains the public `fb_ErrorSetHandler` symbol, captures
the GAS caller's RBP before a C prologue, and tail-branches to the runtime setter.
The original native return address and shadow space remain intact. No GAS
object changes are required for this root-handler control.

The C checkpoint uses the native backend's null-frame `_setjmp` form. Ordinary
SEH unwinding cannot walk the borrowed GAS frame as a C call chain. The runtime
owns the jump buffer; local automatic values are unchanged after this particular
checkpoint. The CRT object name remains `_setjmp`, while an explicitly
returns-twice C declaration uses a nonreserved identifier and an assembler alias.
That preserves linkage and compiler control-flow semantics without a warning
suppression. The generated worker has no GNU address-of-label expressions or
C computed gotos.

| Control | Native passes | C warning observations per tool |
| --- | ---: | ---: |
| Original worker, unchanged GAS owner | 12 / 16 | 24 |
| Token worker with compatible runtime wrapper | 16 / 16 | 0 |

Both C producers, GCC/Clang, GNU11/GNU23 and O0/O2 are covered. All diagnostic
flags remain enabled. The 24 observations are repeated object-profile counts:
each original worker has two label-address expressions and one indirect goto.
The private worker has zero warnings under both aggressive profiles. This is
not a reduction of the accepted project/corpus counts.

Eight independent runtime API executions pass. They cover 128 live nested
records, invalid arguments, out-of-order retirement, one allocation refusal,
real retry/next hooks, preserved/reset ERR behavior, native frame lookup and
TLS destruction with balanced allocation counts. They bind 407 SDK headers.
Strict source-only lint of the three BASIC fixtures has zero errors/warnings.
The wrapper runtime is single-threaded in this packet.

Failed controls remain important evidence. Direct computed goto with no
address-taken labels is rejected by Clang; GCC's four retained executions fail
the handler's owner-data check. A native jump without the owner's frame base
also fails. Supplying the native base while using the ordinary C SEH checkpoint
reaches Windows `STATUS_BAD_STACK` (0xC0000028) in all sixteen cases. Changing
only that checkpoint to the native non-SEH form passes sixteen cases. The final
wrapper then reproduces those passes with the original GAS object unchanged.
The installed SDK's `setjmp.h` explicitly has a `__USE_MINGW_SETJMP_NON_SEH`
branch, and the compiler's native GAS lowering already passes a null frame.

## Current owner and GOSUB prerequisite

The preceding owner fragments were rebased onto the accepted GOSUB source,
preserving both GOSUB unit initialization and ERROR-owner state resets. Three
affected modules pass fresh GCC/Clang checks against the current 221-object
remainder. Later revisions rebuild only HLC and retain the two unchanged AST
objects. That preceding private provider is
`8e647655a5d1ba16334b4d6c8a119804d4e810b5da91e18e5c42e85f44d8615a`.

The former owner walker rejected the new direct GOSUB CALL/RET AST shape and
silently retained raw cross-frame handlers. A focused native control exposed
that fallback. The walker now follows both the GOSUB callee and its return site;
local RET ends that path. Each outlined C handler gets its own local dispatch
lifecycle, while its GOSUB context uses the owner's actual captured storage.
Root procedures without a capture plan keep the ordinary C identifier.

Ninety-six owner/GOSUB executions pass with both C producers/tools,
GNU11/GNU23, O0/O2 and -ex/-exx. They cover root GOSUB, the permanent broad
value matrix and the original recursive ordinary/handler nested-GOSUB fixture,
including BYREF/BYDESC identity and managed strings. Sixty-four completed cases
are reused only after eight exact generated-C comparisons; thirty-two handler
cases execute freshly. Twelve Win32 objects pass. Strict source-only lint of
the six private source files and root fixture has zero errors/warnings.

The fresh mixed-module comparison executes 64 programs with no new failures.
C-owner/GAS-worker improves from 8/16 to 16/16. GAS-owner/C-worker remains
12/16 until the separate native token bridge is integrated. Both GAS object
identities match the accepted provider. These private owner results do not
change the accepted warning totals or qualify complete compiler self-hosting,
semantic facts, all construction/resource lifetimes, or threading.

`owner-current-summary.json` binds this prerequisite. Its source journals and
the failed handler control remain separate from the frozen native bridge.
Three superseded private executables were archived with exact round-trip
checks, saving 7,839,828 bytes; their source/object evidence is retained.

## Selected handler-call ABI repair

The latest private owner provider is `b4afb7323ee6452cffcba5ed80cc0d310c20829471d055e203e50693f27badce`.
Handler registration now leaves the exported source AST unchanged. The emitter
recognizes its original handler label and records the actual selected
`fb_ErrorFrameSetHandler` call, including its status result, three formal
arguments and emitted call text. It no longer opens an ordinary setter ABI
record and closes it without arguments or call text. The semantic validator,
schema and permanent tests are unchanged.

All seven focused procedure-state tests pass, including handler ownership,
GOSUB actions, reduced modes, native backends and rejection of fabricated facts.
Eight additional affected semantic tests, the complete selected backend gate
and the structural gate also pass.
Ninety-six owner/GOSUB native cases qualify: sixty-four execute freshly and
thirty-two bind to earlier runs through identical C. Twelve fresh Win32 objects
pass. The fresh 64-program mixed gate retains the preceding 8/12 versus 16/12
role totals and introduces no failures. Strict source-only lint of the six
private source files and root fixture reports zero errors and zero warnings.

The complete 224-module private compiler graph now passes both unsuppressed C
diagnostic profiles and its 224-unit emitted-C self-hosting fixed point.
Its warning totals are 0 GCC / 1661 Clang. These are private compiler checks,
not a refresh of the accepted corpus or proof of general resource unwinding.
The nine added Clang observations are missing-noreturn diagnostics on private
cleanup callbacks whose unsupported-resource barrier terminates. They remain
visible and counted; this checkpoint does not claim to have repaired them.
`owner-call-abi-summary.json` binds this checkpoint. Two superseded semantic
control executables are retained as round-trip-verified gzip archives.

The [native activation ownership follow-up](c-emitter-native-owners.md)
now gives recursive handlers distinct saved identities and emits native
retirement automatically. Its 32 mixed native programs and sixteen
four-thread programs pass; C worker continuation lowering remains open.

Required work before adoption:

- Compiler lowering is not implemented; the worker is frozen exact emitted C with recorded edits.
- Win64 x86-64 native selection/restoration and ordinary retirement now pass recursive and threaded controls; nested error state and abandoned activations remain open.
- The current private owner graph and focused procedure-state facts pass; broad semantic and lifetime acceptance remain open.
- Handler callbacks now use the local GOSUB lifecycle and captured context spelling; computed/shared regions remain open.
- Managed, partially constructed, temporary, parameter and abandoned activation lifetimes remain open.
- Multithreaded native-owner controls pass; wider target and complete threaded lifetime qualification remain open.
- Non-SEH native transfer does not establish cleanup for foreign C++/SEH resources.
- No project/corpus warning reduction is claimed; accepted stable counts remain 2139 GCC and 8408 Clang.
- PNG and the user-closed intermittent emission investigation remain excluded; 32 changing Optical profiles remain pending.

Receipts are under `build/c-emitter-resume-bridge-20261103/`: `summary.json`,
`abi-wrapper/receipt.json`, `runtime-contract-wrapper/receipt.json`,
`frame-contract/{receipt,nonunwind-receipt}.json`, `native/receipt.json`,
and the exact source/assembly edit journals. Header/footer corrections to
derived proof files have exact comment-only preimages in `header-lineage.json`;
tested source hashes resolve to those preimages without repeating native runs.

Canonical compiler/runtime source and installed binaries remain unchanged by
this experiment. The stage stays below 32 MiB, reuses scratch objects and runs
at most two heavy workers at below-normal priority with 90-second native caps.
The overall warning-cleanup goal remains active.

<!-- end of c-emitter-resume-bridge.md -->
