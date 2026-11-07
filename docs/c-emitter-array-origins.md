<!--
Project: FreeBASIC compiler verification
File: c-emitter-array-origins.md
Purpose: Record the C array-descriptor consumer repair and qualification.
Responsibilities: Explain virtual address provenance, native tests and open limits.
This file intentionally does NOT close runtime descriptor production or all pointer views.
-->

# Descriptor consumer address arithmetic

A legal two-element dynamic array with a lower bound near INTEGER_MAX
reproduces a C pointer-overflow diagnostic during native execution. The stored
DATA word represents the virtual zero-index address, which can lie outside the
allocation. Adding a valid BASIC index using C pointer arithmetic can wrap that
address. Ordinary GCC/Clang runs succeed, and checked BASIC output already uses
integer address arithmetic; the unchecked C path needed the same ABI discipline.

The C emitter now records virtual-origin provenance on its private expression
nodes. Descriptor DATA reads can arrive as either pointers or native integer
words. Native-width casts retain that fact, and declared member lookup carries
it through nested record fields. Reading PTR, taking the descriptor slot address
and accessing unrelated pointer fields keep their existing contracts.

Pointer ADD/SUB with a proven virtual descriptor origin converts the address and
scaled offset to native unsigned arithmetic before forming the final pointer.
Each operand remains present once. Subsequent native address combinations retain
the provenance. The descriptor's layout, stored DATA bits and public ABI are
unchanged. No bounds check is removed or diagnostic flag disabled. Ordinary
pointer steps without this provenance retain their previous C lowering.

## Evidence

The permanent fixture covers one-byte and three-byte element types, positive
and negative extreme bounds, global/local/member-owned arrays, field offsets,
REDIM PRESERVE and exactly one evaluation of each index call. Both C producers,
GCC/Clang, GNU11/GNU23, O0/O2 and checked/unchecked modes pass 96 native runs.
Thirty-two of those enable Clang's trapping pointer-overflow check. Two native
controls compiled with the historical compiler reproduce the original trap
after successful emission, C compilation and linking.

The contract passes 112 C object checks, including sixteen Win32 objects, and
sixteen foreign objects for Linux x86_64, ARMv7, AArch64 and Darwin AArch64.
Object checks do not establish execution on those targets. The existing biased
descriptor and debug-descriptor suites pass, including GAS64 comparisons and
the deliberately short descriptor's retained GCC diagnostic. The ordinary
GCC/Clang backend suite passes; its Clang fixture enables the pointer check and
rejects the historical compiler in actual execution.

Bootstrap and self-host binaries match. All 224 final compiler C outputs close
the fixed point. The accepted noninstalled compiler is
`build/c-emitter-array-origin-20261102/accepted/fbc.exe`, SHA-256
`5c5249b088c86fab357a81a0613a81decfbf2ebfbb1345f95b8d1dd4f0b1ea1e`. The structure guard passes 393 documented files
and 21 host graphs. Strict source-only fblint reports three files, zero errors
and one advice item: FBL012 treats the fixture's sole PASS marker as debug
output. That marker is the native test protocol and remains without suppression.
Compiler-backed semantic lint remains outside this source-only result.

| Queue | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Compiler | 224 | 0 | 4,755 |
| Unit tests | 703 | 16 | 14,048 |
| Production | 1,117 | 443 | 10,547 |
| Additional projects | 239 | 1,683 | 38,181 |
| Total | 2,283 | 2,142 | 67,531 |

The reduction is 2,421 Clang warning occurrences, all in unsafe-buffer-usage;
GCC counts are unchanged. Forty-three selected project C outputs change and
pass their original unsuppressed profiles. Another 2,016 are byte-identical.
Twelve PNG profiles remain excluded. Installed tools and unrelated source and
changelog bytes are preserved. The stage is capped at 64 MiB.

## Open requirements

This is the descriptor consumer repair. The runtime still forms virtual DATA
addresses using C pointer arithmetic in its allocation/reallocation/copy paths;
that producer-side contract remains open. Arbitrary virtual pointers that lose
descriptor provenance through user storage or external calls, packed descriptor
views, allocation-size limits and broader target execution also remain open.
The established runtime prefix is unchanged; its use by these tests does not
qualify new runtime sources or a complete runtime origin redesign.

Receipts and preservation proofs are in
`build/c-emitter-array-origin-20261102/summary.json`. The [current status](c-emitter-current-status.md)
retains the full warning-cleanup scope and remaining work.

<!-- end of c-emitter-array-origins.md -->
