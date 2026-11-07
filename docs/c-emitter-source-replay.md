<!--
Project: FreeBASIC source delivery qualification
File: c-emitter-source-replay.md
Purpose: Record a cold replay of the current compiler/runtime source delivery.
Responsibilities: Distinguish source bootstrap, BASIC self-hosting and platform limits.
This file intentionally does NOT claim another machine or external corpus replay.
-->

# String-capacity source-package replay at a different path

The delivered portable-source.zip, string-length-source-update.zip and
string-capacity-source-update.zip now pass a cold replay in a separate temporary
package location. Every effective source matches those archives. Source data is
shared through checked hardlinks/junctions; the output tree starts empty, with
no old executable, object tree, thin archive or object-cache reference supplied.
The original frozen package and four installed compiler/archive pins stay exact.

The unmodified delivered build programs freshly compile all 224 C bootstrap
units and all 1,045 ST/MT/startup runtime units with GCC and Clang: 2,538 native
checks. Every one of the 224 compiler and 1,045 runtime GCC objects matches its
qualified counterpart. Both newly built normal archives and the C-bootstrap
compiler match the last string-capacity checkpoint byte for byte. Runtime
observations remain ST 820 GCC / 14,470 Clang, MT 820 / 14,477 and startup 2 / 24;
the frozen compiler cut retains its separately recorded diagnostics.

The new compiler then emits all 224 packaged BASIC units at the new source path.
Another 448 fresh GCC/Clang checks build a new compiler from those BASIC inputs.
That compiler emits all 224 units again at the same new path, producing exact C
hashes with no filename-literal or temporary-name normalization. This establishes
a strict BASIC self-host fixed point at the relocated path. The earlier warning
that original-path bootstrap hashes change with source-location metadata still
applies when comparing two different paths; it does not prevent self-hosting.

All five delivered checkers run unmodified on the new BASIC-built compiler and
new runtime: four GCC/Clang ST/MT BASIC smoke executions and 32 C/C++ runtime
client executions. These cover the declared numeric interfaces, console writers,
UTF spans and string storage, including four real MT clients with joined workers.
Both existing full C backend regression suites also pass on the new compiler.
The independent source build and portable checks require no original checkout.
The broader backend runner additionally uses its existing checkout fixtures and
full BI SDK through a separate test-only prefix; its libraries are the new ones.

The complete replay performs 2,986 fresh native compilations, 224 exact relocated
BASIC fixed points and 36 portable value executions. It uses one reused source/
object/program namespace, retires proved object inputs before reusing compiler
slots, and retains compact per-unit command/count/hash evidence instead of a
second object tree or full diagnostic database. Every source/build script in
the replay has the exact delivered bytes. No compiler/library source change,
valid-example edit, PNG work or installation is made by this qualification.

The integration ceiling is 192 MiB, with a monitor that deduplicates NTFS file
identities across package hardlinks. The last accounting preflight incorrectly
charged existing checkout SDK inputs reached through a test-only junction as
new retained work. Both backend suites had already passed. A zero-copy alias
reproducer verifies the accounting defect; the raw failed receipt/controller
remain intact, and the corrected monitor excludes those external input views.
No compilation or value test is repeated to recover that completed evidence.
The recorded owned-allocation bound is conservative, not a rewritten raw peak.
Native commands retain their 600-second
hang bounds; the complete build has no arbitrary short phase deadline. The
temporary package, objects, archives and executables are removed afterward,
and final retained allocation remains below 160 MiB.

The subsequent USTRING conversion update has separate targeted source, object,
archive and value qualification. Its seven source changes are outside this cold
replay; this receipt must not be presented as a full rebuild of that later ZIP.

This is a different-path replay on the same Windows host with the pinned native
Win64 GCC/Clang toolchain. Another-host or foreign-platform rebuilding and a full
external corpus census from the delivered package remain unverified. The package
still freezes the experimental compiler cut before later unrelated main-source
changes, and its broader promotion limits remain open. Main emitted-C queues
stay broad 2,139 GCC / 6,966 Clang and Exampleageddon 3,495 / 8,916 at their measured
cuts. The overall warning-cleanup campaign remains active.

Evidence: portable-current-replay.json.zlib in the existing qualification area,
with its controller, final verifier and seal under
build/c-emitter-forward-views-20261105.

<!-- end of c-emitter-source-replay.md -->
