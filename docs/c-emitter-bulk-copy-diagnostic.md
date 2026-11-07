<!--
Project: FreeBASIC C backend verification
File: c-emitter-bulk-copy-diagnostic.md
Purpose: Resolve the selected GCC containing-record copy warning from local evidence.
Responsibilities: Preserve the reproducer, optimizer trace and native qualification.
This file intentionally does NOT suppress the diagnostic or change copy semantics.
-->

# Current containing-record copy qualification, November 7, 2026

The complete current broad GCC replay retains this one warning in additional
profile 119. Its normal emission also reports the same containing-record copy
diagnostic. The existing standalone C reproducer has been refreshed with the
current GCC and Clang under full unsuppressed diagnostics. Four ordinary Win64
O0/O2 value runs pass and compare every copied byte. GCC O2 reproduces the
array-bounds observation; GCC O0 and both Clang cases do not report that class.
The current receipt binds 29 source, tool and SDK inputs and retains all other
fixture diagnostics separately from emitted-C queue totals. No compiler or
Stunts source was changed for this diagnostic. Win32 and instrumented execution
were not rerun in this refresh.

Current evidence: generated-c-bulk-false-positive.json.zlib in the canonical
audit work area, produced by build/c-emitter-forward-views-20261105/
generated-c-bulk-false-positive.py. The former build/c-emitter-bulk-owner-20261102
directory is no longer retained. The larger qualification and counts below are
historical; the current broad total is 228 and Exampleageddon is eight.

# GCC containing-record copy diagnostic, November 2, 2026

The GCC -Warray-bounds warning in additional profile 119 is a reproduced GCC
16.2.0 diagnostic false positive. Stunts' prediction reconciliation, at
`stunts_multiplayer_prediction.bas:467`, emits a 144-byte copy of the complete
STATE member using that member's address. GCC attributes it to the initial
12-byte POSITION subobject. The warning remains counted without suppression.

The current-input reproduction is byte-identical to the selected C. Initial
GIMPLE keeps distinct addresses for the leading POSITION read and full STATE
copy. GCC's optimized representation merges them into one pointer to POSITION,
then uses it for the larger memcpy. Standalone C without FreeBASIC reproduces
the same warning at O2 in GNU11/GNU23; O0 and Clang do not report it. The local
MSYS2 GCC 16.2.0 and Clang 22.1.7 executables are recorded by hash.

The permanent `tests/compiler-support/c-bulk-copy-subobjects.c` and Python driver
read the leading vector, copy the containing record and compare every copied
byte. Sixteen native runs pass, including eight with pointer-overflow,
object-size and alignment instrumentation. Four Win32 object checks pass;
Win32 execution is not claimed. The SDK dependency closure binds 26 headers.
The fixture retains a separate GCC float-equality observation for the expected
sum of small exactly represented integer-valued floats. Fixture diagnostics
are separate from selected emitted-C totals.

The existing local `stunts_multiplayer_prediction_test.bas` passes eight native
runs, covering both C emitters, both C compilers and ordinary/instrumented
execution. Each builds the actual seventeen units from the project's prediction
test recipe. The test checks local simulation, analog input, reconciliation
replay, presentation continuity and bounded extrapolation. The ASTR JSON asset,
source, compiler, runtime, environment and working directory are bound. No
transport test or game installation is changed.

An earlier experiment used a byte offset from the containing prediction object
instead of the source member address. Static assertions verified its offset,
member size and containing span. GCC still reported the warning, so the
experiment was not promoted. Copy semantics, padding and evaluation are retained.

`build/c-emitter-bulk-owner-20261102/summary.json` binds the selected reproduction,
optimizer dumps, rejected experiment and both native matrices. The retained-work
guard is 32 MiB; compiles run below normal priority with timeouts. Installed
binaries and canonical compiler/project sources are unchanged.

Selected totals remain 2,142 GCC and 67,529 Clang. This resolves the investigation
of this one diagnostic; broader bulk-copy shapes, pointer views, cross-procedure
ERROR handling and remaining warning/coverage queues stay open. Twelve PNG
profiles remain excluded.

<!-- end of c-emitter-bulk-copy-diagnostic.md -->
