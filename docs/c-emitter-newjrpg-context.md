<!--
Project: FreeBASIC C backend warning audit
File: c-emitter-newjrpg-context.md
Purpose: Record recovered JRPG contexts and source-defined debug body review.
Responsibilities: Separate source faults, retained diagnostics and pending coverage.
This file does not claim complete JRPG or foreign-platform qualification.
-->

# Current JRPG build context

The active `C:/Nextcloud/games/newjrpg` root contains 241 BASIC programs. Its
maintained `verify_editor_authoring.ps1` builds with `-w all -exx -mt -gfx3`,
the project include directory and its `omaGui` directory. The C audit uses that
context with the then-selected `55aa2516` development compiler, plus the unsuppressed
GCC and Clang native flags. It emits C and compiles objects; it does not run
applications, build archived variants or exercise image loaders. Platform names
in smoke filenames do not imply foreign-platform qualification.

The three-program baseline pilot accepts the action-catalog smoke, editor main
and editor-authoring smoke. The latter two each expose two GCC null-dereference
warnings. Their cancel callbacks declare a dialog pointer but omit the assignment
from the already-checked root's `data` field. The repaired source adds those two
assignments, matching the neighboring callbacks. The surrounding uncommitted
JRPG work is preserved. No compiler warning was disabled to make them disappear.

Fifteen focused checks retain the original GCC rejection, run the production
callback bodies across GCC/Clang O0/O2 and GAS64/LLVM, and lint the changed header
and generated fixture. The six value runs check null widgets, empty dialog data,
cyclic parent traversal and unchanged choice metadata. The fixture uses the
production data types and root traversal with a small Widget adapter; it does
not establish full GUI layout or graphics behavior. Strict lint has no findings.

The complete 241-program sweep now accepts every profile with GCC 0 /
Clang 10,018. All 676 opened inputs reconcile without drift, and 290 previously
pending BASIC paths have qualified owners. Its compiler remains `55aa2516`;
these counts are separate from the newer selected compiler's retained census.
The first three baseline profiles are historical evidence after the repair.
The full worker reused one C/object/log slot and peaked at 55,902,314 bytes
under its 64 MiB cap. It created no persistent test directory or installed binary.

The next sequential queue recovers 194 fb-ext-lib profiles from its maintained
example/test Makefiles and additional standalone C regressions. Seventeen pending
PNG implementation or regression paths are explicitly assigned to the user's
existing PNG exclusion. No implementation review or compilation is performed for
those paths. The worker waited for the verified JRPG process to exit and its
terminal input reconciliation before starting. It selected the fully recounted
`9d60dd50` compiler by immutable path and pinned that choice at startup. Active
workers never switch compilers when a newer candidate appears.

All 194 fb-ext-lib profiles are now terminal: 186 accepted C units and eight
specialized harness-context refusals, GCC 666 / Clang 816. The 432 opened input
pins reconcile, with a 3,028,074-byte owned peak under the 32 MiB cap. Of the GCC
warnings, 589 are unused private source bodies in debug contexts. A compiler
retention follow-up now completes the same profiles with GCC 77 / Clang 227,
removing 589 warnings per tool. Every other class is identical in each profile;
all 658 source parameter warnings and 250 source variable warnings remain.
The corrected selected compiler, `3da82d36`, completes the separate retained
recount with GCC 18 / Clang 15,827. All 194 fb-ext-lib contexts are freshly
re-emitted and byte-identical to the accepted private-body C cut. No-argument
semantic definition headers now match the final C definitions too.
The eight direct-emission refusals require their maintained runners' production
wrapper, test-only public header overlays, or module include directory. They do
not establish that the protected library interfaces or valid tests are wrong.

A separate [Tiko maintained-entry check](c-emitter-tiko-context.md) is now
complete with GCC 0 / Clang 42 after four genuine source null-access repairs.
It adds fifty non-image BASIC owner paths and remains separate from the JRPG
compiler cut. The combined project coverage snapshot is retained as
`current-tiko-context-reconciliation-20261112.json.zlib`.

## Retained non-image debug functions

The earlier selected compiler's two-profile review verified three `-Wunused-function`
observations: the fb-ext-lib array assignment operator, its sort comparator,
and WSR5_3's omaGUI alpha helper. These have explicit source bodies, including
macro definitions; the earlier description as implicit compiler assignment
operators was incorrect. The compiler intentionally retains source-defined
private bodies for debugger visibility. Their normal-mode outputs have zero GCC
warnings under identical aggressive flags. Raw debug observations remain counted.
The fourth historical function observation belongs to image work and remains
outside this review. No compiler-owned implicit member fault was established by
the three investigated observations.

Evidence remains in `build/c-emitter-forward-views-20261105/`:

- `current-newjrpg-context-20261112.sqlite3`: three baseline profiles, GCC 4 /
  Clang 239, with original diagnostics and source identities.
- `current-newjrpg-cancel-repair-20261112.json.zlib`: exact preimage and repaired
  source, extracted production fragments, fifteen checks and clean strict lint.
- `current-newjrpg-fixed-context-20261112.sqlite3`: completed 241-profile sweep,
  GCC 0 / Clang 10,018.
- `current-retained-functions-20261112.json.zlib`: four fresh debug/normal
  profiles with both native tools and unchanged source pins.
- `current-source-png-boundary-20261112.json.zlib`: seventeen explicit source
  dispositions under the existing user-requested boundary.
- `current-fbext-context-plan-20261112.json.zlib`: 66 maintained examples, four
  additional examples, 112 test modules and twelve standalone C regression
  fixtures; the matching `current-fbext-context-20261112.sqlite3` retains all
  194 terminal results, including the eight harness-context refusals.
- `current-newjrpg-status-20261112.json.zlib`: verified owning-source coverage
  from terminal profiles, with pending counts and the PNG boundary kept separate
  from the last completed warning census.

<!-- end of c-emitter-newjrpg-context.md -->
