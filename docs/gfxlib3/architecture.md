# gfxlib3 architecture

## Design goals

gfxlib3 must:

- Preserve gfxlib2 and QB source behavior closely enough to run the existing
  graphics tests and normal programs without edits.
- Keep the GPU busy without making a BASIC program manage command buffers,
  render passes, synchronization primitives, or swapchains.
- Keep images and pages in video memory when the program chooses GPU surfaces.
- Queue ordinary write-only drawing without waiting for the GPU.
- Make synchronization visible in the design so readback behavior is correct.
- Support Vulkan and OpenGL as real renderers, not only as presentation
  blitters.
- Fail cleanly on unsupported hardware and leave gfxlib2 as the default.

## Non-goals

- Replacing Vulkan or OpenGL with a FreeBASIC-visible low-level API.
- Supporting every gfxlib2 platform in the first release.
- Pretending arbitrary CPU callback code can execute in a shader.
- Changing the layout or direct-memory meaning of existing FB.IMAGE buffers.
- Making pixel readback free.

## Window mode policy

An ordinary framed mode without `fb.GFX_RESIZABLE` has a fixed logical
framebuffer and may still use the desktop's maximize control. The platform
adapter reports the native client size to the presentation backend, which
selects the largest fitting integer scale and centers the result on black.
The OpenGL viewport or Vulkan compute presentation shader performs the native
scaling. The GPU page is not resized or copied, and the BASIC program receives
no logical resize event.

The same shared presentation layout maps native mouse events back to logical
pixels and maps `SETMOUSE` in the other direction. This prevents window
maximize from changing the coordinate system seen by existing programs. A
resizable mode bypasses these conversions because its logical and native
dimensions are identical.

`fb.GFX_RESIZABLE` creates an ordinary framed desktop window whose logical
framebuffer follows its client dimensions. The native thread publishes only a
coalesced size request. The BASIC thread applies it at a safe graphics API
boundary, after direct framebuffer access has ended. The completed operation
posts `fb.EVENT_WINDOW_RESIZE`; `SCREENINFO` and `GET_SCREEN_SIZE` then report
the new dimensions. Full details and pointer lifetime rules are in the
[resizable SCREEN contract](resizable-screen.md).

The flag is incompatible with fullscreen, no-frame, and shaped windows.
gfxlib3 migrates every logical page between GPU surfaces using clear and blit
commands, so an ordinary resize does not require GPU readback. gfxlib2 uses a
failure-safe replacement of its software pages and native presentation image.

`fb.GFX_FULLSCREEN` asks the selected platform adapter for a borderless window
covering the selected monitor. `fb.GFX_NO_FRAME` asks for a borderless window
at the program's requested client dimensions. This preserves the public
gfxlib2 flags while keeping window ownership with the backend that owns the
swapchain or GL context.

gfxlib3 deliberately does not call `ChangeDisplaySettings` or persistently
change the user's desktop mode. The GPU renderer accepts the desktop's current
presentation mode and performs scaling through its presentation pipeline. This
avoids mode-restoration failures if a process terminates unexpectedly and is
also a better fit for modern compositors.

Mode opening retains gfxlib2's complete compatibility retry. A recognized
`FBGFX` or `SET_DRIVER_NAME` backend is tried once, then the complete selected
GPU order is tried including that backend. If those windowed or fullscreen
attempts fail, the same named-first and complete-list passes repeat once with
`GFX_FULLSCREEN` inverted. This handles systems that can provide one
presentation form but not the other. Explicit Null modes have no window and
remain a single attempt.

## Major layers

```text
FreeBASIC program
        |
        v
Compatibility front end
  ABI, validation, QB behavior, VIEW/WINDOW, DRAW parsing, console hooks
        |
        v
Ordered command queue and resource registry
  copied payloads, sequence numbers, fences, deferred destruction
        |
        v
Render thread
  batching, pipeline selection, hazards, uploads, readbacks, presentation
        |
        +---------------------+
        |                     |
        v                     v
GPU backend              Platform adapter
Vulkan/OpenGL/GLES       Win32/X11/Android window/input
        |                     |
        +----------+----------+
                   v
             GPU and display
```

The GPU backend does not own window-system policy or BASIC event translation.
The platform adapter does not know LINE, CIRCLE, PUT, or FB.IMAGE rules.

Between the compatibility front end and raw commands, the typed context layer
owns one renderer and logger and represents each surface as a handle plus its
validated dimensions, depth, usage, and owner. It builds self-contained
commands for lifecycle, transfers, clear, points, line, BOX/BF, PUT, readback,
and explicit barriers. Write-only operations queue and return. Create,
destroy, readback, download, and flush wait for their sequence.

The stateful compatibility layer owns a logical mode and its GPU pages. A
separate draw-state object holds the calling thread's work page, VIEW/WINDOW
mapping, pen position, and colors, matching gfxlib2's distinction between
global visible-page state and TLS drawing state. The exported `fb_Gfx*` front
end installs this state in runtime TLS. A caller-thread cache retains the
resolved draw-state pointer and its mode generation, so a hot primitive stream
does not re-enter the runtime TLS registry for every sprite. Mode replacement
invalidates the generation before the cached pointer is dereferenced. The
front end routes SCREEN,
SCREENRES, PSET, POINT, LINE, BOX/BF, circles and arcs, SCREENSET, page copy,
VIEW, WINDOW, PMAP, POINTCOORD, PAINT, DRAW, text, images, transfers, palettes,
screen queries, file operations, graphical console output, and line input
through it. Win32 and X11 keyboard, mouse, focus, close, and BASIC input hooks,
plus Android focus and touch callbacks, route through a separate synchronized
input state. The graphical console selects FreeBASIC's canonical 8 by 8, 8 by
14, or 8 by 16 font according to the mode or WIDTH request. The platform GPU
presentation paths use the boundary described below.

## Alpha primitive compatibility

`FB.GFX_ALPHA_PRIMITIVES` is a mode option, queried and changed through
`SCREENCONTROL GET_ALPHA_PRIMITIVES` and `SET_ALPHA_PRIMITIVES`. When it is
enabled, a 32-bit primitive with a non-opaque source alpha uses gfxlib2's
integer pixel rule, not the separate `PUT ALPHA` rule:

```text
a   = source alpha
rgb = destination rgb + ((source rgb - destination rgb) * a >> 8)
out = rgb with the source alpha byte
```

The division is deliberately by 256. A zero-alpha primitive preserves the
destination RGB bytes but writes a zero alpha byte. An opaque source, and all
primitive drawing to indexed or RGB565 targets, remains a solid write.

The compatibility layer decides the flag after its normal color conversion and
attaches it to clear, point, line, rectangle, ellipse, arc-point, PAINT, VIEW,
and built-in text commands. CPU `FB.IMAGE` targets apply the same helper
directly. The null renderer applies it in command order. Vulkan and desktop
OpenGL use image-load/atomic shader paths where repeated outline pixels must
remain ordered; GLES samples a GPU snapshot of the destination before its
fragment pass because ES 3.0 does not provide image load/store. Console cell
backgrounds and PRINT remain solid, matching gfxlib2's opaque console transfer
setup. Pattern PAINT remains a raw pattern copy.

For full CIRCLE and ellipse commands, GLES also uses the same midpoint
scanline control sequence as the reference backend. The render thread supplies
that bounded scanline geometry while the fragment pipeline writes the spans or
endpoints in order. This removes the old implicit-distance approximation and
keeps the actual pixels in GPU storage on ES 3.0 hardware.

## Threads and ownership

### Calling threads

Any FreeBASIC thread may call a graphics API. The compatibility front end:

1. Validates arguments and resolves the current TLS drawing state.
2. Copies transient data such as strings, patterns, vertices, or upload bytes.
3. Allocates a monotonically increasing command sequence while holding the
   queue mutex.
4. Publishes the command and signals the render thread.
5. Returns immediately unless the API requires a result or CPU-visible memory.

The queue mutex establishes a total order between calls made by different
threads. That order is the order in which producers acquire the mutex.

The normal graphics-mode queue has 8,192 entries. This is deliberately a
bounded back-pressure limit, not a per-frame allocation policy. It is large
enough for a sprite-heavy Basic frame to arrive as one ordered run so a backend
can form compatible GPU batches; a producer still waits safely once the bound
is reached rather than allowing unbounded command memory growth.

### Render thread

The render thread owns all Vulkan objects and the active OpenGL context. It is
the only thread allowed to submit GPU work or destroy live GPU resources. It:

- Drains commands in sequence order.
- Batches adjacent compatible commands without reordering them.
- Inserts image/buffer transitions and OpenGL memory barriers.
- Signals CPU fences after results and readbacks are visible.
- Defers resource destruction until the last referencing GPU fence completes.
- Presents completed visible pages according to the selected buffering mode.

Android can expose a `PTHREAD_STACK_MIN` of only 16 KiB. Older EGL and shader
compiler call chains exceed that size during context and surface creation.
gfxlib3 therefore requests a 4 MiB renderer stack on every platform, and the
Android packaging helper always selects the thread-safe runtime archive when
gfxlib3 is active. This is part of the renderer contract, not an application
option, because gfxlib3 creates threads even when the BASIC source does not.

### Platform event ownership

Each platform adapter creates its native window on the render thread. On
Windows, OpenGL asks for an owning WGL context while Vulkan receives the native
`HINSTANCE` and `HWND`. On X11, OpenGL asks for an owning GLX context while
Vulkan receives the `Display *` and `Window`. The X11 GLX framebuffer config,
visual, colormap, window, and context all belong to one Display connection;
mixing resources from separately opened connections is invalid even when they
name the same X server.

On Android, the package NativeActivity owns lifecycle dispatch and supplies an
`ANativeWindow` through callbacks. The adapter retains that window under a
mutex, while the render thread owns the EGL display, ES 3 context, window
surface, and buffer swaps. Touch coordinates are scaled from the native window
to the logical mode before entering the common input state. Native key events
and Java IME commits are translated into the same scan-code, SCREENEVENT, and
INKEY queues used by desktop adapters. Android Vulkan uses the same retained
`ANativeWindow`: gfxlib3 dynamically opens `libvulkan.so`, enables
`VK_KHR_surface` and `VK_KHR_android_surface`, and creates its own Android
surface and swapchain on the render thread. If the loader, extension, or a
present-capable device is unavailable, that probe is an ordinary unsupported
result and automatic selection continues to GLES. The connected API 24 device
exercises that fallback because it exposes no Vulkan feature; a Vulkan-capable
Android device is still required for surface and presentation validation.

The optional Vulkan 1.1 `vkEnumerateInstanceVersion` query is first resolved
as a loader export. Desktop retains the standard `vkGetInstanceProcAddr`
fallback for older loader layouts. Android treats a missing export as the
Vulkan 1.0 baseline instead: Android 1.0 stubs can emit a diagnostic for the
null-instance resolver form even though gfxlib3 needs no Vulkan 1.1 feature to
perform its capability probe.

The Android package also installs an unfocused one-pixel `FreeBasicInputView`.
It becomes the IME's served view only after the GPU-presented `KB` control is
tapped. The control occupies gfxlib2's native-pixel rectangle at the upper
right, is consumed before touch coordinates are scaled into BASIC space, and
is drawn by the GLES presentation shader after the logical page is converted.
This keeps it out of `POINT`, `GET`, and the page surfaces. A state change is
noticed by the renderer's bounded idle poll, which submits one presentation
frame on the owning EGL thread; Java never borrows the graphics context. The
input view forwards both normal `InputConnection` commits and direct EditText
changes made by older IMEs, clearing only its invisible helper text after a
native key submission so neither path changes application-visible text.
Both GPU presentation paths compose the KB control after converting the logical
surface. The GLES fragment compositor and the Vulkan compute compositor receive
the same native-pixel rectangle and state: dark idle, blue while the keyboard
is visible, and a short pressed state. The Vulkan command stores this data in
the presentation buffer after the palette so the final shader can draw its
border and glyph without a CPU framebuffer upload. A Vulkan-capable Android
device is still needed to validate that WSI path on real Android hardware.

The adapter pumps Win32 messages or X11 events before renderer batches and
translates them into a common input state protected by a dedicated mutex. It
never takes the FreeBASIC runtime lock or graphics lock. BASIC snapshot queries
hold the normal runtime locks and inspect only the input mutex. SCREENEVENT
matches gfxlib2's queue contract: it reads or peeks the already published
event ring and never sends a render command. Native event pumping proceeds
independently on the window-owning renderer thread. This lock order prevents
native event delivery from deadlocking with mode changes and keeps an empty
SCREENEVENT call independent of both the renderer queue and GPU timeline.
X11 pointer confinement is an actual pointer grab. Focus loss releases the
grab, while focus return restores it only when the BASIC-visible clip request
is still active.

The input state owns a 128-entry scan-code table, a 16-entry INKEY ring, a
128-entry SCREENEVENT ring, mouse coordinates and buttons, focus state, and a
small native-operation mailbox. It also publishes a synchronized window handle,
display handle, desktop dimensions, and outer-window position for SCREENCONTROL.
Full rings discard their oldest entry, matching gfxlib2. A null event pointer
peeks without consuming. SETMOUSE and SET_WINDOW_POS publish requests; the
render thread applies cursor position, visibility, client-area clipping, and
window movement because it owns the native window. The public mouse snapshot is
updated when SETMOUSE accepts the request, so an immediate GETMOUSE does not
wait for the native cursor operation or any GPU work.

On Win32, a `WM_MOUSELEAVE` notification is validated against the desktop
cursor before it changes the common mouse-inside snapshot. `TrackMouseEvent`
can leave an old notification queued while SETMOUSE has already moved the
physical cursor back into the client area. gfxlib2 refreshes its `mouse_on`
value from the desktop for every window message, so gfxlib3 follows that
observable GETMOUSE rule while still posting an exit for a genuine leave.

The renderer still blocks normally on its command queue. Windowed modes add a
small renderer-owned wake producer that considers a `PLATFORM_POLL` every 10
milliseconds, matching gfxlib2's normal Win32/X11 pump interval. An atomic
activity generation suppresses the poll when ordinary renderer work already
woke and pumped the platform during that interval. A required poll has an
ordinary sequence number, so shutdown and other work stay ordered, but a GPU
backend does not create a fence for it because polling submits no GPU work.
This keeps the native-window owner responsive while an application is idle
without duplicating polls during active rendering or turning the render loop
into a busy wait. Shutdown stops and joins the producer before closing the
command queue.

Android publishes up to sixteen native contacts with their platform pointer
IDs through the same synchronized input state used by BASIC touch queries.
Its motion callback replaces the complete snapshot, so lifted and cancelled
contacts cannot remain stale. Other platforms treat a focused left mouse button
as contact zero until they grow a native adapter. Count, coordinates,
rectangular hit, and circular hit all read one ordered snapshot. Android's
NativeActivity looper is already the event owner, and its renderer-side platform
pump is empty. Android SCREENEVENT therefore reads the snapshot directly rather
than sending an empty command to the GPU thread. Android additionally retains
up to sixteen first-seen controller slots. Its
NativeActivity motion and key callbacks publish normalized axis, trigger,
button, and d-pad snapshots under this same mutex. GETJOYSTICK and GETXPAD
read the matching slot directly; the Android mapping and
missing-device values match gfxlib2. On Win32, GETJOYSTICK follows gfxlib2's
separate WinMM path: it lazily loads `winmm.dll`, polls its sixteen indexed
legacy joystick slots directly, and normalizes the six optional axes and POV
hat. Successful capability records are cached. A missing slot is rechecked once
per second so hotplug still works without making every absent-device query call
`joyGetDevCaps`. GETXPAD independently follows the shared snapshot path for up
to four XInput devices. The platform lazily probes the usual XInput DLL names,
polls only on its render thread and no more than once per 8 ms, normalizes sticks
and triggers with gfxlib2's mapping, and retains a seen-but-disconnected slot so
GETXPAD can report the right status. Missing WinMM or XInput support is an
ordinary no-controller result, not a window-creation failure. QB STICK and
STRIG preserve their latch rules above GETJOYSTICK. Other desktop controller
APIs remain adapter work.

### OpenGL capability reporting

The OpenGL and GLES backends capture colour, depth, stencil, sample, and
extension metadata while their context is current on the render thread. The
result is an immutable backend snapshot read by SCREENCONTROL, so a BASIC
program can inspect GET_GL_* values without borrowing a live context. Desktop
core profiles enumerate extensions with `glGetStringi`; GLES retains its native
extension string. Some desktop core drivers report zero default-framebuffer
colour widths despite the RGBA8 window format explicitly requested by gfxlib3.
The snapshot reports the renderer's deliberate RGBA8 presentation contract in
that case. Accumulation buffers and the old OpenGL 2D bridge are not part of
gfxlib3, and therefore report zero and the documented default scale of one.

`SCREENCONTROL SET_GL_*` preserves the legacy request state before a mode is
opened, including gfxlib2's initial active values of 2D mode zero and scale
one. This lets programs which set and inspect those controls retain their
observable contract. A successful OpenGL or GLES mode then replaces the
reported capability fields with the render-thread snapshot. gfxlib3's compute
pipeline has no direct-GL 2D bridge and does not recreate a live context after
a setter, so SET_GL_2D_MODE and SET_GL_SCALE remain setup compatibility values;
the active GPU mode reports 2D mode zero and scale one. The pixel-format
request controls are likewise not a promise that a running GPU mode will be
recreated with a different driver format.

`SCREENCONTROL SET_X86_MMX_ENABLED` is also accepted for source compatibility.
gfxlib3 has no CPU blitter path, so the setting cannot change renderer work and
`GET_X86_MMX_ENABLED` remains false. Keeping that result explicit prevents a
legacy acceleration switch from implying that the GPU renderer has selected a
different pixel implementation.

SCREENGLPROC remains a safe NULL result on the BASIC application thread. A raw
function pointer would invite code to issue GL calls from a thread that does
not own the context, which would break the ordering guarantee of the GPU
command queue. `FB.Gfx3RunOnRenderThread` is the explicit interop boundary:
it queues a synchronous callback after all earlier work has completed, sets a
render-thread-local resolver for the callback's lifetime, and waits for the
callback's GL work before accepting later gfxlib3 work. Within that callback,
SCREENGLPROC resolves OpenGL or GLES functions from the active context; outside
it, and on Vulkan or Null, it returns NULL. Callbacks must not re-enter normal
graphics APIs because those APIs synchronously wait for the same render thread.
They own any raw GL state they mutate and must restore what later work needs.

### Shutdown

Shutdown is an ordered command, not an unsynchronized flag. The caller stops
new submissions, queues shutdown, wakes the render thread, waits for it, then
destroys queue synchronization objects. A partial initialization records every
resource obtained so the same cleanup path is safe after any failure point.
The strict infrastructure executable verifies this with the real OpenGL backend:
a test platform allocates a context, refuses its first required function load,
and confirms the backend shutdown callback destroys that owned context before
the renderer releases its queue and resource registry.

## Command model

Every command currently begins with this private fixed header:

```c
typedef struct FB_GFX3_COMMAND {
    uint32_t type;
    uint32_t size;
    uint32_t flags;
    uint32_t reserved;
    uint64_t sequence;
    FB_GFX3_HANDLE target;
    FB_GFX3_COMPLETION *completion;
    unsigned char payload[0];
} FB_GFX3_COMMAND;
```

The concrete layout may evolve before the ABI is public. The required
properties are stable:

- `size` is checked before reading a payload.
- Sequence numbers never use pointer values as ordering tokens.
- Resources are referenced by validated generation-tagged handles.
- Commands never retain pointers to temporary FreeBASIC strings or stack data.
  A synchronous download may carry its caller-owned destination address because
  the caller blocks and keeps that range alive until completion.
- A command that returns data carries a completion object owned until both
  sides have released it.
- Integer dimensions and byte counts are overflow-checked before allocation or
  copy.

The completion pointer is valid because the queue is private to one process.
It is never copied into a GPU buffer or exposed as a persistent ABI. Completion
objects carry a status, the completed sequence, and four 64-bit result slots so
surface handles and small readback results need no temporary heap allocation.

The first command families are lifecycle, surface create/destroy, state,
clear, point/line/rectangle/ellipse, image transfer, text glyphs, palette,
barrier/readback, page/present, and shutdown.

The initial checked protocol implements surface create/destroy, clipped clear,
variable-length point batches, styled line, BOX/BF, full and filled midpoint
ellipses, built-in PUT modes, and single-pixel readback. Payloads contain only
fixed-width copied values. The null backend consumes the same payloads intended
for Vulkan and OpenGL, which makes it a reference renderer rather than a
separate test-only command language.

Compatibility code must use the typed context layer instead of allocating
backend command payloads itself. This keeps overflow checks, copied transfer
ownership, cross-context surface rejection, and synchronization policy in one
place.

## Surface model

### CPU images

An existing `IMAGECREATE` call returns the current or QB `FB.IMAGE` layout
followed by 16-byte aligned CPU pixels. This is required because programs may
call `IMAGEINFO`, use the returned data pointer, cast the image header, or write
pixels directly. Header parsing, pitch arithmetic, allocation size, and all
row conversions are checked before accessing the allocation.

The current implementation uploads an ordinary CPU image into a temporary GPU
surface for screen PUT and performs the built-in blend on the GPU. GET performs
a synchronized region download. A custom BASIC blender is an explicit CPU
barrier: only the clipped destination rectangle is downloaded, the callback is
run on the calling thread, and the rectangle is uploaded again.

Direct writes to ordinary images cannot be detected reliably, so CPU storage
is authoritative whenever such an image is passed to a later operation. A
future persistent GPU mirror must use an explicit authority transition instead
of guessing whether user code changed the returned pointer.

### GPU surfaces

New GPU surfaces are opaque handles returned by a gfxlib3-specific API. They
can be used anywhere the compiler accepts an image expression because gfxlib3
entry points recognize the descriptor, but they do not expose writable pixel
memory as if it were an FB.IMAGE.

A GPU surface records:

- Width, height, logical depth, and physical GPU format.
- Usage flags such as render target, sampled, transfer source, and transfer
  destination.
- Palette association for indexed modes.
- The last submitted writer and reader sequences.
- Optional staging memory and its authority state.
- A generation number and reference count.

The flags are not advisory allocation hints.  `RENDER_TARGET` is required for
clear and drawing work, `SAMPLED` for a GPU blit source and presentation,
`TRANSFER_SOURCE` for download and read-pixel work, and
`TRANSFER_DESTINATION` for upload.  The checked extension API reports an
unsupported operation when the needed capability is absent.  Existing
write-only BASIC drawing statements retain their void ABI; they simply cannot
write a target that lacks `RENDER_TARGET`. CPU-image or GLES-screen PAINT
fallbacks and PUT CUSTOM also require the transfer roles needed to download,
modify, and re-upload pixels on the CPU. Desktop compute PAINT does not cross
that compatibility boundary.

### Screen pages and swapchain images

Logical `SCREEN` pages are GPU render targets. The visible page is copied or
composited into a swapchain image at presentation. They are not aliases of
swapchain images because swapchain ownership and image count can change after a
resize or device loss.

Double and triple buffering are presentation policies. They do not change the
meaning of BASIC work and visible page numbers.

### Indexed color

1, 2, 4, and 8-bit logical surfaces use an integer GPU image where supported,
or an 8-bit integer image containing palette indices. Palette lookup occurs in
the presentation and sampling shaders. This preserves inexpensive palette
animation without rewriting every pixel.

## Synchronization and compatibility barriers

### Queue-only operations

These normally queue and return without a GPU wait:

- PSET/PRESET, LINE/box, CIRCLE/ellipse, and DRAW-generated primitives.
- DRAW STRING using the built-in glyph atlas.
- Built-in PUT modes between GPU-resident surfaces.
- Palette changes, clears, page copies, and ordinary presentation requests.

### Result barriers

These must wait for relevant earlier commands:

- `POINT` reads one pixel.
- `GET` downloads a region into a CPU image or array.
- `BSAVE` requires a coherent CPU image.
- `SCREENPTR` requires a coherent CPU shadow page.
- `IMAGEINFO` on a GPU surface cannot return fake writable pixel memory.
- CPU custom PUT blenders must see source and destination pixels.
- Mode destruction waits until no queued command can reference old resources.

Waiting is scoped to the target resource and required sequence when the backend
can express that dependency. A global device idle is a last resort.

### SCREENLOCK and SCREENUNLOCK

`SCREENLOCK` pauses presentation and establishes exclusive compatibility
access. If the program requests `SCREENPTR`, gfxlib3 materializes and maps a
shadow page. Every `SCREENUNLOCK` uploads its stated dirty scanline range, or
the whole mapped page when the range cannot be trusted, then allows
presentation. That per-unlock rule matters for nested locks: an inner upload
must not make a pointer write made before the outer unlock disappear. Nested
locks preserve gfxlib2 reference-count behavior. A raw pointer remains a
screen-lock compatibility interface; programs must write it under
`SCREENLOCK`/`SCREENUNLOCK` because a GPU renderer cannot observe arbitrary
later CPU stores through an escaped pointer.

The synchronized shadow also avoids a pathological readback loop for legacy
software rasterizers that repeatedly perform
`PSET (x, y), Blend(POINT(x, y), colour)`. This pattern can occur inside or
outside an explicit `SCREENLOCK`. A small point cache recognizes an exact
POINT/PSET pair at the same coordinate. Two consecutive pairs promote the
active page to a coherent CPU shadow instead of alternating a GPU readback and
GPU write for every pixel.

The promotion waits once for earlier GPU work and downloads the page once.
Following POINT and PSET calls use the shadow. gfxlib3 uploads the accumulated
changes once at the next GPU ordering boundary, result barrier, visible-page
submission, or final unlock. A following GPU primitive therefore sees every
CPU pixel write in BASIC program order. A repeated read of the same unmodified
pixel still uses the one-pixel cache and does not promote the whole page.

The adaptive path supports native 32-bit pages and RGB565 pages. FreeBASIC's
legacy depth 15 modes are normalized to RGB565, so their packed shadow reads
expand through the same five-six-five channel rules as ordinary POINT. Raw
pointer access remains limited to the established 32-bit SCREENPTR ABI. This
is an intentionally bounded compatibility path: ordinary PSET, LINE, CIRCLE,
PUT, text, page copy, and other primitives remain GPU commands unless the
program itself creates a CPU read-modify-write dependency with POINT.

## Rendering primitives

### PAINT execution boundary

PAINT has separate implementations at the memory-ownership boundary. CPU
`FB.IMAGE` targets use the bounded non-recursive flood discovery required by
their directly writable memory layout. Desktop Vulkan and OpenGL 4.3 screen
pages and opaque GPU surfaces submit a bounded solid or patterned fill command
directly to compute. Their device-local scratch storage contains a visited map
and compact run queue, so the target never crosses CPU memory. The conservative
one-million-pixel limit prevents a pathological serial topology search from
tripping a GPU watchdog.

The desktop shader first tests the common border-enclosed rectangle case. One
workgroup finds seed-aligned candidate bounds. A second dispatch verifies every
interior pixel and all four adjacent perimeters across 16 by 16 OpenGL or 16 by
8 Vulkan workgroups before any target write. A third dispatch fills independent
pixels only when the shared validity flag remains set. The final one-workgroup
dispatch runs the exact scanline algorithm only after rejection. An interior
border pixel or perimeter opening therefore cannot race a target write. This
is an exact specialization, not a geometric guess.

The renderer can also discard intermediate commands in one adjacent run of
solid, opaque, non-border PAINT calls with identical target, seed, clip, and
border. Recoloring such a region does not alter its `pixel != border`
reachability, and no public command can observe colours between adjacent
renderer commands. Pattern, alpha, border-coloured, reordered, or otherwise
incompatible commands remain individual operations.

GLES 3.0 uses a GPU-only fragment ping-pong implementation for renderer-owned
GPU surfaces: two integer mask textures expand the frontier once per pixel
distance, then a third texture composites the final result back to the surface.
Patterned commands upload only their copied 8 by 8 bytes to a tiny integer
texture; target pixels never cross CPU memory. The render thread groups 32
expansions under one `GL_ANY_SAMPLES_PASSED` query, then stops when that batch
added no pixels. This retains exact four-neighbour connectivity without forcing
a CPU wait after every expansion. It is deliberately conservative because ES
3.0 lacks compute/storage-image support: a pathological single-pixel maze can
still require one GPU iteration per path edge.

CPU images, the Null backend, and normal transferable GLES screen pages retain
the compatibility algorithm after one scoped download of the active VIEW
region. Consecutive compatible screen fills retain an authoritative shadow and
defer one tightly bounded upload until the next GPU observation. This policy
avoids thousands of serial ES 3.0 raster passes for an ordinary full-screen
region. GPU-only GLES surfaces cannot use that shortcut and retain the exact
shader frontier. Both routes preserve gfxlib2 border, alpha, and absolute 8 by
8 pattern semantics.

The shared PAINT command carries the resolved target point, clipped VIEW
rectangle, fixed border/fill values, alpha flag, copied 8 by 8 pattern bytes,
and the CPU staging origin. Desktop Vulkan, desktop OpenGL, and GLES all use
their already-absolute target coordinates to index the copied 8-, 16-, and
32-bit pattern layouts. They deliberately do not add the staging origin a
second time. The copied protocol bytes prevent an asynchronous renderer command
from observing a temporary FreeBASIC string after the API call returns.

Like every renderer command, `PAINT` was appended to the protocol enumeration.
Keeping existing numeric command values stable matters when an incremental
archive rebuild replaces only the changed common/front-end object while an
unchanged platform backend remains in the archive.

### Vulkan path

Vulkan uses compute shaders for exact pixel-oriented operations and graphics
pipelines where rasterization is naturally exact and faster.

The Vulkan path loads `vulkan-1.dll` on Windows or `libvulkan.so.1` on Linux and
carries private ABI declarations so building gfxlib3 does not require the
Vulkan SDK. It resolves commands through `vkGetInstanceProcAddr` and
`vkGetDeviceProcAddr`, queries the loader version when supported, creates a
Vulkan 1.0 instance, enumerates physical devices, prefers a queue family that
supports both compute and graphics, creates one logical device, and acquires
one queue. Every acquired object is released in reverse order on failure and
normal shutdown.

The render thread owns a fixed ring of six submission-resource slots. Each
slot owns a resettable command pool, a primary command buffer, a fence, a
descriptor set, and, when a swapchain is active, its own image-acquire
semaphore. Render-finished semaphores are indexed by swapchain image because a
submit fence does not prove that `vkQueuePresentKHR` has finished consuming
its wait semaphore.

Up to three adjacent runtime operations are recorded into one owner slot's
command buffer and sent in one `vkQueueSubmit`. The other participating slots
keep the descriptors, staging allocations, and command sequences belonging to
their individual operations, but share the owner's submission fence. Six
slots allow two independent three-operation batches to overlap. Reusing any
participant waits for the shared fence and releases the complete batch group.
The single Vulkan queue preserves BASIC command order.

### Multi-frame Vulkan submission milestone

The first multi-frame implementation replaces the submission owner as one
complete unit. Replacing only the fence or only the command buffer would be
incorrect: a recorded dispatch refers to the descriptor set contents and to
host-visible command, upload, or snapshot buffers that were current when the
dispatch was recorded.

Each frame slot owns all of the following objects:

- one resettable command pool and one primary command buffer;
- one fence and one maximum submitted command sequence;
- one image-acquire semaphore when a swapchain is active;
- one descriptor set, selected before each compute dispatch recorded in that
  slot;
- a list of staging, point, line, blit, and snapshot allocations that cannot
  be freed until its fence signals.

The render thread uses six resource slots as two possible groups of at most
three operations. The first slot in a group owns its command buffer and fence.
The remaining slots retain operation-local descriptors, allocations, and
sequence tags, and point at the owner's fence after submission. Reusing a slot
first waits for that shared fence, releases the group's deferred allocations,
and resets the next owner command pool. Submission order on the one Vulkan
queue remains the BASIC command order. A presentation flushes the currently
recorded group, calls `vkQueuePresentKHR`, and leaves the group in flight. Its
render-finished semaphore belongs to the acquired swapchain image; reacquiring
that same image proves the prior presentation wait has completed before the
semaphore is signalled again. This removes both the old presentation-side CPU
wait and its empty FIFO marker while remaining valid on Vulkan 1.0.

Descriptor writes use the slot-local descriptor set before a dispatch is
recorded. A set already referenced by an in-flight command buffer is never
updated. Temporary staging, point, line, blit, and snapshot allocations are
attached to their owning slot and are not released until its fence signals.
Swapchain resize, mode shutdown, device loss, and backend fallback wait for
all occupied slots, then release slot acquire semaphores, image-owned
render-finished semaphores, deferred allocations, descriptor sets, command
pools, and swapchain objects in dependency order.

Every successful renderer command tags the slot submission it produced. A
synchronous renderer completion, including SURFACE DESTROY and readback,
waits only for tagged slots at or before its command sequence. Later slots
remain retained, so resource collection advances only through the proven
sequence and cannot free a later user early. Idle, resize, mode shutdown,
device loss, and backend fallback still intentionally drain every slot.

Before processing each backend command, the render thread also checks the
occupied slot fences without blocking. A signalled fence immediately releases
only that slot's temporary staging, point, line, blit, and snapshot allocations.
This reduces transient GPU memory retention when the next BASIC command uses a
different slot. Polling deliberately does not advance the renderer completed
sequence: resource retirement remains tied to an ordered synchronous wait, so
a later user cannot be collected merely because the GPU happened to finish
early. Device-loss fault injection remains planned work.

Logical surfaces are device-local storage buffers containing one 32-bit word
per pixel for every source depth. Depth masking preserves 1, 2, 4, 8, and
16-bit semantics while avoiding a pipeline matrix during the compatibility
phase. Upload and download use temporary host-visible coherent buffers and
explicit host, transfer, and compute barriers. The device-local allocation is
never exposed as writable CPU memory.

The header-independent implementation currently publishes a conservative
4096 by 4096 maximum surface dimension. It does not guess beyond that bound
until the complete physical-device limit structure, especially
`maxStorageBufferRange`, is queried and stored. This avoids accepting a surface
that can be allocated but cannot legally be bound as a storage descriptor.

PSET batches, styled lines, and outlined boxes use embedded Vulkan 1.0 SPIR-V
compute modules. Filled boxes reuse the clipped GPU fill path. Line and box
style selection uses the original unclipped step or perimeter index, so
clipping does not restart the 16-bit style. Focused tests send these commands
through the common render thread and compare a complete downloaded surface
with the null reference backend.

Full ellipses use one serial compute invocation for the midpoint state machine
while keeping every scanline write in device memory. The Vulkan runtime queries
`shaderFloat64` before device creation, prefers a compute device that supports
it, and enables only that reported feature. Float64 holds the integer-valued
intermediates exactly across the accepted radius range. A device without
Float64 still provides every other Vulkan operation and reports the ellipse
command as unsupported. Arc filtering remains a compatibility-level point
batch: the CPU currently generates the filtered arc points, while Vulkan owns
the pixel writes and optional radial lines. Moving arc coverage generation into
a shader remains a performance optimization and is tracked separately from API
parity.

Built-in PUT modes use a storage-buffer compute pipeline with destination,
source, and command descriptors. TRANS, PSET, PRESET, AND, OR, XOR, ALPHA, ADD,
and BLEND share the exact depth-specific integer formulas used by the OpenGL
and null backends. When source and destination are the same surface, Vulkan
copies the source storage into a temporary device-local buffer before dispatch.
This keeps overlap deterministic without crossing into CPU memory.

Windowed Vulkan enables `VK_KHR_surface` plus the platform's
`VK_KHR_win32_surface` or `VK_KHR_xlib_surface` extension at instance creation,
and `VK_KHR_swapchain` at device creation. The native surface is created before
device selection, so the chosen compute queue must also pass
`vkGetPhysicalDeviceSurfaceSupportKHR`. This prevents creating a device that
can draw logical pages but cannot present them to the actual window.

The swapchain path has these ownership rules:

1. Query surface capabilities, formats, and present modes for the selected
   physical device and native surface.
2. Require `VK_IMAGE_USAGE_TRANSFER_DST_BIT` and
   `VK_FORMAT_B8G8R8A8_UNORM`. No render pass or swapchain image view is needed
   because presentation uses a checked buffer-to-image copy.
3. Request one image more than the surface minimum, capped by its maximum.
   This normally produces double buffering when the minimum is one and triple
   buffering when the minimum is two. Mailbox mode is preferred; FIFO is the
   required fallback.
4. Allocate a device-local BGRA8 presentation buffer sized to the current
   swapchain extent and a persistently mapped coherent command/palette buffer.
5. Keep logical pages separate from swapchain images. Page count and
   `SCREENSET` semantics therefore remain stable across swapchain recreation.

Presentation is entirely GPU-side after the palette command has been copied:

1. Acquire one swapchain image with the image-available semaphore.
2. Dispatch the embedded presentation compute shader. It performs nearest
   scaling, masks 1/2/4/8-bit indices and looks them up in the 256-entry
   palette, expands RGB565, or masks a 32-bit FreeBASIC color. The result is an
   opaque BGRA8 word in device-local storage.
3. Barrier the presentation buffer from shader write to transfer read.
4. Transition a new image from undefined, or a reused image from present, to
   transfer destination and copy the buffer into it.
5. Transition the image to present source, signal the rendering-finished
   semaphore, and call `vkQueuePresentKHR` with that semaphore as its wait.

`VK_ERROR_OUT_OF_DATE_KHR` recreates the swapchain before retrying.
`VK_SUBOPTIMAL_KHR` completes the acquired present and then recreates it. A
zero-sized minimized surface defers presentation until a later platform poll
sees a usable client extent. Image initialization state is tracked per
swapchain image so the first and later layout transitions use legal old
layouts.

Some Xlib WSI implementations continue accepting an old swapchain after the
X11 client has changed size instead of immediately returning out-of-date. The
platform poll therefore compares the current X11 client extent with the live
swapchain extent and explicitly requests recreation when they differ. This is
required for deterministic resize behavior and was found by repeated exact
displayed-pixel tests.

Win32 WSI functions may send window messages and can block when called from a
thread that does not dispatch that window. gfxlib3 avoids that platform trap by
creating the HWND, Vulkan surface, swapchain, and every acquire/present call on
the same render thread, which also runs the message pump. See the Vulkan
reference pages for
[`vkCreateWin32SurfaceKHR`](https://docs.vulkan.org/refpages/latest/refpages/source/vkCreateWin32SurfaceKHR.html),
[`VkSurfaceCapabilitiesKHR`](https://docs.vulkan.org/refpages/latest/refpages/source/VkSurfaceCapabilitiesKHR.html),
[`VkSwapchainCreateInfoKHR`](https://docs.vulkan.org/refpages/latest/refpages/source/VkSwapchainCreateInfoKHR.html),
and [`VkPresentInfoKHR`](https://docs.vulkan.org/refpages/latest/refpages/source/VkPresentInfoKHR.html).

X11 follows the same ownership rule for a different reason: Xlib connection
state and the event queue belong to the Display connection used to create the
window. The render thread therefore owns that Display, the Xlib surface,
swapchain calls, and event pumping. BASIC-side X11 tests inject events through
a separate Display connection instead of calling Xlib concurrently on the
renderer-owned connection.

- PSET and sparse points are batched into a storage buffer and dispatched.
- LINE and styled LINE use integer endpoint math in a compute shader and match
  the reference backend in current focused fixtures.
- Outlined rectangles use a perimeter compute shader. Filled rectangles use
  clipped device-local fills.
- Full and filled ellipses use gfxlib2's midpoint state machine. OpenGL and
  Vulkan run its serial state in one compute invocation and keep all pixel
  writes on the GPU. Integer-valued intermediates are held exactly in GLSL
  doubles. A later parallel span implementation may replace it only if pixel
  parity is retained. Arcs use GPU point batches plus optional radial lines.
- PUT modes use storage-buffer compute shaders. TRANS, PSET, PRESET, AND, OR,
  XOR, ALPHA, ADD, and BLEND share one mode-selecting compute pipeline.
- Built-in glyphs are decoded from FreeBASIC's canonical 8 by 8, 8 by 14, and
  8 by 16 font data and submitted as one clipped point batch. A later atlas
  path can replace the batch only after exact glyph tests remain unchanged.
- Palette lookup happens while sampling or presenting indexed surfaces.

Win32 and X11 Vulkan native windows, palette sampling, swapchains, visible
presentation, resize recreation, and idle event polling are implemented. A
dedicated glyph atlas and multiple submissions in flight are performance work
rather than API blockers, because current text and presentation already remain
on the GPU. `FB.GFX_VULKAN` selects Vulkan explicitly, while an ordinary
gfxlib3 windowed mode selects OpenGL.

### OpenGL path

The baseline OpenGL backend targets core functionality available on reasonably
modern desktop drivers. It uses instanced point/quad draws and fragment shaders.
OpenGL 4.3 compute is used when present. Earlier supported contexts use bounded
fragment-shader passes and staging transfers for operations that require image
load/store.

The capability table records actual features. Unsupported fast paths fall back
per operation without changing results.

The platform adapters create an OpenGL 4.3 core context on the render thread
through WGL on Windows or GLX on X11 and dynamically load every required entry
point. The X11 adapter loads GLX itself from `libGL.so.1`, so compiling gfxlib3
does not require GLX development headers or a direct libGL link. Each adapter
owns its native window, event pumping, client sizing, title changes, and buffer
swaps. The renderer stores exact logical pixel values in `R32UI` textures,
dispatches clear, point, styled-line, BOX/BF, ellipse, and built-in PUT compute
shaders, and uses `GLsync` fences for asynchronous lifetime tracking.
Single-pixel readback attaches the integer texture to a private read framebuffer
and waits only because POINT requires a result.

A renderer batch creates a GL fence only when it submitted GPU work or changed
a page that must be presented. CPU-only platform polls, input polls, barriers
with no older GPU work, and title changes advance a separate ordered control
sequence. That sequence becomes complete only after all older GL fences have
retired. This preserves resource lifetime and completion order without asking
the OpenGL driver to allocate and query a fence for every idle window pump.

The outline box shader treats the perimeter as one indexed sequence in the
same bottom, top, right, left order used by gfxlib2. The 16-bit line style
therefore continues across edges and clipped portions instead of restarting at
each corner. Filled boxes reuse the clipped GPU clear dispatch.

When PUT reads and writes the same GPU surface, the backend copies the source
rectangle into a temporary GPU texture before dispatch. Compute invocations
may execute in any order, so reading and writing the original image directly
would be a race. The temporary never crosses into CPU memory and preserves the
same snapshot behavior as the reference backend.

OpenGL ES 3.0 uses an ordered fragment shader for destination-reading PUT
modes because the baseline mobile profile has no compute shader or image-load
store requirement. It snapshots the destination rectangle, then draws the
result back to the render target. When source and destination are separate
GPU surfaces, GLES samples the source texture directly with an explicit source
origin; copying it would add a needless GPU allocation and transfer for every
sprite. Self-blits still snapshot the source first, since framebuffer texture
feedback is undefined. Both forms keep the pixel math in shader units rather
than in the BASIC caller or render-thread CPU code.

Presentation uses a fullscreen triangle and a format-aware fragment shader.
Indexed modes perform palette lookup without rewriting the logical surface,
RGB565 is expanded in the shader, and 32-bit pixels retain their stored color.
The visible logical page remains a GPU surface independent of the native back
buffer. Automatic dirty presentation and explicit synchronized PRESENT both
use the same ordered command stream.

Using `R32UI` for every logical depth is an intentional bootstrap choice. It
keeps the first shader set exact and comparable. Typed `R8UI` and `R16UI`
variants will reduce memory use after the command behavior is locked down.

Upload commands own a copy of every input row before entering the queue. They
accept an explicit pitch, validate the complete byte count, and upload each row
with integer format conversion into the R32UI texture. Download commands are
synchronous, validate their caller-owned destination range and pitch, and read
rows back through the integer framebuffer. This is the bridge used by CPU
FB.IMAGE mirrors; GPU-only surface operations do not use it.

### PAINT

Flood fill depends on existing neighboring pixels and is the least natural
primitive for a queued raster API. Desktop OpenGL and Vulkan use the compute
path described below for normal screen pages and renderer-owned surfaces. CPU
image targets use the checked non-recursive compatibility core directly, and
normal GLES screen pages use its retained shadow and deferred upload route.
Solid and 8 by 8 pattern fills are supported. Every route anchors pattern tiles
to absolute target coordinates rather than a temporary staging origin.
Correctness and allocation limits take priority over a nominal all-GPU claim.

### DRAW

The QB DRAW string is parsed on the calling thread into ordinary primitive
commands. Movement, color, scale, right-angle and arbitrary rotation, `P`,
`M`, `B`, `N`, and recursively bounded `X` commands are implemented. Scale and
angle state are caller-local instead of process-global. The resulting screen
pixels are still generated on the GPU.

### Text

The built-in font shares gfxlib2's generated immutable asset but not its code
or framebuffer state. A checked one-time decoder publishes that asset across
threads. Screen strings collect all lit glyph pixels into one command, so text
does not become one queue submission per pixel. Custom fonts are validated as
current or QB images, assembled into one checked temporary image, and sent
through PUT so built-in modes and custom BASIC blenders retain their semantics.
The temporary image begins with gfxlib2's depth-specific TRANS key: zero for
8-bit, RGB565 magenta for 16-bit, and RGB magenta for 32-bit. This matters for
unsupported characters. They occupy their documented font-height advance but
have no source glyph; a zero-filled temporary 32-bit image would incorrectly
overwrite the destination instead of leaving it untouched by TRANS.
Custom-font pixels keep the target's native layout: palette indexes at 8-bit,
packed RGB565 at 16-bit, and RGB at 32-bit. This is observable through POINT,
which expands a 16-bit target value back to RGB888, so conformance checks must
compare that documented expansion rather than the packed source word.
For PSET, PRESET, logical modes, ALPHA, ADD, BLEND, and CUSTOM, unsupported
glyphs are not synthesized at all. The renderer instead submits contiguous
supported runs at their already-translated pixel positions, preserving both
the historical gap and each PUT mode's skip semantics.

CUSTOM follows the ordinary BASIC PUT callback contract even for a GPU target:
the renderer establishes a bounded CPU barrier, invokes the callback for every
pixel in each supported glyph rectangle, and reuploads the result in command
order. A callback result is a public colour, not a pre-packed RGB565 word; the
target conversion occurs after the callback before POINT exposes the expanded
16-bit result. This keeps custom fonts consistent with ordinary PUT CUSTOM
without pretending that an arbitrary BASIC function can run inside a shader.

### Graphical console

The graphical console owns a character/color cell array for every logical page.
Each mode selects a canonical 8 by 8, 8 by 14, or 8 by 16 grid. WIDTH derives
the requested cell height from the physical mode, accepts only those three
FreeBASIC font heights, resets the graphical VIEW and text view, and clears the
current GPU page as gfxlib2 does. Runtime hooks redirect ordinary COLOR, CLS,
WIDTH, LOCATE, POS, CSRLIN, PRINT, and SCREEN(row, column) operations into this
state. SCREENSET selects the visible GPU page in FIFO order. SCREENCOPY
copies both the requested GPU pixel rectangle and its matching character-cell
page. PAGE_SET and a write to the visible surface both mark presentation dirty,
so the backend presents the final page at the end of its ordered drain without
needing a second PRESENT packet. These calls remain asynchronous; SCREENSYNC is
the public operation that waits for completed GPU and presentation work. Text
backgrounds use filled GPU rectangles, lit glyph pixels use one
point batch per write, and scrolling uses an overlap-safe GPU self-blit followed
by a GPU clear. No screen-sized CPU framebuffer is introduced.

The console hook path enters the runtime lock before the graphics lock. Mode
creation and destruction use the same order while replacing the hook table, so
printing from one thread cannot deadlock against another thread changing modes.
All three canonical fonts, page cell separation, foreground/background reads,
and scrolling pass single-threaded and multithreaded null/OpenGL tests.

Narrow and wide LINE INPUT reuse `fb_ConReadLine(TRUE)`, the established rtlib
line editor. Insertion, deletion, arrows, Home/End, tab expansion, line
wrapping, and scrolling therefore stay consistent with gfxlib2. The editor
calls the gfxlib3 console and input hooks instead of touching native state. The
separate `fb_GfxReadStr` hook retains the historical byte reader and graphical
echo used when rtlib reads standard input through the hook table.

The soft cursor currently uses the canonical glyph for character 255 rather
than gfxlib2's exact cursor-cell visualization. Exact cursor appearance and 8
by 14 or 8 by 16 font selection remain parity work. Native Win32
INKEY/GETKEY/KEYHIT hooks are owned by the input layer rather than the console
renderer.

Graphics-aware SLEEP uses the same synchronized keyboard query path. Long or
infinite sleeps poll in bounded intervals and return when a key is pending
without consuming that key. `SLEEP 0` pumps native events and submits a present.
This preserves the old runtime contract without letting a caller thread touch
the render thread's native window.

Indexed modes also own lifecycle-scoped VGA DAC address, component, and partial
color registers. The INP/OUT hooks recognize only ports 0x3C7, 0x3C8, 0x3C9,
and 0x3DA. Six-bit DAC components are converted to the mode palette and queued
to the backend; the status port performs synchronized presentation. Unknown
ports return the runtime fallback result, so this compatibility layer never
executes arbitrary hardware I/O itself.

### Graphics files

BSAVE and BLOAD use the runtime path abstraction. Raw FreeBASIC and QB blocks
are supported. BMP and PNG save handle indexed, RGB565, and 32-bit inputs. PNG
files use gfxlib3's independent copy of the dependency-free codec, including
checked chunks, DEFLATE streams, scanline filters, alpha, and Adam7 input.
Indexed mode palette entries are converted from gfxlib's in-memory RGB layout
to BMP's on-disk BGR records. BMP load currently accepts uncompressed 1, 4, 8,
16, 24, and 32-bit Windows information headers. Screen raw blocks synchronize
through an explicit GPU download before the file write and upload only after
the file read completes. Path conversion
copies the bounded FreeBASIC string directly instead of importing a host CRT
formatter, keeping the runtime archive compatible with the compiler toolchain.
Contiguous 16/32-bit Windows `BI_BITFIELDS` RGB masks are decoded with checked
non-overlap and normalized to 8-bit channels. A 32-bit fourth mask located
before the pixel array is treated as alpha, preserving its value through the
same checked normalization path. The same parser accepts the Windows V3, V4,
and V5 header extensions used by gfxlib2, with their masks in the extended
header. OS/2 V1 `BITMAPCOREHEADER` files load their 1/4/8-bit three-byte BGR
palette and 24-bit RGB pixels. Windows RLE4 and RLE8 are boundedly decoded to
palette indexes on the CPU, then follow the same palette conversion and GPU
upload path. Later OS/2 headers and other compressed BMP codecs remain
unsupported extensions; gfxlib2 does not accept them either.

### Standard mode queries

At mode creation, legacy 15-bit requests normalize to the renderer's 16-bit
RGB565 storage and 24-bit requests normalize to 32-bit BGRA storage. This is
the same public result gfxlib2 publishes through SCREENINFO, and avoids a
separate 15/24-bit surface family whose GPU representation would disagree with
locks, readback, and BLOAD conversion.

`SCREENLIST` preserves gfxlib2's restart-and-resume iterator ABI. A positive
depth begins a new list and zero resumes it. Win32 enumerates the active
desktop's supported display modes, while X11 opens a temporary Display and
dynamically loads the stable RandR size-query ABI. Both paths apply gfxlib2's
15/16 and 24/32 depth equivalence, then sort and de-duplicate packed
width/height values. RandR must remain loaded until after `XCloseDisplay()`:
the Xlib close path invokes extension cleanup callbacks registered by RandR.
This keeps the public result useful to existing display-mode pickers even
though gfxlib3 itself is windowed. If native enumeration is unavailable, or
the requested historical depth is absent from the desktop, gfxlib3 falls back
to its finite standard `SCREEN` table. The fallback requires no window or GPU
and is used by Android, Wayland, and headless X11 operation.

### Custom PUT blenders

An arbitrary FreeBASIC callback is executable CPU code and cannot be translated
into SPIR-V or GLSL safely. Existing CUSTOM PUT therefore uses a synchronized
CPU fallback. A later gfxlib3 shader API may offer precompiled or source shader
blenders as a separate opt-in feature.

## Batching and ordering

The render thread drains up to 64 adjacent asynchronous commands from its FIFO
queue and passes that exact ordered array to the selected backend. This lets a
backend combine command-buffer recording, GPU submission, or native-window
work without changing observable BASIC command order. A backend must process
the entries in sequence; batching is not permission to reorder or merge
pixel-affecting work.

Commands with result data remain hard batch boundaries. This covers readbacks,
CPU mappings, resource creation, and any other request whose caller needs a
value before it can continue. SURFACE DESTROY is the intentional exception:
it has a completion so its caller knows the handle is retired, but no result
payload. It may share a batch with later asynchronous work, and its targeted
sequence wait retires only its own final GPU use. Shutdown and render-thread
interop callbacks are isolated commands. The limit prevents a producer that
is continuously drawing from delaying a waiting readback, input barrier, or
shutdown indefinitely.

Command batching preserves ordered submission. Vulkan owns six
submission-resource slots and records at most three runtime operations into
one command-buffer submission. Two such groups can remain in flight. A
result-bearing command, presentation, targeted wait, explicit idle, resize, or
shutdown flushes a partly recorded group before observing GPU completion.

`FBGFX3_VULKAN_DISABLE_BATCH` and `FBGFX3_VULKAN_BATCH_SIZE` are diagnostic
controls for profiling the submission policy. They are not application API.
The runtime counts actual `vkQueueSubmit` calls separately from completed
runtime operations and reports both through the centralized INFO logger during
backend shutdown. This makes offload changes measurable without confusing
queued operations with driver submissions.

## Backends and capability selection

The renderer backend interface contains lifecycle, surface, transfer, draw,
barrier, fence, and presentation operations plus a capability record. Backend
selection is:

1. `GFX_NULL` selects the deterministic reference backend.
2. `FB.GFX_VULKAN` force-selects Vulkan and disables fallback.
3. `FB.GFX_OPENGL` force-selects desktop OpenGL or Android OpenGL ES, matching
   the effective gfxlib2 flag behavior.
4. A name from `SCREENCONTROL SET_DRIVER_NAME`, or `FBGFX` when no stored name
   exists, gets one dedicated attempt if it names a compiled GPU backend. The
   complete platform order remains a separate fallback pass, including that
   same named backend, exactly as in gfxlib2.
5. An ordinary desktop mode tries Vulkan and then OpenGL 4.3 compute.
6. An ordinary Android mode tries Vulkan and then OpenGL ES 3.0.
7. Mode initialization is attempted for every candidate in order, so a backend
   which probes successfully but cannot create the requested mode still falls
   through to the next GPU backend.
8. If all candidates fail, the named attempt where applicable and the complete
   candidate order are both tried once more with the fullscreen bit inverted,
   matching gfxlib2's final window-mode retry. An explicit null request remains
   a single non-windowed attempt.
9. Initialization fails after both GPU passes are exhausted.

This follows gfxlib2's driver-list policy: a request changes priority, while
the platform list still supplies fallbacks. Selection is capability and
initialization based, not a startup benchmark. Unknown requested names leave
the normal order intact. The accepted aliases are `vulkan`/`vk`,
`opengl`/`gl`, and `opengl es`/`opengles`/`gles`.

Within the Vulkan candidate, gfxlib3 does not trust physical-device enumeration
order. It first retains the original preference for `shaderFloat64`, because
that keeps the exact midpoint ellipse compute path available. Within that
feature group it ranks discrete GPU, integrated GPU, virtual GPU, CPU, then an
unknown device type. A queue supporting graphics and compute ranks above a
compute-only queue on an otherwise equal adapter. The render thread still
attempts logical-device creation in ranked order and remembers a failed
candidate, so driver policy or an unavailable presentation queue cannot stop a
lower-ranked compatible adapter from being tried. Equal candidates retain
loader order for a stable result.

The null backend is selected only by `GFX_NULL` or the test harness. It provides
a deterministic CPU reference surface so headless compatibility tests remain
possible. It is not an automatic substitute for a failed GPU request.

The current null-backend slice supports generation-tagged surfaces at logical
depths 1, 2, 4, 8, 16, and 32, plus clear, point batches, styled lines, BOX/BF,
full and filled ellipses, built-in PUT modes, and pixel readback. It applies the
logical color mask at pixel write time. The compatibility layer above it is
testable but remains below the exported FreeBASIC ABI until mode lifecycle and
runtime hook coverage are sufficient to replace whole symbol families safely.

## Error and device-loss policy

- Public calls validate dimensions, pitches, regions, enum values, and size
  arithmetic before queuing.
- Initialization reports a normal FreeBASIC runtime error and releases partial
  resources.
- A lost Vulkan device or unrecoverable OpenGL context marks the renderer
  failed, wakes all fence waiters, and makes later calls return an error where
  the existing ABI permits one.
- Void legacy APIs store the runtime error and become safe no-ops after a fatal
  renderer failure.
- Debug output goes through one gfxlib3 logging module and is disabled or
  redirected centrally.

Final resource destruction runs on the render thread before backend shutdown.
This ordering is required because OpenGL texture destructors need the owning
context and Vulkan destructors will need the live device. The caller thread
only joins the renderer and releases ordinary queue storage afterward.

## GPU surface extension API

`fbgfx3.bi` now exposes mode-owned opaque surfaces. The first extension slice
supports:

- checked create, information, and destruction
- direct BLOAD-compatible asset decode and one-time GPU upload
- explicit pitched upload and synchronized download
- clear and built-in GPU-to-GPU PUT modes
- GPU scaling, rotation, and Mode 7 projective sampling
- direct ordered presentation with an optional wait
- ordinary PSET, POINT, LINE/BOX/BF, CIRCLE/arc, DRAW, PAINT, DRAW STRING,
  GET, and CPU-image PUT target syntax

`Gfx3SurfaceCreate` defaults to all four public usage capabilities.  Programs
that deliberately create a narrower surface get checked role enforcement:
render-target for writes, sampled for GPU source/present use, transfer-source
for reads, and transfer-destination for uploads.

Descriptors are validated by identity against a mode-owned registry before
they are dereferenced. Mode shutdown destroys any surfaces the program leaves
live while the render thread and GPU context still exist. Descriptors do not
contain a writable pixel pointer and are deliberately not FB.IMAGE headers.

`Gfx3SurfaceMap` is a deliberately scoped CPU-staging extension. It downloads
the complete opaque surface into a CPU allocation, blocks other surface use
while the map exists, and uploads that allocation on writable unmap.
`Gfx3SurfaceMapRect` applies the same authority transition to a checked
rectangle, so small CPU edits do not require a full-surface transfer. A map
therefore needs transfer-source capability in every case and
transfer-destination capability when writable. Its pointer is never GPU memory
and becomes invalid at unmap or mode shutdown. Backend capability queries,
buffering controls, and shader-backed custom blenders remain future extensions.
The extension ABI should be treated as experimental until those ownership and
versioning rules are finalized.

gfxlib2 parity work does not depend on these additions.

## Stable CPU-image specializations

The CPU FB.IMAGE cache holds an exact packed byte snapshot for images through
16 MiB, subject to a 64 MiB desktop or 24 MiB Android cache-wide budget.
Direct caller writes therefore invalidate the corresponding GPU texture before
a later PUT can sample stale data. Images which do not fit the snapshot budget
retain exact content hashing. Snapshot replacement is allocated and populated
before the old copy is released, so allocation failure does not corrupt the
existing cache record.

While making that check, gfxlib3 also records whether the complete
native-format image is one color. Desktop compute backends turn a full-image
`PSET`, `PRESET`, or non-key `TRANS` PUT from such an image into an opaque GPU
rectangle. The source texture is still created and initialized, so a later
AND, XOR, ALPHA, or other source-reading PUT observes the same cached image.
GLES deliberately retains its instanced sprite batch for this shape: its
individual rectangle command path is slower on the supported Adreno ES 3.0
driver. The choice remains renderer-specific but all pixel math and final
drawing remain GPU work.

The ownership table remains the authoritative 128-entry cache and exact LRU.
A 256-slot advisory pointer table fronts it for hot stable FB.IMAGE headers.
Each direct hit is accepted only when the selected entry still contains that
exact header pointer; collisions, eviction, and stale slots fall back to the
bounded exact scan. The shortcut therefore removes repeated cache scans
without weakening direct-memory-write detection or resource ownership.

## Exact opaque ellipse batching

The single OpenGL ellipse command already uses the legacy midpoint algorithm
in a compute shader. A long run of opaque filled ellipses now sends one
workgroup per command in a single GPU dispatch. Each workgroup produces the
same midpoint spans as the individual command and atomically records its
one-based FIFO index in the shared winner texture. A second compute pass
writes only the color belonging to the final winner at each touched pixel.

This is intentionally not a distance-field approximation. It keeps gfxlib2's
circle coverage and overlapping-command order while removing repeated
render-thread dispatch setup. Outlined ellipses, partial arcs, and alpha
ellipses retain the individual exact shader, because their command ordering
or coverage is not represented by this opaque filled batch.

## Multi-adapter selection and Vulkan PUT batching

On a desktop Vulkan normally ranks compatible physical devices by usable
features, then adapter class, with a discrete GPU preferred over an integrated
GPU. `FBGFX3_VULKAN_DEVICE_INDEX` is an intentionally diagnostic-only process
environment variable. When set to a decimal Vulkan loader index, gfxlib3
opens only that adapter. An invalid or unavailable index fails initialization
instead of silently using another GPU. This makes multi-adapter regressions
repeatable while leaving the normal automatic policy unchanged.

The producer combines adjacent compatible PUT operations into one `BLITS`
packet. Vulkan accepts as many as 8,192 records, matching the normal graphics
queue, so a sprite-heavy frame does not have to stop at the old 1,024-command
boundary. Submission slots own persistently mapped command, tile, and winner
buffers. A slot is not rewritten until its shared submission fence signals.
This avoids Vulkan allocation churn while retaining the six-slot, two-group
asynchronous lifetime rule.

Vulkan has three ordered execution strategies. Destination-independent PSET,
PRESET, and TRANS batches use a two-pass winner pipeline on NVIDIA hardware.
One shader invocation evaluates one source pixel, including clipping and the
native transparent key, then atomically records the last one-based BASIC
command index reaching its destination. A resolve shader writes the selected
source pixel once. Overlap order is exact, but thousands of source-pixel
invocations run concurrently instead of making every destination pixel replay
the complete sprite list.

The Intel RGB565-heavy route uses compact 16 by 16 tile records for TRANS.
Host code supplies a 16-bit covered-column mask, a 16-bit covered-row mask,
and a checked source-address bias for each sprite/tile intersection. One
workgroup owns each destination tile. Its 256 shader lanes select their own
coverage bit, fetch the source texel, apply the depth-specific transparent
key, and replay intersecting records in FIFO order. Separate programs for
8-bit, RGB565, and 32-bit targets remove the general PUT mode tree from this
hot loop. The CPU only
constructs bounded command metadata; it does not sample, test, or write image
pixels.

The general Vulkan tile shader remains the route for other compatible modes
and adapters. It bins only command indices intersecting each 16 by 16 tile and
replays them in FIFO order inside the owning workgroup. Destination-reading
modes, self-blits, mixed sources, and excessive working sets retain the exact
ordered fallback with explicit compute dependencies. A 16,384-record packet
was tested and rejected because both desktop adapters developed severe tail
latency. The retained 8,192 limit improved both adapters without that failure.

Mixed-colour opaque filled rectangles use the same ownership rule. The former
same-colour shader specialization remains the cheapest path when overlap order
cannot affect a pixel. When colours differ, a tile workgroup replays the valid
BOX BF commands reaching that tile in Basic submission order, so every colour
decision and write is still performed by Vulkan shader units.

The JRPG editor showed why the dispatch domain must also be compact. Its small
UI rectangles can be spread across a large page, and dispatching the complete
page once for every packet creates thousands of empty workgroups. The host now
provides a compact list of only the 16 by 16 tiles touched by a packet. One
workgroup owns each listed tile. The CPU intersection is conservative metadata
construction; the shader still performs every per-pixel clip, styled-outline,
ordering, colour-mask, and write decision.

Each Vulkan submission slot owns persistent mapped rectangle command, range,
index, and tile-coordinate buffers. The slot fence prevents them from being
rewritten while the GPU can still read them. Point packets use the same
fence-owned storage rule. This removes Vulkan object allocation from normal
primitive packets without weakening asynchronous submission.

The Vulkan backend also flattens adjacent opaque `POINTS`, `LINE`, `LINES`,
`RECTANGLE`, `RECTANGLES`, and `ELLIPSE` commands into one shared 80-byte
primitive record format. The flattening array is retained in backend state and
grown with checked arithmetic, so a normal editor frame does not allocate and
free a temporary array for each mixed packet. A packet contains at most 8,191
records. This keeps its one-based FIFO order within the 13-bit winner tag and
bounds every host-visible allocation.

An ellipse-free packet containing a rectangle uses mixed primitive tile replay.
The renderer builds conservative 16 by 16 tile lists from primitive bounds and
VIEW bounds. Each shader workgroup owns one tile, each invocation owns one
pixel, and that invocation replays only its tile's point, line, and rectangle
candidates in FIFO order. The shader calculates the exact styled-line pixel,
continuous rectangle-outline style phase, filled coverage, clipping, colour
mask, and final write. The CPU performs scheduling and conservative binning;
it does not rasterize or reject individual pixels.

A mixed packet without rectangles uses the atomic winner and resolve path on
qualified NVIDIA and Intel drivers. Its host table contains only useful
64-pixel coverage chunks and maps each workgroup to one primitive plus its
first coverage index. A point therefore contributes one workgroup, and a short
line is not padded to the longest primitive in the packet. The winner shader
retains exact coverage and clipping, while a generation-tagged atomic key
preserves last-command-wins order without clearing the winner allocation for
every packet.

Rectangles and midpoint ellipses deliberately do not share tile replay. A
packet containing both returns to the established exact type-specific paths.
This keeps ellipse state and overlap ordering correct without placing an
approximate distance test in the tile shader.

Adjacent Vulkan ellipse commands use the same ordered-submission policy.
Every command retains its exact midpoint pipeline dispatch and its compute
write-to-read dependency because ellipses can overlap. Their command records
are packed into one host-visible buffer and their descriptor sets are bound in
one command-buffer submission. This removes allocation and queue-submission
overhead without allowing the GPU to reorder public CIRCLE results.

On Windows, WGL does not provide a portable physical-device-index selection
API. gfxlib3 lets Windows and the installed driver choose the OpenGL adapter;
the render-thread interop smoke test reports the actual `GL_VENDOR` and
`GL_RENDERER` strings. Windows' per-application Graphics preference can be
used to request either the power-saving iGPU or high-performance dGPU during
deployment and test runs.

OpenGL batches non-self-referential compatible PUT commands by destination
tile. The CPU contributes only source rectangles, clipped destinations, and
ordered tile lists. One 16 by 16 compute workgroup owns each tile and replays
its list in submission order, so PSET, PRESET, TRANS, AND, OR, XOR, ALPHA,
BLEND, and ADD retain the single-command shader's exact per-pixel calculation
without inter-tile races. Source-equals-destination, mixed source surfaces,
mode changes, invalid geometry, and excessive tile counts deliberately use
the established ordered fallback.

OpenGL ES 3.0 uses one instanced quad per ordinary PSET, PRESET, or TRANS
sprite. Rasterization preserves primitive order, so overlapping sprites keep
last-writer semantics without a CPU raster or an intermediate system-memory
framebuffer. The common TRANS path selects a dedicated 8, RGB565, or 32-bit
fragment program. Those programs fetch the texture's existing byte lanes,
apply exactly one native transparent-key comparison, and write them directly;
they avoid the general mode branch and integer unpack/repack work that was
material on the Adreno 306. One draw may contain up to 4,096 sprites.

Vulkan keeps a reusable host-visible download buffer for synchronous GET and
GPU-surface downloads. A download submission is fence-waited before pixels are
copied to caller memory, so the render thread can safely reuse or grow this
buffer on the next GET. This removes transient Vulkan allocation churn; it
does not make GET asynchronous or relax its Basic-visible ordering boundary.

For opaque filled CIRCLE batches, the validated NVIDIA route has two GPU
passes. One midpoint-coverage workgroup per public command atomically records
the final Basic command reaching each pixel; a resolve workgroup then writes
the selected native colour once. The command tag, rather than a CPU raster,
preserves overlapping CIRCLE order while the shader lanes generate spans and
perform all pixel writes. Outline and alpha CIRCLE commands retain the ordered
midpoint compute path because they read destination pixels. The global atomic
route is currently enabled only for NVIDIA vendor ID `0x10DE`: the Intel UHD
630 driver supports the portable ordered shader but did not complete the
global-atomic workload promptly. This is an explicit adaptive renderer choice,
not a CPU fallback.

## Compact glyph packets and ordered tile replay

The public thread no longer expands built-in font output into one point command
per covered pixel. `DRAW STRING` and the graphical console append compact
`FB_GFX3_GLYPH` records containing the glyph bitmap, destination, colours,
flags, and clip. Up to 8,191 adjacent records share one `GLYPHS` renderer
packet. Memory fonts and unsupported text shapes retain the established exact
path.

Desktop OpenGL and Vulkan bin glyph command indices against destination tiles.
One workgroup owns each tile and every invocation replays its tile's command
list in FIFO order. The shader tests the eight bitmap rows and performs the
foreground or opaque-background write itself. This keeps glyph coverage,
clipping, and pixel writes on shader units while preserving later-glyph-wins
semantics when characters or console cells overlap. It also avoids the former
two-pass winner-image and resolve cost.

The tile size is selected per backend rather than forced to one nominal value.
OpenGL uses 8 by 8 tiles, which match the built-in glyph cell and measured best
on this host. Vulkan uses 16 by 16 tiles because the Intel UHD 630 regressed
substantially at 8 by 8. The packet limit is intentionally 8,191: experiments
with a 65,535-record packet increased worst-case per-pixel replay and tail
latency under repeated console overdraw.

Each Vulkan submission slot owns persistently mapped glyph command, tile-range,
and tile-index buffers. A slot is rewritten only after its shared submission
fence signals. This retains the batch-group lifetime boundary and eliminates
transient Vulkan buffer allocation from normal text submission.

## Cooperative GPU PAINT

PAINT begins with an exact rectangular-region specialization. Phase one finds
the candidate extents crossing the seed. Phase two dispatches across the clipped
VIEW and atomically rejects the candidate if any shader lane finds a border
pixel inside or an opening outside its four-neighbour perimeter. A device-wide
storage barrier precedes phase three, which writes a verified region with
independent per-pixel shader math. Phase four invokes the irregular fallback
only when verification rejected the candidate.

Irregular regions use exact scanline flood topology rather than a parallel
algorithm whose result could depend on invocation scheduling. Lane zero owns
the compact queue of contiguous runs and discovers neighbouring runs in gfxlib2
order. Once a span is known, every lane in the workgroup cooperatively marks
and writes its portion of that span, including pattern lookup, mask handling,
and alpha math. Barriers separate topology discovery from the parallel write
and from the next neighbour scan.

OpenGL uses 16 by 16 workgroups and Vulkan uses 16 by 8 workgroups for rectangle
verification and writing. The irregular fallback uses the same 256 or 128 lanes
within one workgroup because its strict scanline queue cannot be distributed
without changing traversal order. Vulkan keeps its PAINT command, phase state,
and scratch storage in each fenced submission slot, avoiding per-call
host-visible allocations. Adjacent compatible opaque solid recolours execute
only the final command. Desktop screen pages use this path rather than forcing
a transfer merely because they also permit downloads. The public front end
retains a bounded CPU compatibility path outside the documented shader watchdog
limit. This limit is a driver-liveness guard, not the normal desktop rendering
route.

## GPU asset and transformed-surface path

`Gfx3SurfaceLoad` gives asset-oriented code a one-call residency transition.
The runtime decodes the same BMP or PNG family used by BLOAD into temporary
staging pixels, creates a surface with the requested checked usage bits, queues
one upload, and releases the staging allocation. The returned pointer is still
an opaque mode-owned surface descriptor. It is not an `FB.IMAGE` header and
has no CPU pixel address. `GFX3_SURFACE_ASSET` therefore requests only sampled
and transfer-destination authority by default. A caller that needs later
readback must ask for transfer-source authority explicitly.

Ordinary `PUT (x, y), gpu_surface, mode` recognizes the descriptor through the
mode's resource registry. It sends the existing GPU-to-GPU blit command and
does not download or construct a CPU image. A GPU surface may likewise be the
destination of the transform extensions and then become a later PUT source.
This lets a load, scale, rotate, composite, page copy, and presentation chain
remain in graphics memory. CPU synchronization occurs only at an observable
operation such as POINT, GET, DOWNLOAD, MAP, SCREENPTR, or a custom CPU blender.

The scaled, rotated, and Mode 7 entry points reduce to one projective command:

```text
destination pixel centre -> inverse 3 by 3 matrix -> source coordinate
                          -> clamp/repeat -> nearest/linear sample -> PUT mode
```

The Basic-facing front end calculates one inverse matrix and conservative
integer destination bounds per operation. It does not walk destination pixels.
Backends validate that finite matrix and perform the per-pixel mapping and
sampling in shaders:

- Desktop OpenGL uses a 16 by 16 compute dispatch.
- Vulkan uses a 16 by 16 compute pipeline and groups adjacent transforms into
  one fenced command-buffer submission. Each operation retains its own
  descriptor set, matrix, bounds, sampling policy, and barriers.
- OpenGL ES 3.0 uses integer-texture fragment shaders. Adjacent TRANS, PSET,
  and PRESET transforms sharing source and destination surfaces use instanced
  quads, so hundreds of sprites require one driver draw rather than one draw
  per sprite.
- The null backend is the exact CPU reference used for pixel comparisons.

Nearest sampling is exact at destination pixel centres. Linear sampling blends
four native source texels after converting RGB565 or 32-bit packed colour to
channels; indexed surfaces deliberately stay nearest. Clamp rejects mappings
outside the source rectangle before sampling, while repeat wraps within that
rectangle and is used by Mode 7. All standard built-in PUT modes are available
on the single-operation paths. Destination-reading modes cannot join the GLES
instanced batch because each operation must observe preceding destination
pixels in strict order.

The transform command stores a destination-to-source matrix. Scaling produces
an affine matrix, rotation includes pivot translation and independent X/Y
scale, and Mode 7 produces a projective matrix whose denominator changes with
distance below the horizon. This shared representation keeps the public
operations small while leaving room for a future general matrix entry point.
When source and destination are the same surface, the backend takes a temporary
GPU snapshot before dispatch. It never resolves the overlap through CPU RAM.

## GPU-target PUT clipping

Built-in PUT to a screen page or opaque GPU surface does not trim a partial
sprite on the BASIC thread. The public layer retains the requested source
rectangle, destination origin, and active VIEW rectangle in the renderer
command. This keeps command production independent of the number of visible
pixels and avoids rewriting source coordinates before the GPU sees the work.

Desktop OpenGL and Android OpenGL ES pass that original geometry to the raster
pipeline. The framebuffer viewport rejects triangles outside the surface before
fragment work. The fragment shader tests the command's VIEW rectangle when it
is smaller than the surface. Transparent-key rejection, source sampling, logic
modes, and destination writes remain shader work. A hardware-scissor version
was measured and removed because changing scissor state between clip groups was
slower than the retained fragment comparison.

Vulkan compute has a different dispatch boundary. Sending a 64 by 64 dispatch
for a sprite with only one visible column would waste 4,032 invocations. The
renderer thread therefore intersects command rectangles and adjusts the source
origin before recording the dispatch. This is fixed command arithmetic after
the BASIC thread has submitted the PUT. It does not inspect, copy, or clip any
pixels. The compute shaders still perform all source reads, transparent-key
tests, overlap ordering, and destination writes, and retain the clip rectangle
as a final bounds guard.

NVIDIA's destination-independent batch launches one linear shader invocation
per visible source pixel. An atomic maximum records the last BASIC command that
reaches each destination pixel, followed by a shader resolve over the batch
union. Intel normally uses compact 16 by 16 active-tile records and
depth-specific TRANS shaders. If every Intel command was actually clipped to
64 pixels or fewer, the renderer selects the linear winner shader instead;
replaying a long ordered command list in all 256 tile lanes is wasteful for
that sparse case. Ordinary Intel sprites continue to use the tile route.

CPU clipping remains mandatory for a raw FB.IMAGE destination and for CUSTOM
PUT callbacks. Those paths perform pointer arithmetic, may download a source,
or call user code with a CPU-visible rectangle. They cannot safely expose an
out-of-bounds destination and are intentionally separate from the GPU-target
contract.

## Packet handoff and real renderer overlap

Producer batching and renderer offloading are related, but they are not the
same property. A `BLITS` packet reduces allocations and queue traffic on the
BASIC thread. It provides real offloading only after the render thread can see
the packet and the graphics driver has submitted its work to the GPU.

The common context keeps an incomplete compatible packet private so adjacent
PUT operations can still combine. Once that packet reaches the selected
backend's limit, 4,096 sprites for GLES/OpenGL and 8,192 for Vulkan, the
context now submits the completed packet to the renderer queue immediately.
The previous implementation moved a full packet into a second pending array
and waited for 1,024 packet commands or a synchronous operation. An ordinary
307,200-sprite stream contains only 75 GLES packets or 38 Vulkan packets, so
the renderer did not receive any of them until the final POINT. Packet
completion is now an asynchronous handoff boundary, while a partial tail still
flushes at the established ordering boundary.

OpenGL ES requires one additional boundary. `glFenceSync` inserts a fence in
context order, but it does not require a mobile driver to submit buffered work.
The GLES backend calls nonblocking `glFlush` after recording each asynchronous
fence. This makes commands through that fence eligible to run on the GPU while
the BASIC thread prepares later packets. It does not wait for the fence or
turn an asynchronous command into a synchronous one. POINT, GET, surface
mapping, and other readback operations retain their explicit ordered waits.

This distinction is covered by `sprite-offload-benchmark.bas`. Its baseline
reports application submission and ordered completion separately. Its second
phase performs a fixed interval of deterministic application-side integer work
between submission and POINT. A smaller residual GPU wait proves that rendering
advanced during useful CPU work; a fast producer alone does not establish that
claim.

## Asynchronous page boundaries and GPU copy batches

A SCREENSET that changes the visible page, and every SCREENCOPY or FLIP, is a
frame-production boundary. The compatibility layer calls
`fb_gfx3_context_submit_pending()` after staging it. That operation transfers
ownership of the pending packet array to the renderer queue and returns without
waiting for the render thread or GPU. It is not a synchronization operation.
`SCREENSYNC`, POINT, GET, mapping, and other readback paths retain their ordered
completion waits.

Each GPU backend recognizes a drain containing only PAGE_SET and complete page
BLIT operations. Such a drain updates logical page state immediately but does
not swap the native window for every producer handoff. Presentation occurs at
the next explicit barrier, periodic PLATFORM_POLL, or non-page command. The
periodic poll is important for a program which performs a last SCREENSET and
then only computes or waits: the final dirty page still reaches the display.

Full-page PSET copies have a backend-specific GPU route:

- OpenGL copies the integer page texture with `glCopyImageSubData`.
- Vulkan records `vkCmdCopyBuffer` commands against the device-local page
  buffers. One global pre/post memory barrier and a transfer dependency between
  retained copies replace per-copy barrier expansion.
- GLES retains its shader copy path but shares the deferred-presentation rule,
  avoiding one EGL swap for each asynchronous page packet.

OpenGL and Vulkan attach exact content tokens to page handles while scanning a
contiguous page batch. A copy whose destination already has the source token is
redundant. A backward liveness pass may also discard a copy whose destination
is overwritten before a retained copy reads it and before the end of the drain
makes it observable. This is command-graph elimination, not approximate pixel
comparison; unknown content invalidates the cache conservatively.

A VIEW-restricted page copy is not promoted to the transfer route. It falls
back to the existing BLIT shader, which retains clipping and source-coordinate
semantics. The focused page smoke checks one inside pixel and one outside pixel
after a partial copy on OpenGL, both desktop Vulkan adapters, and physical
Android GLES.

The page benchmark records producer and completed durations separately. A
short producer duration establishes that the BASIC thread was released; a
short completed duration establishes actual renderer throughput. Both are
required because queue deferral by itself is not GPU offloading.

## Oversized CPU sprite sheets

A CPU FB.IMAGE can be wider or taller than the selected backend's maximum GPU
surface even when every PUT uses a small source rectangle. QFAK exposes this
case with 10,600 by 40 and 10,960 by 40 bitmap strips, while the Adreno 306
accepts textures no wider than 4,096 pixels. Rejecting the complete image would
make a valid legacy PUT fail even though the requested 40 by 40 sprite is well
within the device limit.

The CPU-image cache therefore changes its ownership unit only when the complete
source exceeds a backend dimension limit. Its key becomes the image header plus
the exact source rectangle. The cache allocates that rectangle in a normal GPU
atlas cell or a dedicated surface, uploads it directly from the original
pitched CPU image, and translates the later blit coordinates into the cached
region. Multiple regions from one sheet can remain resident simultaneously.
Destroying the CPU image retires every associated region.

This is a residency mechanism, not CPU rasterization. The CPU copies source
bytes only for the first upload or a real mutation. Clipping, transparent-key
testing, PUT-mode math, and destination writes still execute in the selected
GPU backend. A requested rectangle which itself exceeds the hardware limit is
rejected because it cannot be represented by this cache.

gfxlib3-created new-format images use the twelve backend-reserved header bytes
for a private generation, ownership marker, and external-write flag. BLOAD and
gfxlib3 image writers advance the generation, allowing an unexposed stable
image to reuse its GPU region without rescanning or duplicating the complete
sheet in a CPU snapshot. IMAGEINFO deliberately marks the pixels externally
writable. Such an image retains exact snapshot validation of the requested
rectangle before cache reuse, so a direct caller edit is visible at the next
PUT boundary.

## Ordered generated masks and writable screen pointers

`Gfx3DrawPoints` is the public bridge for a generated mask, particle field, or
other application-produced point array. Screen coordinates pass through the
active VIEW and WINDOW mapping, but clipping remains in the GPU backend. The
BASIC thread therefore converts point records and command metadata without
testing every point against the visible rectangle.

Point arrays join the same pending stream as ordinary compatible PSET output.
An individual API call is not an observable completion boundary, so adjacent
arrays remain queued together until another primitive, a page boundary,
capacity, or a CPU-visible operation requires ordered submission. This avoids
one command allocation and one shader dispatch for every short string in an
antialiased user interface.

Independent points within one layer execute concurrently. When WINDOW mapping
or separate calls produce the same physical coordinate, the compatibility
layer assigns successive writes to ordered layers. Each layer contains unique
coordinates and can use all available shader units; layers are submitted in
the original BASIC order so destination-dependent alpha remains exact. Opaque
writes to the same coordinate collapse to the last write when no intervening
destination read exists.

`SCREENPTR` has a different ownership rule. Its return value is a writable
address, and FreeBASIC permits the caller to retain and modify that address
outside `SCREENLOCK`. gfxlib3 cannot observe those stores individually.
Returning the pointer therefore makes the complete CPU page shadow
authoritative and dirty. The next GPU operation uploads the changed shadow in
order, while POINT and other CPU reads use the same authoritative copy.
`SCREENLOCK` still provides a stable grouping and resize boundary, but it is
not required for a legal direct pointer write.

This conservative full-page rule applies only after the writable page pointer
has escaped. Programs which remain on GPU surfaces, ordinary primitives,
resident images, page copies, and `Gfx3DrawPoints` do not create that CPU
authority or its upload boundary.

## Resident application caches and current-page blits

An application may build an expensive immutable image in ordinary memory and
then create a `GFX3_SURFACE_ASSET` for it. The upload is an ownership
transition: subsequent draws use the GPU copy until the application explicitly
uploads replacement pixels or asks for a readback. This is the preferred path
for maps, sprite sheets, fonts, and other assets which are drawn many times.

`Gfx3SurfaceBlit` accepts a NULL destination. In that form the destination is
the current work page selected by SCREENSET. The compatibility layer first
flushes older page writes, applies the active VIEW rectangle, queues a
surface-to-page GPU copy, and marks the CPU page shadow and POINT cache stale.
It does not download either image. A later POINT, GET, or other CPU-visible
operation performs the ordered synchronization only if it is actually needed.

OpenSlicks uses this path for its 320 by 200 track cache. The complete track is
uploaded once and copied to the current page once per frame. Its antialiased
font and Android touch overlay use `Gfx3DrawPoints` with alpha values, so the
renderer shader performs clipping and destination blending concurrently. The
BASIC thread describes the pixels but does not read, clip, blend, or store
them itself.

## GLES byte-exact readback

The Adreno 306 driver used by the AGM A8 accepts integer colour attachments,
but an integer `glReadPixels` from the framebuffer can return alternating
empty columns. This is a driver behavior, not a surface row-pitch rule.
gfxlib3 therefore copies the requested integer texture into a reusable
normalized RGBA8 staging texture with a shader and reads that staging texture
through the universally supported `GL_RGBA` path. The shader preserves each
8-bit channel exactly.

The staging program and texture belong to the GLES renderer thread and are
destroyed with that backend. This keeps platform-specific behavior out of the
surface API and makes POINT and explicit downloads correct without weakening
the GPU-resident steady path.

<!-- end of architecture.md -->
