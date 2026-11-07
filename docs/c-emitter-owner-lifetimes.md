<!--
Project: FreeBASIC C backend qualification
File: c-emitter-owner-lifetimes.md
Purpose: Record current object-lifetime prerequisites for ERROR continuation repairs.
Responsibilities: Distinguish qualified scalar construction from open unwind contracts.
This file intentionally does NOT promote private sources or claim general unwinding.
-->

# ERROR owner object lifetimes, November 3, 2026

The current private owner candidate is rebased onto the accepted array-value
compiler, SHA-256 `5e3f320be5322581ae8a35152963d58b62f6c312882108c2636e358555bee8ce`.
It builds three fresh modules against the other 221 accepted objects. This
includes the current record guards and external-prototype dependency graph.
Raw C runtime calls now register actual dependency roots; helper prototypes
also participate in declaration deduplication. An initial build exposed those
missing roots, and its failed evidence remains in the private stage.

Construction tracking qualifies scalar objects with one direct default
constructor call, only a THIS argument, a nonvirtual destructor and plain
fields. The context records unseen, initializing and live states. A completed
call marks the object live; normal destruction retires it before user code
runs. Transfer cleanup destroys live objects in reverse resource order.
Unreached objects, failed constructors and already-retired objects are skipped.

The first experiment watched DECL-owned initializers. The parser flushes these
constructors separately, so that experiment correctly retained the unsupported
barrier and failed its native gate. The corrected prototype watches the actual
constructor call. Neither private experiment changes canonical compiler sources.

The mixed-dialect fixture compiles a modern FB worker and a legacy handler
owner as independent modules. Seven cases cover early transfer, one and two
live objects, ordinary scope exit, failed construction, repeated scope exit
and recursion. Both C producers, GCC/Clang, GNU11/GNU23, O0/O2 and -ex/-exx
pass 224 native executions; eight Win32 object checks also pass. The baseline
control reaches its unsupported-resource termination as expected.

The prior global/LOCAL handler, owner return/recursion, registration, worker,
GOSUB, string, array and const-result cases pass 256 fresh native runs and
twenty Win32 object checks. The runtime objects are rebuilt from current source
and 407 SDK headers are bound. No historical private execution is adopted.

Fresh mixed GAS64/C checks introduce no failures. C-owner/GAS64-worker improves
from 8/16 to 16/16; GAS64-owner/C-worker remains 12/16. The four existing
opposite-direction failures remain open. Both GAS64 objects match the accepted
provider byte for byte. Executable label transport is retained, so these tests
do not remove any continuation warning.

Strict source-only lint passes nine private source/fixture files. Semantic lint
is not qualified: its compiler input closure was unavailable for these ERROR
fixtures, and lifecycle analysis could not resolve their transfer paths. Full
compiler self-hosting on this owner candidate, semantic facts, managed members,
partial managed construction, explicit initialization, UDT arrays, temporary and
parameter lifetimes, threading and wider target qualification remain open.
Unsupported resources keep their termination barrier. The candidate is private.

The receipt is `build/c-emitter-owner-lifetimes-20261103/summary.json`.
Accepted queue totals remain 2,141 GCC and 8,763 Clang warnings. Installed
binaries and unrelated dirty sources are unchanged. PNG and the user-closed
intermittent-crash investigation are excluded. The stage stays below 64 MiB,
reuses scratch objects and runs at most two heavy workers at below-normal priority.

<!-- end of c-emitter-owner-lifetimes.md -->
