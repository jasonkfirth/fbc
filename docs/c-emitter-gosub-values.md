<!--
Project: FreeBASIC C backend qualification
File: c-emitter-gosub-values.md
Purpose: Record local GOSUB value, ownership and warning qualification.
Responsibilities: Separate accepted source, matching runtime and remaining ERROR work.
This file intentionally does NOT claim general ERROR unwinding or suppress diagnostics.
-->

# Local C GOSUB, November 3, 2026

The C backends now save nonzero 32-bit return identifiers in the runtime-owned
GOSUB stack and dispatch RETURN within the same C activation. They no longer
use setjmp/longjmp for local GOSUB. Ordinary automatic variables retain their
values after optimized GOSUB calls. Existing ERROR executable-label transport,
native assembler CALL/RET and LLVM's GOSUB lowering remain separate.

The context remains one pointer per procedure activation. Two additive runtime
APIs push and return local target identifiers. Legacy jump buffers retain their
entry points; mismatched node protocols are rejected without discarding a node.
Successful push/return preserve ERR. RETURN label retains the existing runtime
pop behavior, including its ERR reset. Empty RETURN uses the existing checked
or unchecked error path. A failed node allocation exits with runtime error 4.
The assembler-only gosub-setjmp override still selects native setjmp lowering;
it does not select an alternate C strategy.

The canonical compiler SHA-256 is `f2ee42945411cd7c72ec0eb5be74d1479cb90800c3b69ff4e7e27a4342a598d6`. All 224 modules reach an
exact generated-C fixed point. The compiler retains zero GCC and 1,652 Clang
warnings. The full backend suite, fifteen sidecar tests, structural checks and
strict source-only lint pass. The sole lint warning concerns a test success
marker, which is intentional executable evidence.

The permanent native matrix passes 88 executions under both C producers,
GCC/Clang, GNU11/GNU23 and O0/O2. It checks managed values, dynamic/fixed arrays,
floating/wide values, recursion, nested/computed GOSUB, selector side effects,
RETURN label, thread-local state, empty RETURN, allocation refusal, and one
thousand balanced early owner exits per lifecycle execution. Direct API checks
also exercise 513 nested nodes, unsigned target limits and real legacy
setjmp/longjmp. Earlier 48 dispatch executions, including eight previously
failing optimized automatic-value cases, bind to the canonical provider through
six exact-C identities. Twelve Win32 objects and three native GAS64 controls pass.
The final private strategy also passes eight forced-option native runs and
matches both original GAS64 strategy emissions byte for byte.
The canonical forced C option also matches sixteen completed permanent native
executions through exact generated-C identity.

The fresh mixed GAS/C comparison runs 64 programs with no new failures. It
retains the same 24 existing ERROR lifetime failures, twelve per provider.
This local GOSUB repair does not qualify cross-procedure resource unwinding,
partial construction, naked procedures, or LLVM's automatic-value behavior.

| Queue | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Current compiler | 224 | 0 | 1,652 |
| Unit tests / fbcunit | 703 | 15 | 828 |
| Production corpus/projects | 1,117 | 443 | 2,569 |
| Additional stable projects | 207 | 1,681 | 3,359 |
| Total stable profiles | 2,251 | 2,139 | 8,408 |

The comparable stable queues lose one Clang observation, with every other
warning class unchanged. This repair closes a correctness defect; it is not
a large warning-count reduction. The larger continuation-label, startup and
ABI-padding families remain explicitly visible. The previous complete bound
snapshot was 2,141 GCC / 8,490 Clang across 2,283 profiles. Its 32 Optical
profiles changed after qualification and remain pending stable current inputs;
they are excluded from the current stable table. Twelve PNG profiles remain
excluded at the user's direction. No Optical sources are changed here.

Receipts are in `build/c-emitter-c-gosub-20261103/`: `summary.json`,
`canonical/receipt.json`, `audit/receipt.json`, `native-canonical/receipt.json`,
`canonical-gates/receipt.json`, `canonical-runtime/receipt.json` and
`mixed-native/receipt.json`. The private audit database has an exact verified
gzip preimage, avoiding another retained full database. Source promotion has
exact guarded preimages in `promotion-source-edits.json`.

The new compiler needs a runtime containing its two new GOSUB APIs. Validation
uses the matching private prefix in this stage. Installed compiler/runtime
binaries and the prior SDK prefix remain unchanged. Work uses reused scratch
objects/C, at most two heavy workers, below-normal priority, bounded native
timeouts and a 64 MiB stage cap. The overall warning cleanup remains open.

## Current compiler after pointer ordering

The [ordered-pointer repair](c-emitter-pointer-order.md) has a complete current
canonical compiler graph: 224 exact C fixed points, four changed C outputs
recompiled and 220 exact objects reused. Its compiler SHA-256 is
392237bf3f151ab7b986b50d6360f76227fd2620af23261fa0e9067adcd1d041.
It retains zero GCC and 1,652 Clang observations, all in the previously
documented padding/global-initialization classes. Backend, semantic-origin and
pointer value bindings pass. No runtime archive or installed binary changes.
The historical stable corpus table above is not a refreshed full-queue count;
the new selected Exampleageddon cohort separately loses 30 GCC warnings.
Private ERROR integration and broader current corpus qualification remain open.

<!-- end of c-emitter-gosub-values.md -->
