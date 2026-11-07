<!--
Project: FreeBASIC runtime qualification
File: c-emitter-string-capacity.md
Purpose: Record checked string storage arithmetic and allocation-failure handling.
Responsibilities: Explain ownership, native diagnostics and reproducible source delivery.
This file intentionally does NOT claim whole-campaign or another-host completion.
-->

# String storage capacity and failure paths

The shared string allocator now checks signed capacities before alignment,
growth and terminator addition. Ordinary requests retain 32-byte rounding,
12.5 percent reallocation growth and the existing exact-size retry. Near the
signed limit, optional rounding/growth is omitted when it cannot fit. Invalid
counts are rejected before changing a descriptor or calling the heap allocator.
Concatenation checks combined lengths before addition. Public descriptors,
calling conventions and temporary-ownership flags remain unchanged.

Five string entry-point units now avoid copying after allocation failure.
Preserving growth retains the old descriptor and payload on failure; ordinary
assignment retains its non-preserving policy. Source temporaries still retire
through shared cleanup. A supplied concatenation output keeps its descriptor
identity so its caller can retire it. A newly allocated failed result is NULL,
matching the existing empty-concatenation convention. Temporary descriptor moves
use the source owner's mutable data field rather than discarding a const view.

| Native profile | GCC before / after | Clang before / after |
| --- | ---: | ---: |
| ST, 522 members | 837 / 820 | 14,487 / 14,470 |
| MT, 522 members | 837 / 820 | 14,494 / 14,477 |
| Startup | 2 / 2 | 24 / 24 |

Twenty fresh actual-checkout checks remove 34 GCC and 34 Clang observations,
with no class increase or diagnostic suppression. All five affected units are
GCC warning-free in ST and MT. Ten GCC objects change; 2,070 unrelated native
checks inherit exact source/tool/flag identities. Only the five units include
the new private sizing helper. No public record layout changes.

Thirty-two before/after GCC/Clang GNU11/GNU23 O0/O2 ST/MT executions preserve
the normal capacity/output hash. Tests cover rounding boundaries, exact-size
retry, retained data, concatenation, self append and the existing NULL empty
result. Deterministic heap hooks reject both allocation attempts in failure
cases, bound each real allocation at 4,096 bytes, and check descriptor/payload
preservation, source retirement, supplied-output ownership and a fully reusable
256-entry temporary pool. Single-threaded MT hooks verify balanced lock calls;
they do not stand in for the real MT runtime.

Eight additional C/C++ runs use both matched normal libraries. Four MT runs
join two workers exercising real string allocation and temporary-pool locking.
Both archives preserve 522-member order and the complete export multiset. A
rejected preflight's unnecessary runtime-context reference was removed by using
the established NULL-result convention; the export guard stays unchanged.
Twenty actual Win32 definition compiles and four C/C++ clients preserve all
16 public C symbol decorations. Win32 execution remains unverified.

The compiler image changes to e8a7f770c8f0, so all 224 self-host C identities
and both existing GCC/Clang backend regression suites run afresh with the
matched runtime prefix. All 126 runtime graph cases and three collision guards
pass across 1,256 files and 21 groups. A concurrent integration exposed a
bookkeeping error in the value controller's final storage check: its 32 passing
runs are retained, the accounting-only inverse is verified, and the authorized
192 MiB transient limit applies until integration slots retire. The final seal
checks the separate 160 MiB retained limit after cleanup.

The 148,660-byte string-capacity-source-update.zip includes every preceding
word, RND, declaration, console and UTF-span repair. Extract portable-source.zip,
then string-length-source-update.zip, then string-capacity-source-update.zip
before the first build. Separate intermediate update archives are not required.
All 613 runtime input hashes pass the unchanged delivered source verifier.
The exact portable checker bytes pass through both rebuilt libraries; the
heap-hook fixture is also included for the checkout's before/after controller.
Follow [the portable build](c-emitter-portable-build.md), then run the shipped
verify-runtime-string-capacity.py with --tool-dir after rebuilding.

Lossless compression saves 933,888 allocated bytes on one completed diagnostic
database while preserving its path and content SHA-256. Accepted evidence,
installed binaries, valid examples and unrelated dirty work remain intact. No
PNG or user-closed optional/native investigation is involved. Reused source,
object and program slots are retired; no full tree or dated test ladder is
copied. Retained data stays below 160 MiB and integration below 192 MiB, with at
most two owned heavy workers.

Another-host rebuilding, full corpus replay from this frozen source package,
foreign SDK execution and experimental compiler promotion remain unverified.
Main emitted-C totals remain broad 2,139 GCC / 6,966 Clang and Exampleageddon
3,495 / 8,916 at their measured cuts. The complete warning campaign remains active.

Evidence: string-capacity screen/native/values/Win32/archive/structure/storage/
source-update receipts in the existing qualification directory; the component
seal and verifier are in build/c-emitter-forward-views-20261105.

<!-- end of c-emitter-string-capacity.md -->
