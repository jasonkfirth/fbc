# FreeBASIC gfxlib3

## Purpose

gfxlib3 is an opt-in graphics runtime for FreeBASIC. It preserves the existing
FreeBASIC and QB graphics language while moving normal rendering work to a GPU
owned by a dedicated render thread.

gfxlib2 remains the default runtime. gfxlib3 is intended for systems with a
working Vulkan implementation, sufficiently modern desktop OpenGL, or OpenGL
ES 3.0 on Android. The headless null driver remains available for tests and
non-windowed image work.

This directory is the design and implementation record for gfxlib3. A feature
is only marked implemented after it is built and tested.

## Current implementation state

The common core is now buildable with `make gfxlib3`. It includes:

- checked command and payload allocation
- a bounded multi-producer queue with ordered final shutdown
- synchronous completion objects and renderer-failure wakeups
- generation-tagged resources with fence-aware deferred destruction
- a dedicated render-thread lifecycle and private backend interface
- gfxlib2-style ordered backend selection with requested-driver priority and
  complete GPU fallback
- a backend-independent surface and primitive command protocol
- a typed context/surface layer used by the FreeBASIC compatibility front end
- logical modes, GPU pages, per-caller drawing state, VIEW/WINDOW, and PMAP
- a FreeBASIC ABI covering the complete public `-lang fb` graphics command
  sweep without linking gfxlib2 fallback objects
- CPU-compatible current and QB `FB.IMAGE` allocation, information, drawing,
  GET, PUT, row conversion, BSAVE, and BLOAD paths
- PAINT, the QB DRAW language, the canonical bitmap font, and custom font
  assembly above GPU primitives and explicit compatibility barriers
- SCREENINFO, SCREENCONTROL, SCREENPTR/SCREENLOCK, palette state, page control,
  Android controller, Win32 WinMM joystick and XInput snapshots, and bounded
  no-device behavior
- a headless reference backend with 1, 2, 4, 8, 16, and 32-bit surfaces
- reference clear, point, styled line, BOX/BF, ellipse, PUT, and readback commands
- a header-independent Win32 and Linux/X11 Vulkan backend with real
  instance/device/queue ownership, reusable fenced command submission,
  embedded SPIR-V pipelines, device-local logical pixel surfaces, and a
  resize-aware swapchain
- Win32/WGL and Linux/GLX OpenGL 4.3 compute paths with GPU surfaces and GPU
  fences
- an Android NativeActivity/EGL adapter and OpenGL ES 3.0 shader backend with
  GPU logical pages, primitives, blits, palette conversion, and presentation
- Android NativeActivity touch, hardware-key, and Java IME key dispatch into
  the common SCREENEVENT, MULTIKEY, and INKEY state
- Vulkan GPU clear, point, styled line, BOX/BF, full ellipse, arc point-batch,
  and built-in PUT paths with exact readback
- OpenGL GPU clear, point, styled line, BOX/BF, ellipse, and built-in PUT
  compute shaders
- visible 1, 2, 4, 8, 16, and 32-bit Win32 and X11 Vulkan presentation with
  GPU palette/RGB565/32-bit conversion and double or triple swapchain buffering
- visible Win32 and X11 OpenGL presentation
- separate Win32 and X11 platform adapters for graphics-neutral windows and
  input, native Vulkan handles, WGL/GLX contexts, titles, event polling, and
  swaps
- checked pitched upload and synchronous download commands
- opaque GPU-only surfaces with standard primitive, DRAW, PAINT, text, GET,
  PUT, explicit transfer, GPU blit, and direct presentation paths
- an explicit ordered OpenGL/GLES interop callback that preserves render-thread
  context ownership while allowing advanced backend-specific work
- a page-specific graphical console with GPU glyphs/backgrounds and GPU scroll
- COLOR, CLS, 8 by 8 WIDTH, LOCATE/POS/CSRLIN, PRINT, and SCREEN cell hooks
- graphical standard-input reading and narrow/wide LINE INPUT through the
  runtime line editor
- synchronized Win32 and X11 keyboard, mouse, focus, close, INKEY, MULTIKEY,
  GETMOUSE, SETMOUSE, and SCREENEVENT compatibility
- graphics-aware SLEEP and bounded indexed VGA palette/status port emulation
- Android native touch snapshots with up to 16 pointer IDs, bounded
  missing-controller defaults, desktop mouse-as-touch fallback, and QB
  STICK/STRIG compatibility
- centralized bounded logging
- normal, multithreaded, PIC, and multithreaded PIC archive variants

The reference commands are pixel-tested through the real render thread,
including clipping, overlapping blits, logical color masks, and resource
destruction. Stateful compatibility tests cover page isolation/copy, relative
coordinates, VIEW fill and border, WINDOW Y direction, PMAP, pen position, and
primitive routing. A FreeBASIC program using the public ABI passes those same
core paths through GFX_NULL. The OpenGL path has also passed actual GPU readback
comparisons against the reference line, box, ellipse, transfer, PAINT, DRAW,
and text paths. Source selection and its normal, multithreaded, and PIC suffix
handling are working.

The same GPU-surface, image, command-compatibility, and graphical-console
smokes now pass through Vulkan in normal and `-mt` builds. That exercises
CIRCLE and arcs, PAINT, DRAW, text point batches, GET, CPU custom PUT fallback,
all built-in GPU PUT modes, overlap-safe scrolling, page cells, BMP round trips,
visible pages, direct GPU-surface presentation, and ordered surface cleanup.

The complete FreeBASIC and QB command sweeps now link against gfxlib3 alone and
exit successfully under `GFX_NULL`; the QB sweep also passes automatic Vulkan
in normal and `-mt` builds. All 46 unchanged sources in `tests/gfx` now meet
their expected result with gfxlib3 selected. The combined fbcunit executable
passes all 464,128 graphics assertions across 19 tests (464,774 total
assertions, including its support suites), and the standalone device
and compile-result cases also pass. The image and command compatibility smoke
programs pass through the real OpenGL backend. A visible presentation smoke
passes indexed, RGB565, and 32-bit modes, page changes, synchronized presents,
and native title propagation. A separate Vulkan test captures exact displayed
pixels for all six supported depths and recreates the swapchain after a client
resize on both Win32 and X11. Focused Win32 input and graphical line-input
smokes pass in normal and `-mt` builds. A separate X11 input smoke passes both
GPU backends in normal and `-mt` builds. The Android on-screen keyboard control
now has physical GLES show/hide, input-consumption, and IME-commit coverage;
its Vulkan presentation path awaits Vulkan-capable Android hardware. Wayland,
exact soft-cursor appearance, alternate console fonts, multiple in-flight
Vulkan frames, and broader codec compatibility remain open.

Repeated public mode lifecycle tests pass 1,000 null opens, automatic and
forced desktop GPU opens, and two physical Android runs of 32 automatic GLES
opens each. They cover both explicit teardown and replacement of an active
mode, plus recovery from a rejected mode request. Exhaustive per-stage backend
fault injection is still open.

Vulkan is now an explicitly selectable renderer through `FB.GFX_VULKAN`. A
normal or `-mt` FreeBASIC program can create 1, 2, 4, 8, 16, or 32-bit
device-local pages and use PSET, POINT, solid or styled LINE, BOX, BF, CIRCLE,
arc point batches, and all built-in PUT modes through the common render thread.
Same-surface overlapping PUT stays in GPU memory by snapshotting the source
before dispatch. Pitched transfer and readback also work.
The Vulkan backend converts the visible page into BGRA8 with an embedded
compute shader, copies it directly into an acquired Win32 or X11 swapchain
image, and presents it without CPU pixel conversion. OpenGL is the desktop GPU
fallback and OpenGL ES 3.0 is the Android fallback. An ordinary mode tries
Vulkan first, then the platform OpenGL backend. `FBGFX` and
`SCREENCONTROL SET_DRIVER_NAME` first give a recognized GPU backend a dedicated
attempt, then retain gfxlib2's complete ordered-list fallback. `FB.GFX_OPENGL`
and `FB.GFX_VULKAN` remain force-only requests.
A failed GPU request never falls back to the null renderer.

## Documents

- [gfxlib2 API inventory](gfxlib2-api-inventory.md) lists the compatibility
  contract discovered in the existing runtime, compiler, include files, and
  tests.
- [Architecture](architecture.md) describes threads, commands, surfaces,
  synchronization, GPU backends, compatibility barriers, and failure handling.
- [Resizable SCREEN contract](resizable-screen.md) defines the opt-in desktop
  window flag, resize event, page migration, and pointer lifetime rules shared
  by gfxlib2 and gfxlib3.
- [Parity ledger](parity.md) records the implementation and test status of
  every API family.
- [Benchmark coverage](benchmark-coverage.md) maps both the internal command
  protocol and the public gfxlib2/QB API families to repeatable timing fixtures.
- [Performance record](performance.md) separates BASIC-thread submission,
  completed GPU throughput, and measured renderer overlap across the tested
  adapters.
- [newJRPG qualification](newjrpg-qualification.md) records the desktop,
  Win32, Android, compatibility, and performance results for the new JRPG
  engine workload.
- [Test plan](test-plan.md) defines the checks required before gfxlib3 can be
  considered compatible.
- [Verification record](verification.md) records exact builds, test commands,
  hardware output, and the boundary of each proof.

## Activation contract

The intended source-level selection is:

```freebasic
#define __FB_GFXLIB3__
#include once "fbgfx.bi"
```

The define should appear before the first inclusion of `fbgfx.bi`. Graphics
intrinsics also recognize it when no explicit include is present, and the
compiler's `-gfx3` preinclude takes the same intrinsic-only path. The compiler
selects the correct normal, multithreaded, PIC, or multithreaded PIC gfxlib3
archive and records `-gfx3` in object metadata for separate compilation.

Programs that need opaque GPU surfaces may instead include `fbgfx3.bi`.
That extension header selects gfxlib3 itself and then includes `fbgfx.bi`; a
second explicit source define is unnecessary.

Without either opt-in, including `fbgfx.bi` continues to select gfxlib2. The
Android packaging helper applies the same rule and does not switch archives
merely because the source uses ordinary graphics commands.

`-gfx3` is the equivalent compiler opt-in. It pre-includes the public selector
before parsing source, selects the gfxlib3 archive, and writes the same object
metadata. The selector uses the same bare `__FB_GFXLIB3__` spelling shown
above, so an unchanged source that also defines it does not become a conflicting
macro redefinition. It is useful for a project-wide build setting when editing
every graphics source is undesirable:

```text
fbc -gfx3 game.bas
```

The Android packaging helper accepts the same option. Some standalone Android
cross compilers predate the public `-gfx3` switch, so the helper translates it
to a private preprocessor marker and overlays the checkout's `fbgfx.bi` and
`fbgfx3.bi` declarations. The header maps that marker to
`__FB_GFXLIB3__` without redefining a source-level selection. Both forms select
the multithreaded gfxlib3 Android archive even when BASIC code creates no
threads explicitly.

Vulkan can be requested explicitly on a Vulkan-capable desktop system:

```freebasic
screenres 1280, 720, 32, 2, FB.GFX_VULKAN
```

This flag never silently selects the null renderer. If the Vulkan runtime,
present-capable adapter, or required compute support is unavailable, mode
creation fails. Android Vulkan surface creation and presentation are built,
but remain physical-hardware validation work because the connected Android
device only exposes OpenGL ES 3.0.

An opt-in link contains gfxlib3 and does not contain gfxlib2. The two runtimes
cannot be mixed safely because their archive objects and process-global runtime
state are not compatibility boundaries. The current gfxlib2 public graphics ABI
is exported by gfxlib3; a missing GPU capability is reported by the relevant
mode or operation rather than silently entering gfxlib2 with incompatible
runtime state.

The extension header selects gfxlib3 automatically and exposes opaque GPU
surfaces:

```freebasic
#include once "fbgfx3.bi"

dim as any ptr layer = fb.Gfx3SurfaceCreate( 1920, 1080, 32 )
line layer, (0, 0)-(1919, 1079), rgb( 255, 255, 255 ), bf
fb.Gfx3SurfacePresent layer, true
fb.Gfx3SurfaceDestroy layer
```

The pointer is an opaque runtime descriptor, not pixel memory. Ordinary target
syntax keeps compatible drawing source concise while the pixels remain in GPU
storage.

When a program genuinely needs CPU access, use a scoped staging map rather
than treating that descriptor as an `FB.IMAGE`:

```freebasic
dim as any ptr pixels
dim as long pitch

if fb.Gfx3SurfaceMap( layer, fb.GFX3_MAP_WRITE, pixels, pitch ) = 0 then
    cptr( ulong ptr, pixels )[0] = rgb( 255, 0, 0 )
    fb.Gfx3SurfaceUnmap( layer )
end if
```

`Gfx3SurfaceMap` downloads the full surface into CPU staging memory. For a
smaller transfer, `Gfx3SurfaceMapRect` takes `(x, y, width, height)` before
the access argument and stages only that rectangle; its pitch is based on the
rectangle width. Writable unmap uploads the same mapped region. Both forms
require `TRANSFER_SOURCE` in every case and `TRANSFER_DESTINATION` when
writable; drawing, transfer, presentation, and destruction reject the surface
while it is mapped. The returned pointer becomes invalid at unmap or `SCREEN
0` and must never be retained.

### Loading and transforming GPU assets

`Gfx3SurfaceLoad` decodes a BLOAD-compatible BMP or PNG, uploads it once, and
returns an opaque sampled surface. The temporary decoder pixels are released
before the function returns. Ordinary PUT accepts this GPU surface directly:

```freebasic
#include once "fbgfx3.bi"

screenres 1280, 720, 32, 2

dim sprite as any ptr = fb.Gfx3SurfaceLoad( "ship.bmp" )
if sprite = 0 then end 1

put ( 40, 40 ), sprite, trans
fb.Gfx3SurfaceBlitScaled( 0, sprite, 0, 0, 64, 64, _
    160, 40, 192, 192, fb.GFX3_PUT_TRANS, 255, fb.GFX3_FILTER_LINEAR )
fb.Gfx3SurfaceBlitRotated( 0, sprite, 0, 0, 64, 64, _
    500.0, 180.0, 35.0, 2.0, 2.0, -1.0, -1.0, _
    fb.GFX3_PUT_TRANS, 255, fb.GFX3_FILTER_LINEAR )

fb.Gfx3SurfaceDestroy sprite
```

A null destination means the current work page. `Gfx3SurfaceBlitRotated`
places the chosen source pivot at `(destination_x, destination_y)`. Negative
pivot coordinates select the centre of the source rectangle. Angles are in
degrees, positive angles rotate clockwise because screen Y grows downward,
and X/Y scale may be selected independently.

Mode 7 maps a repeating source rectangle across a projective ground plane:

```freebasic
fb.Gfx3SurfaceMode7( 0, track, 0, 0, 256, 256, _
    0, 0, 1280, 720, _
    camera_x, camera_y, 80.0, heading_degrees, _
    280.0, 420.0, fb.GFX3_PUT_PSET, 255, fb.GFX3_FILTER_LINEAR )
```

Camera position and height are measured in source texels. `horizon_y` and
`focal_length` are destination pixels. Only pixels below the horizon are
generated. The source rectangle repeats independently of the underlying
texture size, so one atlas region can be used without sampling neighbouring
assets.

Scaling, rotation, and Mode 7 send a projective matrix and bounds to the render
thread. Per-pixel coordinate math, filtering, wrapping, transparency, and PUT
math run in OpenGL, Vulkan, or GLES shaders. Source and destination stay in GPU
memory until code requests an observable CPU result through POINT, GET,
DOWNLOAD, MAP, SCREENPTR, or a CPU-only custom blender.

### Advanced OpenGL/GLES interop

gfxlib3 deliberately does not return a live `SCREENGLPROC` pointer to ordinary
BASIC code. Its context belongs to the render thread, so calling a raw entry
point from the application thread would race queued graphics work. Programs
that need a backend-specific extension can instead submit a bounded callback:

```freebasic
#include once "fbgfx3.bi"

sub inspect_gl cdecl( byval ignored as any ptr )
    dim get_string as function stdcall( byval name as ulong ) as const ubyte ptr
    get_string = cptr( function stdcall( byval name as ulong ) as const ubyte ptr, _
        screenglproc( "glGetString" ) )
    '' Use get_string only inside this callback.
end sub

fb.Gfx3RunOnRenderThread( cptr( fb.Gfx3RenderCallback ptr, @inspect_gl ) )
```

The call waits for earlier GPU work, executes the callback on the owning
OpenGL/GLES thread, and waits again before later gfxlib3 work resumes.
`SCREENGLPROC` returns a non-NULL procedure only during that callback and only
for OpenGL or GLES backends. The callback must not call ordinary `SCREEN`,
drawing, image, or gfxlib3 surface APIs, because those APIs would wait for the
same renderer thread. It must also leave the GL state expected by its own
following work, or use and restore state that it changes. Vulkan intentionally
does not offer this OpenGL interop route.

## Initial platform scope

The currently implemented windowed targets are:

- Windows using automatic Vulkan to OpenGL 4.3 fallback.
- Linux/X11 using automatic Vulkan to OpenGL 4.3 fallback.
- Android NativeActivity using automatic Vulkan to OpenGL ES 3.0 fallback.
- The null backend on any target where gfxlib3 itself can be built.

Android Vulkan window-system presentation is implemented through the dynamic
loader, `VK_KHR_android_surface`, and the retained `ANativeWindow`. The
connected API 24 AGM A8 exposes only a stub Vulkan HAL, so automatic selection
correctly falls through to the tested GLES path there. Android surface creation,
swapchain presentation, resize, and the Vulkan KB compositor still require a
Vulkan-capable physical device before they can be marked hardware-verified.
Darwin is a later OpenGL and optional MoltenVK target. DOS, Haiku, JavaScript,
NuttX, Wii, and Xbox stay on gfxlib2 unless a gfxlib3 platform adapter is added
deliberately.

## Compatibility policy

Existing source code should keep its current behavior. In particular:

- Existing `IMAGECREATE` images remain CPU-visible FB.IMAGE buffers.
- Drawing commands preserve command order and FreeBASIC/QB pixel semantics.
- `GFX_ALPHA_PRIMITIVES` keeps gfxlib2's exact 32-bit primitive blend rule;
  it is deliberately separate from `PUT ALPHA`.
- Read operations wait for prior GPU work when necessary.
- `SCREENPTR` and direct CPU access use a synchronized shadow buffer.
- Arbitrary FreeBASIC custom PUT blenders use a CPU compatibility path.
- New GPU-only surfaces are opt-in and never masquerade as ordinary writable
  pixel memory.

The fast path is therefore explicit: keep surfaces on the GPU, use built-in
blend modes, and avoid readback APIs inside a frame.

<!-- end of README.md -->
