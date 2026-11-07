<!--
Project: FreeBASIC compiler verification
File: c-emitter-error-owner-current.md
Purpose: Record the current-source private ERROR-owner integration and its limits.
Responsibilities: Bind compiler construction, native behavior and outstanding work.
This file intentionally does NOT claim canonical promotion or complete unwinding.
-->

# Current-source ERROR-owner integration, November 2, 2026

This integration remains private. Canonical compiler sources, corpus/project
inputs and installed binaries have not changed. The accepted queue totals remain
2,142 GCC and 67,531 Clang warning occurrences. The input check verifies all
9,223 source/protected hashes behind that receipt.

The current private compiler is
`build/c-emitter-error-owner-integration-20261102/r11/fbc.exe`, SHA-256
`517eda5550f6742601e65d867f93428582ce99bb1caca7bdf8c265e2d9c1a0fd`.
Its complete 224-unit graph passes fresh GCC/Clang object checks and a 224-unit
C fixed-point check. The full-provider native checks pass 256 lifetime cases,
eight module-startup cases and 20 Win32 object checks. Win32 execution is not
claimed. The runtime/observer dependency closure binds 407 SDK headers.

## Rebase and corrections

The old v62 source snapshot was unavailable. The stage reconstructs its owner
fragment from retained preimages and literal controller substitutions under
`E:/fbc-c-audit-01a166ce`, records their hashes, and rebases the owned changes
onto the accepted current compiler. It does not replace canonical files with
an old compiler snapshot. The exact reconstruction and revision changes are
under `build/c-emitter-error-owner-20261102/`.

The integration separates cleanup registration at activation entry from the
checkpoint established when a handler is selected. Owner/GOSUB values use heap
storage across returning transfers; cleanup-only workers retain references to
the resources and array descriptors their callbacks actually need. The latter
keeps unrelated locals in their original C scopes and types.

Fixed and dynamic STRING arrays use the existing declared runtime descriptor
view. Storage is initialized before source declarations so an early transfer
can retire an activation without inspecting uninitialized descriptors. The
array observer checks actual string deletion and dynamic allocation release.
It does not register or perform cleanup on behalf of the BASIC source.

Capture types are selected after the private read-only implementation's
parameter contract is known. Return storage uses the procedure's declared C
result type before frame retirement. The retained const-result control emits
two qualifier warnings per object with r9 and none with r10; the current full
compiler also executes both borrowed-result return forms.

The cleanup-boundary hook now uses its existing handler-tree predicate.
Unconditionally inserting a label had retained empty module constructors.
The current control checks that those constructors disappear while a separate
reader still observes a real initialized managed global. Scalar reference
declarators omit redundant grouping; pointer-to-array grouping is preserved.

## Diagnostics and execution receipts

| Compiler cut | GCC warnings | Clang warnings |
| --- | ---: | ---: |
| Accepted canonical compiler | 0 | 4,755 |
| Private r7 | 10 | 22,123 |
| Private r9 | 1 | 6,937 |
| Private r10 | 0 | 6,936 |
| Private r11 | 0 | 5,109 |

Private r11 retains 1,639 padding, 3,145 buffer-use, 301 declaration-placement,
13 startup, nine missing-noreturn and two reserved-identifier warnings. All
remain enabled. The 301 placement warnings come from return-slot declarations
being written to the entry body rather than its preceding declaration section;
that correction and its qualification remain unfinished. The nine noreturn
observations belong to the explicit unsupported-resource cleanup barrier.

The native lifetime matrix covers both C producers, GCC/Clang, GNU11/GNU23 and
O0/O2. It retains the original global/LOCAL `-ex` and `-exx` regressions, owner
return and recursion, registration timing, worker transfer, handler GOSUB,
fixed/dynamic array transfer before and after declaration, and const results.
Source-only strict Windows lint passes the four r11 source files and the five
fixture files. This does not qualify compiler-backed semantic lint.

Receipts:

- `build/c-emitter-error-owner-integration-20261102/r11/receipt.json`
- `build/c-emitter-error-owner-20261102/lifetimes-full-r11/receipt.json`
- `build/c-emitter-error-owner-20261102/module-startup-r11/receipt.json`
- `build/c-emitter-error-owner-20261102/const-return-control/receipt.json`
- `build/c-emitter-error-owner-20261102/canonical-queue-input-check.json`
- `build/c-emitter-error-owner-20261102/summary.json`

The startup controller's initial invalid string initializer and reused C
scratch failure are preserved separately. The completed receipt comes from
the corrected controller and actual successful executions.

## Required work

Canonical promotion is not qualified. Managed UDTs, partially constructed
objects, shared ordinary/handler control-flow regions, temporary and managed
results, varargs, 128-bit values, threading, other target execution and semantic
source/model facts remain open. The unsupported-resource barrier exits with
an error; it is not a production unwinder. These tests do not establish those
contracts and do not replace the outstanding project/corpus qualification.

The GCC project triage freshly reproduces 171 label-extension warnings and
66 possibly-uninitialized warnings in the selected Scorched Earth graphics
profile, and 89 label-extension warnings in the selected fb-VNC UI profile.
Those are unchanged accepted-C outputs under the original diagnostics.
`build/c-emitter-gcc-triage-20261102/` retains the receipts. This is evidence
for the next control-flow repairs, not resolution of those warnings.

The existing remaining source/target coverage, array allocation-size and other
descriptor/runtime contracts in the current-status report are still required.
PNG work remains excluded.

The owner and compiler-integration stages share a 64 MiB retained-artifact
limit, use one C/object scratch set per worker, and run no more than two heavy
workers at below-normal priority. Closed binaries, objects and databases are
compressed only after byte-for-byte verification; journals retain original
hashes and archive paths. No compiler or runtime was installed.

<!-- end of c-emitter-error-owner-current.md -->
