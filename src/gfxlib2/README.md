<!--
Project: FreeBASIC gfxlib2
File: README.md
Purpose: Map the software graphics runtime for maintainers.
Responsibilities: Describe module ownership, shared state and platform layering.
This file intentionally does NOT contain a language reference or test receipts.
-->

# Software graphics runtime

gfxlib2 implements the default BASIC graphics ABI. Drawing normally updates CPU
memory; a selected native driver presents the visible page. Start with the
[graphics maintenance guide](../../docs/gfx-maintenance.md) for change recipes
and validation commands.

## Source ownership

Shared implementation directories are listed in `mk/gfx-sources.mk`. Headers
stay at this directory's root as shared internal contracts. Implementation
files include those headers by explicit relative paths. Native adapters and
CPU kernels live together under [drivers](drivers/README.md).

| Directory | Responsibility | Useful starting points |
| --- | --- | --- |
| `core` | Mode lifetime, shared context, pages, screen queries, access locks | `gfx_screen.c`, `gfx_core.c`, `gfx_access.c`, `gfx_vars.c` |
| `draw` | Drawing commands, clipping, VIEW/WINDOW coordinates, GET | `gfx_line.c`, `gfx_draw.c`, `gfx_paint.c`, `gfx_view.c` |
| `pixels` | PUT operators, row transfer and pixel conversion | `gfx_put.c`, `gfx_put_custom.c`, `gfx_blitter.c` |
| `image` | CPU image allocation, inspection, conversion, BLOAD/BSAVE, codecs | `gfx_image.c`, `gfx_bload.c`, `gfx_png.c` |
| `text` | Graphics console, bitmap text, line input, Unicode adapters | `gfx_print.c`, `gfx_drawstring.c`, `gfx_drawstring_utf8.c` |
| `input` | Events, keyboard, mouse, touch, joystick, extended controllers, drops | `gfx_event.c`, `gfx_xpad.c`, `gfx_drop.c` |
| `palette` | Palette operations and VGA port compatibility | `gfx_palette.c`, `gfx_vgaemu.c` |
| `presentation` | OpenGL presentation helpers, software cursor, vsync | `gfx_opengl.c`, `gfx_softcursor.c` |
| `data` | Built-in fonts and palettes | `gfx_data.c` |
| `drivers` | Native windows, input, presentation, OS services and CPU kernels | `drivers/unix`, `drivers/win32`, `drivers/linux`, `drivers/x86_64` |

The driver table and `__fb_gfx` are process-wide. A drawing context is
thread-local. `gfx_vars.c` owns the common globals; `gfx_core.c` initializes a
thread's context when its mode identifier no longer matches the active mode.
Its pixel transfer function selection and event-posting helpers are legacy
shared services, so moving either requires checking all drivers, including DOS
code-range locking markers.

## A drawing call

1. The ABI entry point takes the graphics lock and obtains a drawing context.
2. `fb_hPrepareTarget()` chooses a screen page or a caller-owned image, sets its
   row pointers, pitch, pixel width and viewport.
3. Coordinate helpers apply relative coordinates, WINDOW and VIEW semantics.
4. The command clips against that target and selects depth-specific pixel or
   PUT functions. SIMD is an implementation choice below these semantics.
5. Writes to the visible framebuffer mark dirty scanlines. The native driver
   copies or presents them under its own synchronization rules.

`fb_hPrepareTarget()` and `fb_hGetContext()` require the graphics lock. The
driver lock protects framebuffer access against presentation. The event mutex
protects the event ring. Native event polling occurs before taking the event
mutex because posting an event takes that same mutex. Preserve that order.

`SCREENLOCK` nests driver access through `lock_count`. Its matching unlock
also releases the graphics lock when no mode exists. Do not add an early
unlock to only one side of that pair. A `SCREENPTR` result is borrowed memory;
mode changes and applied resizes can replace the storage.

Native resize callbacks only publish a pending size. `gfx_screen.c` applies it
at a graphics API boundary, with the graphics lock held, and allocates
replacement storage before discarding the current mode's buffers. See the
[resize contract](../../docs/gfxlib3/resizable-screen.md).

## Stable contracts

- `fb_gfx.h` defines `FBGFX`, `GFXDRIVER`, the software drawing context, image
  headers, driver flags and internal entry points. Drivers and generic code
  must agree on these layouts.
- `../rtlib/fb_gfx_private.h` contains runtime-facing graphics hooks and shared
  ABI data. The compiler's graphics declarations live under
  `../compiler/runtime`; the BASIC declarations live in `inc/fbgfx.bi`.
- The packed `PUT_HEADER` has both legacy and current forms. Pixel storage
  begins after the appropriate header, and pitch can differ from width times
  bytes per pixel. Preserve those facts in GET, PUT, file I/O and direct access.
- `gfx_simd.h` specifies accelerated kernels. ARMv7 kernels require a runtime
  NEON check; generic ARMv7 objects must retain the non-NEON baseline.
- `gfx_font.h` and `gfx_paint_pattern.h` are shared with gfxlib3. Unicode
  adapters are also compiled by gfxlib3 against that library's implementations.
  Changing them requires both libraries' tests.

## Platform replacement rules

Make selects `drivers/OS/CPU` before `drivers/OS`, then shared `drivers/unix`,
then generic code.
Architecture kernels are selected separately. Replacements use the same
basename, even though objects retain their directory paths. Archives store
basenames, so duplicate selected members are rejected before compilation.

For example, an AROS m68k BLOAD wrapper includes `image/gfx_bload.c` with two
format adaptations. Linux GLES has a similar source-inclusion adapter in
gfxlib3. These are deliberate policy wrappers; changing a shared source path
must update the wrapper as well as the Make list.

Run `make gfx-structure-test` after adding or moving a shared source. It checks
the actual Make graphs for target and architecture combinations, include
resolution, header dependencies and ownership headers. It does not compile
unavailable cross toolchains.

<!-- end of README.md -->
