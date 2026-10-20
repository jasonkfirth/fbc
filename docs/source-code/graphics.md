<!--
Project: FreeBASIC source documentation
File: source-code/graphics.md
Purpose: Explain the software and queued graphics source trees.
Responsibilities: Map drawing flow, state ownership, driver boundaries and images.
This file intentionally does NOT replace backend contracts or performance records.
-->

# Graphics source trees

FreeBASIC has a software graphics runtime in
[src/gfxlib2](../../src/gfxlib2/) and an opt-in queued graphics runtime in
[src/gfxlib3](../../src/gfxlib3/), selected with `-gfx3`. Both implement BASIC
graphics operations. Their internal execution and synchronization models
differ, so an operation must be followed through the selected library.

The compiler parses graphics syntax in
[parser-quirk-gfx.bas](../../src/compiler/parser/intrinsics/parser-quirk-gfx.bas)
and builds runtime calls in
[rtl-gfx.bas](../../src/compiler/runtime/rtl-gfx.bas). Those compiler modules
do not create windows or draw pixels themselves.

## Software graphics

```text
gfxlib2/
    fb_gfx.h, *.h          Shared context, image, driver and helper contracts
    core/                 Modes, pages, drawing contexts and access locks
    draw/                 Coordinates, clipping and drawing operations
    pixels/               PUT operators, row transfer and pixel conversion
    image/                CPU images, BLOAD/BSAVE and codecs
    text/                 Graphics console and bitmap text
    input/                Events, keyboard, mouse, controllers and drops
    palette/              Palette and VGA compatibility
    presentation/         Presentation helpers, software cursor and vsync
    data/                 Fonts and palettes
    drivers/              Native display/input adapters and CPU kernels
```

A drawing entry point acquires the graphics lock and obtains its drawing
context. `fb_hPrepareTarget()` chooses a screen page or caller-owned image.
The operation applies coordinate rules, clips and writes through the selected
pixel implementation. Visible framebuffer changes mark dirty scanlines for
the native presentation path.

Start with [draw/gfx_line.c](../../src/gfxlib2/draw/gfx_line.c) for a bounded
drawing operation, then read the target/context helpers and selected driver.
[core/gfx_vars.c](../../src/gfxlib2/core/gfx_vars.c) owns common global state;
the drawing context is thread-local. The graphics lock, native driver lock and
event mutex have different jobs. Native polling must not hold the event mutex
while calling code that posts events through the same mutex.

The [gfxlib2 map](../../src/gfxlib2/README.md) documents these locks, mode
changes, image layouts and borrowed framebuffer pointers in detail.

## Queued graphics

```text
gfxlib3/
    *.h                   Shared layer interfaces and resource contracts
    api/                  ABI entry points and argument validation
    compat/               BASIC state, coordinates, console and CPU images
    core/                 Context, commands, queue, renderer and resources
    backend/              Null, OpenGL and Vulkan execution
    platform/             Common mode policy and unsupported-platform adapters
    codec/                PNG streams and encoding/decoding
    drivers/              Native windows, event pumping and context integration
    shaders/              Vulkan shader sources and SPIR-V generation
```

`api` and `compat` interpret the operation. `core/gfx3_context.c` constructs
validated payloads, `core/gfx3_command.c` owns queue/completion coordination,
and `core/gfx3_renderer.c` executes commands through the selected backend on
the renderer thread.

A queued command must own the bytes needed after its caller returns. Resource
handles carry generations and references; they are not native object pointers.
CPU readback, direct access and custom PUT callbacks require the corresponding
synchronization boundary. Retaining a caller's temporary image pointer cannot
make those operations safe to queue.

Backend-private OpenGL and Vulkan headers belong inside `backend`. Platform
drivers own native windows, event handling and context integration. Loading a
GPU entry point does not make it callable from any thread. See the
[gfxlib3 map](../../src/gfxlib3/README.md) and
[renderer architecture](../gfxlib3/architecture.md) for the ownership and
completion rules.

## Where to make a graphics change

| Behavior | Owner to inspect |
| --- | --- |
| BASIC syntax or runtime argument selection | Compiler parser and `runtime/rtl-gfx.bas` |
| Coordinates, clipping, palettes or language-visible drawing state | `gfxlib2/draw` or `gfxlib3/api` and `compat` |
| Software pixel transfer or accelerated CPU kernel | `gfxlib2/pixels` and its architecture driver |
| GPU rendering or resource completion | `gfxlib3/core` and the selected backend |
| Native window, display event or controller behavior | The selected library's `drivers` layer |
| File encoding or caller-owned image layout | The library's image/codec modules and shared image headers |

[mk/gfx-sources.mk](../../mk/gfx-sources.mk) lists shared groups;
[mk/source-graph.mk](../../mk/source-graph.mk) selects native implementations
and derives variants. Some font, paint and Unicode helpers are shared across
the libraries. Read the [graphics maintenance guide](../gfx-maintenance.md)
before changing a shared contract, and cover both library paths where the
behavior is shared.

<!-- end of source-code/graphics.md -->
