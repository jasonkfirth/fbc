<!--
Project: FreeBASIC C backend warning cleanup
File: c-emitter-static-callbacks.md
Purpose: Record static callback address lowering and GCC qualification.
Responsibilities: Bind relocations, typed callback execution and target limits.
This file intentionally does NOT claim a complete corpus recount or promotion.
-->

# Static erased callback addresses

Static ANY PTR initializers containing procedure addresses now pass through the
target's native unsigned address type. This expresses FreeBASIC's stored address
representation through the C toolchain's pointer/integer mapping. Typed function
pointer initializers, data addresses, nonzero offsets, record layouts and actual
call signatures retain their existing lowering. No startup code or diagnostic
suppression is introduced. This is a target ABI contract, not a claim that all
C implementations share pointer representations.

All six checked/default profiles for Allegro dialog and the two polymorphism
examples now compile with zero GCC warnings. All six GCC objects are identical
to their pre-repair objects. This removes all 42 observations in that pedantic
class without modifying the examples or executing their interactive applications.

The added noninteractive regression restores exact callback signatures and
calling conventions after loading the static slots. Eight before/after native
runs pass across GNU11/GNU23 and O0/O2; eight Win32 definition checks pass. Every
before/after fixture object is identical. The regression is registered in the
existing backend runner. Strict source-only lint passes the emitter and fixture.

The repaired experimental compiler is
`a99d1327841dc804c6c2db9dba55474321df14ba77c74e0fa3c56c350e19f151`.
One compiler object is rebuilt under unsuppressed GCC, with zero compiler GCC
warnings and 223 qualified native objects reused. A fresh producer sweep of all
224 compiler units matches the qualified C graph, including the newly compiled
HLC input. Matching normal ST/MT runtime archives remain in use.

The controllers and compressed receipts use the `gcc-static-callback-` prefix
in the existing forward-views stage and canonical qualification directory.
The admission's first overbroad address matcher and its corrected static-only
scope remain recorded separately. Its one completed native check is reused.

Together with the parameter batch this removes fourteen broad and forty-nine
Exampleageddon GCC observations. The last complete queue measurements remain
247 broad / 83 Exampleageddon; these focused changes are not a new full census.
The remaining reports include retained debug bindings, source-level warnings
and the recorded GCC array-copy false positive. Their counts remain visible.
Next is merging and qualifying the experimental compiler in main, preserving
newer parser/semantic work and qualifying the platform-specific runtime source
graph. Installed binaries remain unchanged. Clang-only work is deferred; PNG
and the user-closed investigations remain excluded.

<!-- end of c-emitter-static-callbacks.md -->
