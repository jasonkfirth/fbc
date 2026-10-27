<!--
Project: FreeBASIC compiler verification
File: c-emitter-dispatch.md
Purpose: Record the constant SELECT dispatch repair and its qualified scope.
Responsibilities: Explain control-flow invariants, diagnostics and verification.
This file intentionally does NOT claim the overall warning cleanup is complete.
-->

# Constant SELECT dispatch in C

The canonical C emitter now lowers IR jump-table dispatch to a C `switch` with
explicit `goto` destinations. The target C compiler chooses the machine jump
table or comparison sequence. General indirect branches, GOSUB return addresses
and error-handler control flow keep their existing interfaces.

The previous generated arrays of label addresses made every address-taken label
in a procedure a possible computed-goto destination to diagnostic/dataflow tools.
That obscured which BASIC initializations dominated a later case. The new form
exposes the actual destinations without adding dummy initialization or warning
suppression. This repairs emitted-C structure; the source SELECT semantics are
unchanged.

## Invariants

The selector is the same already-evaluated unsigned IR temporary. Bias subtraction
wraps in its original native-unsigned or 64-bit unsigned datatype. The existing
out-of-span branch, every normalized case value and the default destination are
retained. Empty tables still consume the selector and take the default edge.
One original unique-ID allocation is retained for nonempty dispatches, preserving
subsequent source/model naming. Constant dispatch no longer marks the procedure
as needing an address-taken entry label; other indirect branches still do so.

## Qualified current-source comparison

Receipts are in `build/c-emitter-audit-20261022/switch-dispatch-v46/`.
The comparison covers all 189 Make-selected current compiler modules under
GCC 16.2 and Clang 22.1.7 at O2, with aggressive unsuppressed warning profiles.
It reuses 376 unchanged baseline controls and 206 byte-identical after-C objects;
all changed C is actually compiled. Both full compiler graphs link and their
native compiler executables emit byte-identical C for the regression.

| Compiler-source warning class | Before | After |
| --- | ---: | ---: |
| GCC, all warnings | 43,171 | 30,634 |
| GCC maybe-uninitialized | 41 | 9 |
| GCC pedantic | 12,856 | 351 |
| Clang, all warnings | 76,816 | 63,459 |
| Clang conditional-uninitialized | 51 | 9 |
| Clang sometimes-uninitialized | 5 | 10 |
| Clang GNU label-as-value | 12,635 | 130 |

The before counts use the same revised HLC source program as the after counts;
they differ slightly from the previous v45 source epoch. All dollar-token counts
remain zero. These numbers describe this compiler graph, not a new corpus sweep.
The extra five Clang sometimes-uninitialized reports point to the same existing
unsupported-type/default paths in `rtl-file`, `rtl-print` and `rtl-string` that
already warned at the range guard. The explicit switch default supplies a second
diagnostic path. Those compiler-source preconditions/recovery paths still need
repair; they are not hidden or counted as resolved.

All 378 public-definition comparisons match. There are 376 complete symbol-set
matches; two Clang objects drop only undefined helper imports (`memcpy` and the
Windows stack probe `___chkstk_ms`). The exact two changes are classified in
`compiler-matrix/results-with-import-classification.json`. Both compiler graphs
link successfully against the same pinned complete Win64 runtime.

## Runtime and structural evidence

- Eight old/new native O0/O2 regression executions pass under matching GCC and
  Clang. The fixture covers signed/unsigned limits, sparse/default dispatch,
  nested case scopes, single selector evaluation and constructor/destructor
  counts. All four adopted canonical regression executions also pass.
- Eight Win32 C objects pass with four exact public-symbol pairs. No Win32
  runtime execution is claimed.
- Four source-model packets validate; both before/after pairs are byte identical.
- Eight old/new computed/nested GOSUB executions pass, including out-of-range
  continuation and RETURN to an explicit label.
- The earlier assembly work retains eight real fbsound CPU/DSP object checks and
  sixteen native assembly-contract executions.
- Four real fbfrog corpus modules pass sixteen matching before/after C object
  checks under their saved Make context. Their aggregate GCC warnings fall
  from 3,249 to 2,506 and Clang warnings from 6,366 to 5,595. Inputs stay pinned.
- Clang's C analyzer reports six undefined-binary-result paths before and none
  after on the regression. One BASIC-source dead store remains in both versions;
  the analyzer is not reported as warning-free.
- Strict nonsemantic Windows lint reports two changed files and zero errors,
  warnings or informational findings. Physical line-ending normalization is
  separately proven to leave the qualified C byte identical.

The first native harness used a main-module name different from the source
basename and failed to link an entry point. Corrected native checks are kept in
`qualification-corrected-main/`; the earlier failure receipts remain preserved.
A read-only jump-table probe also failed to remove the original dataflow warnings
and was not adopted.

## Ownership and remaining work

Only `backend/c/ir-hlc.bas` and `tests/quirk/c-switch-dispatch.bas` are adopted by
this repair. Exact preimages and protected hashes are in `source-adoption.json`.
Installed compiler/runtime artifacts, the error ABI and AST procedure sources are
unchanged. The work directory stays below 128 MiB; the old corpus is not recopied.
The compiler structure guard has no C-backend finding. Its eleven existing
header/footer findings in seven semantic-tooling files remain recorded.

The later [v47 defined-path repair](c-emitter-defined-paths.md) eliminates the
remaining uninitialized compiler-source diagnostics without changing this dispatch
contract. The v46 numbers above remain their own source epoch.

The overall goal stays active. Alignment, qualifiers, initialization/recovery,
array virtual-origin contracts, error-owner lifetime/transfer lowering and broader
project/platform qualification remain required. The private v43 error outliner
must be rebased onto the canonical v46 emitter rather than replacing it with an
older whole file.

<!-- end of c-emitter-dispatch.md -->
