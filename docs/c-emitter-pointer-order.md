<!--
Project: FreeBASIC C backend qualification
File: c-emitter-pointer-order.md
Purpose: Record native address ordering and null boundary evaluation.
Responsibilities: Explain the C lowering contract, preservation checks and warning delta.
This file intentionally does NOT claim a refreshed whole-corpus warning total.
-->

# Ordered pointer comparisons

FreeBASIC's native backends compare unsigned native addresses. Those values
include null, unrelated objects and procedure addresses. C relational pointer
operators instead describe positions within one object. The C emitter now
converts ordered pointer operands to the target's unsigned native integer before
comparing them. Equality retains its existing typed-null and procedure-identity
handling. Each operand remains a single evaluation.

Null is the lowest address. Comparisons equivalent to nonzero or equality use
those operations. A comparison with a constant result, such as pointer >= 0,
retains evaluation through a discard-value expression: ((void)(operand), 1).
Its private expression node owns the operand until normal expression teardown.
This preserves side effects and BASIC's integer/Boolean result conventions
without introducing unsigned constant-range warnings for the pointer test.

The canonical change is in src/compiler/backend/c/ir-hlc.bas. Two permanent
fixtures, tests/compiler-support/c-pointer-order.bas and
tests/compiler-support/c-pointer-null-order.bas, are included in the existing
backend runner. No previously functional example is changed.

## Verification

The final matrix passes 64 old/new C executions and four GAS controls under
both C producers, GCC/Clang, GNU11/GNU23 and O0/O2. Native assembly remains
byte-identical before and after the C-only change. Tests cover 102 pointer
pairs per general value execution, full-width/high-bit addresses, unrelated
objects, procedure pointers, comparison values, branches, inverses and operand
evaluation counts. Each null execution checks four addresses and 52 observed
pointer-operand evaluations. Eight additional objects cover both fixtures on
Win32, Linux x86-64, ARMv7 and AArch64. These are object checks, not execution
proof for those foreign targets.

All six Exampleageddon profiles with the retained GCC ordered-null warning
class are re-emitted and compiled before and after under the same unsuppressed
flags. Their GCC total falls from 50 to 20, removing all 30 observations of
that class. Clang remains at 21; no selected real profile gains a warning class
or count. Full example source hashes remain unchanged. These selected counts
do not replace the historical full-queue totals.

The null fixture deliberately includes unsigned mathematical reference
comparisons and impossible failure branches. Its raw C diagnostics remain
visible: the reference constant-range diagnostics persist, and Clang identifies
failure arms made unreachable by the now-proven pointer results. Those test
observations are not counted as corpus warning reductions.

The complete permanent backend suite, four semantic pointer-origin checks and
the full compiler structure guard pass. Canonical HLC and both permanent tests
pass strict source-only lint with zero findings. One HLC object and its C fixed
point reuse the other 223 qualified compiler objects. The final candidate hash
is 301dfe1122f141eb27505a925d3fc1df30c010534f93f0e59534785785f255e1.

## Preservation and unfinished work

Exact compressed preimages preserve the dirty HLC source and backend runner;
the runner changes only by adding the two fixture entries. Documentation and
changelog updates have separate guarded preimages. Installed compiler/runtime
binaries and runtime archives remain unchanged. Work uses at most two heavy
workers, below-normal priority, bounded commands and a 32 MiB stage cap, with
reused scratch outputs and no asset-tree or archive copies.

The initial HLC bootstrap changes four of its own emitted pointer comparisons;
the second bootstrap closes that fixed point. A metadata-key error is resolved
from the authoritative installed-input profile without recompiling objects.
The initial test-driver preprocessor error is replaced by the completed
explicit macro-presence check. Promotion's initial lint guard expected staging
advice; the permanent test paths correctly produce zero findings, and guarded
resume completes the journal without repeating source mutations. These early
records remain as classified harness observations, not unresolved compiler
failures.

The complete canonical compiler graph refresh is now closed. All 224 modules
are re-emitted from current canonical paths. Four changed C outputs are compiled
with both unsuppressed tools; 220 byte-identical outputs retain their exact
qualified objects. All 224 C fixed points pass. Current compiler diagnostics
are zero GCC and 1,652 Clang observations: 1,639 ABI-padding observations and
13 generated global-initialization observations. Those intentional classes
retain the existing ABI and initialization contracts.

The fully refreshed compiler also passes the permanent backend suite and four
semantic-origin checks. Four exact C identities bind the final pointer matrices
without recompiling their unchanged C. Two exact GAS assembly identities and
two fresh GAS executions close the native producer controls. Compiler SHA-256:
392237bf3f151ab7b986b50d6360f76227fd2620af23261fa0e9067adcd1d041.
Receipts: build/c-emitter-pointer-order-compiler-20261103/verification.json.
Installed binaries and runtime archives remain unchanged. This refresh uses
one reused scratch C file and four new objects under a separate 32 MiB cap.
The earlier one-module bootstrap proof remains historical component evidence;
the complete current compiler graph is the authoritative provider now. The private ERROR
continuation work also needs rebinding to this new canonical source snapshot
before integration. Native-handler GOTO recovery remains unqualified: shared
ordinary code may still lead to RESUME, so a jump alone cannot establish that a
C worker is abandoned. That recovery mechanism is not changed by this repair.
The remaining broad corpus/project checks and the user's PNG exclusion remain
in force. Overall warning cleanup is still open.

Receipts: build/c-emitter-pointer-order-20261103/summary.json and verification.json.

<!-- end of c-emitter-pointer-order.md -->
