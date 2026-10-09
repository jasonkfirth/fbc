<!--
Project: FreeBASIC compiler semantic tooling
File: compiler-semantic-foundation-audit.md
Purpose: Map tooling foundation requirements to compiler semantic facts.
Responsibilities: Record coverage, ownership boundaries, and verification commands.
This file intentionally does NOT contain linter rules or project policy.
-->

# Compiler semantic foundation audit

This audit covers schema 27 and the independent Python reader. The compiler
owns language facts and their provenance. A consumer owns its rule selection,
suppression language, project configuration, and presentation formats. Keeping
that boundary prevents a compiler transport from acquiring one tool's policy.

## Coverage

| Requirement | Status | Implementation and boundary |
| --- | --- | --- |
| Compiler semantic-model ingestion and validation | Implemented | `FBCSEM` and `END` or provisional `RECOVERY` define the transaction. The independent `Model` reader validates record shapes, identities, references, totals, ordering, capability groups, and mode exclusions. `build_scripts/validate-compiler-semantic-model.py` provides a bounded command-line validation entry point. Frozen self and corpus audits retain the reader's complete import closure. |
| Source encodings and coordinates | Implemented | `FILE` records the selected source encoding and exact byte revision. `LOC` records source occurrence, UTF-16 coordinates, byte offsets, and mapping state. Validation decodes requested offsets once per verified revision and encoding, rejects split characters or conflicting coordinates, and checks complete mapped lines without rejecting legacy bytes on unrelated lines. |
| Macro provenance | Implemented | `MD`, `MT`, `MI`, `MA`, `MS`, `MC`, `ME`, `ML`, and `MR` preserve definitions, tokens, invocations, arguments, substitutions, callbacks, results, lifetimes, and source origins. Compact mode explicitly reports unavailable macro graph capabilities instead of inventing provenance. |
| Symbol and ownership indexing | Implemented | `S`, `T`, `DCL`, `H`, `ORIG`, `OWN`, `ST`, `STE`, `BLK`, and `BEND` provide stable identities, declaration occurrences, relationships, source contexts, statement owners, and lexical containment. The reader validates module ownership, origin/statement occurrence agreement, and physical attachment ownership even for identical repeated includes. A persistent project index remains a consumer database. |
| Control-flow graphs | Implemented conservatively | `PH`, `NP`, `EV`, `CB`, `CN`, `CE`, and `CL` describe exported procedure phases, evaluation order, blocks, nodes, edges, and labels. Label transfers require an actual label in the same phase and agreement with native branch or jump-table targets. Unknown indirect and assembly transfers remain explicit unknown edges. This is not an SSA graph or an interprocedural call graph. |
| Lifetime facts | Implemented conservatively | `I` records selected constructor and destructor operations, including temporary and scope-exit cleanup. Procedure exit labels and CFG edges retain actual cleanup paths where advertised. The model does not infer escape, allocation success, resource ownership, or callee behavior. |
| Configuration | Implemented | `CTX`, `OPT`, and `USE` preserve effective compiler and language option snapshots and attach them to accepted facts. `Q` and `Y` preserve module target and primitive layout. |
| Suppression machinery | Consumer-owned | `OPT` records compiler warning configuration and `DI` records diagnostics actually emitted by fbc. Linter suppression comments, unused-suppression reporting, and rule-specific exceptions remain consumer policy. `FILE`, `SRC`, and `LOC` provide the exact source evidence needed to implement them safely. |
| Output formats | Implemented transport boundary | FBCSEM is the canonical versioned ASCII TSV transport with percent-escaped bytes and transactional publication. FBCDIA and FBCLNK are separate compiler diagnostic transports. JSON, SARIF, editor messages, and other presentations are consumer renderings; the validator offers text and JSON validation summaries without redefining the model. |
| Source caching | Implemented | The producer hashes the actual opened stream and verifies it again before publication. The reader has a per-operation cache keyed by resolved path, byte count, and SHA-256, reads each unique revision once, shares coordinate decoding across equal verified byte revisions and encodings, and discards cached bytes after validation or edit planning. |
| Source identity | Implemented | `D` identifies opened dependencies, `FILE` identifies byte revisions, `SRC` identifies individual opens, and `SRE` records closure verification. Repeated includes retain distinct occurrences while sharing a revision when appropriate. Logical `#line` names never replace physical identity. |
| Target and dialect handling | Implemented | `Q`, `Y`, `CTX`, `OPT`, and `USE` retain target, CPU, backend, pointer size, byte order, WString unit size, dialect, language defaults, and the context active when each fact was accepted. |
| Safe-fix infrastructure | Implemented as a non-writing gate | `Model.plan_edits()` requires `END` and accepts only mapped and physically eligible binding, declaration, or expression `LOC` subjects with validated origin and ownership. It revalidates source digests and coordinates, checks replacement encoding, rejects missing, empty, overlapping, stale, or oversized edits, and returns original and updated bytes with both digests. Provisional recovery is not edit authorization. The consumer must recheck the expected digest and perform its own atomic replacement. |
| Project-specific policy | Consumer-owned | The compiler supplies facts and explicit availability, not naming rules, risk thresholds, path exclusions, or organization-specific policy. Consumers combine their configuration with `CTX`, source identity, and capability records. |
| Resource and work bounds | Implemented | Producer module buffers, identities, details, macro history, coordinate lines, dependencies, and specialized indexes are bounded and fail closed. `ReaderLimits` additionally bounds sidecar bytes, record bytes/count, individual and aggregate source bytes, edit count, and planned output. Declared arities and action counts are compared with actual bounded records before ordinal checks; they cannot drive count-sized allocations. Coordinate validation shares decoding across repeated revisions. |

## Safe consumption sequence

1. Invoke fbc with the exact target, dialect, includes, defines, and semantic
   mode required by the project.
2. Require compiler success and a complete `END`, unless the consumer has an
   explicit provisional-recovery workflow.
3. Parse with explicit `ReaderLimits` and validate all identities and groups.
4. Before source-backed analysis or edits, validate `FILE` revisions. Before
   projecting editor ranges, also validate every mapped `LOC` coordinate.
5. Apply consumer suppression and project policy only after provenance checks.
6. For a fix, require `END`, use `plan_edits()`, recheck each expected SHA-256 immediately
   before writing, and publish through an atomic same-filesystem replacement.

The planner never writes a source file. This is intentional. Planning can be
shared by editors, linters, and review tools, while each caller retains control
of backups, permissions, line-ending policy, and atomic publication.

## Verification

The focused reader and compiler checks are:

```text
python build_scripts/test-compiler-semantic-model.py --fbc bin/fbc.exe --backend gcc \
  --test test_reader_bounds_sidecar_records_and_source_cache \
  --test test_standalone_validator_reports_machine_readable_status \
  --test test_safe_edit_planning_requires_exact_fresh_nonoverlapping_locations \
  --test test_file_revisions_include_occurrences_and_remaps \
  --test test_physical_locations_map_encodings_boms_and_line_endings
```

The review regressions additionally exercise swapped source occurrences,
forged billion-element counts with an allocation guard, variable/foreign/native
CFG target mismatches, legacy bytes before mapped lines and after forged mapped
boundaries, repeated-revision decoding, and unmatched recovery points:

```text
python build_scripts/test-compiler-semantic-model.py --fbc bin/fbc.exe --backend gcc \
  --test test_physical_locations_require_subject_origin_and_owner \
  --test test_reader_rejects_untrusted_counts_before_allocating \
  --test test_control_flow_targets_require_phase_labels_and_native_agreement \
  --test test_legacy_bytes_do_not_invalidate_other_physical_lines \
  --test test_malformed_unmapped_lines_preserve_later_encoded_locations \
  --test test_repeated_revision_coordinates_are_decoded_once \
  --test test_recovery_points_remain_readable_and_cannot_be_edited
```

Validate a retained model and its current source bytes with:

```text
python build_scripts/validate-compiler-semantic-model.py model.fbcsem \
  --validate-locations --format json
```

The complete compiler suite remains `make compiler-semantic-model-test`.
Passing focused checks is not a claim that every backend, target, corpus, or CI
job has run.

### Local repair verification, 2026-10-08

The rebuilt compiler passed 32 focused reader and compiler tests, including all
seven review regressions above. Recovery diagnostics and source-construct
emission were also checked with GCC, GAS64, and LLVM; enabling the observer did
not change diagnostics or generated output. The build reused the existing
object directory. The checked executable replaced only the workspace compiler,
with its previous binary retained for rollback.

The changed FreeBASIC module passed strict Windows lint with zero errors and
two existing warnings: FBL301 for shared state and FBL-DOC-FILE-002 for resource
ownership documentation. Neither warning was suppressed by this repair.

A broader GCC run during the repair executed 185 test methods but was not
clean. Two failures in `test_interned_callback_headers_preserve_parameter_bindings`
and `test_declaration_repetition_inputs_preserve_contracts` also reproduced with
the saved pre-repair reader and compiler: `Invalid source assignment destination`.
Link/object tests additionally lacked tools expected under the workspace SDK,
including `llvm-nm.exe`, `clang.exe`, and Win32 assembler/compiler tools. These
are separate baseline or environment problems, not evidence of complete-suite
qualification. The final per-line decoder was subsequently verified by the
focused checks, not by a second complete-suite run.

### Reused formal repair verification, 2026-10-09

The original-default failures above were repaired by exporting the typed
written formal before signature reuse frees its parameter list. The reader
checks the original initializer against the explicit canonical signature and
matching procedure owner, rejecting replacement cycles. Both affected test
methods passed with GCC, Clang, and LLVM. The compiler build reused its existing
private object directory and did not replace the installed executable.

<!-- end of compiler-semantic-foundation-audit.md -->
