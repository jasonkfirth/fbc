<!--
Project: FreeBASIC C backend
File: c-emitter-gosub-labels.md
Purpose: Record owner and outlined-handler label retention.
Responsibilities: Explain graph ownership, AST lifetimes and qualification.
This file does not change BASIC GOSUB or error-handler semantics.
-->

# GOSUB and outlined error-handler labels

An outlined error handler has a separate C function body. A label referenced
only by that callback must not remain in the original function merely because
the BASIC procedure also uses GOSUB. Conversely, a source label shared by a
GOSUB target and an error handler must remain in both functions.

The owner now uses its proved live AST graph to collect label references, even
when the procedure uses GOSUB. Direct GOSUB destinations are typed branch edges;
generated return-site C labels are separate from source label symbols. Unknown
owner control flow retains the existing conservative path.

Each callback collects its own typed label references before loading its cloned
statements. Procedure-wide source access flags cannot distinguish different
callbacks. The callback set owns copied hash keys and borrows label symbols from
the live procedure symbol table. It never indexes the owner's consumed AST.
An earlier candidate used those expired statement nodes during helper emission;
that candidate and its partial recount were rejected and never selected.

The regression transfers to a callback-only label, a shared GOSUB/handler label,
and two RESUME NEXT continuations. GCC and Clang enforce
`-Werror=unused-label`. All command directives precede `OPTION GOSUB`, including
in the two preceding GOSUB regressions, so the compiler actually applies them.

## Qualification

Selected development compiler SHA256:
`55aa25168871aefc36edaae881c1c47a5537f35417690f419061f93d258743c1`.
Its immediate rollback is `fbc-before-gosub-labels-20261112.exe`, SHA256
`da4f90d862bb0649c1ecae0009eef5dce5d85870026274662bf293b9be3ebfa6`.
The installed release compiler and normal runtime archives remain unchanged.

The 224-unit self-host reaches an exact emitted-C fixed point with zero GCC
warnings. Both complete backend suites pass in 188.843 seconds, including
462 source-warning checks. The focused control has 18 build, rejection, object
and execution checks, including Win32 C objects and LLVM execution. Ten further
selected-prefix executions pass. Strict lint reports no findings in the five
owned BASIC files; compiler structure checks cover 408 files and 21 host graphs.

All four current queues bind the selected compiler. The 5,845-profile core queue
keeps GCC 194 / Clang 10,030, and both TurboTrek queues keep their previous maps.
The 208 extracted legacy examples lose five unused-label warnings per native
compiler, reaching GCC 3 / Clang 70. No new native warning class appears. See
[current totals and remaining scope](c-emitter-current-status.md).

The core worker completed every profile but rejected concurrent input drift at
its final guard. Its raw database remains unchanged. A separate reconciliation
proves that the new DOS errno branch and two DOS-only test blocks leave the
qualified target behavior intact; the changed DOS unistd binding is not selected
by Win64/Win32 routing. Ten controlled emissions yield five byte-identical
old/new output pairs across GCC, Clang and LLVM. The one changed fbctest profile
is freshly compiled with both native tools and retains its warning maps. This
resolves the input boundary for this batch and makes no DOS qualification claim.

The subsequent sync merges an independent gfxlib3 update, including a native C
file retained in the broad source census. That file is not a compiler/backend
gate input or a retained profile source. Compiler inputs and installed archives
remain unchanged; `gosub-label-post-merge-boundary-20261112.json.zlib` records the
new census pin separately. Native qualification of those graphics changes
remains outside this emitted-C batch.

Receipts remain in the existing `build/c-emitter-forward-views-20261105/`
directory: `gosub-label-selfhost-v3-`, `gosub-label-gates-v3-`,
`gosub-label-values-v3-context-`, `gosub-label-errno-reconciliation-`,
`gosub-label-v3-terminal-reconciliation-` and `gosub-label-v3-publish-` packets;
the raw `gosub-label-v3-queue-refresh-` database; and the three
`current-*-labels-v3-` databases. Scratch names are reused and no persistent
test directories are added.

<!-- end of c-emitter-gosub-labels.md -->
