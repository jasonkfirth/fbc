<!--
Project: FreeBASIC build maintenance
File: build-consolidation.md
Purpose: Explain the consolidation repairs and their integrated qualification.
Responsibilities: Record ownership, failure contracts, exact inputs and limits.
This file intentionally does NOT qualify every optional SDK or graphics driver.
-->

# Build consolidation

The 2026-10-20 consolidation starts from
`6016fb099cab5e1c37f6b8c74f30782e1af94bcd`, which combines the compiler,
runtime and graphics restructuring with the Make review and source guides.
The [original Make review](make-system-review.md) retains its own earlier
inventory and receipts. This follow-up addresses configuration tracking,
artifact ownership and native integration.

## Repairs and ownership

Two accidentally tracked Cppcheck dumps occupied 146,734,642 bytes under
`src/gfxlib2/input`. They have been removed from the maintained tree. The
generated `tests/log-tests-obj-fb.lst` is also removed: the language harness
creates and deletes that list as part of its normal operation. The ignore
and source-copy rules already distinguish generated test lists from source;
ignore rules now cover analyzer dumps too. Git history is retained.

`make source-artifacts-test` checks actual source directories, including in
Git-free source archives. It rejects `.dump` and `.ctu-info` files outside
object directories. Quick/full structural gates include this check.

[`mk/build/config-rules.mk`](../mk/build/config-rules.mk) owns normal-build
configuration records. Its fingerprints include effective commands, flags,
target identity and included product Make rules. Each compiler variant owns
a record in its object directory; each runtime archive or startup object owns
a record under `$(libdir)/.build-config`. Installation excludes that private
metadata.

An artifact's `.pending` marker is created before its objects are changed and
cleared after publication. This also handles reverting flags after a failed
attempt: the old successful record alone cannot establish the configuration
of objects left by the interrupted build.

The record changes only after successful publication. A failed archive or
compiler link leaves the old record, so retrying still rebuilds the pending
configuration even when file timestamps have coarse resolution. Variants
that were not built keep their previous records. Command-line link/archive
flag changes republish outputs without recompiling their objects. Edits to
included Makefiles, including local `config.mk`, conservatively invalidate
compile signatures. Unchanged builds and dry runs do not rewrite records.

Compiler links now write a temporary executable beside the destination and
publish it only after the linker succeeds. The cleanup trap owns that one
temporary path. A failed linker can no longer truncate the compiler already
being used. Archives retain their earlier successful-indexing requirement.

The structural checker, Make contracts and compiler-harness runner now use
the GNU Make executable chosen by their parent. This matters on FreeBSD,
where the unqualified system `make` has a different grammar.

The warning harness now uses the extended regular-expression syntax shared
by GNU and BSD sed, with a literal tab replacement. The previous GNU-specific
expression left diagnostic locations in BSD output. The 340 checked-in
warning snapshots remain reference data; the repaired runner reproduces them
without accepting changed diagnostic text.

## Qualification

Completed on 2026-10-20. The [compact results](build-consolidation-results.json)
retain the base revision, changed-input hashes, compiler/archive identities,
stage exits and receipt hashes.

| Native host | Compiler/libraries | Unit tests | Self-contained runs | Final Make contracts | Final input audit |
| --- | --- | --- | --- | --- | --- |
| Windows x86-64 | Passed | 2,441 passed | 654/654 | 42/42 | 12,637 unchanged |
| Debian CT133 x86-64 | Passed | 2,441 passed | 654/654 | 42/42 | 12,637 unchanged |
| FreeBSD VM134 x86-64 | Passed | 2,441 passed | 654/654 | 42/42 | 12,637 unchanged |

All 340 warning reports were regenerated on each host and matched their
references exactly. Immediate repeat builds changed no outputs: 2,212 on
Windows, 3,690 on Debian and 3,454 on FreeBSD. Final dry-run graph checks also
left those outputs untouched. Compiler, runtime and graphics structural
checks passed across 21 compiler host graphs, 126 runtime graph cases and
31 graphics configurations.

The full example corpus contains 1,773 files. Compilation passed for 1,586
on Windows, 1,530 on Debian and 1,583 on FreeBSD. The remaining compile
failures/timeouts stay in the reports; the self-contained gate has no compile
or runtime failures. Interactive, audio, network and system-changing examples
are outside that execution gate.

The compiler/runtime builds and full language/example gates use the candidate
product sources. The final in-progress-marker refinement changes Make
bookkeeping, with no compiler/runtime implementation-source changes. Its
latest 42-case production-fragment suite exercises real C/C++ values, failed
builds followed by reverted flags, compiler variants, publication failures,
unchanged builds and dry runs on each host. Nonmutating native graph checks
cover the full final dependency graph and verify that every output stays
untouched. Full language/example sweeps are not repeated for that final
metadata refinement.

The work uses one owned source/object tree per host and two build jobs.
Later repairs are small overlays on those trees. The root checkout's compiler
is preserved as a build seed, with SHA-256
`17c0fe049c35ec18764f7a5bb3e40c33d3e763a90f686dcd79e088a00492be4c`.
The original source archive is 46,638,327 bytes; no repeated whole-tree backup
is needed for each retry.
After recording success, task-owned example binary/work directories were
removed, reclaiming 1,051,403,594 logical bytes. Reports, CSVs, logs, native
compilers, libraries and object trees remain available for review and reuse.

Early failed runs are retained as evidence. FreeBSD exposed the incorrect
Make selection. Both Unix hosts then exposed a missing `zip` prerequisite in
the packaging fixtures. The initial Windows run completed its behavioral
gates, but its final input check detected normal cleanup removing the
tracked generated test list. The repairs address these causes rather than
excluding the tests.
Midcom's pinned SSH preflight timed out, so Debian CT133 supplies the Linux
lane; no qualification is claimed for Midcom in this batch.

FreeBSD's candidate worker later exited at the input audit because its warning
reports retained diagnostic locations. The corrected sed runner, repeated
contracts and final source audit passed. The original failed worker receipt
is preserved alongside the final maintenance result.

## Remaining boundaries

The optional GNU awk-based Haiku grep fixture was unavailable on Debian and
FreeBSD. FreeBSD also lacked GNU timeout for the compiler-timeout fixture;
that case passed on Windows and Debian. The remaining compiler-harness
checks passed on each host.

One build process must own a given output tree. Configuration records and
temporary executable publication do not provide interprocess locking.
POSIX `cksum` is a configuration change detector, not a security identity.
Replacing a tool's executable at the same path requires explicit component
cleanup. Emitted bootstrap sources and the separate bootstrap compiler retain
their explicit cleanup workflow. See the [Make guide](../mk/README.md).

The focused ownership repair concerns compiler publication. Broader graphics
locking and platform-driver threading behavior require separate review.
Headless compiler/example gates also do not establish visible graphics,
audio playback or device interaction. Optional
external-library compile failures remain separate from the self-contained
example gate.

The source guides now describe these build contracts in both
[Markdown](source-code/tools-and-tests.md) and the native
[Wakka manual](../doc/manual/cache/DevBuildConfig.wakka). The changed manual
pages were regenerated with the existing converter in HTML, fbhelp and text;
this batch does not produce new release CHM/ZIP packages.

<!-- end of build-consolidation.md -->
