<!--
Project: FreeBASIC semantic sidecar audit
File: semantic-sidecar-gap-audit.md
Purpose: Assess schema 19 against the compiler's semantic contracts and phases.
Responsibilities: Define the audit boundary, present verified losses, and specify a coordinated completion scope.
This file intentionally does NOT claim missing functionality is implemented or infer unavailable runtime facts.
-->

# Semantic sidecar gap audit

The semantic model is not complete. Schema 19 contains substantial symbol,
type, binding, value, expression, and typed-tree information, but it does not
preserve the compiler's complete source context, semantic relationships, or
late actions. Some source facts are missing, and there are two serious output
failure paths. The existing completion footer and passing suite do not prove
language-wide semantic completeness.

This audit, dated October 1, 2026, defines 51 work items:

| Evidence/scope | Items | Meaning |
| --- | ---: | --- |
| Reproduced behavior | 11 | Small compiler or reader probes demonstrate the loss or failure. |
| Producer/schema omissions | 27 | Reviewed compiler contracts or transformation phases have no complete exported representation. |
| Derived analyses/representations | 5 | A normalized type contract, evaluation order, CFG, use-def/escape, and effects need an explicit new representation or analysis. |
| Verification gaps | 6 | Current tests do not establish the necessary behavioral and failure coverage. |
| Optional extensions | 2 | Resolver explanations and final backend emission metadata require a separate scope decision. |

The 51 items are coordinated implementation scopes, not 51 independent bugs
or 51 proposed record tags. One source-event identity design closes several
items; one late-action graph closes several more.

The detailed [worklist](semantic-sidecar-gap-catalog.tsv) gives every item its
source/probe evidence, current behavior, proposed change, positive acceptance
cases, negative acceptance cases, and related dependencies. The
[class matrix](semantic-sidecar-class-coverage.tsv) accounts for all AST and
symbol classes. The [field matrix](semantic-sidecar-field-coverage.tsv)
accounts for every indexed layout field and distinguishes exported, partial,
derivable, missing, bookkeeping, and optional backend detail.

## What completeness means here

The finite boundary is the compiler in the captured source inventory. A
complete semantic contract must preserve each observable source/semantic
decision that its parser, resolver, typed-tree builders, and lowering stages
actually make. Each fact needs an explicit owner, source/context identity,
phase, and validity. Each current source route and active payload must either
have a representation, a documented derivation from other records, or an
explicit implementation-only disposition.

Derived analyses must report uncertainty. The compiler does not know the
runtime value of an input, every alias target, the target of every indirect
call, the exact effects of arbitrary external code, or which conditional path
will execute. A complete model can represent those unknowns. It cannot turn
them into invented resolved facts.

Comments, whitespace, and formatting remain source text. Private allocator
addresses, hash buckets, callback addresses, traversal cursors, and unused
union members are not semantic facts. Exact machine instructions, register
allocation, stack-frame placement, and debugger numbering are the optional
emission scope, G50. Language/runtime behavior that the compiler does not
describe needs a separate runtime contract, rather than an assertion inferred
from a procedure name.

## Evidence and source boundary

The audit froze a compiler executable, the compiler source tree, and public
include files before running the evidence probes. Their identities are kept
separately; the executable hash identifies the binary actually used, rather
than claiming a reproducible build of the source snapshot.

| Inventory | Count |
| --- | ---: |
| Compiler source/header files | 226 |
| Compiler subsystems | 13 |
| Indexed function/sub bodies | 3,390 |
| Parser/preprocessor token case sites | 624 |
| AST classes | 46 |
| AST operators | 122 |
| Symbol classes | 17 |
| Named layouts | 238 |
| Indexed layout fields, including union alternatives | 1,402 |
| Frozen public include files | 1,796 |
| Evidence observations | 58 |
| Unexpected probe compiler/reader outcomes | 0 |

The compiler SHA-256 is
`980403d652a68844167591775e83ac4f562a7e166ea24ca511523918fe662f8d`.
The retained evidence is in
`out/semantic-sidecar-gap-audit-20261001/`:

- `source-manifest.json`: captured source/include file hashes and binary identity.
- `compiler-inventory.json`: file hashes, named contracts, function locations,
  observation points, token dispatch sites, and field dispositions.
- `compiler-files.tsv` and `compiler-layouts.tsv`: searchable source inventories.
- `audit-validation.json`: catalog/source cross-checks, exact class/field
  coverage counts, reproduction verification, and source changes since freeze.
- `reproductions-final/results.json`: commands, hashes, expected statuses,
  record/fact summaries, and reader mutation outcomes.
- `reproductions-final/<case>/`: actual source, diagnostics, full/compact model,
  and compiler emission for each language probe.
- `reproductions-final/transport/`: disposable alias inputs and retained original
  input bytes. These probes never use production inputs as their output path.

The original probe draft contained an invalid BYDESC declaration and a derived
type without the required parent initialization support. Both fixtures were
corrected before the final run. Their initial compile failures are not exporter
findings. The final 26 language scenarios all compile with the expected status;
the deliberately invalid diagnostic-recovery case fails in both modes as
expected. The 58 observations comprise 52 language compilations, 4 transport
probes, and 2 reader corruption probes.
The two legacy-byte full models are expected to be unreadable by the supplied
reader in this dated audit. That observation records a defect, not a production
acceptance expectation.

Concurrent workspace changes after the freeze include a procedure-pointer
canonical replacement hook. That correction is not counted as an outstanding
gap in this report. The dated matrices refer to the captured contracts; future
changes require reviewing the affected rows and reproductions. None of the
production compiler files was changed by this gap-audit work.
The final validation records 16 changed compiler files since the freeze. The
serializer payload/class tables and the reproduced loss mechanisms remain
unchanged; the source-range reader changes use equivalent power-of-two
alignment checks. The retained executable remains the authority for the dated
probe outcomes.

This inventory is a breadth and traceability check. A textual hook match does
not prove a path is observed, absence of a local hook does not prove a shared
routine is unobserved, and a vocabulary entry does not prove its payload works.
The field dispositions are reviewed rules, not execution coverage. Completing
the behavioral manifest is explicitly part of G42-G48.

## Reproduced losses and output failures

| Probe | Observed result | Work items |
| --- | --- | --- |
| `same_path`, `hardlink`, `symlink` | The compact model output aliases the temporary input, changes its bytes, exits zero, and leaves an accepted `END` model with no expression facts. | G01 |
| `dev_full` | A valid compile directed to `/dev/full` exits zero with no failed-write diagnostic. | G02 |
| `node_plain`, `node_debug` | All N source lines are zero without `-g`; with `-g`, lines 7 and 8 appear along with generated/unknown line zero. | G10 |
| `operators` | IS, address-of, dereference, and index have some E concepts; O is empty. CAST, CPTR, LEN, SIZEOF, NEW, and DELETE lose source operation identity. | G12 |
| `prototype_names` | Both written formal names have declaration bindings but empty symbol metadata names. | G13 |
| `implicit_declaration` | A source implicit variable has a reference binding and compiler origin, without an implicit declaration occurrence. | G13 |
| `special_definitions` | Written constructor/destructor/operator procedures lack declaration bindings; the written operator is marked compiler origin. | G14 |
| `forward_alias` | The first FutureType source use has no B record; a forward symbol and its canonical resolved type exist. Ordinary later alias uses are present. | G15 |
| `assembly` | Assembly nodes export their kind, but no ordered instruction/text/operand token payload or operand-to-node relationship. | G25 |
| `copyback` | N contains one FB_STRASSIGN call; the generated C contains two calls, including the deferred writeback after MUTATE. | G29 |
| `unknown_node_kind`, `invalid_argument_bytes` | The reader accepts an impossible node kind and a nonnumeric argument byte property while footer totals remain correct. | G47 |
| `legacy_macro`, `legacy_literal` | Legal unmarked Latin-1 input compiles successfully, but full metadata contains raw 0xE9 and fails the supplied UTF-8 reader. The compact model remains readable. | G51 |

Other probes confirm omissions whose boundaries are visible in the producer:
macro invocation context, preprocessing branches, OPTION history, module
constructor/destructor priorities, base/chained-constructor action ownership,
late runtime-check transfers, Unicode indexed-read calls, and source DATA
ownership. These are distinguished from the eleven reproduced work-item scopes
in the catalog.

Several suspected losses were ruled out:

- Static and global destructor wrapper bodies do produce P/N records. The
  missing part is their explicit instance/registration/lifetime relationship,
  condition, and order.
- DATA descriptor arrays, initializer trees, link expressions, and stored
  values are exported. The missing part is source DATA item and label ownership,
  not the whole lowered runtime representation.
- `CVA_START` retains its ordinary formal-variable binding through the shared
  expression route. Its source operation/ABI/effect graph is the larger gap.
- Virtual calls retain their statically selected method and override/slot
  information. Their runtime-selected dynamic target is legitimately unknown.
- ARRAY bounds, exact C values, ordinary signature/formal links, default trees,
  namespace imports, canonical replacements, and the unique dependency summary
  already exist. Their existence is reflected in the matrices.
- A denied include that was never resolved/opened was absent from the dependency
  list in the supplemental probe. Do not claim an observed failed-open
  dependency defect from that result. Include-attempt/outcome context remains
  absent, and `fbIncludeFile` records a dependency before the final OPEN.

## Where information is lost

The current phase order explains why filling a few serializer switches would
not complete the model.

1. The lexer knows physical/logical coordinates, token spelling/suffix/escapes,
   include contexts, and active macro stacks. Only selected physical ranges and
   unique filenames become public records.
2. The preprocessor knows actual expansion arguments/results, conditional
   branches, option changes, include attempts, and definition lifetime. Definition
   bodies are exported, but this event history is not.
3. The parser/resolver knows source constructs, selected symbols, declaration
   names, original argument positions, source operation intent, and implicit
   declaration decisions. Observation is uneven and E/B/O/I do not form a common
   source graph.
4. `astUpdate` lowers/optimizes initializers, bitfields, string operations, and
   cleanup before procedure serialization. Source form and adaptation reasons
   can already have disappeared.
5. `astProcEnd` adds base/member initialization, cleanup, profiling/error state,
   result loads, and exit behavior. It exports body roots and live symbol
   metadata, but not all wrapper/block/side-list ownership.
6. `astLoadCALL` and other load routines add or execute deferred copybacks,
   hidden ABI actions, check-handler transfers, and Unicode indexed-read calls
   after the exported tree snapshot. These affect execution semantics.
7. Backends assign final ABI placement and machine/debug metadata. That phase
   needs explicit availability and optional emission coverage, rather than
   pretending early offsets and signatures are final machine layout.

The source graph, typed tree, semantic action graph, and optional emission graph
therefore need separate phase identities and correspondence edges. Keeping
the phases distinct also preserves compiler architectural boundaries.

## Compiler-wide coverage review

| Subsystem | Files | Current semantic contribution and remaining scope |
| --- | ---: | --- |
| core | 4 | Module/source lifecycle, target and language configuration; missing source revision/context and effective option history. G03-G09. |
| driver | 33 | Invocation, compilation attempts, output and module roles, target policies; missing manifest and reliable publication. G01-G04, G07-G08, G38-G41. |
| lexer | 3 | Observed ranges and physical flags; missing token intent and macro/include/remap identities. G03-G05, G09-G15, G23. |
| preprocessor | 5 | Definitions and retired definition snapshots; missing actual expansion/conditional/context events. G04-G07. |
| parser | 59 | Many resolved declarations/references/expressions/operators/lifetimes; missing complete source constructs, alternate binding/operation routes, and call adaptation graph. G10-G16, G23, G28, G31-G33. |
| symbols | 16 | Substantial final type/signature/layout/value/relationship inventory; missing selected payloads, source order/origin, linkage, ABI, table ownership, and snapshot contracts. G13-G23, G33, G41. |
| ast | 31 | Typed body and initializer trees with many exact payloads; source destruction, unvisited side chains, omitted block/options/scope payloads, and later actions remain. G11, G24-G37. |
| runtime lowering | 20 | Registered intrinsic signatures and typed runtime calls; missing uniform source intrinsic identity and normalized bridge/action/effect contracts. G12, G19, G30-G31, G37. |
| backend | 24 | Frontend snapshots precede some semantic lowerings and final ABI/link/assembly choices; distinguish required late semantic actions from optional machine/debug detail. G19, G21, G25, G30, G35, G50. |
| diagnostics | 2 | Compiler diagnostic codes/context are centralized; no structured sidecar diagnostics or per-fact recovery validity. G38-G39. |
| support | 22 | Containers, bounded allocation, strings, numeric policy; private state stays excluded while encoding/FP/failure outcomes need coverage. G03, G07, G23, G44, G46. |
| host platform | 2 | DOS/RISC OS numeric-policy overrides; selected semantic results/configuration and alternate policy behavior need target/host coverage. G07, G23, G44. |
| tooling | 5 | Record writers, transactions, identities, and traversal; publication, phase/capability/closure/integrity, byte-safe encoding, and behavioral verification remain. G01-G02, G29, G40-G48, G51. |

Every parser file is assigned to one of these finite source-route families:

| Parser family | Files | Required behavior/exclusion scope |
| --- | ---: | --- |
| Declarations | 12 | CONST/DEF/ENUM/OPTION, procedures/formals, UDTs/fields, types/typedefs/forwards, variables/arrays and initializers; source declarators, scopes, defaults, layout, access, and invalid/recovery variants. |
| Expressions | 6 | Atom/function/variable/constant, unary, all precedence levels, calls/member/index/type tests; source graph, selected symbols/operators, adaptations, and lookalike/ambiguous/erased exclusions. |
| Procedures | 3 | Prototype/body replacement, special headers, direct/indirect/virtual/property/operator calls, arguments/defaults, function results, BASE and constructor chaining. |
| Statements | 15 | Assignment, comments/directives, DO/FOR/WHILE/IF/SELECT variants, EXTERN/NAMESPACE/SCOPE/WITH, EXIT/CONTINUE/labels/assembly; source/CFG/access/lifetime ownership and nonexecuted branches. |
| Intrinsics | 18 | Array, casts, console, DATA, errors, file, graphics, goto/return, IIF, math, memory, ON, PEEK/POKE, sound, strings, threads, varargs, and shared dispatch. |
| Shared parser context/dispatch | 5 | Declaration/identifier/helper/top-level/context routes, statement ownership, physical/logical source context, and recovery/attempt rollback. |

Individual paths, line numbers, token cases, and observation sites are retained
in the generated inventory. These family assignments do not substitute for
G43's per-route behavior tests. Internal AST opcodes such as stack operations,
branch variants, and vectorization helpers are not all source operators.

The field matrix currently classifies 122 rows as exported, 278 as partial,
146 as missing, 10 as derivable, 459 as bookkeeping, and 387 as backend detail.
These are field-disposition counts, not percentages of semantic completeness:
union alternatives, container fields, and many record instances represent
different amounts of information.

## Coordinated completion scope

I recommend solving G01-G48 and G51 in one schema revision with one behavioral
acceptance manifest. The implementation can proceed in these stages, while
the schema and identities are designed together:

| Stage | Work | Required outcome |
| --- | --- | --- |
| 1. Reliable publication and identity foundation | G01-G04, G07-G09, G17-G19, G38-G41, G51 | Checked staged output and byte-safe fields; file revisions and invocation/module/context/phase identities; normalized type/flag/ABI policy; diagnostic/validity/capability contracts. |
| 2. Source and resolution observations | G05-G06, G10-G16, G20-G23, G28, G31 | Source statements/declarators/expressions/calls, expansion/conditional/config events, selected bindings and source operator roles, finalized symbol/layout/link metadata. |
| 3. Complete typed/lowered actions and relationships | G24-G27, G29-G30, G32-G34 | Active payloads and auxiliary edges; semantic execution order; lifetime/initialization/cleanup/registration reasons; observed late actions and source correspondence. |
| 4. Conservative analyses | G35-G37 | CFG, storage access/use-def/escape, and effect summaries with explicit unknown/conditional states. |
| 5. Acceptance and maintenance gate | G42-G48, applied throughout | Every required capability/route/payload has positive and exclusion tests; checked failure/limit/publication behavior; schema integrity and off/full/compact noninterference. |

Catalog dependency links identify work that must be designed together. Some
are mutual, such as source exits and cleanup ownership, or capabilities and
reader validation. They are not an instruction to block every change until a
strict ticket dependency graph becomes acyclic.

G49 can be included if tooling needs explanations for rejected candidates and
overload ranking. G50 can be included as a separately requested emission
artifact if consumers need final backend/debug/machine correspondence. Neither
should be silently folded into a promise of complete source semantics.

The schema should use a small set of consistent concepts: file revisions,
committed source contexts/events, symbols/types, typed nodes, semantic actions,
and relations with phase/validity. Source origin must be an explicit fact,
not a guess from a generated name or nearest debug line. Source forms that are
folded or lowered away should remain source events, without manufacturing N
nodes the compiler does not have.

The acceptance gate is behavioral:

- Every live class/payload and source route has a positive fixture that checks
  its meaning, identities, source ownership, values, relationships, and order.
- Each family has negative cases for wrong/lookalike/inactive/generated/
  unresolved/ambiguous/inaccessible/invalid behavior as appropriate. Expected
  absence is specified, including unknown indirect/dynamic targets.
- Per-kind reader checks reject semantic corruption even when record counts
  are correct. Partial/recovery/compact/capacity states cannot claim a complete
  required capability.
- Off/full/compact runs retain identical compiler decisions, diagnostics,
  emissions, and meaningful runtime results. Observation cannot execute a
  callback twice, reselect a target, change options, or reorder actions.
- Boundary and injected allocation/write/close/publication failures leave
  inputs intact and no newly published complete invalid model.

The existing suite and corpus are useful foundations. Their previous passing
results should continue to be reported as evidence of their defined contracts,
not as proof that the newly identified work is already complete.

## Reproduce and review

The retained frozen compiler and source prefix are at
`/tmp/fbc-semantic-gap-audit/fbc` and
`/tmp/fbc-semantic-gap-audit/source`. Use a fresh output directory for the
probe runner. Its alias probes operate only on files that invocation creates.

```sh
python3 build_scripts/audit-compiler-semantic-model.py \
    --root /tmp/fbc-semantic-gap-audit/source \
    --coverage-rules docs/semantic-sidecar-field-coverage.rules.tsv \
    --output out/semantic-sidecar-inventory-new

python3 build_scripts/audit-compiler-semantic-gaps.py \
    --compiler /tmp/fbc-semantic-gap-audit/fbc \
    --prefix /tmp/fbc-semantic-gap-audit/source \
    --output out/semantic-sidecar-gap-probes-new
```

The reader used for probes is identified by hash in `results.json`; these
dated observations require its schema 19 contract. To audit a later compiler,
capture its source/binary/include identities and review the changed contracts
and expected observations rather than replacing the old evidence silently.

<!-- end of semantic-sidecar-gap-audit.md -->
