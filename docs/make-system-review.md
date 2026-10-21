<!--
Project: FreeBASIC build system
File: make-system-review.md
Purpose: Track the full Make system review and its verified corrections.
Responsibilities: Preserve review scope, findings, evidence, and remaining work.
This file intentionally does NOT claim native qualification for every target.
-->

# Make system review

Date: 2026-10-20. Source review and focused verification complete.

The subsequent [consolidation record](build-consolidation.md) covers native
integration, generated-source hygiene, configuration tracking and failed-link
publication. The inventory and receipts below remain the original review
snapshot; later fixes do not retroactively change that evidence.

The review covers the root `GNUmakefile`, all 53 `mk/**/*.mk` fragments,
the 13 test-suite makefiles, nine documentation makefiles, four example
makefiles, three runtime driver makefiles, the Win32 import-library Makefile,
and the NSIS installer makefile.
With all 44 multi-module `.bmk` test inputs, that is 129 reviewed Make inputs:
85 Makefiles/fragments and 44 fixtures. `doc/common.mk` was added during this
review. Final input hashes are recorded in
[`make-system-review-inputs.json`](make-system-review-inputs.json).
Generated bootstrap and build output and imported legacy library trees are
outside this inventory.
Source-copy filters and scripts called by a reviewed rule are inspected when
their behavior determines the rule's correctness.

Existing graphics/runtime restructuring and earlier log-harness repairs are
preserved. Reproductions use small temporary trees and the actual Make
fragments, without cleaning or rebuilding the working compiler.

## Coverage

| Area | State | Review focus |
| --- | --- | --- |
| Root orchestration | Reviewed | Prerequisite ordering, entry points, configuration, help |
| Platform, tool selection, feature and flag policy | Reviewed | Host/target separation, overrides, effective tools, multilib |
| Layout, naming, archives | Reviewed | Artifact identity, incremental rebuilds, installation paths |
| Source graphs and compile/dependency rules | Reviewed | Complete selection, collisions, generated dependencies |
| Build targets, archive and clean rules | Reviewed | Relinking, errors, bounded cleanup |
| Bootstrap rules | Reviewed | Emission, recovery, response files, archive staging |
| Distribution and installation | Reviewed | Parallel ordering, completeness, ownership, uninstall |
| Build-system test harness | Reviewed | Tests that verify actual behavior, failures, cleanup scope |
| Language-suite makefiles and all 44 `.bmk` inputs | Reviewed | Actual Make parsing, source existence, dialects, recursive options |
| Documentation, examples, drivers and installer | Reviewed | Entry-point consistency, prerequisites, outputs, cleanup |

## Findings

| ID | Finding | State |
| --- | --- | --- |
| M01 | Compiler links use only an order-only runtime dependency, so changing the embedded runtime need not relink any compiler variant. | Fixed; all four compiler variants relink after either embedded input changes |
| M02 | `dist` declares cleanup, staging, and packaging as sibling prerequisites, allowing destructive races under `-j`. | Fixed; delayed-cleanup regression passes under `-j4` |
| M03 | `dist-includes` copies only top-level files and omits nested BI contracts. `dist-zip` has no staging prerequisite. | Fixed; tar and standalone zip contain nested headers |
| M04 | Missing uninstall manifests exit only the first recipe shell. Later recipe lines still try to read the missing file. Removal errors are ignored before the manifest is deleted. | Fixed; absent manifests succeed and failed removals preserve the retry manifest |
| M05 | Include installation records every destination file, including files the installation did not copy; uninstall can therefore remove pre-existing unrelated headers. | Fixed; source-based ownership records preserve foreign destination headers |
| M06 | Archive indexing errors are ignored and archive replacement removes the prior archive before the replacement succeeds. | Fixed; indexing failure preserves the prior archive and removes temporary files |
| M07 | `_mt_find_new_archives` ignores the producer's exit status and detects only new names, so failed producers or replacement archives can be misclassified. | Fixed; failed producers fail, replacement archives are validated, and cleanup records only this test's archives |
| M08 | Bootstrap emission tests its output directory for any file; stale bootstrap objects can satisfy the check even if the compiler emits no sources. The hidden response file does not match the glob. | Fixed; stale-object/no-emission regression fails as required |
| M09 | DOS graphics timer-profile prerequisites retained the paths from before the driver move. | Fixed; toggling the profile rebuilds the actual relocated object |
| M10 | Documentation parents launched competing child builds per binary and drifted from optional spelling/insert selections. | Fixed; each child runs once and owns its incremental graph |
| M11 | Documentation children assumed all non-Linux systems used `.exe`. | Fixed; shared host policy agrees across Linux, macOS, BSD, Haiku, and Windows shells |
| M12 | NSIS included the old version path, the DOS export helper used lowercase `.phony`, and import-library aliases were not phony. | Fixed; installer parse and same-named-file alias checks pass |
| M13 | Manual formats launched duplicate HTML generators and could reuse stale CHM output after failure. | Fixed; shared generation is ordered and fresh HHC output is required |
| M14 | DOS provider-enabled thread sources were compiled but omitted from the link manifest. | Fixed; thread objects follow the selected scheduler |
| M15 | Compiler sanity checks returned success after failed compiler/execution commands. | Fixed; failures propagate to Make and `all` is phony |
| M16 | Graphics/sound C++ and graphics assembly recipes lost CPPFLAGS. | Fixed; all twelve affected PIC/MT paths forward them |
| M17 | Semantic smoke validators accepted recognized records after their completion footer. | Fixed; footer guards run before record dispatch |
| M18 | Aggregate prerequisite checks did not hold back parallel object compilation. | Fixed; concrete compiler/runtime objects and startup artifacts wait for checks |
| M19 | Removing a source could leave its old archive member behind. | Fixed; committed member records force changed selections independently of timestamps, survive failed retries, and stay out of SDK payloads |
| M20 | Packaging used a bootstrap cleanup alias with no implementation; a bootstrap phony continuation was malformed. | Fixed; the central cleanup alias removes emitted C/ASM/LLVM sources and declarations parse correctly |
| M21 | Missing inferred cross compilers silently fell back to host compilation. | Fixed; concrete builds reject fallback while allowing an explicit equivalent SDK driver |
| M22 | MULTILIB changed only some flags and reused the wrong target directory. | Fixed; x86 width resolves before layout and reaches BASIC, C, C++, and links; incompatible requests fail |
| M23 | Manual edits/removals did not invalidate output, and ZIP updates retained removed pages. | Fixed; cache inputs are dependencies, obsolete HTML is removed, ZIPs are recreated, and the default target remains `all` |
| M24 | Example helpers missed header changes; cleanup missed objects/executables and exceeded process argument limits. | Fixed; parent/child incremental graphs and bounded dry-run-safe cleanup are covered |
| M25 | A target compiler could replace host identity and select a target-only build compiler. | Fixed; HOST_CC or an independent native driver identifies the build machine |
| M26 | Sound C/C++ sources could compete for one archive member. | Fixed; source selection rejects ambiguous members |
| M27 | Distribution dropped platform runtime headers retained by installation. | Fixed; distribution delegates runtime payload selection to `install-runtime` |
| M28 | The help viewer accepted partial target names such as `win`. | Fixed; one exact supported target is required |

The README, file map, porting guide, documentation index, and changelog were
updated with the resulting ownership and configuration contracts.

## Verification

The root `make make-contracts-test` run passed all 37 cases with zero failures.
It includes every correction and actual parsing/source checks for all 44
multi-module fixtures. The 25-case compiler log harness also passed, including
the real compiler timeout case. Source checks passed for 264 documented compiler
files across 21 host graphs, 31 graphics configurations, and 1,241 runtime files
across 126 graph cases and three guard cases.

Local receipts are in `build/make-review-20261020/`. The input manifest records
their hashes and the validator identities. `git diff --check` passed. The
existing `bin/fbc.exe` retained SHA-256
`17c0fe049c35ec18764f7a5bb3e40c33d3e763a90f686dcd79e088a00492be4c`.
No compiler/SDK rebuild or installation was performed for this review.

`make make-contracts-test` uses small disposable trees and production Make
fragments. It is included in quick/full workflows without requiring a compiler
build. Compiler link ordering and flag propagation use recording tools;
archive, tar, ZIP, shell, and Make behavior use the real host tools. The
compiler log harness runs the actual `bin/fbc.exe`.

## Remaining constraints and improvements

At this review snapshot, arbitrary compiler-command or feature/optimization
flag changes required cleaning the affected objects. The subsequent
[consolidation](build-consolidation.md) adds automatic configuration tracking
for normal builds. The component-clean workflow still handles tools replaced
in place and preserves compiler executables and emitted bootstrap sources. Compiler BI dependencies
remain conservative, so an interface edit can rebuild more modules than a
minimal include graph would require.

Quick/full workflows perform shared clean/bootstrap/package operations; run
one per owned worktree. The isolated contract target owns only temporary
fixtures. The examples' default best-effort behavior for optional external
libraries remains intentional; `STOP_ON_ERROR=y` requests strict behavior.

Cross-target checks here validate graphs, flags, ownership, and failure
behavior. Native SDK/device execution, complete language suites, full manual
production with HHC, and complete quick/full rebuild workflows require
separate qualification. The source review is broader than those runtime
receipts.

<!-- end of make-system-review.md -->
