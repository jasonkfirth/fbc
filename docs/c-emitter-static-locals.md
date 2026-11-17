<!--
Project: FreeBASIC C backend qualification
File: c-emitter-static-locals.md
Purpose: Explain unread static storage and required scalar evaluation.
Responsibilities: Record compiler changes, source diagnostics and native evidence.
This file intentionally does NOT claim a completed whole-corpus recount.
-->

# Static locals and evaluated values, November 13

The candidate compiler extends the existing debug-storage acknowledgement to
named private static locals. With debug information enabled, their declarations,
assignments, initialization and lifetime remain intact. The acknowledgement is
an enum containing `sizeof(&variable)`: it never evaluates the stored value and
adds no runtime instruction. Opaque bodies retain the conservative fallback.

Without debug information, unread private floating statics use the existing
scalar-store proof. Explicit initializers, address escapes, external visibility,
thread-local storage and object lifetime still prevent removal. Required RHS
effects remain even when the destination disappears. FreeBASIC's ordinary
unread-variable warning remains available through `-w all`.

## Floating evaluation

The native controls found an existing weakness in the evaluated-value helper.
An inline identity function is insufficient: GCC and Clang can inline it and
discard a narrowing conversion whose result has no reader. The original
assembly backend sets the floating-point inexact flag for the tested conversion;
the preceding C output loses that effect at O2.

The scalar helper now materializes its typed argument in a volatile local and
returns that local. This keeps the required value evaluation and destination
rounding while avoiding a reference to the discarded BASIC storage. The helper
is used only where the scalar analysis already requires a value context.
Ordinary discardable stores and call-only effects keep their existing handling.

## Completed focused evidence

Candidate `c7f06b1d1f043734815b071b8255f3bbc2691fd230626b74600a0a532f22371a`
reaches a 224-unit generated-C fixed point with zero GCC warnings. It includes
the preceding private-ASM and library-name repairs. Two independently changing
Wii compiler inputs use their exact qualified predecessors in the isolated build;
their canonical files and pending changes remain untouched.

The new fixtures pass 64 real executions across Win32/Win64, both C emitters,
both native tools, GNU11/GNU23 and O0/O2, with zero native warnings. They retain
two intentional FBC unread-variable diagnostics per emission. The debug fixture
checks the random stream, repeated assignments and one-time static construction
and destruction. The floating fixture checks the actual inexact status after
both static and automatic narrowing assignments.

Both original corpus reports are freshly compiled before and after the repair:

| Original context | GCC before/after | Clang before/after |
| --- | ---: | ---: |
| WSR `raider.bas`, original debug context | 1 / 0 | 30 / 29 |
| Legacy `Santa.bas`, original QB context | 1 / 0 | 1 / 0 |

Every original FBC warning line and all other native warning classes match.
Neither application is edited or executed. The public backend suite includes
the two fixtures and the C floating-environment observer. Strict lint passes for
all three changed compiler helpers and both BASIC fixtures when using the
matching compiler explicitly.

Receipts reuse `build/c-emitter-forward-views-20261105/`:
`static-locals-selfhost-v1-20261113.json.zlib`,
`static-locals-values-v1-20261113.json.zlib` and
`static-locals-originals-v1-20261113.json.zlib`.
The older static-only assembly baseline and native C controls remain separate
receipts; they are not executions of the later extended fixture.

The 703-profile fbctests replay is complete at zero GCC and 779 Clang warnings.
Six profiles receive fresh native compilation; 697 retain exact generated-C
identities with the preceding compiler. This replay does not execute the test
applications. The complete GCC/Clang backend suite passes in 1,042.732 seconds.
The compiler is now selected through `build/main-gcc/bin/fbc.exe`; eight further
native values pass through that path after promotion. Its immediate rollback is
the preceding ASM-procedure compiler `d3c1a61e`. Selection is recorded in
`static-locals-selected-v1-20261113.json.zlib`.

Installed release binaries and runtime archives remain unchanged. The
whole-corpus recount now completes at 12 GCC and 15,800 Clang warnings across
6,419 profiles. Its compiler source cut and independently updated formatting
test are explicitly reconciled. The later
[ASM reference repair](c-emitter-asm-reference-analysis.md) is now selected
from the full canonical compiler source. The
[current status](c-emitter-current-status.md) keeps selected-compiler and
whole-corpus results separate.

<!-- end of c-emitter-static-locals.md -->
