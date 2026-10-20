<!--
Project: FreeBASIC gfxlib2
File: drivers/README.md
Purpose: Map native graphics adapters and CPU-specific implementations.
Responsibilities: Explain source selection, platform boundaries and shared hooks.
This file intentionally does NOT define BASIC drawing or image semantics.
-->

# Native graphics drivers

This directory owns platform-facing implementations. Start with the
[shared runtime map](../README.md) for drawing, image and console behavior.
Public contracts remain in the gfxlib2 root headers.

| Directories | Responsibility |
| --- | --- |
| OS names, such as `win32`, `linux`, `darwin`, `freebsd`, `haiku` and `dos` | Windows, input, native presentation, mode queries and OS-specific replacements |
| `unix` | Shared Unix/X11 adapters, event handling and joystick helpers |
| `x86`, `x86_64`, `arm`, `aarch64` | MMX, SSE2 and NEON pixel kernels |
| `aros/m68k` | Architecture-specific overrides within an OS adapter |

Make selects the target's OS/CPU implementation before its OS implementation,
then the shared Unix layer, then the shared subsystem implementation. Overrides
use the same basename. CPU kernels are selected independently; their special
instruction flags must stay confined to the kernel objects. ARMv7 generic code
must remain usable on systems without NEON.

Native callbacks report events and presentation requests through the shared
interfaces. The common graphics API applies BASIC coordinate rules and owns
mode-storage replacement. Observe the graphics, driver and event lock ordering
described in the [maintenance guide](../../../docs/gfx-maintenance.md).

Some adapters include another implementation's source deliberately. In
particular, `aros/m68k/gfx_bload.c` wraps the shared image decoder. Preserve the
included implementation's header paths when moving either side. Local quoted
includes resolve relative to the including source file, including those used
by the assembler preprocessor.

Headers and `.inc` fragments below this directory are clean-build dependencies.
Haiku's runtime debug module also consumes `haiku/fb_gfx_haiku.h`; update its
include search and structural checks when changing that contract.

Run `make gfx-structure-test` after changing driver placement or selection.
Native compilation and rendering checks are still required for changed driver
behavior on the affected platform.

<!-- end of drivers/README.md -->
