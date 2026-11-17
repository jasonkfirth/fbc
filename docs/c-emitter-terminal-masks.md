<!--
Project: FreeBASIC runtime qualification
File: c-emitter-terminal-masks.md
Purpose: Record native-width terminal masks and standard escape bytes.
Responsibilities: Bind source changes, native objects, values and serial execution.
Platform-specific application qualification retains its own source cuts.
-->

# Native terminal masks and escape bytes

Termios clear masks are complemented at the native `tcflag_t` width. The file
attribute clear mask uses `mode_t`, and GPM's event/modifier fields receive
explicit unsigned-short values. Existing named flags and bit selections remain
in their original configuration paths. Unix, Solaris and RISC OS terminal
strings and character constants spell byte 27 with the standard `\033` escape.

The complete captured Linux MT runtime moves from GCC 195 / Clang 10,188 to
GCC 163 / Clang 10,156. This repair removes fourteen sign-conversion and eighteen
escape-sequence observations under each tool. Every other class retains its
captured count. All 1,036 native object comparisons, both runtime archives and
both startup objects are byte-identical.

Two subsequent concurrent descriptor-source edits change leading indentation.
Their four exact Make-command replays retain their original native objects and
remove two misleading-indentation observations per tool. The complete current
1,328-input runtime cut therefore records GCC 161 / Clang 10,154. Every input
hash is checked against canonical source after the refresh. The other 1,032
native objects retain their exact qualified source and tool contexts.

## Native values and serial execution

Eight GCC/Clang executions at 32/64 bits and O0/O2 compare sixteen original and
changed flag expressions with independent mask values. Each checks 4,096 inputs
per expression, totaling 524,288 cases. Each execution also compares all 32
changed character/string literal pairs. Strict C11 accepts eight repaired
literal controls and rejects all eight original-extension controls.

The existing `tests/serial-pty` program runs eight times against the fresh GCC
and Clang runtime archives, with both C producers and O0/O2. It checks the
requested serial framing and flow-control settings, binary bytes in both
directions, and restoration of the previous terminal state on close. Source-only
strict lint of its unchanged BASIC source reports zero findings.

The initial serial fixture emitted a constructor-only BASIC module. Selecting
its normal `-m testee-open-com` entry fixes the native link. Clang also rejected
the initial strict literal fixture because its arrays appeared only in `sizeof`.
The corrected fixture reads the actual array bytes through a variable index.
Both failures and their corrected native executions remain archived.

Solaris and RISC OS share the checked escape tokens and typed mask expressions.
Their complete native platform builds remain outside this Linux runtime matrix.

## Reproduction evidence

The existing `build/c-emitter-forward-views-20261105` directory contains
`runtime-terminal-qualification-v1-20261115.json.zlib`. The source pair is
`runtime-terminal-preimages-v1-20261115.json.zlib`. Complete Make-selected builds
use the v6 baseline and the v9 accepted terminal receipt. The current source
refresh uses the v10 input packet and v12 native receipt, recording exact
commands and all four fresh object identities.

GCC retains the aggressive runtime warning list and Clang retains `-Weverything`.
Both workers run serially at reduced priority. Whole-runtime and native-control
scratch peaks stay below 15 MiB within 64 MiB. The final incremental refresh peaks
at 4,242,775 bytes within 32 MiB. Each scratch directory is removed on exit.

<!-- end of c-emitter-terminal-masks.md -->
