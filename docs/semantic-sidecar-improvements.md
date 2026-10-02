<!--
Project: FreeBASIC compiler tooling
File: semantic-sidecar-improvements.md
Purpose: Record the source-driven semantic exporter improvements and their checks.
Responsibilities: Explain consumer-visible facts, compatibility, verification, and limits.
This file intentionally does NOT claim complete program effects or alias analysis.
-->

# Semantic exporter improvements from the corpus review

The [source-to-export review](fb-corpus-semantic-content-review.md) identified
missing associations even in models that passed structural validation. Schema
25 adds the following compiler-owned facts for editor and linter consumers.

| Information | Representation | Availability |
| --- | --- | --- |
| Surviving typed expression observations | H node/expression `source-expression` links | Full models; partial lowering coverage |
| Written variable/member occurrences | H node/binding `source-binding` links | Full models |
| Read, write, update, address, BYREF, and callee roles | ACC binding-ordinal records | Full and bindings models; uncovered routes remain unknown |
| Omitted optional arguments | K argument `default-argument` boolean | Full models |
| File statement operations | SOP linked to ST/STE identities and ranges | All modes |
| Mode and analysis availability | CAP per-module records | All modes |

These additions use the statement/compound identities and OWN relationships
introduced alongside this work. Readers obtain source locations from the
associated expression/binding or statement LOC instead of guessing from
overlapping spans or debug-only line markers.

## Design and source constraints

Expression association chains live for one module and are immutable. AST copies
can share them, and fresh pool allocations reset their handles. Several parser
precedence observations may refer to the same surviving AST. The exporter
publishes these observed relationships without claiming an expression was
executed. A missing link alone does not identify why an expression disappeared.

Access roles are captured at the actual variable/member binding and updated
by the assignment, address, argument, or procedure-pointer grammar route.
Field/index receivers retain their own read occurrences. An explicit pointer
dereference does not classify the pointer variable as written. BYREF is an
address/argument role, not a claim that an unavailable callee writes.

The default-argument flag is captured before `hCreateOptArg` clones the default
tree. It distinguishes omitted and explicitly supplied equal values, while
the existing passing-mode property continues to describe ABI convention.

File operations are observed after their grammar route succeeds. Graphics
GET/PUT and identifier lookalikes do not become file operations. Statement
ownership supplies full source extents and the generated operations, without
implying that a runtime OPEN or transfer succeeded.

Capability records separate a valid completed artifact from analysis coverage.
External-call effects and alias analysis remain unavailable. Source lifetime
relationships, access coverage, and expression lowering are explicitly partial.
Bindings mode exposes access roles without retaining the full AST. Consumers
must request full models for AST argument/default-origin facts.

All new storage is bounded, checked, reset on module rollback/restart, and
released at exporter shutdown. The existing 256 MiB staging limit remains.
Models that exhaust a resource still fail rather than advertise completion.

## Corpus findings and corrections

The memory-driver model now has 131 expression/node links, 128 binding/node
links, and 125 source access roles. BASICVERSE has 3,371 expression/node links,
1,873 binding/node links, 1,975 access roles, three file statement operations,
and 23 marked default arguments. These are the actual inspected source inputs
and compiler options from the review, not invented source graphs.

The broader check exposed duplicate symbol/macro-declaration origins in OHR's
vector declarations under LLVM and Clang. Repeated visits to the same symbol
and expansion now publish that declaration-origin relationship once, using
a bounded module-local set. The independent reader continues rejecting
duplicate origin records. OHR's pathfinding module passes all three backends
after this producer correction.

## Consumer compatibility

The VS Code decoder in
`/home/jkfirth/vscode/fb-vscode-language/lib/compiler-semantic-sidecar.js`
recognizes the schema, validates the new links/owners, and exposes accessRoles,
statementOperations, capabilities, and expression relations. Full and bindings
models were decoded from the real compiler; forged access references are
rejected. Its regression checks invalid statement operations and owners.

The fb-linter reader in
`/home/jkfirth/fb_corpus/fb-linter/src/fb_linter_semantic.bi`
accepts the current versioned core layouts and the declared provenance record
shapes while preserving footer/detail counts and rejecting unknown versions
or records. Earlier MAP layouts retain their original width. Compiler traces
confirm acceptance of schema 25. FBL428's pointer-size fixture reports exactly
the two expected compiler-backed findings, and the shift fixture reports its
four expected diagnostic-based findings.

The linter's new source-role/effect rules remain future consumer work. This
change makes the facts available and keeps its existing compiler-backed rules
operating; it does not silently enable rules based on unsupported effects.

## Verification

- 94 semantic contract methods pass with optimized and assertion-enabled builds.
- All 174 Make-selected compiler modules pass 522 GCC/LLVM/Clang cases with
  export off/full/expression, independent validation, and equal emitted code.
- The eight corpus review invocations pass on all three backends after the
  OHR origin fix. Their models retain the source flags and main-module roles.
- The editor decoder's 40 enabled unit tests pass; separate real compiler
  full/bindings checks validate access and capability transport.
- Compiler structure validation covers 245 documented files and 21 host graphs.

New regressions cover default provenance, causal expression/node relationships,
unexecuted short-circuit calls, source use roles in bindings mode, and accepted
file operations with precise statement owners. Runtime output and emitted code
remain independent of semantic export.

Artifacts, compiler snapshots, logs, and consumer probes are retained under
`out/semantic-sidecar-improvements-20261002/`. The schema contract is documented
in [compiler-semantic-model.md](../doc/compiler-semantic-model.md).

The verified compiler is available as `bin/fbc`, with SHA-256
`aec83ef007518d67d28fa4c484c287e77bdd377626e4a655261a706f845d63bb`.
Replacing `/usr/bin/fbc` requires interactive sudo authentication that is not
available in this session. The consumer changes are local to their repositories;
the linter compatibility diff is retained as `fb-linter-schema25.patch` in the
artifact directory for separate review and version control.

<!-- end of semantic-sidecar-improvements.md -->
