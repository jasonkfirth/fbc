<!--
Project: FreeBASIC C backend verification
File: c-emitter-startup-groups.md
Purpose: Explain shared MinGW startup registrations and their qualification.
Responsibilities: Callback identity/order, semantic facts, receipts and target limits.
This file intentionally does NOT claim that the remaining warning classes are resolved.
-->

# Shared MinGW startup registrations

The C backend emits one runtime registration for each constructor/destructor
priority group in a module. The original callback functions keep their C and
object names. A small generated wrapper calls them in the order used by the
selected C compiler. Callbacks remain visible to the compiler's reachability
analysis, and `used` retains their original object symbols through inlining.

GCC and Clang arrange MinGW startup tables differently. GCC constructors run
in reverse definition order and destructors in forward order. Clang constructors
run forward and destructors in reverse order. The wrapper selects the matching
order with the physical C compiler's predefined macros. Priorities and linked
module order remain unchanged. Default priority and explicit priority 65535
belong to the same registration group.

Singleton groups and groups containing a non-returning callback keep their
individual attributes. Grouping applies to the MinGW target. Other targets,
including NetBSD's finalizer arrays, keep their existing C emission. The helper
owns copied callback names in per-unit pools; compiler procedure symbols remain
borrowed. Unit initialization and cleanup release these pools between modules.

The full semantic model records each grouped callback as a `wrapper-call`.
The callback retains its original procedure identity, kind and requested
priority. Placement names the generated C wrapper, and spelling names the
selected C callback call. The independent reader checks those facts against
the emitted C name and rejects altered kinds, priorities, placements and callees.
Reduced models keep their existing exclusions. The wrapper does not invent a
source procedure or change source registration-list order.

## Qualification

The canonical compiler passes all 224 compiler-unit C fixed-point checks under
the original unsuppressed GCC and Clang profiles. The staged/canonical assertion
filename difference is retained in separate build receipts; the canonical
provider is built from the actual source paths.

The permanent `tests/compiler-support/c-startup-groups.py` regression passes
16 Win64 executions and 32 Win32/Win64 object checks. It compares two linked
units, both link orders, explicit/default constructor and destructor priorities,
GCC/Clang and `-O0`/`-O2`. Each object retains all eight callback names while its
registration table shrinks from eight pointers to four. A separate real fbcunit
gate runs all 137 union-initializer cases, with 461 assertions, before and after
grouping under each C compiler. Eight lifecycle executions retain identical
observable initialization and teardown output.

Nine semantic tests cover raw ABIs, transports, module reset, guarded static
paths, backend priorities, shared-library routes, execution and damaged facts.
Twenty foreign C emissions are byte-identical to the accepted provider.
Four Windows ARM64 object/assembly controls check table sizes and callback order.
Those object checks do not claim foreign runtime execution. Non-returning
startup callbacks retain their separate registrations and visible warnings.

The installed fbcunit archive did not satisfy the accepted provider's ABI
control. The qualification therefore builds its four source modules privately;
the installed archive remains unchanged. Compiler/runtime binaries are also
unchanged. Broad queue qualification reuses checks only after exact C hash
matching. One worker uses one reusable C/object scratch pair, with a 64 MiB
stage cap and verified archives for retired bootstrap duplicates.

Receipts are under `build/c-emitter-startup-groups-20261103/`:

- `canonical/receipt.json` and `canonical/results.sqlite3`
- `canonical-gates.json` and its six individual gate directories
- `audit-canonical/receipt.json` and `audit-canonical/results.sqlite3`
- `canonical-source-edits.json` and the exact source preimages
- `summary.json`, containing accepted queue totals and class reductions

The [current status](c-emitter-current-status.md) contains the warning counts.
The [class priorities](c-emitter-class-priorities.md) retain the outstanding
continuation, buffer-access and aggregate-layout work. PNG remains excluded.

<!-- end of c-emitter-startup-groups.md -->
