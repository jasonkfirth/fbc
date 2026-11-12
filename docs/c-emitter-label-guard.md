<!--
Project: FreeBASIC C backend qualification
File: c-emitter-label-guard.md
Purpose: Resolve the guarded label payload warning with the unchanged real test.
Responsibilities: Retain native diagnostics, runtime evidence and archive identity.
This file does not suppress bounds checks or qualify new graphics sources.
-->

# Guarded label payload, November 12

The additional TurboTrek queue retains one GCC `-Warray-bounds=2` observation
in `label_alignment_smoke.bas`. The test deliberately gives a non-label Widget
an eight-byte integer payload. `label_SetTextStyle` checks the widget, its data
pointer and the `label_Destroy` function identity before casting the payload to
LabelData. A wrong widget must return zero without reaching that typed access.
The requested style is valid, so style validation cannot explain that zero.

The unchanged full test now passes actual execution with both GCC and Clang.
It exercises the wrong-kind and null calls, normal style round trips, framebuffer
alignment, fonts, clipping and cleanup using the test's null graphics driver.
The emitted C is byte-identical to the completed queue's retained C hash.
GCC still reports exactly the one bounds observation; Clang retains its thirty-six
padding observations. The warning remains counted and unsuppressed.

This resolves the investigated observation as a guarded-path optimizer false
positive for this fixture. It does not establish that arbitrary caller-supplied
payloads are valid. The normal record offsets and kind checks remain intact.
No example, compiler source, function-identity comparison or native flag changes.

| Exact emitted C | GCC native warnings | Clang native warnings | Runtime |
| --- | ---: | ---: | --- |
| Original label fixture | 1 | 36 | Both tools pass |

The native runtime uses pinned existing `libfbmt.a`, `fbrt0.o` and installed
`libfbgfx3mt.a`. A temporary copy of only the graphics archive supplies its
missing search path without allowing installed runtime libraries to shadow the
qualified development prefix. No archive is rebuilt or installed. These results
qualify this fixture and binary combination; they make no claim about the
independent dirty graphics sources.

The first execution had a one-minute runtime cap and was terminated at that cap.
Its receipt is a failed check and receives no pass credit. The corrected replay
allows the real test to finish: GCC takes 82.926 seconds and Clang 82.155 seconds.
All 130 opened source paths, compiler/tool hashes and archive pins still match
after execution. Owned disk usage peaks at 11,360,895 bytes under a 64 MiB cap;
the C, object, executable, log and temporary archive slots are cleaned afterward.

Evidence is `build/c-emitter-forward-views-20261105/`:

- `current-label-runtime-20261112.json.zlib`: original capped execution.
- `current-label-runtime-v2-20261112.json.zlib`: exact C, unsuppressed diagnostics,
  link commands, both observed pass markers and final source/archive guards.

Completed warning counts are unchanged. The wider project sweep remains active.

<!-- end of c-emitter-label-guard.md -->
