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
context with the selected development compiler, plus the existing unsuppressed
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

The complete 241-program sweep is pending. Its counts must remain separate from
the previous completed queues until all profiles and opened input pins are
reconciled. The first three baseline profiles are historical evidence after the
source repair, not a passing current-project census. The full worker checkpoints
each profile, reuses one C/object/log slot, and bounds its owned working set at
64 MiB. The baseline pilot peaks at 53,881,786 bytes; no persistent test directory
or installed binary is created.

The next sequential queue recovers 194 fb-ext-lib profiles from its maintained
example/test Makefiles and additional standalone C regressions. Seventeen pending
PNG implementation or regression paths are explicitly assigned to the user's
existing PNG exclusion. No implementation review or compilation is performed for
those paths. The queued worker waits for the verified JRPG process to exit and
requires its terminal input reconciliation before starting, keeping one owned
native project queue active at a time. The not-yet-started fb-ext-lib queue now
uses the qualified conversion candidate by its immutable executable path. These
planned profiles have no warning totals yet.

A separate [Tiko maintained-entry check](c-emitter-tiko-context.md) is now
complete with GCC 0 / Clang 42 after four genuine source null-access repairs.
It adds fifty non-image BASIC owner paths and remains separate from this
unfinished JRPG sweep. The combined project coverage snapshot is retained as
`current-tiko-context-reconciliation-20261112.json.zlib`.

## Retained non-image debug functions

The selected compiler's two-profile review verifies three `-Wunused-function`
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
- `current-newjrpg-fixed-context-20261112.sqlite3`: pending full current-source
  sweep; inspect live/terminal metadata before citing totals.
- `current-retained-functions-20261112.json.zlib`: four fresh debug/normal
  profiles with both native tools and unchanged source pins.
- `current-source-png-boundary-20261112.json.zlib`: seventeen explicit source
  dispositions under the existing user-requested boundary.
- `current-fbext-context-plan-20261112.json.zlib`: 66 maintained examples, four
  additional examples, 112 test modules and twelve standalone C regression
  fixtures; qualification remains queued.
- `current-newjrpg-status-20261112.json.zlib`: verified owning-source coverage
  from terminal profiles, with pending counts and the PNG boundary kept separate
  from the last completed warning census.

<!-- end of c-emitter-newjrpg-context.md -->
