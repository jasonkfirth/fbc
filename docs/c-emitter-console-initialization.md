<!--
Project: FreeBASIC runtime qualification
File: c-emitter-console-initialization.md
Purpose: Record Unix LOCATE's explicit coordinate initialization.
Responsibilities: Bind source, native diagnostics and console callback contracts.
Physical terminal operation retains its normal platform qualification.
-->

# Unix LOCATE coordinate initialization

`fb_ConsoleLocate` initializes its local coordinates from the supplied column
and row. When either coordinate is omitted, the existing query obtains both
current coordinates. The later assignments preserve either explicitly supplied
coordinate. The change clears two Clang conditional-initialization observations.

The original paths already supply both values. A fully specified call assigns
both coordinates before reading them. A partially specified call invokes
`fb_ConsoleGetXY`, which writes both non-null output arguments, including its
uninitialized-console fallback. Explicit local initialization expresses this
relationship without relying on correlation between the separate conditions.

The native observer compares returned coordinate/visibility bits, stored cursor
clamping, queried coordinates, output sequence arguments and background-lock
ordering. It supplies a single-threaded console with controlled callbacks.
The query is called outside the caller's background lock, matching the actual
query's ownership of that lock. Cursor movement and visibility callbacks execute
inside the caller's lock.

Sixteen before/after native executions cover GCC/Clang, Linux x86/x86_64 and
O0/O2. Four additional executions use the exact Make-selected multithreaded
runtime flags. The combined 17,280 cases cover omitted and explicit coordinates,
`INT_MIN`/`INT_MAX`, initialized/uninitialized consoles, three queried positions
and visibility changes. Every execution passes.

Both original Make-flags objects reproduce their captured parent hashes.
Fresh before/after objects retain every other warning class and remove two
Clang observations. Object code changes are recorded with paired disassembly.
The source change affects only initial local values and its explanatory comment.

The captured 1,328-file runtime cohort therefore has derived GCC 1,825 / Clang
10,268 counts after this source delta. Its original full build recorded
1,825 / 10,270. Subsequent live runtime edits have separate source identities.
The paired object update retains the earlier complete cohort as its parent.
Current whole-tree rebuilding and archive publication remain separate checks.
Protected installed products and runtime archives retain their hashes.

## Receipts

The existing `build/c-emitter-forward-views-20261105` work area contains
`locate-initialization-packet-v1-20261115.json.zlib`,
`locate-initialization-linux-v1-20261115.json.zlib` and
`locate-initialization-fullflags-linux-v2-20261115.json.zlib`.
They retain exact before/after source, the native observer, commands, actual
output, object hashes and disassembly. Workers reuse one temporary source tree
within a 64 MiB bound and remove it after completion.

<!-- end of c-emitter-console-initialization.md -->
