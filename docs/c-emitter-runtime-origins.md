<!--
Project: FreeBASIC runtime verification
File: c-emitter-runtime-origins.md
Purpose: Record native and BASIC runtime virtual-origin repairs.
Responsibilities: Explain representation, qualification and remaining contracts.
This file intentionally does NOT claim completion of the warning campaign.
-->

# Runtime virtual origins, November 2, 2026

Array DATA holds the native address bits of the virtual zero-index origin.
PTR identifies the physical element storage. These addresses need not describe
the same C object, and DATA need not point inside the allocation. Descriptor
layout, exported entry points and calling conventions remain unchanged.

The native runtime now accumulates the descriptor bias with unsigned native
arithmetic, including the most negative signed bound. Allocation, preserving
resize and descriptor copy form DATA through uintptr_t. Copy recovers the bias
with unsigned subtraction instead of ordering unrelated pointers. The matching
four complete BASIC runtime units in C:/fb_corpus/fbrtLib use CUINT arithmetic.
The BASIC translation also checks allocation before fixed-string clearing and
resets the destination descriptor when a descriptor-copy allocation fails.

`build/c-emitter-runtime-origin-20261102/summary.json` binds the ten source
changes, saved preimages, compiler, diagnostics, SDK headers and runtime inputs.
The private runtime objects are linked before the existing archive; installed
compiler binaries and runtime archives are unchanged.

## Qualification

The matrix covers native C and both GCC/Clang emissions of the complete BASIC
implementations, GCC and Clang C compilation, GNU11/GNU23, O0/O2, and ordinary
and instrumented Win64 execution. Native C additionally uses -fno-wrapv and
signed-integer-overflow traps; generated BASIC C retains its -fwrapv contract
and uses pointer-overflow traps. No diagnostic suppressions were added.

- 672 public runtime API runs pass, covering positive/negative bounds, bias,
  allocation, preserving resize, descriptor copy, object callbacks, fixed-string
  allocation failure and a failed destination copied again.
- 48 actual BASIC caller runs pass, including checked and unchecked indexing.
- 216 runtime object checks and 108 fixture/hook object checks pass. These
  include 36 Win32 objects; Win32 execution is not claimed.
- 34 historical failures are reproduced, alongside eight historical successes.
  Allocation controls observe null-clear attempts without accessing null memory.
- Historical and current exported symbol sets match for each of the twelve
  Clang Win64 runtime comparisons. The dependency closure binds 90 SDK headers.

The harness needed a header-free allocation-hook declaration for generated C
under -nostdinc and one corrected historical expectation: the old native bias
calculation traps before allocation failure for the most negative bound.
The ledger preserves both harness corrections and the complete unchanged-input
checkpoints; passing groups were retained by whole-group digest verification.

All 2,283 selected non-PNG profiles were re-emitted. The 224 compiler units keep
their C fixed point; eight changed project outputs were compiled with both
tools and 2,051 project outputs were byte-identical to their qualified inputs.
The selected queues contain 2,142 GCC and 67,529 Clang warnings, two fewer Clang
warnings than the preceding descriptor-consumer receipt. Runtime C fixture
diagnostics are recorded separately from these emitted-C queue totals.

Source-only strict Windows lint reports zero errors and 19 existing whitespace
findings in the five corpus files, down from 23 findings before the changes.
No new lint finding was introduced. Compiler-backed semantic lint remains open.

## Remaining contracts

The producer arithmetic repair is complete within this matrix. Metadata query
allocation tests still inspect DATA instead of PTR. A two-byte ordinary BASIC
REDIM PRESERVE example and an independent C API probe both reproduce incorrect
bounds when an allocated array's virtual DATA address is zero. Their receipts
are `zero-basic-receipt.json` and `zero-origin-receipt.json`; this is an open
query contract, not a passing check.

Allocation-size overflow, broader pointer views and packed descriptors,
cross-procedure ERROR handling and the remaining selected/unselected corpus
coverage retain their existing open status. Twelve PNG profiles remain excluded.

<!-- end of c-emitter-runtime-origins.md -->
