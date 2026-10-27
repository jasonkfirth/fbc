<!--
Project: FreeBASIC gfxlib3
File: README.md
Purpose: Map the queued graphics runtime and its implementation boundaries.
Responsibilities: Explain source ownership and render-thread backend modules.
This file intentionally does NOT contain the language reference or test receipts.
-->

# Queued graphics runtime

gfxlib3 is the opt-in `-gfx3` runtime. It preserves BASIC graphics behavior
while a renderer thread executes copied commands. Read the
[maintenance guide](../../docs/gfx-maintenance.md) alongside the detailed
[architecture](../../docs/gfxlib3/architecture.md) and
[test plan](../../docs/gfxlib3/test-plan.md).

## Source ownership

| Directory | Responsibility |
| --- | --- |
| `api` | Public C ABI entry points, argument validation and BASIC-facing operations |
| `compat` | Current drawing state, coordinate rules, console and CPU image representations |
| `core` | Context, producer batching, queue, renderer thread, resource registry, input state and logging |
| `backend` | Null, OpenGL and Vulkan execution plus backend selection and GLES fallback |
| `platform` | Common mode-list policy and unsupported-platform adapters |
| `codec` | PNG streams and bounded encoding/decoding |
| `drivers` | Native windows, event pumping, context and loader integration; see the [driver map](drivers/README.md) |
| `shaders` | Editable Vulkan GLSL and its deterministic SPIR-V generator |

Headers at the root define the established layer interfaces. New private
OpenGL and Vulkan headers live in `backend` and may only be included by that
backend's implementation units. The common renderer and producer APIs must
not depend on GPU types or the private backend state layouts.

## From BASIC to a completed draw

The `api` entry point and `compat` state interpret the BASIC operation.
`core/gfx3_context.c` builds validated command payloads and serializes producer
submissions. `core/gfx3_command.c` owns queue and completion synchronization.
`core/gfx3_renderer.c` drains commands in order, runs the selected backend on
its owning thread, and releases or fails pending waiters during shutdown.

A queued payload owns the bytes it needs after the caller returns. A handle
names a generation-tagged resource, not a pointer to a driver object. The
registry retains resources across command execution and defers destruction
until the last GPU sequence using them has completed. Keep submission order,
resource references and GPU completion distinct when reviewing a change.

Write-only operations can return before GPU completion. Readback, explicit
flushes and CPU compatibility operations form barriers. `SCREENPTR`, custom
PUT callbacks and caller-owned `FB.IMAGE` memory cannot be made asynchronous
by retaining a borrowed pointer. See `gfx3_image.h`, `gfx3_gpu_surface.h` and
the architecture's cache and direct-access contracts before changing these
paths.

`SCREENSET` queues a presentation for the selected visible page. When that
page's content has not changed, `SCREENSYNC` waits behind the queued present
instead of submitting a second swap. If new visible writes arrive first,
`SCREENSYNC` still queues and waits for a fresh presentation.

## OpenGL implementation

All these units operate on one render-thread-owned `FB_GFX3_OPENGL_STATE`:

| File in `backend` | Responsibility |
| --- | --- |
| `gfx3_backend_opengl.c` | Backend vtable, initialization, teardown, dispatch and GPU fences |
| `gfx3_opengl_program.c` | Entry-point loading, program compilation and uniform setup |
| `gfx3_opengl_shaders.h` | Private GLSL strings used by program construction |
| `gfx3_opengl_surface.c` | Integer textures, surface handles, transfers, palette and presentation |
| `gfx3_opengl_draw.c` | Individual geometric and paint dispatches |
| `gfx3_opengl_batch.c` | Ordered primitive, glyph, rectangle and blit batch construction |
| `gfx3_opengl_blit.c` | Individual and transformed image copies, pixel readback |
| `gfx3_opengl_private.h` | Backend storage and the small cross-unit helper interface |

The platform adapter creates the native window and makes the GL context
available on the renderer thread. Loading an entry point does not make it
safe to call from a producer thread. Program setup owns the GLSL strings;
other units use program identifiers and uniform locations in the state.

Batch limits are coupled to shader-visible ordered winner indices. Preserve
their masks, generation tags and ordering when changing packet sizes. They
are protocol constraints, not tuning constants that can be raised alone.

## Vulkan implementation

The public internal `gfx3_vulkan.h` retains an opaque implementation pointer.
Only the following backend units share the concrete private layout:

| File in `backend` | Responsibility |
| --- | --- |
| `gfx3_vulkan.c` | Loader, physical-device selection, logical device, pipelines and runtime lifetime |
| `gfx3_vulkan_submit.c` | Command buffers, fences, submission slots, deferred buffers and barriers |
| `gfx3_vulkan_present.c` | Surface formats, swapchain resources, resize and presentation |
| `gfx3_vulkan_surface.c` | Logical surface validation, allocation, clear, transfer and destruction |
| `gfx3_vulkan_draw.c` | Points, glyphs, lines, rectangles, ellipses and paint commands |
| `gfx3_vulkan_blit.c` | Copies, transformed blits and ordered PUT batches |
| `gfx3_vulkan_private.h` | Header-independent Vulkan declarations, state layout and cross-unit helpers |

`gfx3_backend_vulkan.c` remains the common backend-vtable adapter. Vulkan
runtime implementation functions do not interpret BASIC coordinates or own
input events. Submission slots retain staging and deferred allocations until
their fence completes; destroying those allocations at command-recording
time would create a use-after-free on the GPU.

The SPIR-V arrays in `gfx3_vulkan_shader.h` are generated from `shaders/*.comp`.
Regenerate with `shaders/regenerate_vulkan_shaders.py` when editing those
shaders. The packed C records in the private header must still match GLSL
alignment and field offsets. Moving the backend does not change that binary
contract or require shader regeneration.

## Platform limits

Native adapters in `drivers` currently exist for Windows, Linux, Android and Darwin. BSD
uses gfxlib2's native graphics implementation; it has no gfxlib3 window
adapter. Building generic gfxlib3 fallback units on BSD is useful for
portability, but does not qualify a native GPU renderer there.

The common `backend/gfx3_backend_gles.c` is the unsupported fallback. Android
owns the GLES implementation; Linux's adapter includes it when GLES headers
are available, otherwise it includes the fallback. Keep these source-inclusion
paths valid when moving either implementation.

Use `make gfx-structure-test` for ownership and source graph checks. Full
language tests, gfxlib3 parity wrappers and hardware-specific tests have
different scopes; the maintenance guide gives the required order.

<!-- end of README.md -->
