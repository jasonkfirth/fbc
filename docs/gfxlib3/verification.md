# gfxlib3 verification record

## 2026-07-17 common and reference core

The common sources and standalone infrastructure test were compiled with:

```text
gcc -std=gnu11 -Wall -Wextra -Werror
```

The test result was:

```text
gfxlib3 infrastructure: all checks passed
```

This proves checked command storage, queue behavior, completions, resource
generation and deferred destruction, renderer failure wakeups, null/reference
surface operations, the typed context/surface layer, and centralized logging.
It does not prove the public FreeBASIC ABI.

The same strict executable was rerun after adding the non-blocking queue drain
used by the render thread. It verifies empty and closed queue results through
both blocking and non-blocking APIs. Its held-startup test queues three
asynchronous commands before allowing a controlled backend to initialize,
then proves they reach that backend in one ordered call. A completion barrier
follows in a separate call. The render thread therefore dispatches up to 64
adjacent asynchronous commands as one ordered backend call; result-bearing
completions, shutdown, and render-thread interop callbacks remain separate.
This is bounded command batching. Vulkan now separately owns a three-slot
submission ring, so its renderer can retain multiple command buffers, fences,
descriptor sets, and staging allocations in flight.

## 2026-07-17 Android ARM64 renderer retry and batching

The Android ARM64 normal and multithreaded position-independent gfxlib3
archives were rebuilt with NDK 27. Their staged package copies matched SHA-256:

```text
libfbgfx3pic.a    749156D8009ACC22823668E2D39820C003E08CA7222864CF0B23D57BDCE6FE19
libfbgfx3mtpic.a  A65B44AB586DFE97DF68D2F14F663301129360787B9FD6D86F5663C6B61E23B4
```

`android-renderer-smoke.bas` was then repackaged from those archives and run
on the connected AGM A8 (Android API 24, Adreno 306). The device exposes no
Vulkan feature, so automatic selection takes the ordinary Vulkan probe and
falls back to OpenGL ES. The test requires that selected driver, exact POINT
and rectangle readbacks, page selection, and a synchronized present; it ended
with `FREEBASIC_ANDROID_EXIT:0`. The graphical BASIC print marker is not a
reliable logcat channel on this NativeActivity, so the exit status and the
program's assertions are the recorded result.

## 2026-07-17 Win32 OpenGL compute slice

The same test executable created the real render-thread context and reported:

```text
gfxlib3 OpenGL smoke: OpenGL gfxlib3 backend initialized: 4.3.0 NVIDIA 595.79
gfxlib3 infrastructure: all checks passed
```

It verified GPU texture creation, compute clear, compute point batches, compute
solid and patterned lines, styled BOX and BF, full and filled midpoint
ellipses, all nine non-custom built-in PUT modes, overlapping same-surface PUT
through a temporary GPU texture, GLsync completion, integer framebuffer
readback, and render-thread texture destruction. It also compared clipped,
reversed, steep, shallow, horizontal, and vertical GPU line cases plus box and
ellipse fixtures against the reference backend at every pixel.

Pitched upload and download were verified with distinct active pixels and row
padding sentinels. No CPU pointer is retained after the synchronous download
completion is signaled.

This initial checkpoint was an offscreen renderer proof. Visible presentation
was added and verified in the later checkpoint below. Input handling, Vulkan,
and Linux OpenGL remain outside the proof.

## 2026-07-17 repository archives

The repository build was run in MSYS2 MINGW64 with:

```text
make -j4 gfxlib3 ENABLE_PIC=YesPlease
```

It produced and indexed all four archives:

- `libfbgfx3.a`
- `libfbgfx3mt.a`
- `libfbgfx3pic.a`
- `libfbgfx3mtpic.a`

## 2026-07-17 compiler selection

The compiler was rebuilt with `make -j4 compiler`. Two headless FreeBASIC
programs were then compiled and executed with the MINGW64 toolchain pinned.
The default program linked `-lfbgfx` without `-lfbgfx3`. The initial opt-in
program linked gfxlib3 first and both GFX_NULL pixel round trips exited with
status zero.

The opt-in program was also compiled to an object. Its `.fbctinf` section
contained `-gfx3`; linking that object in a separate invocation again selected
gfxlib3, and the resulting executable passed.

The same selection passed without including `fbgfx.bi`, proving that the
graphics intrinsic callback observes the define. The `-mt` executable selected
`libfbgfx3mt` and passed. Win32 rejects `-pic` as a target option, but the link
diagnostics selected the expected `libfbgfx3pic` and `libfbgfx3mtpic` names.

The temporary gfxlib2 fallback was then removed. A broad command-sweep link had
shown that gfxlib2's `gfx_line.o` was pulled for an internal DRAW helper and
also redefined gfxlib3's `fb_GfxLine`. This proves that archive-object boundaries
cannot safely mix the runtimes. Opt-in links now contain gfxlib3 only and report
unimplemented APIs as linker errors.

## 2026-07-17 stateful compatibility slice

The strict infrastructure executable now also creates a three-page logical
mode above the null backend. It verifies independent caller work-page state,
packed SCREENSET results, visible-page state, GPU page copy, absolute and
relative PSET, POINT readback, pen queries, LINE, ellipse routing, VIEW border
and fill, WINDOW Y direction, PMAP, reset behavior, and stale-state rejection
after mode shutdown.

## 2026-07-17 public FreeBASIC ABI slice

`tests/gfx3/api-smoke.bas` was compiled with `__FB_GFXLIB3__`, linked against
gfxlib3, and run with GFX_NULL. It opened a 16 by 16, 32-bit, three-page mode
and passed RGBA PSET/POINT, STEP coordinates, POINTCOORD, LINE, BF, CIRCLE,
VIEW fill and border, WINDOW, SCREENSET, SCREENCOPY, and mode shutdown. Both the
compiler and program returned status zero.

Executable symbol inspection found `fb_gfx3_mode_init`, `fb_GfxScreenRes`,
`fb_GfxPset`, `fb_GfxPoint`, `fb_GfxLine`, `fb_GfxEllipse`, `fb_GfxView`,
`fb_GfxWindow`, and the page entry points. The proof therefore exercises the
gfxlib3 ABI rather than an identically named gfxlib2 path.

## 2026-07-17 public command and image closure

The complete `tests/command-sweep/gfxlib-command-sweep.bas` source was compiled
with `__FB_GFXLIB3__`. It linked only gfxlib3, reported no unresolved graphics
symbols, and exited with status zero under `GFX_NULL`.

The unchanged source was also freshly packaged as an Android arm64
`fbc-android -gfx3` NativeActivity APK and run on the connected AGM A8. Its
deterministic GFX_NULL route completed the full image/file, page, palette,
lock, input-fallback, query, and shutdown sequence, including its temporary
BSAVE/BLOAD round trip, with:

```text
FREEBASIC_ANDROID_EXIT:0
```

The new `tests/gfx3/command-compat-smoke.bas` checks observable PAINT, recursive
DRAW movement, built-in text, CPU image targets, and a 32-bit BMP round trip.
It exits with status zero through both the null backend and the real OpenGL
backend. `image-smoke.bas` also exits with status zero through both backends,
including GET, all built-in PUT modes, and a custom BASIC blender.

All gfxlib3 C sources added for images, queries, text, DRAW, PAINT, data, and
file codecs compile separately with `-Wall -Wextra -Werror`. All four archive
variants rebuild successfully.

## 2026-07-17 unchanged gfxlib fixtures

The following existing `tests/gfx` sources were compiled unchanged except for
selecting gfxlib3 and linked with fbcunit:

- `line.bas` and `point.bas`: 1,570 of 1,570 assertions passed.
- `draw.bas`: 22 of 22 assertions passed, including recursive `X` and PAINT.
- `image-expr.bas`: 896 of 896 assertions passed.
- `palette.bas`: 1,384 of 1,384 assertions passed against the canonical VGA
  palette.

The combined verified total is 3,872 passing assertions. This is not the full
gfxlib suite. At this checkpoint graphical console hooks, platform input,
Vulkan, and the remaining BMP variants were outside this particular proof;
later focused sections record the console and input coverage added afterward.

## 2026-07-17 Win32 OpenGL presentation and platform boundary

WGL context creation, the native window, message pumping, client sizing,
title changes, and buffer swaps were separated into `gfx3_platform_win32.c`.
The OpenGL backend now owns only GPU work and presentation policy.

`tests/gfx3/presentation-smoke.bas` opens visible 8, 16, and 32-bit modes,
draws through the GPU command stream, presents both pages, and synchronizes
through `SCREENSYNC`. The indexed path changes a palette entry, the RGB565
path checks gfxlib2's 24-bit POINT expansion rule, and the 32-bit path retains
the expected color. A Win32 native lookup verifies ordered WINDOWTITLE
propagation. The executable exits with status zero.

The four archive variants rebuilt after the separation. The complete command
sweep exits with status zero, and one combined post-presentation fbcunit
executable passes 3,872 of 3,872 assertions across 13 tests.

## 2026-07-17 GPU-only surface extension

`inc/fbgfx3.bi` and the opaque mode-owned surface registry were added. The
extension exposes create, information, destroy, pitched upload/download,
clear, built-in GPU blit, and direct present calls. Its descriptors do not
expose writable memory or backend handles.

Ordinary PSET/POINT, LINE/BOX/BF, full CIRCLE and arc, PAINT, DRAW, DRAW STRING,
GET, and CPU-image PUT target syntax now recognizes those descriptors. GPU
surfaces left live are destroyed in command order before mode shutdown.

`tests/gfx3/gpu-surface-smoke.bas` checks those paths through GFX_NULL and the
real Win32 OpenGL backend. Both executables exit with status zero. All changed
C modules also compile individually with `-Wall -Wextra -Werror`, and all four
archive variants rebuild.

## 2026-07-17 graphical console output

A page-specific graphical console and runtime hook lifecycle were added. It
renders cell backgrounds with GPU rectangles, glyphs with point batches, and
scrolling with overlap-safe GPU self-blits. Character, foreground, and
background cells remain available for `SCREEN(row, column)` reads without a
CPU framebuffer.

`tests/gfx3/console-smoke.bas` verifies COLOR, CLS, WIDTH, LOCATE, POS, CSRLIN,
ordinary PRINT pixels, SCREEN character/color reads, independent page cells,
and bottom-row scrolling. Normal and `-mt` executables pass through GFX_NULL
and the real Win32 OpenGL backend. The complete command sweep and the freshly
linked 3,872-assertion fbcunit executable also remain clean.

## 2026-07-17 Win32 input and event adapter

A mode-owned input state now separates native event production from the BASIC
compatibility APIs. The Win32 window procedure writes only through its own
mutex. Input queries submit a synchronous renderer barrier before reading, so
the render thread pumps its native message queue without exposing HWND or WGL
ownership to caller threads. SETMOUSE uses a request mailbox consumed by that
same native owner.

`tests/gfx3/input-smoke.bas` locates the real OpenGL window and injects native
messages. Normal and `-mt` executables pass press, repeat, release, MULTIKEY,
CP437 INKEY, GETKEY, key-buffer status, mouse movement, button state, vertical
and horizontal wheels, SETMOUSE, focus transitions, non-consuming SCREENEVENT
peek, close delivery, extended KEY_QUIT, native handle/display, desktop size,
and window-position get/set checks. The changed C files compile individually
with `-Wall -Wextra -Werror`, and all four gfxlib3 archive variants rebuild.

A renderer-owned wake producer now queues a platform-only poll every 16
milliseconds for windowed modes. Those commands preserve queue order but skip
OpenGL fence creation because they submit no GPU work. The close test sleeps
for 80 milliseconds after posting WM_CLOSE and observes that the native window
was hidden before any later graphics or input query, proving idle dispatch.

## 2026-07-17 graphical line input

`fb_GfxReadStr`, `fb_GfxLineInput`, and `fb_GfxLineInputWstr` are installed in
the active runtime hook table. LINE INPUT delegates editing to rtlib's existing
soft-cursor editor while gfxlib3 supplies the console and keyboard operations.
The standard-input read hook keeps the established byte-oriented echo and
carriage-return behavior.

`tests/gfx3/line-input-smoke.bas` passes as normal and `-mt` Win32 OpenGL
executables. It enters `ac`, moves left, inserts `b`, and verifies `abc`; checks
wide input conversion; invokes `fb_ReadString` on the actual C standard-input
stream to prove the installed read hook; and checks the corresponding console
cells. The neighboring null/OpenGL console smokes and normal/`-mt` input smokes
also remain clean after the hook-table change.

## 2026-07-17 SLEEP and VGA port hooks

The active hook table now includes graphics-aware SLEEP plus indexed VGA DAC
and status-port emulation. The input smoke proves a nominal one-second SLEEP
returns promptly for a queued character, leaves that character available to
INKEY, and accepts SLEEP 0. `tests/gfx3/vga-port-smoke.bas` writes a six-bit
RGB palette entry through ports 0x3C8/0x3C9, reads it back after selecting
0x3C7, and verifies the 0x3DA retrace value. Normal and `-mt` executables pass
through both GFX_NULL and the real OpenGL backend. The new C module compiles
with `-Wall -Wextra -Werror`, and all four archives rebuild.

## 2026-07-17 portable device fallbacks

The unchanged gfx GETJOYSTICK, GETXPAD, and GETTOUCH programs now build with
gfxlib3 selected and exit successfully. Missing joystick state uses buttons -1
and axes -1000; missing XPad state returns status zero with cleared outputs; and
touch is empty before and during a null mode. The Win32 input smoke additionally
passes left-mouse contact count, coordinates, rectangular and circular hits,
and release in normal and `-mt` OpenGL builds. A dedicated QB smoke opens SCREEN
13 through gfxlib3 and passes STICK(0..3) and STRIG(0..7) in both runtime
variants. Native controller enumeration and multitouch are not claimed by this
checkpoint.

## 2026-07-17 complete tests/gfx directory

All 46 unchanged sources in `tests/gfx` now meet their declared result with
`__FB_GFXLIB3__` selected. The expanded fbcunit executable reports all 464,128
graphics assertions passing across 19 tests and eight source suites. Including
the wrapper's non-graphics support suites, the executable reports 464,774
passing assertions:

- alpha blender: 458,752
- FB and QB RGB macros: 1,504
- palette: 1,384
- image expressions: 896
- LINE and POINT: 1,570
- DRAW: 22

The three standalone GETJOYSTICK, GETXPAD, and GETTOUCH cases exit zero. All
five `COMPILE_ONLY_OK` cases compile, and all 30 `COMPILE_ONLY_FAIL` cases fail
as expected. `fb_hPixelSetAlpha4` was added as an alignment-safe compatibility
helper to admit the unchanged blender suite. This checkpoint covers the entire
focused gfx directory but is not a claim that every repository-wide test has
run.

## 2026-07-17 Vulkan runtime bootstrap

`gfx3_vulkan.c` now compiles with `-Wall -Wextra -Werror` without Vulkan SDK
headers or an import library. The standalone `vulkan-bootstrap.c` executable
opens and closes the real runtime twice. Both iterations report Vulkan loader
version 1.4, two physical devices, and selected queue family zero with compute
support. Failure paths release partial ownership, and close clears the public
runtime state. The normal, PIC, multithreaded, and multithreaded PIC gfxlib3
archives all rebuild with the module included.

That initial proof stopped at loader, instance, physical device, logical
device, and queue ownership. The next checkpoint records the later command,
memory, compute, and surface work separately.

## 2026-07-17 Vulkan device-local compute renderer

The Vulkan runtime now creates a resettable command pool, primary command
buffer, reusable fence, descriptor layout/pool/set, and embedded SPIR-V compute
pipelines. The standalone test builds with `-Wall -Wextra -Werror` from only
`vulkan-bootstrap.c` and `gfx3_vulkan.c`. It opens and closes the Vulkan 1.4
loader twice, sees two physical devices, submits four empty command buffers per
iteration, verifies a 257-word GPU fill, and verifies a 257-word compute add
across five workgroups including the partial final group.

The same test creates a 19 by 13 device-local 32-bit logical surface, performs
a pitched upload with source padding, performs a clipped fill, downloads to a
pitched destination, and verifies every active pixel plus every untouched
padding byte. Both lifecycles close with the exact expected submission count
and no published runtime state left behind.

`tests/gfx3/infrastructure.c` builds under the same strict warning gate and
passes the Vulkan backend through the real common render thread. It verifies
1/2/4/8/16/32-bit logical surface support, transfers, depth masks, points,
styled lines, outlined boxes, filled boxes, full and filled midpoint ellipses,
command completion, and resource destruction. A combined primitive surface,
all nine built-in PUT modes, and
an overlapping same-surface PUT match the null reference backend byte for byte.
The overlap snapshot remains entirely in device-local memory. The OpenGL and
common infrastructure checks continue to pass in the same executable.

`tests/gfx3/vulkan-api-smoke.bas` passes in normal and `-mt` builds. It selects
`FB.GFX_VULKAN` explicitly and verifies ordinary SCREENRES, PSET, POINT, solid
LINE, BF, CIRCLE, CPU-image upload and PUT, 32-bit colors, and 8-bit color
masking. All four gfxlib3 archive variants rebuild with the Vulkan backend and
embedded compute modules.

The GPU-surface, image, command-compatibility, and graphical-console smokes
also pass through `FB.GFX_VULKAN` in normal and `-mt` builds. They extend the
proof to arc point batches, PAINT download/upload barriers, DRAW, built-in text,
GET, all built-in PUT modes, custom CPU PUT fallback, overlap-safe GPU console
scrolling, page cells, BMP save/load, explicit destruction, and mode-owned
cleanup. PRESENT is accepted in sequence, but remains non-visible.

The final archive rebuild includes the conservative 4096 by 4096 storage-buffer
surface limit. The standalone test confirms 4097 by 1 is rejected before an
allocation is published. The complete FreeBASIC command sweep was freshly
linked against the rebuilt archive and exits zero under GFX_NULL. The expanded
fbcunit executable was also freshly relinked and again passes all 464,128
graphics assertions across the 19 focused gfx tests (464,774 total assertions).

The installed Vulkan environment reports an NVIDIA RTX 2060 and Intel UHD 630.
No Khronos validation layer is installed, so this checkpoint does not claim a
clean validation-layer run. It also does not claim Vulkan swapchains, visible
presentation, or multiple in-flight submissions.

## 2026-07-17 Win32 Vulkan visible presentation

The Win32 platform contract now separates graphics-neutral window ownership
from WGL. Both GPU backends create and pump the same render-thread-owned native
window; OpenGL asks for a context, while Vulkan receives checked `HINSTANCE`
and `HWND` values. The Vulkan runtime enables the two required instance WSI
extensions and the device swapchain extension, requires the selected compute
queue to support the actual window surface, and keeps all WSI calls on the
window-owning render thread.

The runtime selects BGRA8 transfer-destination swapchain images, requests one
image above the surface minimum within the reported maximum, prefers mailbox
and falls back to FIFO, and owns acquire/render semaphores. A new embedded
compute module converts all logical depths into a device-local BGRA8 buffer.
The command stream barriers that buffer into transfer-read state, transitions
the acquired image, copies the buffer into it, transitions to present state,
and waits on the rendering-finished semaphore during queue presentation.
Out-of-date and suboptimal results recreate the swapchain, while per-image
initialization state selects the legal old layout.

`tests/gfx3/vulkan-presentation.c` builds from only the test and
`gfx3_vulkan.c` with `-Wall -Wextra -Werror -fno-strict-aliasing`. It opens a
real Win32 surface and captures the displayed client pixels. Exact quadrant
checks pass for 1, 2, 4, and 8-bit palette conversion, 16-bit RGB565 expansion,
and 32-bit color conversion. The test repeats every present, resizes the client
from 8 by 8 to 16 by 12, confirms the new displayed quadrants, confirms the
new public swapchain extent, and exits zero.

The strict combined infrastructure executable still passes both the Vulkan
device-local reference comparisons and the OpenGL 4.3 checks. Fresh normal and
`-mt` builds of `tests/gfx3/vulkan-api-smoke.bas` both exit zero with the
visible swapchain active. All four gfxlib3 archive variants rebuild after the
platform, WSI, shader, and idle-poll integration changes. The Vulkan mode now
uses the same 16 millisecond idle platform-poll producer as OpenGL so native
input, focus, close, and resize messages continue to dispatch while BASIC code
is otherwise idle.
The same input smoke passes once each in normal and `-mt` Vulkan builds after
this integration, including HWND discovery, keyboard, mouse, focus, close,
SCREENEVENT, and idle close dispatch.

This checkpoint does not claim a Khronos validation-layer run because the
layer is not installed. It also does not claim Linux/X11 presentation,
fullscreen policy, or multiple Vulkan submissions in flight.

## 2026-07-17 Linux/X11 platform and GPU backends

The Linux implementation was staged and built on the real x86-64 host at .99,
running kernel 7.0.0-27. The host's installed FreeBASIC 1.20.2 compiler was used
to bootstrap the modified repository compiler. The modified compiler then
compiled the gfxlib3 FreeBASIC tests against the repository archives. Its
verbose selection link contained `-lfbgfx3` and did not contain `-lfbgfx`.

The production archive build was:

```text
make -j4 gfxlib3 ENABLE_PIC=YesPlease
```

It produced all four Linux x86-64 variants after the final X11 input changes:

```text
libfbgfx3.a       365056 bytes
libfbgfx3pic.a    365112 bytes
libfbgfx3mt.a     371336 bytes
libfbgfx3mtpic.a  371840 bytes
```

The X11 adapter opens one Display connection for its visual, colormap, window,
and optional GLX context. GLX entry points are loaded from `libGL.so.1`. The
Vulkan runtime loads `libvulkan.so.1`, enables `VK_KHR_xlib_surface`, and creates
the Xlib surface from the same renderer-owned Display and Window. No Vulkan SDK
headers or GLX development header are required by the gfxlib3 source.

The strict Vulkan bootstrap opened and closed the Linux loader twice. Each run
reported loader version 1.4, one physical device, and compute queue family zero.
The strict X11 presentation test passed exact displayed quadrants for logical
depths 1, 2, 4, 8, 16, and 32. It repeated every present, resized the client
from 8 by 8 to 16 by 12, and passed ten consecutive complete runs.

That repetition exposed an Xlib WSI behavior that the first implementation did
not handle: an old 8 by 8 swapchain could remain usable after the client became
16 by 12, so no out-of-date result forced recreation. The platform poll now
compares the native client size with the live swapchain extent and explicitly
requests recreation. The ten-run resize result above was recorded after that
fix.

The combined strict infrastructure executable reported:

```text
gfxlib3 Vulkan smoke: device-local surfaces passed
gfxlib3 OpenGL smoke: OpenGL gfxlib3 backend initialized: 4.5 (Core Profile) Mesa 26.0.3-1ubuntu1
gfxlib3 infrastructure: all checks passed
```

The modified Linux compiler built and ran 22 normal and `-mt` FreeBASIC
executables through the null, OpenGL, and Vulkan paths. The matrix covered the
public API smoke, Vulkan API smoke, GPU-only surfaces, command compatibility,
images, graphical console hooks, and VGA port compatibility. Every executable
exited zero.

`tests/gfx3/input-x11-smoke.bas` separately passed normal and `-mt` builds
through both OpenGL and Vulkan. It verifies key state, mouse movement, X1
mapping, unknown-button rejection, vertical and horizontal wheels, focus and
close events, native Display/Window publication, and pointer-confinement release
and restoration across focus changes.

The host had no active display, so `xvfb` was temporarily installed for this
verification and tests used an isolated `:97` server. The OpenGL renderer was
Mesa llvmpipe. These results prove Linux compilation, Xlib/GLX/Vulkan WSI,
render-thread behavior, exact readback, exact displayed pixels, resize, and API
selection. They are not a discrete-GPU performance result. Fullscreen, Wayland,
multiple Vulkan submissions in flight, and a validation-layer run remain open.

## 2026-07-17 automatic selection and Android device

The backend plan now follows gfxlib2's ordered driver-list behavior. An
ordinary desktop mode tries Vulkan and then OpenGL 4.3. Android tries Vulkan
and then OpenGL ES 3.0. `FBGFX` and stored `SET_DRIVER_NAME` requests move a
known GPU backend to the front, unknown names retain the normal order, and the
existing `FB.GFX_OPENGL` and new `FB.GFX_VULKAN` flags remain force-only. The
null backend is never an automatic fallback.

The strict Windows infrastructure executable was rebuilt with the selector in
the linked gfxlib3 archive. It reported:

```text
gfxlib3 OpenGL partial context cleanup passed
gfxlib3 Vulkan smoke: device-local surfaces passed
gfxlib3 OpenGL smoke: OpenGL gfxlib3 backend initialized: 4.3.0 NVIDIA 595.79
gfxlib3 infrastructure: all checks passed
```

The public `renderer-selection-smoke.bas` program selected `Vulkan compute`
with no request, selected `OpenGL 4.3 compute` with `FBGFX=OpenGL`, and returned
to `Vulkan compute` for an unknown `FBGFX` value. Explicit `FB.GFX_OPENGL` and
`FB.GFX_VULKAN` runs selected the requested backends, including Vulkan flag
precedence over `FBGFX=OpenGL`. All five runs passed exact 32-bit PSET/POINT
readback and exited zero.

The Android test used the connected AGM A8, serial `b857d433`, Android API 24,
arm64-v8a, and a Qualcomm Adreno 306 OpenGL ES 3.0 driver dated 2016. Android
reported no Vulkan system feature or Vulkan loader. The APK was compiled with
the repository Android compiler and NDK 27.2.12479018, then installed and run
through ADB without setting `FBGFX` or a renderer flag.

The first integration runs exposed two Android-only defects. A dashed BASIC
source name was being changed before `-m`, which made the generated module run
as a constructor before NativeActivity supplied its window. The wrapper now
passes the original module name. The renderer also inherited Android's 16 KiB
minimum pthread stack and overflowed inside the older EGL driver while creating
its window surface. The renderer now requests a checked 4 MiB stack.

A later repeated-launch failure showed that the packaging helper chose
non-thread-safe runtime archives when the BASIC source contained no explicit
thread calls. gfxlib3 creates renderer and event-pump threads internally, so
the helper now always selects the multithreaded rtlib and gfxlib3 archives for
a gfxlib3 APK.

After those fixes, two consecutive launches automatically rejected Vulkan,
selected `OpenGL ES 3.0`, displayed the GPU-rendered pass frame, passed exact
alpha POINT and rectangle readback, exercised two logical GPU pages, and
reported:

```text
FREEBASIC_ANDROID_EXIT:0
```

The saved device frame visibly reads `GFX3 ANDROID PASS: OPENGL ES 3.0`.
The old Adreno driver emits informational messages for fullscreen draws which
use `gl_VertexID` without a vertex attribute. They do not correspond to a GL
error or failed assertion.

The first run also reported an unresolved Java keyboard-button native method.
gfxlib3 now exports that policy entry point, both NativeActivity/IME key JNI
routes, and the native key translator. While testing it, an Android source-mask
bug was found in the package input loop: all keyboard sources shared a low
class bit with gamepad sources and were incorrectly sent to the gamepad path.
The loop now compares complete source masks.

`android-input-smoke.bas` was installed on the same device. One launch received
ADB keycode 29 and a second received `input text a`. Both reached ordinary
FreeBASIC INKEY and reported:

```text
GFX3_ANDROID_INPUT_PASS a
FREEBASIC_ANDROID_EXIT:0
```

Neither run logged an unresolved native method. Rendering and input now cross
the real Android package boundary. Drawing an on-screen keyboard button and
explicitly showing or hiding the software keyboard remain open UI work.

## 2026-07-17 QB-only command sweep

The unchanged `tests/qb/gfxlib-command-sweep.bas` source was compiled with
`-lang qb` and gfxlib3 selected. Normal and `-mt` executables both exited zero
with `FBGFX=null`, then both exited zero again through automatic Vulkan. This
covers QB SCREEN mode syntax, SCREEN page changes, PSET, LINE/box, CIRCLE,
PAINT, DRAW, STICK, and STRIG.

Symbol inspection of the normal executable found `fb_gfx3_mode_init`,
`fb_GfxScreenQB`, `fb_GfxStickQB`, and `fb_GfxStrigQB`, with no gfxlib2 driver
initializer. The result therefore proves the QB-only calls use gfxlib3 rather
than a mixed-runtime fallback.

The unchanged source was also freshly packaged for Android arm64 with
`fbc-android -lang qb -gfx3`, installed on the connected AGM A8, and launched
without a renderer request. The device followed its normal Vulkan-to-GLES
selection path and completed the QB SCREEN, STICK, STRIG, primitive, PAINT,
DRAW, and shutdown sequence with:

```text
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-17 repeated mode lifecycle

`tests/gfx3/mode-lifecycle-smoke.bas` uses only the public FreeBASIC graphics
API. Every iteration varies the mode dimensions and GPU page count, performs
exact 32-bit PSET/POINT readback, and alternates active-mode replacement with
two consecutive `SCREEN 0` calls. Every fourth iteration submits an invalid
zero-width request and then proves the existing valid mode still renders.

The Win32 executable completed these runs with exit status zero:

```text
gfxlib3 mode lifecycle: Null (1000 opens)
gfxlib3 mode lifecycle: Vulkan compute (32 opens)
gfxlib3 mode lifecycle: OpenGL 4.3 compute (24 opens)
gfxlib3 mode lifecycle: Vulkan compute (24 opens)
```

The same source was packaged for the connected API 24 AGM A8 without a
renderer request. Two consecutive installs-and-launches stayed in one
NativeActivity process for all opens, automatically selected the OpenGL ES
fallback on every mode, and reported:

```text
ANDROID RUN 1
gfxlib3 mode lifecycle: OpenGL ES 3.0 (32 opens)
FREEBASIC_ANDROID_EXIT:0
ANDROID RUN 2
gfxlib3 mode lifecycle: OpenGL ES 3.0 (32 opens)
FREEBASIC_ANDROID_EXIT:0
```

This closes bounded public mode replacement and teardown coverage on desktop
and Android. Exhaustive injected failure at every allocation, shader, device,
context, and native-window initialization stage remains open.

## 2026-07-17 Android cold lifecycle stress

`tests/gfx3/android-lifecycle-stress.bas` varies dimensions and page counts,
performs exact PSET/POINT verification, and alternates replacement with double
`SCREEN 0` teardown for 256 public GPU modes. A freshly packaged ARM64 gfxlib3
APK ran on the connected AGM A8 twice as separate cold NativeActivity launches.
Both physical GLES fallback processes completed with:

```text
GFX3_ANDROID_LIFECYCLE_STRESS_PASS OpenGL ES 3.0 (256 opens)
FREEBASIC_ANDROID_EXIT:0
```

This verifies 512 physical public mode opens across two Android process
lifecycle transitions. Android Vulkan remains hardware-validation work because
the connected device exposes no Vulkan feature.

A later live ADB capability check still reports attached serial `b857d433` as
model `A8`, API 24, with `GLES: Qualcomm, Adreno (TM) 306, OpenGL ES 3.0`.
`SurfaceFlinger` reports no Vulkan capability. This remains physical GLES
fallback coverage, not Android Vulkan hardware coverage.

## 2026-07-17 alpha primitive compatibility and Android GLES

gfxlib3 now carries `FB.GFX_ALPHA_PRIMITIVES` through the SCREEN flag and
`SCREENCONTROL` GET/SET controls. The implementation uses the exact gfxlib2
32-bit primitive formula: source alpha is a 0 through 255 factor divided by
256, RGB is blended against the destination, and the source alpha byte is
stored unchanged. Opaque colors and non-32-bit targets remain solid writes.
This is intentionally distinct from `PUT ALPHA`.

The focused public test is `tests/gfx3/alpha-primitives-smoke.bas`. Compiled
once against gfxlib2 it exited zero through OpenGL. The gfxlib3 build exited
zero through the Null backend, explicit Vulkan, and automatic Vulkan. The
same source was then cross-compiled with NDK 27.2.12479018 into an API 24
arm64 APK, installed on the connected AGM A8, and run through its actual
NativeActivity/EGL path. The device selected the expected renderer and
reported:

```text
gfxlib alpha primitives: OpenGL ES 3.0
FREEBASIC_ANDROID_EXIT:0
```

The Android archive build used `-Wall -Wextra` plus the repository's implicit
declaration and format-security warning gates. It compiled the GLES shader
backend, installed both PIC gfxlib3 archives into the package runtime, and
did not rely on the Windows OpenGL implementation.

The first desktop OpenGL run exposed a readback hazard: after `POINT` attached
the screen texture to the reusable read framebuffer, a later GPU-only-surface
BF could leave its target unchanged. The clear dispatch now detaches that
read framebuffer before binding a writable image. The focused alpha smoke now
also exits zero through explicit OpenGL 4.3. A staged public regression test
then passed every setup stage from 0 through 6, including the complete
PSET/POINT, LINE, BF, PAINT, and CPU-image sequence before the independent
GPU-surface BF.

## 2026-07-17 CIRCLE, ellipse, and arc conformance fixture

`tests/gfx3/circle-compat-smoke.bas` draws a deterministic 96 by 80 opaque
fixture containing full circles, aspect 0.5 and 2.0 ellipses, filled circles,
and ordinary and wrapped arcs. It hashes every logical pixel and requires the
gfx2-derived value `6BDC39D7`.

The identical source passed through gfxlib2 OpenGL and gfxlib3 Null, OpenGL
4.3 compute, and Vulkan compute. Every run reported the same hash and exited
zero. This provides focused public verification for representative CIRCLE
geometry while randomized geometry remains future coverage.

The first physical Android GLES run exposed the old implicit-distance fragment
approximation and produced `A7E8B8F5`. GLES full ellipses now drive the same
midpoint scanline sequence as the reference backend while retaining fragment
pipeline rasterization. After rebuilding the arm64 archives with NDK
27.2.12479018 and reinstalling the APK on the AGM A8, the real EGL/Adreno run
reported:

```text
gfxlib circle fixture hash 6BDC39D7
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-17 custom `DRAW STRING` font compatibility and Android GLES

`tests/gfx3/custom-font-smoke.bas` constructs the public custom-font image
format directly: a byte header for A through B, two three-pixel glyphs, and
the 32-bit magenta TRANS key. It verifies that glyph pixels retain their raw
font colors, TRANS leaves the masked interior unchanged, PSET copies that
mask, and an unsupported character advances by the font height without
painting a pixel in both TRANS and PSET on screen and CPU image targets. Under
gfxlib3 it also exercises an opaque GPU-only surface.

The gfxlib2 baseline was compiled and run through the Linux FreeBASIC 1.20.2
Null driver on `.99`. It reported the following hashes and exited zero:

```text
gfxlib custom font hashes B1C32E2D 5EDE6DDD
DOT99_RUN_EXIT=0
```

The initial gfxlib3 run exposed an unsupported-character defect: its assembled
temporary image left the spacing region zero-filled. In 32-bit TRANS, gfxlib2
uses RGB magenta, not zero, as the mask, so zero overwrote the target. The
assembler now initializes synthetic TRANS pixels with the exact 8/16/32-bit
gfxlib2 key. Non-TRANS strings with unsupported characters instead submit only
their contiguous supported glyph runs at the resolved pixel offsets, so PSET
and every other PUT mode preserve gfxlib2's true no-write gap. The public
fixture then passed through gfxlib3 Null, forced OpenGL, forced Vulkan, and
automatic selection with the same two hashes.

The Android arm64 PIC and multithreaded PIC archives were rebuilt with NDK
27.2.12479018's `aarch64-linux-android24-clang`, checked as AArch64 ELF, and
packaged into a fresh API 24 APK. The connected AGM A8 selected OpenGL ES 3.0
through automatic selection and logged:

```text
gfxlib custom font hashes B1C32E2D 5EDE6DDD
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-17 native-depth custom-font conformance and Android automatic selection

`tests/gfx3/custom-font-depth-smoke.bas` extends the preceding 32-bit
screen/image/GPU-surface fixture with a deliberately small screen-only font
at each native public screen depth: indexed 8-bit, RGB565 16-bit, and 32-bit
RGB. The test writes the public version-0 A/B header directly, keeps a masked
pixel inside A, fills B, and checks TRANS masking, PSET mask copying, and the
historical font-height advance for unsupported X. The 16-bit expected colors
use gfxlib2's POINT expansion of packed RGB565 storage to RGB888.

The unchanged source first passed the gfxlib2 Null baseline. It then passed
gfxlib3 Null, forced OpenGL, forced Vulkan, OpenGL `-mt`, and Vulkan `-mt` on
Win64. The same source was packaged as a fresh API 24 arm64 NativeActivity APK
with no forced backend request, installed on the connected AGM A8, and run
through the automatic backend policy. Its physical-device log contained:

```text
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-17 custom-font CUSTOM blender conformance

`tests/gfx3/custom-font-custom-blender-depth-smoke.bas` constructs a
version-0, one-glyph custom font at 8, 16, and 32 bits and draws `AXA` through
`DRAW STRING ... CUSTOM`. Its callback returns a controlled public colour and
counts calls. Each A is three by four pixels, so both supported glyphs require
24 callback invocations. X is unsupported, advances by the four-pixel font
height, and must leave its destination gap unchanged. This catches both a
mistaken transparent copy in place of CUSTOM and a synthetic unsupported-glyph
write.

The fixture first passed unchanged through gfxlib2 Null. It then passed
gfxlib3 Null, forced OpenGL, forced Vulkan, OpenGL `-mt`, and Vulkan `-mt` on
Win64. The gfxlib3 cases cover screen, CPU `FB.IMAGE`, and opaque GPU-surface
targets. The 16-bit assertion records the gfxlib2 rule that a BASIC callback
returns a public RGB value, which is converted to RGB565 before POINT expands
the stored pixel.

The same automatic-selection arm64 APK was installed and run on the connected
AGM A8. Its physical Android GLES run ended with:

```text
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-17 QB SCREEN page compatibility and Android automatic selection

`tests/gfx3/qb-screen-smoke.bas` opens QB SCREEN 7 with the third SCREEN
argument set to one, writes page one, selects page zero through the public
`fb_GfxPageSet` ABI, and proves page zero was not modified. It then writes
page zero and verifies both independent page values after explicit page
switches. This is a focused check of gfxlib2's historical QB wrapper behavior:
the `(visible, active)` arguments received by `fb_GfxScreenQB` are forwarded
to the generic page hook in that order, where they become `(active, visible)`.

The gfxlib2 baseline compiled and ran on `.99` with FreeBASIC 1.20.2-12 and
exited zero. The gfxlib3 source then exited zero through Null, forced OpenGL,
and forced Vulkan on Win64. The connected API 24 arm64 AGM A8 package was
built from the same QB source with no `FBGFX` request, installed, and launched
through NativeActivity. Its automatic Vulkan-to-GLES policy completed the
fixture and logged `FREEBASIC_ANDROID_EXIT:0`. The current archive rebuild was
also checked with `renderer-selection-smoke.bas` on that same device; it
reported the selected automatic renderer explicitly:

```text
gfxlib3 automatic renderer: OpenGL ES 3.0
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-17 graphical console 8 by 14 and 8 by 16 compatibility

gfxlib3 now decodes all three canonical FreeBASIC graphical-console fonts from
the shared generated data: 8 by 8, 8 by 14, and 8 by 16. Standard SCREEN modes
carry their gfxlib2 font selection into mode initialization. WIDTH derives a
fixed 8-pixel font width and accepts the same three resulting heights as
gfxlib2, replacing the cell grid, resetting the graphical/text view, and
clearing the current GPU page.

`tests/gfx3/console-font-smoke.bas` checks SCREEN 9's 80 by 25 EGA grid,
SCREEN 11's 80 by 30 VGA grid, an 8 by 8 WIDTH switch, restoration to 8 by 16,
an invalid WIDTH request that must leave the grid unchanged, and LOCATE plus
SCREEN(row, column) character reads. The gfxlib2 baseline compiled and ran on
`.99` with FreeBASIC 1.20.2-12 and reported:

```text
gfxlib console font grids PASS
DOT99_RUN_EXIT=0
```

The same source passed gfxlib3 Null, forced OpenGL, forced Vulkan, automatic
selection, and `-mt` OpenGL on Win64. Both ARM64 PIC archives were rebuilt with
NDK 27.2.12479018, packaged into an API 24 APK, installed on the connected AGM
A8, and executed through its real NativeActivity/EGL path. Automatic selection
used its OpenGL ES fallback and logged:

```text
gfxlib console font grids PASS
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-17 raw BLOAD GPU-page route and file-API linkage

`tests/gfx3/bload-raw-smoke.bas` proves both kinds of raw FreeBASIC block:
a seven-byte explicit memory round trip and a 16 by 16 32-bit screen-page
round trip. The latter requires a renderer download before BSAVE, a clear, an
upload after BLOAD, and exact alpha-bearing POINT reads. It passed gfxlib3
Null, forced OpenGL, forced Vulkan, automatic selection, and `-mt` OpenGL.

The fixture exposed a hidden Win64 linkage problem: the previously unexercised
file API pulled an incompatible imported `snprintf` symbol when BLOAD/BSAVE was
linked through the UCRT compiler. gfxlib3 now performs a bounded copy from the
FreeBASIC string before runtime path conversion, removing that dependency.

The ARM64 PIC archives were rebuilt and the APK was run on the connected AGM
A8 through automatic OpenGL ES selection:

```text
gfxlib raw bload PASS
FREEBASIC_ANDROID_EXIT:0
```

The `.99` gfxlib2 baseline verified the explicit raw-memory contract cannot
currently serve as a screen-raw oracle: the host's Null driver faults while
restoring its own raw screen dump. BMP round-trip coverage remains separately
verified by `command-compat-smoke.bas`; OS/2, RLE, and BMP bitfield parsing are
still open parity work.

## 2026-07-17 BMP RGB565/RGBA bitfields BLOAD and Android GLES

gfxlib3 BLOAD now accepts Windows `BI_BITFIELDS` BMPs with checked contiguous,
non-overlapping RGB masks. The loader accepts 16-bit and 32-bit source pixels,
normalizes each mask field to 8-bit RGB, and uses a fourth 32-bit alpha mask
when it is supplied before the pixel array. The normalization multiply is
64-bit so an all-ones source field cannot wrap before conversion to 8-bit.

`tests/gfx3/bload-bitfields-smoke.bas` writes bounded one-pixel RGB565 and
RGBA BMPs. The latter uses the fourth alpha mask after a 40-byte information
header and asserts exact RGBA(64,32,16,128) output. The fixture passed through
desktop Null, forced OpenGL, forced Vulkan, automatic selection, and `-mt`
OpenGL after all four Win64 gfxlib3 archives rebuilt. The ARM64 PIC archives
were then rebuilt with NDK 27.2.12479018 and packaged for the connected API 24
AGM A8. Its real NativeActivity/GLES run logged:

```text
gfxlib bitfields bload PASS
FREEBASIC_ANDROID_EXIT:0
```

OS/2 BMP headers, RLE, non-contiguous masks, and a larger externally produced
BMP corpus remain open compatibility coverage.

## 2026-07-17 compiler graphics ABI prototype sweep

`tests/warnings/rtl-prototypes.bas` was compiled with the gfxlib3 selection
define in its supported default `-lang fb` mode and exited zero. Its generated
prototype list includes the public gfxlib3-linked declarations for SCREEN,
SCREENRES, BLOAD, BSAVE, FLIP, page selection, lock/unlock, input, image, and
query APIs. The same source cannot be used as a QB compiler gate because its
earlier generic variadic descriptor declarations are rejected in that dialect;
this occurs before graphics prototypes are reached and is documented as a
harness limitation, not a gfxlib3 failure.

## 2026-07-17 OS/2 V1 BITMAPCOREHEADER BLOAD

The BMP loader now recognizes the 12-byte OS/2 V1 core header in addition to
Windows information headers. It validates positive unsigned dimensions and
one plane, accepts uncompressed 1/4/8-bit indexed and 24-bit RGB sources, and
uses OS/2's three-byte BGR palette entries. The source-pixel path handles
packed one-bit and high-nibble four-bit indexes before the existing image
conversion and GPU upload barrier.

`tests/gfx3/bload-os2-core-smoke.bas` writes a bounded 4-bit one-pixel core
file with a 16-entry BGR palette and asserts exact RGB(128,64,32) output. It
passed Null, forced OpenGL, forced Vulkan, automatic selection, and `-mt`
OpenGL on Windows. The rebuilt ARM64 PIC runtime was packaged and launched on
the connected API 24 AGM A8 under automatic GLES, logging:

```text
gfxlib os2 core bload PASS
FREEBASIC_ANDROID_EXIT:0
```

Later OS/2 headers and non-Windows RLE variants remain explicitly unsupported.

## 2026-07-17 Windows 1-bit and 4-bit indexed BLOAD

The Windows BMP loader now accepts uncompressed 1-bit and 4-bit information
headers alongside the existing 8-bit indexed path. The palette count defaults
to the source depth rather than assuming 256 entries, validates against the
pixel offset, and extracts packed high-bit/high-nibble indexes before the
normal palette conversion and GPU upload barrier.

`tests/gfx3/bload-indexed-smoke.bas` writes both formats. It verifies a red
high-bit 1-bit pixel followed by black, then an exact RGB(128,64,32) high-
nibble 4-bit pixel. The fixture passed Null, forced OpenGL, forced Vulkan,
automatic selection, and `-mt` OpenGL. After rebuilding the ARM64 PIC archive,
the connected API 24 AGM A8 passed it through automatic GLES:

```text
gfxlib indexed bload PASS
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-17 bounded Windows RLE4/RLE8 BLOAD

The BLOAD parser now accepts Windows RLE4 and RLE8 indexed BMP streams. It
decodes into a checked width by height palette-index buffer and rejects
truncated records, coordinates outside the image, invalid run lengths, and
invalid deltas before any palette conversion or GPU upload occurs. Encoded,
absolute, end-of-line, end-of-bitmap, and delta records are handled; top-down
and OS/2 RLE sources remain rejected.

`tests/gfx3/bload-rle-smoke.bas` writes a two-row RLE8 image containing an
encoded bottom-row run split by a delta move and an absolute top-row sequence,
plus an RLE4 image with packed absolute indexes. It also requires BLOAD to
reject a two-pixel image with a three-pixel encoded run. It passed Null,
forced OpenGL, forced Vulkan, automatic selection, and `-mt` OpenGL. The
rebuilt ARM64 PIC archive also
passed on the connected API 24 AGM A8 through automatic GLES:

```text
gfxlib rle bload PASS
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-17 standard SCREENLIST compatibility iterator

`fb_GfxScreenList` no longer returns an unrelated five-mode placeholder. It
retains gfxlib2's positive-depth restart and zero-depth resume contract while
providing a sorted standard `SCREEN` fallback with no window or GPU required.
Later verification below adds Win32 physical display enumeration before that
fallback.

The focused iterator fixture checks packed ordering, restart/resume behavior,
and invalid-depth reset behavior. It remains the deterministic fallback proof
for hosts that do not expose a native mode enumerator.

## 2026-07-17 Android native touch snapshots

gfxlib3 now retains up to 16 Android motion contacts in its synchronized input
state, preserving each platform pointer ID and replacing the complete snapshot
on every callback. Pointer-up and cancel events therefore remove contacts
before the public query can observe them, while the first active contact still
updates the historical left-mouse compatibility state. Desktop platforms retain
their existing focused-left-button fallback.

`tests/gfx3/android-touch-smoke.bas` opens the automatic renderer and waits
for a held device contact. On the connected API 24 AGM A8, the test observed a
native GETTOUCH contact with in-range coordinates, a non-negative ID, and a
successful rectangular GETTOUCHHIT, then exited normally:

```text
GFX3_ANDROID_TOUCH_PASS
FREEBASIC_ANDROID_EXIT:0
```

The adapter is bounded to 16 contacts. A physical multi-contact gesture is
still recorded as a manual device-matrix check rather than inferred from a
single-contact ADB injection.

## 2026-07-17 legacy 15-bit and 24-bit SCREENRES normalization

gfxlib3 now accepts the two legacy true-colour depth spellings that gfxlib2
normalizes internally. A request for depth 15 becomes a 16-bit RGB565 mode;
depth 24 becomes a 32-bit mode. The normalization happens before mode creation
so SCREENINFO, page surfaces, locks, readback, and presentation all use one
actual storage depth.

`tests/gfx3/depth-normalization-smoke.bas` first established the gfxlib2
baseline on `.99` with its GFX_NULL driver, then verified gfxlib3 through Null,
forced OpenGL, forced Vulkan, and automatic desktop selection. Each run checks
the SCREENINFO depth and byte size plus 16-bit red and 32-bit direct POINT
readback. The Android ARM64 archive was rebuilt and the physical API 24 AGM A8
passed the same fixture through automatic OpenGL ES:

```text
GFX_DEPTH_NORMALIZATION_PASS
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-17 indexed BMP BSAVE palette order and format coverage

The indexed BSAVE writer had emitted gfxlib's in-memory RGB palette bytes in
that order. Windows BMP stores palette entries as BGR plus a reserved byte, so
an indexed BMP round trip swapped red and blue. The writer now explicitly
serializes blue, green, red, and reserved while leaving the shared palette
representation unchanged.

`tests/gfx3/bsave-format-smoke.bas` writes GPU-backed indexed, RGB565, and
32-bit screens. It verifies indexed palette order, RGB565's expected 24-bit
expansion, and the public 24-bit BSAVE override removing the 32-bit alpha byte
by loading every file into a 32-bit GPU page. It passed Null, forced OpenGL,
forced Vulkan, automatic selection, and `-mt` Null. The ARM64 runtime was
rebuilt and the connected API 24 AGM A8 passed through automatic OpenGL ES:

```text
GFX_BSAVE_FORMAT_PASS
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-17 indexed and RGB565 graphical-console reads

The public `SCREEN(row, column[, colourflag])` hook must not expose an
implementation-specific text colour representation. In indexed modes gfxlib2
returns the foreground and background indexes packed into the low and next
byte. In RGB565 modes it returns the requested foreground or background as an
expanded opaque RGB value. gfxlib3 already used those representations; the
new focused test makes both cases an explicit compatibility gate.

`tests/gfx3/console-depth-smoke.bas` writes one cell in each mode, validates
the character and colour results, and passed Null, forced OpenGL, forced
Vulkan, automatic selection, and `-mt` Null. The connected API 24 AGM A8 also
passed under automatic OpenGL ES:

```text
GFX_CONSOLE_DEPTH_PASS
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-17 compiler -gfx3 option

The compiler previously understood `-gfx3` only as object metadata. The
command-line parser did not accept it, so project-wide gfxlib3 selection still
required manually adding the source define to every graphics module. The option
now selects the gfxlib3 link mode and adds the same global
`__FB_GFXLIB3__` define used by `fbgfx.bi`.

`tests/gfx3/compiler-gfx3-option-smoke.bas` intentionally contains no source
define. It was compiled with `-gfx3`; its probe verifies both the injected
define and the gfxlib3-only `FB.GFX_VULKAN` constant. A separate, ordinary
main module was then linked without `-gfx3`. The verbose link line contained
`-lfbgfx3` and no gfxlib2 archive, and the program printed:

```text
GFX3_OPTION_PASS
```

The same separate-compilation check with `-mt` selected `-lfbgfx3mt` and also
printed `GFX3_OPTION_PASS`.

The supported `-lang fb` form of `tests/warnings/rtl-prototypes.bas` also
compiled with `-gfx3` and no ABI errors. Its gfx section listed the expected
public SCREEN, BLOAD/BSAVE, page, lock, input, image, query, and built-in PUT
entry points.

## 2026-07-17 repeatable public archive ABI audit

`tests/gfx3/audit-public-exports.ps1` reads the current gfxlib2 `fb_gfx.h`,
extracts every public `fb_Gfx*` and compiler-selected `fb_hPut*` declaration,
then also extracts the non-FBCALL runtime graphics hooks installed after a
mode opens. It compares the combined set with `nm -g --defined-only` output
from the built Win64 archives. It refuses a suspiciously short parse and also
requires gfxlib2 to define the same set, so a malformed header scan cannot
silently make gfxlib3 look complete.

The current Win64 run used the UCRT64 GNU `nm` and reported:

```text
Public graphics declarations: 59
Runtime graphics hooks: 29
Required graphics archive symbols: 88
GFX3_PUBLIC_EXPORT_AUDIT_PASS
```

The same script then inspected the exact Android arm64 threaded-PIC archives
staged for physical APK packaging, `libfbgfxmtpic.a` and `libfbgfx3mtpic.a`.
It again found all 88 public and runtime-hook symbols and reported the same
pass marker.

## 2026-07-17 Android compiler -gfx3 option

`fbc-android` previously inferred its required multithreaded gfxlib3 archive
only from a visible source define or a `-d __FB_GFXLIB3__` argument. It now
also recognizes `-gfx3`. The bundled Android cross compiler used for this run
does not itself parse that newer option, so the wrapper translates it to a
private marker and overlays the checkout's `fbgfx.bi` and `fbgfx3.bi` over the
target include tree. `fbgfx.bi` maps that marker to the public define only if
the program has not already made a source-level selection.

`tests/gfx3/android-gfx3-option-smoke.bas` deliberately contains no source
define. It was freshly packaged for Android ARM64 with `fbc-android -gfx3` and
run on the connected API 24 AGM A8. The public selector program was also
packaged and run using only its existing `#define __FB_GFXLIB3__`. Both forms
reported `OpenGL ES 3.0`; Vulkan was unavailable, so automatic selection
correctly reached GLES:

```text
GFX3_ANDROID_OPTION_PASS OpenGL ES 3.0
FREEBASIC_ANDROID_EXIT:0
```

The companion `tests/gfx3/android-gfx3-extension-header-smoke.bas` contains no
source define and no compiler option. Its direct `#include once "fbgfx3.bi"`
must make the wrapper choose gfxlib3, expose the GPU-surface declarations, and
link the threaded archive. The freshly packaged physical run created, cleared,
read, and destroyed an opaque 32-bit surface before reporting:

```text
GFX3_ANDROID_EXTENSION_HEADER_PASS OpenGL ES 3.0
FREEBASIC_ANDROID_EXIT:0
```

The complementary `tests/gfx3/android-gfx2-default-smoke.bas` includes only
the ordinary `fbgfx.bi`. It rejects any accidental gfxlib3 marker, opens a
gfxlib2 GFX_NULL mode, and verifies a PSET/POINT round trip. The freshly
packaged physical Android run reported:

```text
GFX2_ANDROID_DEFAULT_PASS
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-17 render-thread OpenGL/GLES interop

gfxlib3 keeps the live OpenGL context on its renderer thread, so returning a
raw `SCREENGLPROC` pointer to the BASIC application thread would violate its
ordering and ownership rules. `FB.Gfx3RunOnRenderThread` now queues a
synchronous callback behind previous work and exposes `SCREENGLPROC` only for
that callback's lifetime. The resolver is implemented by the desktop WGL/GLX
or Android EGL platform adapter, not by a hard-coded GL library assumption.

`tests/gfx3/gl-interop-smoke.bas` verifies that `SCREENGLPROC("glGetString")`
is NULL before and after the callback, then resolves and calls `glGetString`
inside it. It also opens a Null mode and verifies that an interop request is
rejected without entering the callback or breaking orderly mode shutdown. The
desktop OpenGL `-mt` run printed:

```text
GFX3_GL_INTEROP_PASS
```

The same fixture also passed the normal archive variant after the public
extension header selected gfxlib3 itself.

The Android ARM64 gfxlib3 PIC archives were rebuilt directly with the NDK 27
aarch64 API 26 toolchain and hash-copied into the existing device-test runtime.
The test was packaged with `fbc-android -gfx3`, installed on the connected API
24 AGM A8, and completed through automatic OpenGL ES 3.0 selection:

```text
GFX3_GL_INTEROP_PASS
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-17 Win32 XInput controller bridge

The Win32 platform adapter now lazily loads the XInput DLL supplied by the
host, then polls its four standard controller slots on the renderer thread.
It publishes normalized sticks, triggers, buttons, and d-pad state through the
same locked snapshot used by Android. A seen slot remains available after a
disconnect so GETXPAD returns `XPAD_STATUS_DISCONNECTED` instead of treating
an unplugged controller as never present.

The strict infrastructure executable was rebuilt with the lifecycle test for
missing, connected, and disconnected snapshots and passed. The public
`tests/gfx3/xinput-fallback-smoke.bas` fixture was then run in an automatic GPU
mode with no controller attached against both normal and `-mt` archives; both
completed the render-thread poll and printed `GFX3_XINPUT_FALLBACK_PASS`. The
unchanged gfxlib2 `getjoystick.bas` and `getxpad.bas` tests also passed with
gfxlib3 selected. This is no-controller verification only; a physical Windows
controller matrix remains pending.

## 2026-07-17 Android physical-device recheck

After the shared controller-state update, the Android ARM64 gfxlib3 normal and
multithreaded PIC archives were rebuilt with NDK 27. The current interop smoke
was packaged with `fbc-android -gfx3`, installed on the connected API 24 AGM
A8, and run through automatic renderer selection. The device selected OpenGL
ES 3.0 and exited cleanly:

```text
GFX3_GL_INTEROP_PASS
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-17 broader public-command and reopen checks

The portable `tests/macos/gfxlib-command-sweep.bas` was compiled unchanged for
Win64 with `-gfx3` and completed with status zero. A current uninstalled-tree
rerun used `bin\\fbc.exe -gfx3 -i .\\inc`; the explicit include directory is
needed only because this compiler has not been installed beside its include
files. The public `-gfx3` selection remains the mechanism under test. It is an
independent public command fixture that covers mode/page control, primitives,
CPU images, all standard PUT modes, BLOAD/BSAVE, palette, input fallbacks, and
teardown.

`tests/interactive/screen-opengl-reopen.bas` was also compiled unchanged with
`-gfx3`. It created a real 1024 by 768 OpenGL mode, replaced it with an 800 by
600 OpenGL mode, and exited zero. This adds a direct public check for GPU-mode
replacement outside the focused gfxlib3 lifecycle fixture.

## 2026-07-17 Windows V3/V4/V5 BMP header parity

gfxlib2 accepts the 56-byte Windows V3, 108-byte V4, and 124-byte V5 BMP
headers in addition to the original 40-byte information header. gfxlib3's
existing extended-header bitfield parser was given a focused public proof:
`tests/gfx3/bload-v4v5-bitfields-smoke.bas` writes one-pixel RGBA bitfield
files in all three forms and checks exact `RGBA(64,32,16,128)` output.

The unchanged fixture passed against gfxlib2 and gfxlib3 Null, forced OpenGL,
forced Vulkan, and automatic `-mt` desktop modes. It was then packaged with
`fbc-android -gfx3`, installed on the connected AGM A8, and passed through
automatic OpenGL ES 3.0 selection:

```text
gfxlib v3/v4/v5 bitfields bload PASS
FREEBASIC_ANDROID_EXIT:0
```

This closes the BMP-header compatibility family gfxlib2 implements. OS/2 V2
and other compression formats remain intentionally unsupported extensions in
both runtimes.

## 2026-07-17 Win32 WinMM GETJOYSTICK parity

gfxlib2 keeps WinMM `GETJOYSTICK` polling separate from XInput `GETXPAD`.
gfxlib3 now does the same. Its new adapter loads `winmm.dll` once on the first
joystick query, dynamically resolves `joyGetDevCapsA` and `joyGetPosEx`, and
handles the same sixteen slots, six optional axes, buttons, and POV conversion
as gfxlib2. Missing devices retain buttons `-1`, axes `-1000`, and the normal
illegal-function result. XInput remains a render-thread snapshot for GETXPAD.

All four Win64 gfxlib3 archives rebuilt. The unchanged `getjoystick.bas` test
passed in normal and `-mt` forms; the normal and `-mt` real GPU-mode fallback
fixture each printed `GFX3_XINPUT_FALLBACK_PASS`. QB `STICK`/`STRIG` fallback
also passed through the new GETJOYSTICK route. The ARM64 Android normal and
multithreaded PIC archives rebuilt with the non-WinMM stub, and the connected
AGM A8 passed the installed gamepad smoke:

```text
GFX3_ANDROID_GAMEPAD_PASS joystick=1 xpad=0
FREEBASIC_ANDROID_EXIT:0
```

No physical WinMM or XInput controller was attached for an axis/button matrix.

## 2026-07-17 Win32 physical SCREENLIST modes

gfxlib3 now mirrors gfxlib2's Win32 display-mode behavior before using its
standard-mode fallback. A small platform boundary calls `EnumDisplaySettings`,
accepts the same 15/16-bit and 24/32-bit equivalent depths, bounds packed
dimensions, sorts, and removes duplicate resolutions. The public iterator owns
that returned list under the graphics lock and falls back only when the desktop
has no matching mode or native enumeration is unavailable.

All four Win64 archive variants rebuilt. The updated generic iterator smoke
passed, and `screenlist-win32-smoke.bas` compared the public result against the
current desktop and printed:

```text
GFX3_SCREENLIST_WIN32_PASS 1366x768x32
```

The rebuilt ARM64 archives were installed on the connected AGM A8. Android
uses the deterministic fallback and its packaged iterator smoke passed with
`gfxlib screenlist PASS` and `FREEBASIC_ANDROID_EXIT:0`.

## 2026-07-17 X11 RandR SCREENLIST implementation

gfxlib3 now matches gfxlib2's X11 mode-discovery boundary. It opens a temporary
X display and dynamically loads the three stable RandR 1.0 entry points needed
to obtain `XRRConfigSizes`. The implementation accepts gfxlib2's 8, 15, 16,
24, and 32-bit request family, applies the same depth aliases, bounds packed
dimensions, sorts, and removes duplicate screen sizes. If X11, RandR, or a
display is unavailable it returns unsupported so the existing public standard
mode fallback remains authoritative.

All four Win64 gfxlib3 archives rebuilt after the shared collector refactor,
and the normal and multithreaded public iterator smokes each printed
`gfxlib screenlist PASS`. `screenlist-x11-smoke.bas` requires the current
root-window dimensions exactly once in the public 32-bit list. The staged Linux
public `-gfx3` compiler now runs it under Xvfb in both normal and `-mt` forms,
as recorded below.

## 2026-07-17 X11 RandR live boundary regression

The supplied Linux host at `.99` now has an isolated `xvfb` display for
noninteractive X11 verification. `tests/gfx3/x11-screenlist-live.c` was
compiled with `-std=gnu11 -Wall -Wextra -Werror` together with the checkout's
current `gfx3_screenlist.c`, then run through `xvfb-run`. It dynamically loaded
the host's actual `libXrandr`, required native 8-, 16-, and 32-bit mode lists,
checked 15/16 and 24/32 aliases, sorted uniqueness, and clean unsupported-depth
results:

```text
gfxlib3 X11/RandR screenlist passed
```

The first live run exposed a shutdown crash. `XRRGetScreenInfo()` registers
RandR extension cleanup with the temporary Xlib `Display`; unloading
`libXrandr` before `XCloseDisplay()` left Xlib with a stale callback. The
cleanup order now frees the RandR configuration, closes the Display, and only
then unloads the optional library. The corrected strict run passes. This proves
the native boundary and its unload order.

The same isolated source tree built all four Linux gfxlib3 archives and its
public compiler. Its verbose normal link line selected `-lfbgfx3` and did not
select gfxlib2. The existing public X11 fixture then opened an OpenGL mode,
obtained the Xvfb root size through `SCREENCONTROL`, and found that size exactly
once through the public iterator in both normal and `-mt` programs:

```text
GFX3_SCREENLIST_X11_PASS 1280x1024
GFX3_SCREENLIST_X11_PASS 1280x1024
```

## 2026-07-17 Win32 borderless fullscreen

The platform configuration now carries `GFX_FULLSCREEN` and `GFX_NO_FRAME`
through Vulkan and OpenGL creation. Win32 fullscreen uses a borderless primary
monitor window rather than changing the persistent desktop display mode.
`fullscreen-win32-smoke.bas` can force either OpenGL or Vulkan, checks
`WS_POPUP`, and confirms the native window exactly matches the monitor
rectangle before reporting the backend.

The smoke now also opens a `GFX_NO_FRAME` mode and verifies its popup client
area remains exactly 96 by 64 pixels. The multithreaded `-gfx3` run passed
both checks on Win64 with forced OpenGL and forced Vulkan.

## 2026-07-17 X11 borderless fullscreen implementation

The X11 adapter now carries `GFX_FULLSCREEN` and `GFX_NO_FRAME` through both
the Vulkan-window and GLX-window paths. Before mapping either kind of
borderless window it writes the conventional `_MOTIF_WM_HINTS` decoration hint.
For fullscreen it uses the current root dimensions as a fallback and sends the
standard `_NET_WM_STATE` / `_NET_WM_STATE_FULLSCREEN` EWMH request after map.
This deliberately does not use XRandR or otherwise change the user's desktop
mode.

`tests/gfx3/fullscreen-x11-smoke.bas` opens fullscreen and no-frame modes,
then verifies their X11 client dimensions through a separate `Display`
connection. It can force either OpenGL or Vulkan. On 2026-07-17 the test was
compiled with the staged public `-gfx3` compiler on `.99` and run under the
host's isolated Xvfb display. Normal and `-mt` forms of both forced backends
completed their fullscreen root-size and no-frame 96 by 64 client-size checks:

```text
GFX3_FULLSCREEN_X11_PASS OpenGL
GFX3_FULLSCREEN_X11_PASS Vulkan
GFX3_FULLSCREEN_X11_PASS OpenGL
GFX3_FULLSCREEN_X11_PASS Vulkan
```

The same fixture then ran under an isolated Xvfb display managed by Openbox,
an EWMH-capable window manager. Normal and `-mt` OpenGL/Vulkan runs again
reported the two backend pass markers. This verifies that the public root-size
and no-frame client-size behavior survives window-manager ownership without
changing XRandR. The fixture intentionally does not prescribe visual decoration
style, so no theme-specific assertion is claimed.

## 2026-07-17 Android rebuilt-archive GL interop

The ARM64 normal and multithreaded PIC archives were rebuilt after the window
configuration changes and the multithreaded archive was staged by matching its
SHA-256 to the packaged Android runtime copy. The existing
`gl-interop-smoke.bas` fixture was then compiled with `fbc-android -gfx3`,
installed on the connected AGM A8 (Android API 24, Adreno 306), and launched.

The device selected its GLES renderer, executed the ordered render-thread
callback, resolved `glGetString` from the current graphics context, and
terminated normally:

```text
GFX3_GL_INTEROP_PASS
FREEBASIC_ANDROID_EXIT:0
```

The automatic-selection fixture was also made safe for both invocation forms:
it defines `__FB_GFXLIB3__` only when `-gfx3` has not already supplied it. It
was then packaged explicitly with `-gfx3` and installed on the same device.
The fixture accepts success only when `GET_DRIVER_NAME` contains `OpenGL ES`
and its primitive `POINT` readbacks agree. Android's graphical activity does
not forward this fixture's ordinary `PRINT` output to logcat, but its required
clean outcome was recorded as `FREEBASIC_ANDROID_EXIT:0` after the assertions
ran.

## 2026-07-17 pre-mode SCREENINFO parity

gfxlib2 reports the native desktop when `SCREENINFO` is called before any
`SCREEN` or `SCREENRES` mode. gfxlib3 now retains that behavior through a
small platform-vtable query: Win32 reads the current DEVMODE, X11 opens a
temporary display, and Android reads the retained NativeActivity window. No
renderer is created for this query, so bpp/pitch remain zero and the driver
name remains empty as in gfxlib2.

`screeninfo-desktop-win32-smoke.bas` compared the public results with the
active Win32 DEVMODE through gfxlib2, normal gfxlib3, and multithreaded gfxlib3;
all three printed:

```text
GFX3_SCREENINFO_DESKTOP_WIN32_PASS 1366x768x32@60
```

The rebuilt ARM64 multithreaded archive was hash-matched into the Android
runtime package and installed on the connected AGM A8. The no-mode fixture
reported the real NativeActivity dimensions and exited normally:

```text
GFX3_SCREENINFO_DESKTOP_PASS 720x1280x32
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-17 Android GPU keyboard control

gfxlib2's Android keyboard target is now present in gfxlib3 without a CPU
framebuffer overlay. The NativeActivity template installs an unfocused hidden
`FreeBasicInputView`; tapping the upper-right native-pixel KB rectangle consumes
that touch before BASIC coordinate conversion, asks Java's UI thread to show or
hide the IME, and leaves normal IME commits on the existing native key bridge.
The GLES presentation shader draws the target after page conversion, so it
cannot affect page pixels or `POINT`/`GET` results. An idle renderer poll
detects its state change and performs the required EGL presentation on the
render thread.

The ARM64 archives rebuilt with NDK 27 and the multithreaded archive was
hash-matched into the Android package runtime. `android-keyboard-overlay-smoke`
was packaged with `-gfx3` and installed on the connected AGM A8. Physical
screenshots established the full visual state sequence: launch begins without
the keyboard and with a dark KB target, one device tap opens the system keyboard
and changes the target blue, and a second tap hides the keyboard and restores
the dark target. The running fixture did not report a leaked mouse-button
failure during that sequence. The final package was then opened once more,
its KB target tapped, and sent lowercase `a` through Android text input. The
IME bridge reached public `INKEY` and completed cleanly:

```text
GFX3_ANDROID_KEYBOARD_PASS
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-17 Android Vulkan WSI build and fallback

The Vulkan runtime is no longer compiled as an Android unsupported stub. Its
header-independent loader now opens `libvulkan.so`, enables
`VK_KHR_android_surface`, creates `VkAndroidSurfaceCreateInfoKHR` from the
retained `ANativeWindow`, and accepts Android's window-only native-handle
contract while retaining the existing swapchain path.

The ARM64 normal and multithreaded PIC archives rebuilt cleanly with NDK 27,
and the multithreaded archive was SHA-256 matched into the device-test runtime.
The connected API 24 AGM A8 has `/system/lib64/libvulkan.so` but advertises no
Vulkan feature. A freshly packaged `android-renderer-smoke.bas` therefore
exercised the new Vulkan probe, fell through to its tested GLES path, and
exited cleanly:

```text
FREEBASIC_ANDROID_EXIT:0
```

This proves the non-Vulkan-machine selection path after enabling Android WSI.
A Vulkan-capable Android device is still required to validate Android surface
creation, swapchain presentation, resize, and the Vulkan version of the KB
overlay.

## 2026-07-17 Vulkan keyboard compositor and Android regression

The Vulkan presentation command now carries the same native-pixel KB rectangle
and state used by GLES. Its final compute shader draws the two-pixel border,
idle or active fill, and K/B glyph after palette, RGB565, or 32-bit conversion.
`vulkan-presentation.c` was extended to resize a real Win32 Vulkan swapchain to
72 by 64, present the active control at 8,8 through 64,48, and capture an
interior `40,120,180` blue pixel. The strict executable rebuilt with warnings
as errors and passed. The normal and multithreaded Android ARM64 PIC archives
also rebuilt with NDK 27 and were SHA-256 matched into the package runtime.

The updated `android-keyboard-overlay-smoke.bas` package was installed and run
on the connected API 24 AGM A8. Its Vulkan probe correctly encountered the
device's stub Vulkan HAL and continued to EGL/OpenGL ES. After the physical KB
tap and an Android `A` key event, the fixture completed:

```text
GFX3_ANDROID_KEYBOARD_PASS
FREEBASIC_ANDROID_EXIT:0
```

The device cannot validate Android Vulkan presentation because it advertises no
Vulkan feature. The desktop WSI capture proves the Vulkan compositor itself;
surface creation, swapchain resize, and KB presentation still require a
Vulkan-capable Android device.

## 2026-07-17 gfxlib2 fullscreen retry parity

gfxlib2 does not stop after trying its driver list with the caller's requested
window mode. A recognized named driver is tried once, then the complete driver
list is tried including that same driver. If every attempt fails, it repeats
both passes with its fullscreen flag inverted. gfxlib3 now keeps that behavior
in a bounded attempt plan: it makes the corresponding named-first and complete
Vulkan/OpenGL or Vulkan/GLES passes, then repeats them with `GFX_FULLSCREEN`
toggled. Unknown names begin directly at the complete list. Forced Vulkan and
OpenGL retain their single backend on both passes. Explicit Null has no native
window and is therefore deliberately kept as one attempt.

The strict Win64 infrastructure executable was rebuilt with warnings enabled
and verified automatic windowed, automatic fullscreen, forced Vulkan, and
Null attempt order and flags before running its real Vulkan and OpenGL backend
checks:

```text
gfxlib3 OpenGL partial context cleanup passed
gfxlib3 Vulkan native window cleanup passed
gfxlib3 Vulkan smoke: device-local surfaces passed
gfxlib3 OpenGL smoke: OpenGL gfxlib3 backend initialized: 4.3.0 NVIDIA 595.79
gfxlib3 infrastructure: all checks passed
```

All four Win64 archives and both Android ARM64 PIC archives rebuilt. The
public desktop automatic-selection smoke selected `Vulkan compute` and passed.
The rebuilt Android package ran on the connected AGM A8; its lack of a Vulkan
feature correctly reached OpenGL ES and all renderer, primitive, and page
assertions completed with `FREEBASIC_ANDROID_EXIT:0`. Android graphical PRINT
is not forwarded to logcat, so the exit marker is the fixture's authoritative
pass evidence.

## 2026-07-17 nested SCREENPTR lock authority

gfxlib3 now matches gfxlib2's per-unlock dirty-range rule for nested
`SCREENLOCK` calls. Previously, an inner `SCREENUNLOCK` could clear gfxlib3's
conservative shadow-dirty marker; a later write through the same pointer before
the outer unlock would not upload. `SCREENUNLOCK` now uploads the stated range
for every active lock, so the outer release retains authority for its later
pointer write.

`tests/gfx3/screenptr-nested-lock-smoke.bas` obtains one pointer under an outer
lock, writes row one under an inner lock, unlocks that row, writes row two
through the same pointer, and unlocks row two at the outer level. Both POINT
reads passed against installed gfxlib2, plus gfxlib3 Null, forced OpenGL,
forced Vulkan, OpenGL `-mt`, Vulkan `-mt`, and the freshly packaged automatic
GLES run on the connected AGM A8. The Android package exited cleanly with
`FREEBASIC_ANDROID_EXIT:0`.

## 2026-07-17 Vulkan submission-ring regression

The Vulkan runtime now owns three complete submission slots. Every slot has a
command pool, command buffer, fence, descriptor set, deferred temporary
allocation list, and, for a windowed runtime, its own acquire semaphore. An
asynchronous operation records into the next slot. Reuse waits for that slot's
fence before it releases temporary storage or resets command recording.
Render-finished semaphores are indexed by acquired swapchain image. Reacquiring
an image proves its previous presentation wait completed before that image's
semaphore is signalled again. The initial empty FIFO marker described by this
checkpoint was removed by the later asynchronous-presentation pass.

The strict header-independent Vulkan bootstrap passed twice on the desktop
Vulkan loader. Its four asynchronous submissions forced the three-slot ring
to reuse one slot and verified the retained-submission high-water mark before
the existing GPU fill, compute, device-local clear, and pitched-transfer
checks. The strict Win32 WSI presentation and resize test, plus the combined
Vulkan/OpenGL infrastructure executable, also passed. All four Win64 gfxlib3
archives and the Android ARM64 normal and multithreaded PIC archives rebuilt.

The two Android archives were SHA-256 matched into the actual package runtime.
`android-renderer-smoke.bas`, which activates gfxlib3 with its source define,
was packaged for the connected AGM A8 and exited with:

```text
FREEBASIC_ANDROID_EXIT:0
```

That API 24 device has no Vulkan feature, so this validates the freshly built
Android Vulkan probe and its automatic OpenGL ES fallback, not Android Vulkan
WSI itself. Vulkan-capable Android surface, swapchain, resize, and presentation
coverage remain open.

## 2026-07-17 Vulkan targeted sequence waits

The slot ring now records both a runtime submission serial and the renderer
command sequence that created that submission. Each backend command tags at
most one new submission. `wait_sequence` waits only slot fences tagged at or
before its requested sequence; it does not call `vkDeviceWaitIdle`. Renderer
completion accounting advances only to the requested sequence, so a completed
command in a batch cannot make later asynchronous resource use collectible.

The strict standalone bootstrap tagged four submissions as 10, 20, 30, and 40.
After waiting through 20, exactly two later slots remained retained. Waiting
through 40 then released the remaining slots. The strict combined
infrastructure executable and the Win32 Vulkan presentation/resize executable
also passed after this change.

Both Android ARM64 PIC archives were rebuilt with NDK 27 and SHA-256 matched
into the package runtime. A freshly packaged source-defined gfxlib3 renderer
smoke then ran on the connected AGM A8 and reported:

```text
FREEBASIC_ANDROID_EXIT:0
```

The device still has no Vulkan feature, so the physical run exercises the
compiled Vulkan probe followed by the OpenGL ES fallback. Vulkan-capable
Android WSI remains separate hardware coverage.

## 2026-07-17 Vulkan completed-fence polling

The render thread now polls each occupied Vulkan submission fence before it
processes a backend command. A completed slot releases its deferred temporary
allocations immediately without waiting for that slot to be reused. The poll
never waits and never changes the renderer's completed command sequence;
resource destruction remains governed by the ordered targeted-sequence wait.

The strict standalone bootstrap exercises the poll after a sequence-20 wait
while later tagged submissions can still be active. The strict warning-as-error
bootstrap, WSI presentation test, and combined infrastructure executable
passed on the desktop Vulkan loader after the change.

Both Android ARM64 PIC archives rebuilt with NDK 27 and were SHA-256 matched
into the package runtime. The fresh `org.freebasic.gfx3.renderer.poll` package
installed and ran on the connected AGM A8. Its log records the expected absent
Vulkan HAL, Adreno OpenGL ES fallback, native activity display, and
`FREEBASIC_ANDROID_EXIT:0`. This is physical Android renderer-selection and
GLES coverage. It is not Vulkan-on-Android coverage because that device does
not expose a Vulkan implementation.

## 2026-07-17 Vulkan descriptor and staging isolation

The standalone Vulkan bootstrap now queues three distinct point streams through
the submission ring before readback. Each stream has a different point color,
row, staging buffer, and slot-local descriptor set. The exact readback proves
that a later descriptor update does not redirect an earlier dispatch and that
host-visible point staging survives until the owning slot fence is reused.

The strict warning-as-error executable passed twice against the desktop Vulkan
loader after adding this regression.

## 2026-07-17 Destruction completion batching

SURFACE DESTROY is now the one completion-bearing command allowed to join an
otherwise asynchronous renderer batch. It has no result payload, so its caller
only needs assurance that its own final GPU use has completed. The renderer
therefore executes a destroy and later work together, then calls the backend's
targeted sequence wait for the destroy rather than treating the batch tail as
complete.

The controlled renderer lifecycle regression holds backend initialization,
queues CLEAR, SURFACE DESTROY with a completion, and a later CLEAR, then
verifies a single three-command backend call and the exact destroy sequence
passed to `wait_sequence`. The strict combined infrastructure suite passed.
All four Win64 archives and both Android ARM64 PIC archives rebuilt. A freshly
packaged source-defined gfxlib3 renderer smoke passed on the connected AGM A8:

```text
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-17 BMP decoder-to-GPU-page regression

The OS/2 V1 and Windows RLE BLOAD fixtures now load each generated bitmap twice:
first into a CPU `FB.IMAGE`, then into the active graphics page. The second
path is important because it proves the checked palette conversion reaches the
renderer upload command rather than only producing correct temporary memory.

Both fixtures passed through Null, forced OpenGL, and forced Vulkan in normal
builds. They also passed forced OpenGL and Vulkan `-mt` builds. The richer RLE
fixture was packaged as `org.freebasic.gfx3.bloadrle`, installed on the
connected AGM A8, selected its expected OpenGL ES fallback after the absent
Vulkan HAL, proved the rejected over-wide RLE run leaves its prior CPU and GPU
pixels intact, printed `gfxlib rle bload PASS`, and exited with
`FREEBASIC_ANDROID_EXIT:0`.

## 2026-07-17 Android Vulkan no-HAL probe cleanup

The Vulkan runtime now resolves the optional Vulkan 1.1
`vkEnumerateInstanceVersion` as a direct loader export before considering the
normal global resolver. On Android only, a missing export leaves the runtime at
its Vulkan 1.0 baseline; gfxlib3 does not need a 1.1 feature to decide whether
to use Vulkan. This avoids calling Android's no-HAL stub with the optional
null-instance resolver form, which reported a misleading error despite the
subsequent GLES fallback being correct.

The strict header-independent desktop bootstrap still reports Vulkan 1.4 and
passes twice. Both Android ARM64 PIC archives rebuilt with NDK 27 and were
SHA-256 matched into the packaged runtime. A fresh
`org.freebasic.gfx3.renderer.probe` APK installed on the connected AGM A8,
reported only its expected no-Vulkan-HAL fallback and Adreno EGL initialization,
contained no invalid `vkGetInstanceProcAddr` diagnostic, and exited with
`FREEBASIC_ANDROID_EXIT:0`. This does not constitute Android Vulkan WSI
coverage because the device has no Vulkan implementation.

## 2026-07-17 GPU-surface usage capability enforcement

The public GPU-surface usage flags are now enforced in the common surface
layer rather than treated as backend allocation hints. `RENDER_TARGET` gates
clear and drawing, `SAMPLED` gates GPU blit-source and presentation use,
`TRANSFER_SOURCE` gates download/readback, and `TRANSFER_DESTINATION` gates
upload. The default public surface still requests every capability, preserving
the uncomplicated path for existing gfxlib3 programs.

`gpu-surface-smoke.bas` now creates four intentionally narrow surfaces and
checks each accepted and rejected path. It passed automatic, forced OpenGL,
and forced Vulkan selection in normal builds and the forced OpenGL/Vulkan
`-mt` builds. The strict warning-as-error infrastructure executable passed
after the change. Both Android ARM64 PIC archives rebuilt and their staged
copies matched by SHA-256. The freshly packaged
`org.freebasic.gfx3.surfacecaps` APK then passed on the connected AGM A8 with
`FREEBASIC_ANDROID_EXIT:0`, exercising the physical OpenGL ES fallback. The
device has no Vulkan HAL, so this is not Android Vulkan coverage.

The same regression now also proves that PAINT and PUT CUSTOM cannot use their
internal readback/upload barriers to modify a transfer-source/destination-only
surface. Both are destination drawing operations and require `RENDER_TARGET`;
the Android archive and the freshly installed surface-capability APK were
rebuilt and rerun after that guard was added.

## 2026-07-17 PAINT pattern target-origin conformance

Patterned PAINT originally indexed the clipped screen-download staging buffer
from zero. That shifted its 8 by 8 tile whenever VIEW began away from the
target origin. gfxlib2 tiles from absolute target coordinates, so the flood
core now carries the staged VIEW origin into its pattern lookup.

The new `paint-pattern-smoke.bas` first passed unmodified under gfxlib2, then
passed gfxlib3 Null, OpenGL, Vulkan, OpenGL `-mt`, and Vulkan `-mt`. Both
Android ARM64 PIC archives rebuilt and SHA-256 matched into the package
runtime. The fresh `org.freebasic.gfx3.paintpattern` APK passed on the
connected AGM A8 with `FREEBASIC_ANDROID_EXIT:0`, covering the physical
OpenGL ES path.

## 2026-07-17 PAINT pattern depth-layout conformance

The absolute-origin PAINT regression now covers the three practical native
pattern layouts: an indexed byte, a little-endian RGB565 word, and a 32-bit
pixel. Its RGB565 oracle uses gfxlib2's historical POINT expansion rule, not a
generic colour conversion.

`paint-pattern-depth-smoke.bas` passed unchanged under gfxlib2 and passed
gfxlib3 Null, OpenGL, Vulkan, OpenGL `-mt`, and Vulkan `-mt`. The freshly
packaged `org.freebasic.gfx3.paintpatterndepth` APK passed on the attached AGM
A8 with `FREEBASIC_ANDROID_EXIT:0`, including the 8/16/32 OpenGL ES mode
sequence.

## 2026-07-17 forced gfxlib3 fbcunit rebuild

`tests/gfx3/gfx3-unit-tests.mk` is a narrow wrapper around the stock
`unit-tests.mk`. It limits the source list to the unchanged gfx fbcunit suites,
adds `-gfx3` to each source compilation after the stock flags are assembled,
and replaces the normal link with a separate `fbc-tests-gfx3fresh.exe` linked
against `-lfbgfx3mt`. This avoids timestamp reuse and avoids overwriting the
ordinary test runner.

The first direct command-variable attempt exposed that `unit-tests.mk` treats
an argument-bearing `FBC` value as only its executable path. The wrapper keeps
the compiler path absolute for the nested fbcunit submake and puts `-gfx3` in
the actual compile and link flag lists. The forced fresh run showed every gfx
source being compiled with `-gfx3`, the linker selecting `-lfbgfx3mt`, and:

```text
464128 graphics assertions passed, 0 failed
464774 total assertions passed, 0 failed
```

## 2026-07-17 Vulkan physical-device ranking and Android recheck

Automatic gfxlib3 selection already followed gfxlib2's backend policy, but the
Vulkan runtime accepted the first compatible physical device returned by the
loader. It now keeps the existing Float64 preference for the exact midpoint
ellipse shader, then ranks discrete, integrated, virtual, CPU, and other
device types. At equal device type it prefers a graphics-and-compute queue to
a compute-only queue. Logical-device creation remains authoritative: a failed
creation marks that candidate attempted and continues to the next ranked
compatible device.

The new strict, header-independent `vulkan-adapter-ranking.c` check compiled
with `gcc -std=gnu11 -Wall -Wextra -Werror` alongside `gfx3_vulkan.c` and
reported:

```text
gfxlib3 Vulkan adapter ranking: all checks passed
```

All four Win64 gfxlib3 archive variants rebuilt with
`make -j4 gfxlib3 ENABLE_PIC=YesPlease`. A freshly compiled public
`renderer-selection-smoke.bas` then selected `Vulkan compute` automatically
and exited zero.

The Android ARM64 normal and multithreaded PIC archives also rebuilt with NDK
27.2.12479018's `aarch64-linux-android24-clang` and were SHA-256 matched into
the actual package runtime. A new
`org.freebasic.gfx3.adapterranking` APK using those archives installed and ran
on the connected AGM A8. It logged:

```text
gfxlib3 automatic renderer: OpenGL ES 3.0
FREEBASIC_ANDROID_EXIT:0
```

The AGM A8 has no Vulkan feature, so this is a current proof of Vulkan-to-GLES
fallback and Android archive integration, not a hardware proof of the Vulkan
adapter ranking.

## 2026-07-17 rebuilt-archive compatibility sweep

The renderer-selection rebuild was followed by a forced execution of the
gfx-only fbcunit wrapper. Every graphics source was recompiled with `-gfx3`,
and the fresh runner linked `-lfbgfx3mt` rather than gfxlib2. The rebuilt
runner reported:

```text
464128 graphics assertions passed, 0 failed
464774 total assertions passed, 0 failed
```

The exact rebuilt Android ARM64 threaded-PIC archive was then packaged through
the Android helper with `-gfx3` for the unchanged
`tests/command-sweep/gfxlib-command-sweep.bas` fixture. That source selects
`GFX_NULL` deliberately, so it is a deterministic API and NativeActivity file
lifecycle check rather than a GLES pixel-rendering claim. It exercises the
registered language graphics commands, CPU images, built-in and CUSTOM PUT,
BLOAD/BSAVE, screen and image queries, palette conversion, locking, event and
input fallbacks, and clean mode shutdown. The fresh
`org.freebasic.gfx3.commandsweepcurrent` package installed on the connected
AGM A8 and logged:

```text
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-17 current Android GPU renderer and surface recheck

The current Android ARM64 threaded-PIC archive was packaged into two fresh
NativeActivity applications after the Vulkan adapter-ranking change. Both ran
on the connected AGM A8, whose automatic path selects OpenGL ES 3.0 because
the device does not expose Vulkan.

`org.freebasic.gfx3.androidrenderercurrent` runs the normal automatic renderer
fixture. Its source rejects any non-GLES driver, draws filled and outlined
primitives with alpha, reads exact GPU pixels back through POINT, changes the
visible/active page, presents it, and then closes the mode. It exited with:

```text
FREEBASIC_ANDROID_EXIT:0
```

`org.freebasic.gfx3.gpusurfacecurrent` runs `gpu-surface-smoke.bas` through
the GLES backend. It creates GPU-only surfaces, draws lines, boxes, circles,
PAINT, DRAW, and text into them, validates GET/PUT compatibility, performs
pitched explicit upload/download, GPU blits, direct presentation, capability
rejection, destruction, and mode-owned cleanup. It also exited with:

```text
FREEBASIC_ANDROID_EXIT:0
```

These are current physical GPU-rendering checks, not GFX_NULL tests. Android
Vulkan device selection and presentation remain hardware validation work until
a Vulkan-capable Android device is available.

## 2026-07-17 renderer startup failure cleanup

The common render-thread lifecycle now has an explicit controlled-backend
regression for the two initialization states that real GPU drivers expose:
probe rejection before the backend has claimed state, and init rejection after
it has claimed state. The fake backend records probe, init, and shutdown calls
so the test can require one safe shutdown in each case, while also requiring
that the failed `fb_gfx3_renderer_init()` leaves no thread, command-queue
storage, or resource-registry storage published.

The updated `tests/gfx3/infrastructure.c` compiled with
`-std=gnu11 -Wall -Wextra -Werror`, linked against the current threaded
`libfbgfx3mt` archive, and ran successfully. The same executable also ran the
real Vulkan device-local-surface smoke and real OpenGL 4.3 backend smoke:

```text
gfxlib3 Vulkan smoke: device-local surfaces passed
gfxlib3 OpenGL smoke: OpenGL gfxlib3 backend initialized: 4.3.0 NVIDIA 595.79
gfxlib3 infrastructure: all checks passed
```

This now also proves real OpenGL partial-context cleanup. The test-local
platform allocates one owned context, then rejects the first required OpenGL
function load. `opengl_init()` has already claimed its backend state at that
point, so the render thread must call `opengl_shutdown()`; the fixture requires
one create, one failed load, one destroy, and no published renderer thread,
queue, or resource storage. It then restores the ordinary platform adapter and
runs the real OpenGL smoke shown above.

The same test-local selector also drives the real Vulkan backend through a
platform that allocates one native-window record and rejects its
`native_handles()` handoff. `vulkan_backend_init()` must close its empty Vulkan
runtime, destroy that owned window exactly once, clear `backend->state`, and
leave no renderer thread, queue, or resource registry published. The strict
desktop Vulkan probe, this forced failure, and the ordinary device-local Vulkan
smoke all passed in one executable.

Driver-specific fault injection at individual allocation, shader,
and logical-device stages is still broader hardening work.

## 2026-07-17 SCREENRES refresh-query compatibility

gfxlib3's GPU modes leave the physical desktop mode unchanged. An audit of
gfxlib2 showed that its windowed OpenGL and GFX_NULL drivers likewise do not
publish a `SCREENRES` refresh request as the active refresh. Both return zero
from SCREENINFO and GET_SCREEN_REFRESH even when the call supplied 73 Hz.

`screen-refresh-smoke.bas` locks that behavior down. It was compiled and run
once against the current gfxlib2 archive and once against the newly rebuilt
gfxlib3 archive. Both runs reported:

```text
GFX_SCREEN_REFRESH_PASS 73->0
```

The four Win64 gfxlib3 normal, PIC, threaded, and threaded-PIC archives were
rebuilt directly from their archive targets before the gfxlib3 run.

The same fixture was then packaged with `-gfx3` and forced OpenGL selection
for the connected Android AGM A8. It ran through the physical OpenGL ES 3.0
renderer and logged:

```text
GFX_SCREEN_REFRESH_PASS 73->0
FREEBASIC_ANDROID_EXIT:0
```

Android owns display cadence through the NativeActivity compositor, so this
confirms API compatibility without claiming a physical refresh-rate override.

## 2026-07-17 legacy SCREENCONTROL GL setup state

gfxlib3 now retains the public SET_GL_* request values instead of silently
discarding them. Before a graphics mode exists, the corresponding GET_GL_*
queries expose the stored request values. The initial active 2D mode is zero
and the initial scale is one, matching the current gfxlib2 behavior rather
than the separate values queued for a later legacy OpenGL driver initialization.

When desktop OpenGL or Android GLES opens, gfxlib3 replaces the capability
query values with the snapshot captured by its render thread. It deliberately
does not recreate a live compute renderer in response to later legacy GL
pixel-format requests, and it has no direct-GL 2D bridge. Consequently an
active gfxlib3 GPU mode reports 2D mode zero and scale one.

`gl-set-control-smoke.bas` was compiled and run against the current Win64
gfxlib2 archive and the rebuilt gfxlib3 archive. Both runs reported:

```text
GFX_GL_SET_CONTROL_PASS
```

The existing forced-OpenGL capability smoke was then run with gfxlib3 and
reported:

```text
GFX3_GL_CONTROL_PASS color=32 depth=0 extension-bytes=10408
```

The four Win64 gfxlib3 normal, PIC, threaded, and threaded-PIC archives were
rebuilt directly from their archive targets for these runs.

The shared query source was then rebuilt as the Android ARM64 threaded-PIC
archive with the NDK 27 API 26 toolchain. Its staged package-runtime copy was
checked against the build archive by SHA-256 before packaging the existing
`gl-control-smoke.bas` fixture with `-gfx3`. On the connected Android API 24
AGM A8, the real Adreno GLES renderer reported:

```text
GFX3_GL_CONTROL_PASS color=32 depth=0 extension-bytes=1200
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-17 legacy SCREENCONTROL MMX setter

The control-switch audit compared every `case` in gfxlib2's `gfx_control.c`
with gfxlib3's query implementation. It found the legacy
SET_X86_MMX_ENABLED case was accepted only incidentally, rather than being an
explicit compatibility control. gfxlib3 now records that it accepts the setter
as a no-op. Its GPU command renderer has no CPU MMX blitter, so
GET_X86_MMX_ENABLED remains false on every supported architecture.

`mmx-control-smoke.bas` was compiled and run against the current Win64 gfxlib2
archive, then against rebuilt gfxlib3 normal and `-mt` archives. All three
runs reported:

```text
GFX_MMX_CONTROL_PASS
```

The Android ARM64 threaded-PIC archive was then rebuilt with the same source,
SHA-256 staged into the test runtime, packaged with `-gfx3`, installed on the
connected AGM A8, and run through its NativeActivity lifecycle. It reported:

```text
GFX_MMX_CONTROL_PASS
FREEBASIC_ANDROID_EXIT:0
```

The post-preinclude current-source rebuild of the unchanged fbcunit gfx wrapper
compiled every selected module with the public `-gfx3` option and linked only
`-lfbgfx3mt`. It passed 464,128 graphics assertions with zero failures
(464,774 total assertions, including the wrapper's non-graphics support
suites).

## 2026-07-17 current gfxlib3 non-fbcunit log suite

`tests/gfx3/gfx3-log-tests.mk` is the permanent gfxlib3 wrapper for the
non-fbcunit portion of `tests/gfx`. It limits `log-tests.mk` to that directory
and selects gfxlib3 with the public `-gfx3` option. The compiler's preinclude
uses the documented bare marker, so unchanged fixtures that explicitly define
`__FB_GFXLIB3__` and ordinary fixtures without that define both compile.

The generic log harness now forwards the selected FreeBASIC compiler, C
compiler, and source-level option into every recursive `bmk-make.mk` run.
`bmk-make.mk` applies the source-level option while it compiles each module,
before emitting object metadata. This prevents a false pass where only a final
link operation selects gfxlib3.

From `tests/`, the fresh run was:

```text
bash -lc 'make -B -f gfx3/gfx3-log-tests.mk all FB_LANG=fb'
```

The generated logs reported:

```text
GFX3_LOG_CASES=38
GFX3_LOG_PASSED=38
GFX3_LOG_FAILED=0
GFX3_LOG_SUITE_PASS
```

## 2026-07-17 `-gfx3` source-define and driver-precedence regression

The compiler previously implemented `-gfx3` by inserting a command-line
`__FB_GFXLIB3__=1` definition. The documented source opt-in is a bare
`#define __FB_GFXLIB3__`; when both were present, the preprocessor correctly
identified their different replacement text and rejected an otherwise
unchanged program.

`-gfx3` now selects the gfxlib3 archive as before, but pre-includes
`fbgfx3-option.bi`. That small selector uses the documented bare definition
before source parsing. The regular no-source-define option module/main smoke
and `driver-selection-precedence-smoke.bas`, which deliberately repeats the
bare source definition, both compile, link only gfxlib3, and exit zero with the
rebuilt Win64 compiler.

The precedence fixture sets `FBGFX=Vulkan`, requests `nUlL` through
`SCREENCONTROL SET_DRIVER_NAME`, and verifies that Null wins. It then clears
the stored request, sets `FBGFX=NuLl`, and verifies that the environment again
selects Null. It passed current gfxlib2, gfxlib3 normal, and gfxlib3 `-mt`:

```text
GFX_DRIVER_PRECEDENCE_PASS
```

The packaged Android ARM64 gfxlib3 run passed on the connected AGM A8 as well:

```text
GFX_DRIVER_PRECEDENCE_PASS
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-17 `-gfx3` intrinsic-only selection

`selection-gfx3-intrinsic.bas` deliberately has no `fbgfx.bi` include. It
retains the documented bare source define, opens GFX_NULL through graphics
intrinsics, then verifies a PSET/POINT round trip. The rebuilt Win64 compiler
compiled it with `-gfx3`; its emitted linker command named `-lfbgfx3` and no
gfxlib2 archive. The resulting executable exited zero:

```text
GFX3_INTRINSIC_PUBLIC_OPTION_PASS
```

The same source was packaged as Android ARM64 through `fbc-android -gfx3` and
ran on the connected AGM A8. Its only output is the NativeActivity lifecycle
marker because the fixture intentionally does not depend on console text:

```text
FREEBASIC_ANDROID_EXIT:0
GFX3_ANDROID_INTRINSIC_PUBLIC_OPTION_PASS
```

## 2026-07-17 scoped GPU-surface staging map

`fbgfx3.bi` now exposes `Gfx3SurfaceMap` and `Gfx3SurfaceUnmap`. They expose a
full CPU staging allocation rather than a backend pointer: map downloads the
surface, ordinary operations and destruction reject the mapped descriptor, and
a writable unmap uploads the staging allocation. This preserves partial writes
without pretending that device-local Vulkan or OpenGL/GLES storage is ordinary
CPU memory.

The expanded `tests/gfx3/gpu-surface-smoke.bas` verifies exact mapped
readback, a partial writable-map update, rejection of download and destruction
while mapped, forced release of an outstanding map at `SCREEN 0`, and the
required transfer-source/transfer-destination capability combinations. Fresh
public `-gfx3` builds pass through Null, OpenGL, Vulkan, and their `-mt`
variants.

The Android ARM64 threaded-PIC archive was rebuilt with the API-24 NDK clang,
checked to export both map symbols, and packaged into
`org.freebasic.gfx3surfacemap`. The connected AGM A8 launched the actual GLES
fixture and logged:

```text
FREEBASIC_ANDROID_EXIT:0
```

On `.99`, all four current Linux gfxlib3 archive variants rebuilt from the
staged source. The public fixture then linked only the staged gfxlib3 archive
and passed with `GFX_NULL`; a second build with `GFX3_OPENGL_TEST` passed under
`xvfb-run`, exercising a real Linux GLX context. The installed FreeBASIC
compiler predates the public `?` suffix substitution, so the disposable stage
used a literal `libfbgfx3?.a` symlink solely to test the unchanged header while
its normal gfxlib2 archive was explicitly suppressed. No source or installed
host library was altered.

`tests/gfx3/audit-public-exports.ps1` was also extended to extract every
public `fbgfx3.bi` alias. The current Win64 and Android ARM64 threaded-PIC
archives both pass all 88 gfxlib2-compatible symbols and all 12 gfxlib3
extension symbols.

## 2026-07-17 indexed and RGB565 staging-map layout

`tests/gfx3/gpu-surface-map-depth-smoke.bas` verifies the raw CPU staging
layout used by `Gfx3SurfaceMap`: indexed 8-bit surfaces expose one byte per
pixel, while RGB565 16-bit surfaces expose two bytes per pixel. The fixture
writes the first and final pixel only, unmaps, and checks the raw downloaded
GPU values rather than using POINT's public color conversion.

Fresh public `-gfx3` runs pass through Null, OpenGL, Vulkan, OpenGL `-mt`, and
Vulkan `-mt` on Win64. A fresh Android ARM64 package,
`org.freebasic.gfx3surfacemapdepth`, completed through the connected AGM A8's
actual GLES renderer with:

```text
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-17 rectangular GPU-surface staging map

`Gfx3SurfaceMapRect` stages only a checked `(x, y, width, height)` rectangle.
Its tight pitch is based on the rectangle rather than the full surface, and
writable unmap uploads only that same region. The existing GPU-surface fixture
checks a 2 by 2 mapped upload rectangle against all four source pixels, commits
a separate 1 by 1 rectangle at its correct target coordinate, and rejects an
out-of-bounds rectangle without allocating staging memory.

Fresh public `-gfx3` runs pass through Null, OpenGL, Vulkan, OpenGL `-mt`, and
Vulkan `-mt`. The rebuilt Android ARM64 threaded-PIC archive exports the new
symbol, passes the 12-symbol extension audit, and the physical package
`org.freebasic.gfx3surfacemaprect` completed through the AGM A8 GLES path with:

```text
FREEBASIC_ANDROID_EXIT:0
```

The installed package was launched again after the focused regression rerun.
The connected AGM A8 remains online as API 24 and reports `Qualcomm, Adreno
(TM) 306, OpenGL ES 3.0` from SurfaceFlinger. The newly started NativeActivity
again logged `FREEBASIC_ANDROID_EXIT:0`, confirming the physical GLES fallback
path rather than an emulator or a stale build record.

## 2026-07-17 focused gfx regression rerun

The current-source fbcunit wrapper was rebuilt and run from `tests/` using
`make -B -f gfx3/gfx3-unit-tests.mk all`. Every selected source compiled with
the public `-gfx3` option and the resulting executable linked only
`-lfbgfx3mt`. It passed all 464,128 graphics assertions in the 19 focused gfx
tests. The wrapper's non-graphics support checks bring the process result to
464,774 assertions, with zero failures.

The separate current-source non-fbcunit wrapper was then run with
`make -B -f gfx3/gfx3-log-tests.mk all FB_LANG=fb`. It reports success for all
38 gfx cases: five required compile successes, 30 required compile failures,
and the three standalone joystick, XPad, and touch runtime cases. This keeps
the focused `tests/gfx` directory at 46 of 46 source files meeting their
declared result through the public gfxlib3 selection path.

## 2026-07-17 exact named renderer retry

The selector was corrected to match gfxlib2's `gfx_screen.c` retry ordering.
For a recognized `FBGFX` or `SET_DRIVER_NAME` backend it now attempts that
backend once, then attempts the complete platform list including it, and
repeats both passes with the fullscreen bit inverted. Unknown names have no
dedicated attempt and begin at the complete list. Explicit Null and force-only
Vulkan/OpenGL requests retain their documented bounded behavior.

`gfx3_backend_select.c` compiled under `-Wall -Wextra -Werror`; the updated
strict infrastructure executable linked the rebuilt threaded gfxlib3 archive
and passed its selector assertions before also passing the real Vulkan
device-local-surface and OpenGL 4.3 backend checks. The public Win64 automatic
selection fixture selected Vulkan and completed its exact PSET/POINT readback.

The Android ARM64 threaded-PIC selector object was rebuilt with NDK 27 and
packaged in `org.freebasic.gfx3namedfallback`. On the connected AGM A8, the
fixture set only `FBGFX=Vulkan`; the device has no usable Vulkan renderer, so
the named preference correctly fell through to its physical GLES driver:

```text
GFX3_ANDROID_NAMED_RENDERER_FALLBACK_PASS OpenGL ES 3.0
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-17 broader public graphics rerun

Three additional repository fixtures were freshly compiled with the current
uninstalled Win64 compiler, explicit checkout include directory, and public
`-gfx3` selection. The portable FB and QB command sweeps completed unchanged:
`tests/macos/gfxlib-command-sweep.bas` and
`tests/macos/gfxlib-qb-command-sweep.bas`. The bounded
`tests/interactive/screen-opengl-reopen.bas` also opened a real 1024 by 768
OpenGL mode, replaced it with an 800 by 600 OpenGL mode, and exited zero.
These broaden public syntax and lifecycle coverage, while the remaining
interactive and platform-specific matrix is still tracked separately.

## 2026-07-17 PAINT dirty-rectangle transfer check

The threaded Win64 `gfx3_paint_api.c` object was rebuilt with warnings enabled,
then inserted into the threaded gfxlib3 archive. `paint-pattern-smoke.bas`,
`paint-pattern-depth-smoke.bas`, `alpha-primitives-smoke.bas`, and
`gpu-surface-smoke.bas` were compiled with the public `-gfx3` selection and
exited zero. The PAINT CPU compatibility barrier now returns only its modified
rectangle to GPU memory; this is not a claim of a GPU-resident flood fill.

The ARM64 threaded-PIC object was rebuilt with Android NDK 27 and packaged as
`org.freebasic.gfx3paintdirty`. The connected AGM A8 selected OpenGL ES 3.0
automatically, completed the indexed, RGB565, and 32-bit patterned PAINT
fixture, and logged `FREEBASIC_ANDROID_EXIT:0`.

## 2026-07-17 OpenGL GPU-resident solid PAINT

The common PAINT command and OpenGL 4.3 compute backend were rebuilt with
strict C warnings. The shader uses one bounded device-local FIFO/visited
storage allocation and performs solid screen or GPU-surface PAINT without a
surface download or upload. The public `command-compat-smoke.bas`,
`alpha-primitives-smoke.bas opengl`, and `gpu-surface-smoke.bas` fixtures were
freshly linked with `-gfx3` and exited zero. The alpha fixture reported
`OpenGL 4.3 compute` while validating gfxlib2's 256-divisor primitive-alpha
result.

Patterned PAINT, Vulkan, GLES, CPU-image, and oversized OpenGL fills were not
part of this compute dispatch and remain covered by the bounded compatibility
path described above.

The command was added at the end of the protocol enumeration after an initial
Android incremental-archive run demonstrated why this is required: inserting a
command between established values makes unchanged backend objects decode later
commands incorrectly. With the append-only layout, the Android ARM64 context
and PAINT objects were rebuilt and the physical patterned 8/16/32-bit GLES
package `org.freebasic.gfx3paintcomputeapi3` again logged:

```text
FREEBASIC_ANDROID_EXIT:0
```

## 2026-07-18 Vulkan GPU-resident solid PAINT

`vulkan_paint.comp` was compiled for Vulkan 1.0 and embedded through the
checked shader generator. Its descriptor set uses the existing three storage
bindings for the surface, immutable command, and transient device-local
visited/FIFO storage. Allocation ownership is deferred to the submission slot,
so the queue remains valid until the GPU fence signals.

The rebuilt threaded Win64 archive passed `alpha-primitives-smoke.bas vulkan`,
`command-compat-smoke.bas` with `GFX3_VULKAN_TEST`,
`gpu-surface-smoke.bas` with `GFX3_VULKAN_TEST`, and the patterned depth
fixture through its deliberate CPU fallback. The alpha fixture reported
`Vulkan compute` and exited zero.

## 2026-07-18 GLES GPU-resident bounded solid PAINT

The Android GLES backend adds an ES 3.0 fragment-mask ping-pong path for solid
targets up to 4,096 pixels. It initializes one seed mask, expands between two
integer textures exactly once per possible target pixel, then composites into a
temporary GPU texture before copying the finished texture to the surface.
There is no CPU surface download or upload on this path. The cap prevents an
unbounded series of fragment passes on older mobile hardware.

The ARM64 Android archive was rebuilt with strict warnings. On the connected
AGM A8, `command-compat-smoke.bas` packaged as
`org.freebasic.gfx3paintpingpong` and `alpha-primitives-smoke.bas` packaged as
`org.freebasic.gfx3paintpingpongalpha` both completed through physical OpenGL
ES 3.0 with `FREEBASIC_ANDROID_EXIT:0`. The alpha package exercises a 64 by 64
screen, exactly the 4,096-pixel GPU-path limit.

## 2026-07-18 patterned PAINT command payload groundwork

The common PAINT protocol now owns a fixed 256-byte pattern representation,
its byte count, and the absolute target origin used for gfxlib2's 8 by 8 tile
rule. The context packs every byte into explicitly little-endian protocol
words, avoiding host-layout assumptions in the eventual Vulkan/OpenGL/GLES
shader readers. Patterned GLES deliberately remains on the verified
compatibility path while the desktop backends consume those fields. Strict
Win64 and Android compilation, plus rebuilt common archive objects, passed.

The desktop OpenGL backend now consumes that payload in its PAINT compute
shader. Its byte reader reconstructs native 8-, 16-, and 32-bit pattern values
from the explicitly little-endian command words, applies the absolute target
origin before the 8 by 8 wrap, and keeps pattern writes distinct from primitive
alpha blending as gfxlib2 requires. The public OpenGL `paint-pattern-depth`
fixture compiled with `-gfx3` and `GFX3_OPENGL_TEST` then exited zero.

The Vulkan PAINT storage command now carries the same packed fields. Its
Vulkan 1.0 shader reconstructs the native pattern value from the command
buffer, applies the absolute origin before wrapping, and bypasses primitive
alpha exactly as the CPU pattern path does. The regenerated SPIR-V module,
strict runtime/backend compilation, and public `GFX3_VULKAN_TEST`
`paint-pattern-depth` fixture all passed.

## 2026-07-18 physical Android GLES PAINT boundary

The connected AGM A8 was checked directly. Its Adreno 306 exposes OpenGL ES
3.0 and no Vulkan HAL, so automatic gfxlib3 selection reaches GLES after the
Vulkan probe. The Android compiler was rebuilt from this checkout as
`1.20.2-14`, then the current `src/tools/android/fbc-android` wrapper packaged
the ARM64 `libfbgfx3mtpic.a` archive. The wrapper needed one correction: its
temporary public-header overlay now also copies `fbgfx3-option.bi`, which the
compiler's `-gfx3` pre-include requires.

`org.freebasic.gfx3paintpatterndepthgpu` ran the current
`paint-pattern-depth-smoke.bas` fixture with `-gfx3 -mt` and logged
`FREEBASIC_ANDROID_EXIT:0`. It exercises indexed, RGB565, and 32-bit
absolute-origin patterns through the bounded GLES GPU path. Its tiny integer
pattern texture is command data only; the target never downloads to the CPU.
`org.freebasic.gfx3selectcurrent` then ran the named-renderer fixture from the
same current toolchain and logged:

```text
GFX3_ANDROID_NAMED_RENDERER_FALLBACK_PASS OpenGL ES 3.0
FREEBASIC_ANDROID_EXIT:0
```

The focused `paint-pattern-gles-smoke.bas` probe initially caught a doubled
VIEW-origin offset. GLES raster coordinates already address the whole target,
so the final reader uses a zero staging origin while Vulkan/OpenGL retain their
own protocol handling. The corrected probe and full depth fixture both pass on
the physical device. GLES now keeps the device-proven solid and patterned
ping-pong paths through 4,096 pixels; larger targets use the documented CPU
compatibility path.

The same physical depth fixture was then packaged as
`org.freebasic.gfx3paintpatternquery` after the GLES backend gained an
`GL_ANY_SAMPLES_PASSED` convergence query. Each expansion compares the new
cumulative mask with its predecessor and terminates when no newly reached
pixel remains. It again logged `FREEBASIC_ANDROID_EXIT:0`, proving the query
does not alter indexed, RGB565, or 32-bit pattern results.

`org.freebasic.gfx3paintalphaquery` then ran the current 64 by 64
`alpha-primitives-smoke.bas` package through the same renderer archive and
logged `FREEBASIC_ANDROID_EXIT:0`. This covers the shared solid PAINT route at
the 4,096-pixel acceleration limit and preserves gfxlib2's alpha primitive
result.

## 2026-07-18 current Win64 public export audit

The public archive audit was repeated against the current Win64 release
archives with the UCRT64 GNU `nm`. It parsed 59 FBCALL graphics declarations
and 29 runtime hook functions from gfxlib2's public header, for 88 required
gfxlib2-compatible exports. It also parsed all 12 public gfxlib3 extension
aliases. `libfbgfx.a` and `libfbgfx3.a` satisfied their respective required
sets and the audit printed `GFX3_PUBLIC_EXPORT_AUDIT_PASS`.

The freshly built Win64 compiler also compiled and ran
`tests/gfx3/api-smoke.bas` with the public `-gfx3 -mt` selection and exited
zero. Because this is an uninstalled source checkout, the invocation supplied
`-i .\\inc`; an installed compiler obtains the same headers from its normal
prefix include directory.

This is an ABI/export check, not an assertion that every behaviour is already
at complete QB parity. Broader randomized PAINT coverage and non-compute GLES
throughput characterization remain separate verification work.

## 2026-07-18 GLES PAINT continuation beyond the former size gate

GLES no longer selects CPU staging merely because a PAINT target has more than
4,096 pixels. The exact cumulative frontier remains in two GPU integer mask
textures. For each 32-step batch the renderer makes a GPU-local copy of the
starting frontier, expands the mask, and runs one discard-based comparison draw
under `GL_ANY_SAMPLES_PASSED`. The comparison emits samples only for pixels that
were newly reached, so the query can end the flood correctly. Earlier code
included the full-frame expansion in the query, which necessarily reported
samples and forced every PAINT to its pixel-count limit. The seed pass also
rejects a pixel already equal to the border colour, matching gfxlib2's no-write
boundary case.

`tests/gfx3/paint-large-gpu-surface-smoke.bas` creates surfaces with
RENDER_TARGET and TRANSFER_SOURCE only. It intentionally omits
TRANSFER_DESTINATION, so any CPU download-and-upload fallback fails instead of
masking a renderer regression. The physical AGM A8 package
`org.freebasic.gfx3paintlarge` logged `FREEBASIC_ANDROID_EXIT:0` with the
current ARM64 threaded archive. Its largest case is an 80 by 120 surface with
alternating GPU-drawn walls and a 4,720-pixel serpentine route from `(0,0)` to
`(0,118)`. This proves exact GPU continuation beyond the old target-size gate
and preserves the wall pixels. On this older Adreno 306, such a deliberately
pathological fragment-only flood is materially slower than the ordinary fill;
it is an exact compatibility result, not a throughput claim for modern
compute-capable backends.

The current archive was also packaged as
`org.freebasic.gfx3paintpatternbatch` for the existing indexed, RGB565, and
32-bit `paint-pattern-depth-smoke.bas` fixture. It logged
`FREEBASIC_ANDROID_EXIT:0`, confirming that batched convergence preserves the
command-owned 8 by 8 pattern payload and absolute tile placement.

## 2026-07-18 current public gfx regression rebuild

The current uninstalled Win64 compiler rebuilt the focused gfx fbcunit wrapper
with public `-gfx3` selection and linked `-lfbgfx3mt`. The executable reported
464,774 passed assertions and zero failures. The separate current gfx log
matrix regenerated all 38 `tests/gfx/*.log` results: every log carries
`RESULT=PASSED`, with zero `RESULT=FAILED` markers. This includes five required
compile successes, 30 required compile failures, and the GETJOYSTICK,
GETTOUCH, and GETXPAD runtime fixtures.

The legacy `failed-log-tests-fb.inc` file contains entries from earlier
historical runs and is not used as the current result source; the per-case logs
are authoritative for this rebuild.

The current compiler also rebuilt and ran the unchanged broader public sweeps
with `-gfx3 -mt` and the checkout include directory. The ordinary FB command
sweep, `tests/macos/gfxlib-command-sweep.bas`, exited zero under GFX_NULL. The
QB parser sweep, `tests/macos/gfxlib-qb-command-sweep.bas`, also exited zero
under `-lang qb`. These runs extend the focused gfx directory evidence to
public command syntax and QB-only SCREEN, STICK, and STRIG forms.

## 2026-07-18 current desktop compute backend repair

A fresh physical desktop run selected Vulkan automatically and passed the
explicit `vulkan-api-smoke.bas` fixture. The same run then exposed an OpenGL
initialization failure that older aggregate evidence had not caught: the line
compute shader accidentally contained PAINT-only pattern references without
their uniform declarations. The shader now contains only LINE state.

That repair exposed a second independent PAINT issue. Vulkan and desktop
OpenGL operate in absolute surface coordinates, but their pattern readers also
added the CPU VIEW staging origin. The shared 8-, 16-, and 32-bit depth fixture
therefore failed its first nonzero-VIEW assertion before the correction. Both
compute shaders now tile directly by their absolute pixel coordinate; the
Vulkan SPIR-V header was regenerated from the corrected source.

The rebuilt threaded Win64 archive was exercised on the actual desktop driver.
An explicit OpenGL mode reports `OpenGL 4.3 compute` and passes the renderer
selection smoke. All seven stages of `opengl-gpu-surface-state-smoke.bas` pass,
including alpha primitives, PAINT, and a following GPU-only surface draw.
`paint-pattern-depth-smoke.bas` then exits zero with forced OpenGL and forced
Vulkan, confirming exact indexed, RGB565, and 32-bit nonzero-VIEW pattern
placement on both compute backends.

The corrected OpenGL and Vulkan objects were inserted into all four Win64
gfxlib3 archive variants, including the default non-threaded `libfbgfx3.a`.
A fresh public `-gfx3` build without `-mt` then selected OpenGL 4.3 compute
and passed the renderer-selection fixture. The fix is therefore not limited to
the threaded test configuration.

The current desktop archive then ran a matched compatibility slice on each
explicit backend. OpenGL 4.3 and Vulkan both passed the alpha-primitive,
CIRCLE, command-compatibility, GPU-surface, custom-font-depth, and image smoke
fixtures. This exercises compute image writes, primitive alpha ordering, PUT
and GET transfers, opaque surface ownership, built-in and custom font paths,
and mode teardown through both actual drivers.

The desktop API-state fixtures were also rerun against the repaired archive.
Forced OpenGL passed the ordered `Gfx3RunOnRenderThread`/SCREENGLPROC callback,
immutable SCREENCONTROL capability snapshot, legacy SET_GL compatibility state,
nested SCREENPTR lock upload, and visible SCREENSYNC presentation checks.
The shared nested-lock and presentation fixtures also passed with Vulkan forced.
This covers the render-thread ownership boundary and the CPU compatibility
bridge in addition to the compute primitive tests.

The current desktop file and mode-management slice passed as well. The indexed,
RGB565, and 24-bit-override BSAVE/BLOAD BMP round trips pass with forced
OpenGL and forced Vulkan. SCREENLIST and windowed refresh reporting pass, and
the lifecycle fixture completed 32 full GPU mode replacements on each backend,
including invalid-request preservation, alternating explicit closes, exact
readback, and final double `SCREEN 0` teardown.

## 2026-07-18 current Win32 GETMOUSE state repair

The focused desktop input fixture exposed an asynchronous Win32 edge case on
both explicit compute backends. A delayed `WM_MOUSELEAVE` from
`TrackMouseEvent` could clear gfxlib3's common mouse-inside state after
SETMOUSE had already returned the physical cursor to the window. gfxlib2
refreshes `mouse_on` from the desktop cursor before every window message, so
its GETMOUSE result remains valid in this sequence.

The gfxlib3 Win32 adapter now validates a leave notification against the
current client rectangle before publishing an exit. The updated platform
object was rebuilt into all four Win64 gfxlib3 archives. The actual
`tests/gfx3/input-smoke.bas` executable then exited zero with explicit
OpenGL and explicit Vulkan. The fixture covers injected key transitions,
mouse movement, buttons, wheel axes, SETMOUSE, touch fallback, focus changes,
SCREENEVENT, native window controls, close delivery, and KEY_QUIT.

The connected Android device was also rechecked live: serial `b857d433` is an
AGM A8 on API 24 with an Adreno 306 advertising OpenGL ES 3.0, while its Vulkan
capability query is empty. Its installed automatic renderer-selection package
was launched through ADB without an override and logged `gfxlib3 automatic
renderer: OpenGL ES 3.0` followed by `FREEBASIC_ANDROID_EXIT:0`. This is a
fresh device confirmation of the intended gfxlib2-style automatic fallback;
Android Vulkan WSI remains hardware-validation work because this device has no
Vulkan feature.

## 2026-07-18 page-flip compatibility repair

The public `SCREENCOPY` hook previously submitted its GPU blit without an
explicit presentation boundary, while only the separately exported `FLIP`
symbol waited. That differed from gfxlib2, where `SCREENCOPY` is an alias for
`FLIP`, and it allowed page-flip games to observe a stale frame until later
graphics work happened to present it. gfxlib3 now treats a copy to the visible
page as a waited PRESENT. Visible `SCREENSET` changes use the same boundary.

gfxlib2 also copies its parallel graphical-console page during FLIP. gfxlib3
now duplicates the matching cell array under the active mode lock as part of
the GPU page copy. This keeps subsequent `SCREEN(row, column)` reads aligned
with copied text as well as copied pixels.

The rebuilt threaded Win64 archive passed
`tests/gfx3/page-flip-presentation-smoke.bas` through automatic renderer
selection and forced Vulkan. The fixture checks two independent page colours,
visible/work-page transitions, `SCREENCOPY 1, 0`, and the copied console cell.
`page-flip-visual.bas` was also run through automatic selection and captured
from the physical desktop: the copied page showed its expected red outer field,
green middle field, and blue centre field.

## 2026-07-18 high-volume PSET page-flip repair

The initial page-copy repair was correct for ordinary primitive workloads but
not sufficient for OMA's software-rasterizer style game loops. Behold draws
tens of thousands of individual `PSET` pixels on its non-visible page before
each `SCREENCOPY`. gfxlib3 formerly allocated and queued one render command
per BASIC statement, which delayed the copy boundary long enough for the
visible page to remain black or stale.

gfxlib3 now accumulates compatible caller-local PSETs into 4,096-point GPU
commands. It flushes that batch before operations with observable ordering:
pixel reads, LINE/CIRCLE and other primitives, VIEW/WINDOW changes, CLS/text,
SCREENSET, SCREENCOPY, and SCREENSYNC. A batch belongs to its logical work
page and captures its VIEW clipping rectangle, so it cannot cross a page or
coordinate-state transition. Thread-local batch storage is released with its
draw state.

`tests/gfx3/page-flip-visual.bas` now writes 49,152 individual PSET pixels to
page 1, overlays two rectangles, copies page 1 to visible page 0, and remains
open for native capture. The rebuilt multithreaded OpenGL executable completed
the heavy page flip and its capture contained the expected gradient plus green
and blue rectangles.

The unchanged `OMA/Behold/Behold.bas` source was rebuilt with public
`-gfx3 -mt`, launched from its own asset directory, and driven through its
existing menu by native Win32 key messages. A capture after the game consumed
the existing `1` menu selection showed its live `Shields:*****` HUD and blue
playfield. The same program had shown an empty black frame before batching.
This is an active gameplay confirmation for Behold, not a title-screen pass.
The remaining OMA titles still require the same menu-to-gameplay validation.

## 2026-07-18 locked alpha-font compatibility repair

The initial PSET batching repair still left one hostile but common compatibility
pattern: a locked software renderer that calculates each output pixel from
`POINT` and then writes it with `PSET`. Demolition Derby's production alpha
font uses this exact read-modify-write sequence for every glyph pixel. The
previous path submitted a point batch and waited for a GPU readback for every
pixel, so the first setup frame was still being composed after a minute.

For a 32-bit page under `SCREENLOCK`, gfxlib3 now obtains one coherent shadow
after prior GPU commands, serves locked POINT/PSET reads and writes from that
shadow, and uploads it at `SCREENUNLOCK`. A later GPU primitive in the same
lock commits the dirty shadow first, then invalidates it after its GPU write.
This preserves mixed CPU/GPU BASIC statement order without moving ordinary
primitive rendering off the graphics card.

The expanded `tests/gfx3/screenptr-nested-lock-smoke.bas` passes through the
automatic desktop renderer. It retains nested SCREENPTR dirty-range coverage,
locked LINE and PSET checks, and now verifies a locked LINE followed by a
per-pixel POINT/PSET read-modify-write row.

The unchanged `OMA/DemolitionDerby/main.bas` was rebuilt with public
`-gfx3 -mt`. Its setup screen reached the desktop within seconds. Native Enter
input then drove the existing menu into the local game; the captured frame
contains the arena, active vehicle sprites, vehicle labels, and gameplay HUD.
This is gameplay evidence rather than a title or setup-only capture.

The same current Win64 gfxlib3 build has now reached gameplay without source
changes in `OMA/Behold/Behold.bas`,
`OMA/RamboVsKittyCat-Win32-0.1/killquest.bas`,
`OMA/StarPhalanx-win32-0.5/entryv2.bas`,
`OMA/ArkanoidTest/ArkanoidTest.bas`, and
`OMA/DemolitionDerby/main.bas`. The first four were driven through their
existing menus with their native controls; captures show their live playfields
and HUDs. Further OMA entry points remain tracked separately for the requested
full game sweep.

`OMA/kinematics/kinematic_man_two_bodies_self_collision_friction.bas` was
also rebuilt unchanged with `-gfx3 -mt`. Its live frame shows two articulated
bodies under simulation with collision/friction diagnostics and moving joint
geometry. `OMA/Tamper/tamper/src/openmarket_bootstrap.bas` was rebuilt the
same way, driven through its splash, New Game menu, and player-name flow, and
reached the active board. The captured board reports player cash and worth,
round state, and live Next/Express/Stats actions. Both are gameplay-level
captures rather than title or setup-only evidence.

## 2026-07-18 OMA Win64 gfxlib3 sweep

The native, Windows-targeted OMA entry points were rebuilt from their shipped
sources with the public compiler selection `-gfx3 -mt`. No game source was
changed to accommodate the renderer. Each executable was launched from the
directory that contains its bundled assets.

| Entry point | Result | Evidence and boundary |
| --- | --- | --- |
| `ArkanoidTest/ArkanoidTest.bas` | active play | Bricks, paddle, ball, score and lives are visible after the existing menu flow. |
| `Behold/Behold.bas` | active play | The live shield HUD and blue invasion playfield appear after selecting the shipped `1` menu item. |
| `DemolitionDerby/main.bas` | active play | The arena, labelled vehicles and gameplay HUD appear after the normal setup menu. |
| `duel999/SD_Main.bas` | active play | The unchanged game was rebuilt with `-gfx3 -mt`, started its local bot-driven session, and remained alive through the initial high-volume sprite frame under the selected OpenGL backend. No firewall rule was changed. |
| `kinematics/kinematic_man_two_bodies_self_collision_friction.bas` | active simulation | Two articulated bodies, contacts and friction diagnostics animate in the live simulation. |
| `QuestForAKing-Win32-1.5/src/win11.bas` | active play | The new-game prologue completes into its live map under the same native input sequence used for the gfxlib2 reference build. The captured frame shows the player, NPCs, tile map, buildings and water. |
| `RamboVsKittyCat-Win32-0.1/killquest.bas` | active play | Player sprite and platform scene are present after its own menu path. |
| `StarPhalanx-win32-0.5/entryv2.bas` | active play | The combat scene shows sprites and projectiles in flight. |
| `Tamper/tamper/src/openmarket_bootstrap.bas` | active board | A new game reaches the board with cash/worth, round state and Next/Express/Stats actions. |

`NietzscheSE-MSDOS-1.1` and `QuestForAKing-MSDOS-1.1` are DOS package roots,
not native Win64 gfxlib3 targets, and are excluded from this renderer sweep.
The shared `common` and `android-output` directories are support/output trees,
not standalone games.

## 2026-07-18 Quest for a King small-rectangle batching repair

The direct gfxlib2 reference run and the pre-repair gfxlib3 run used the same
native `SendInput` space-bar sequence. gfxlib2 reached Quest for a King's map
within the sequence. gfxlib3 initially remained in the scripted prologue,
revealing that its frame time was too high to consume the same gameplay input
cadence.

The game expresses each logical 8-bit pixel as an opaque two-by-two
`LINE ... BF` rectangle. gfxlib3 had correctly rendered those boxes but
submitted one compute rectangle command per box. The compatibility layer now
expands only small, opaque, unlocked filled boxes into the existing ordered
point batch. Alpha-blended, larger, patterned, and SCREENLOCK rectangles keep
the normal rectangle path, preserving their distinct ordering and blending
semantics.

The current rebuilt Win64 threaded archive passes
`tests/gfx3/small-filled-rectangle-batch-smoke.bas`. The fixture writes 19,200
two-by-two boxes to a non-visible page, copies that page to visible, and checks
both pixels within each box plus neighbouring colour transitions. The unchanged
Quest for a King source was then rebuilt with `-gfx3 -mt`; the same 44-input
new-game sequence as the gfxlib2 reference reached the live map and produced
the active-play capture.

## 2026-07-18 CPU image PUT staging-surface repair

Duel uses RGB565 `FB.IMAGE` sprites loaded through `BLOAD` and can issue more
than one thousand transparent `PUT` operations in its first active frame. The
old gfxlib3 path created and destroyed a separate GPU source surface for each
operation. Although simple cases worked, that lifetime pattern made the
OpenGL renderer sensitive to the game's burst submission rate.

gfxlib3 now owns one reusable sampled upload surface in each rendering context.
CPU image data is copied into that surface, then blitted in the same ordered
command stream. A later upload cannot overwrite pixels needed by an earlier
blit because the render thread executes both commands in sequence. The staging
surface grows only when its depth or required dimensions change, and it is
retired on the render thread before context shutdown.

`tests/gfx3/put-depth-conversion-smoke.bas` now reproduces Duel's relevant
conditions: a 15-bit request normalized to RGB565, a non-default work page,
`VIEW SCREEN`, a BLOADed 13-by-16 sprite, and 1,024 transparent image PUTs.
The rebuilt threaded archive passes this fixture and
`tests/gfx3/page-flip-presentation-smoke.bas`.

Duel also stores inactive star positions at integer-limit sentinel values.
gfxlib2 clips those PUTs away. gfxlib3 now saturates finite coordinates at the
conversion boundary to safe offscreen values before its signed clipping math,
so the sentinel remains a no-op without risking integer overflow. The fixture
includes that exact offscreen PUT. The unchanged Duel source was then rebuilt
with `-gfx3 -mt`; its local `SD_TEST_BOT` game session stayed alive for 15
seconds through the initial sprite burst using the OpenGL backend.

## 2026-07-18 current OMA rebuild and launch audit

After the shared image-transfer and offscreen-coordinate repairs, every native
Win64 OMA entry point listed above was rebuilt from its unchanged shipped
source with `-gfx3 -mt`: ArkanoidTest, Behold, DemolitionDerby, Duel,
kinematics, Quest for a King, Rambo vs Kitty Cat, Star Phalanx, and OpenMarket.
All nine compile successfully against the current threaded gfxlib3 archive.

The eight non-network games were then launched from their asset directories
with the OpenGL backend selected and remained alive for a three-second startup
smoke interval. Duel was launched separately because it hosts a local game;
its `SD_TEST_BOT` session remained alive for fifteen seconds. The active-play
captures documented in the OMA sweep remain the gameplay-level evidence for
each title; this rebuild audit confirms the current renderer archive did not
regress their startup path.

## 2026-07-18 ordered GPU sprite batch and desktop selection

The OMA sprite profile exposed a dispatch-bound path rather than a pixel-fill
limit. A 640-by-480 RGB565 fixture issued 1,024 transparent 13-by-16 image
PUTs for each of 30 page-flipped frames. Before this work gfxlib3 required
20.11 seconds on the original path. The automatic desktop selection initially
picked Vulkan and completed the current fixture in 14.71 seconds. Vulkan keeps
its explicit `GFX_VULKAN` entry point, but it does not yet have the equivalent
ordered sprite batch implementation.

The desktop default now prefers the tested OpenGL 4.3 compute renderer, with
Vulkan retained as the fallback. It maintains a bounded GPU cache for stable
CPU `FB.IMAGE` sources, so repeated PUT calls do not repeatedly upload the
same sprite. For compatible contiguous `PUT PSET` and `PUT TRANS` commands
using one source and one target, the render thread sends one GPU command
buffer. A first compute pass atomically records the last applicable command
for every destination pixel; a second pass resolves those winners. This
preserves source-overlap order without requiring one GPU dispatch per sprite.
Other PUT modes, self-blits, depth conversion, and custom blenders retain the
established compatibility path.

On the same fixture, automatic desktop selection now completed in 0.816
seconds, including 30 visible page copies and their display pacing. This is a
roughly 18-fold reduction from the pre-optimization gfxlib3 result. The
remaining duration is presentation-bound, not a per-sprite CPU raster loop.
The unchanged `tests/gfx3/image-smoke.bas` and
`tests/gfx3/put-depth-conversion-smoke.bas` pass against the rebuilt OpenGL
archive. `tests/gfx3/oma-sprite-benchmark.bas` is retained as the repeatable
profile fixture.

The current OMA sources available in this workspace were rebuilt without game
changes: Behold, Duel, kinematics, Quest for a King, Rambo vs Kitty Cat, Star
Phalanx, and Demolition Derby. Each executable was launched from its asset
directory with automatic gfxlib3 selection and stayed alive for a three-second
startup interval. These runs use the new desktop OpenGL preference.

## 2026-07-18 gfxlib example parity sweep

The eight standalone `examples/graphics` gfxlib programs and all 73
`examples/manual/gfx` programs were compiled with both gfxlib2 and gfxlib3,
for 162 successful backend/example compilations. The standalone programs were
then launched individually for bounded startup checks. `flame` was checked
with its required `fblogo.bmp` asset adjacent to the test executable.
`getput.bas` was repaired to reserve `sizeof(FB.IMAGE)` for the current GET
header instead of the obsolete four-byte QB header. The unchanged demonstration
sequence now completed all raw-array, pointer, and UDT buffer forms with exit
status zero on both gfxlib2 and gfxlib3.

The manual set was launched in paired bounded groups and any timing-sensitive
result was repeated in isolation. `bload` consistently reports the missing
manual `picture.bmp` asset on both backends, and `screenglproc` exits normally
on both. The only real difference found was `imageconvertrow.bas`: it opens a
valid 1-by-1 `GFX_NULL` image-factory mode, which gfxlib2 permits. gfxlib3 now
keeps that graphics mode active while omitting impossible text-console hooks
for modes smaller than one console cell. The repaired example reaches its
interactive state, matching gfxlib2. `tests/gfx3/api-smoke.bas` also passes
after this lifecycle adjustment.

## 2026-07-18 Vulkan ordered PUT batch and dual-GPU audit

This desktop exposes two Vulkan adapters: loader index 0 is the NVIDIA GeForce
RTX 2060 (Vulkan 1.4.329) and index 1 is the Intel UHD Graphics 630 (Vulkan
1.3.215). gfxlib3 now accepts `FBGFX3_VULKAN_DEVICE_INDEX` for a deliberate,
process-local adapter selection during diagnostics. The normal policy remains
automatic and ranks the RTX above the Intel adapter.

The Vulkan sprite path previously submitted one command buffer and allocated
one host-visible command buffer per `PUT`. The retained 30-frame OMA fixture
contains 30,720 transparent sprite PUTs, so this submission pattern took
13.73 seconds. The new ordered batch records up to 64 dispatches, each with
its own descriptor set and dependency, in one submission. It keeps exact
command order and does not apply to self-blits. The same explicit Vulkan
fixture now completed in 0.70 seconds on the RTX and 1.50 seconds on the Intel
iGPU in the final back-to-back run, with the expected final pixel value of zero
on both. These short runs include normal desktop scheduling and presentation
timing, so their ordering is diagnostic rather than a cross-adapter ranking.

`image-smoke.bas` and `command-compat-smoke.bas` were rebuilt with
`GFX3_VULKAN_TEST` and run once on each forced adapter. All four runs passed.
That covers image upload/download, built-in and custom blender barriers,
PAINT, DRAW, text, and BMP round-tripping in addition to the ordered sprite
and page-flip workload.

The OpenGL interop smoke now prints the actual context adapter. With normal
Windows selection it reported `NVIDIA Corporation`, `NVIDIA GeForce
RTX 2060/PCIe/SSE2`, and passed. With the temporary per-application Windows
power-saving preference for the smoke executable it reported `Intel`,
`Intel(R) UHD Graphics 630`, and passed. The previous preference value was
restored immediately after the run. This proves the OpenGL renderer on both
installed adapters rather than only proving that their drivers are present.

## Ordered OpenGL destination-reading tile batch verification, 2026-07-18

`image-smoke.bas` was rebuilt with `GFX3_OPENGL_TEST` after extending the
tile-binned destination-reading shader from ALPHA to BLEND and ADD, and exited
zero on the normally selected RTX 2060. `transfer-benchmark.bas` also exited
zero, reporting its expected final pixel `4290510847` after all 4,096 ordered
operations in each mode.

A fresh executable directory was then assigned the temporary Windows
power-saving preference. `gl-interop-smoke.bas` printed `Intel` and
`Intel(R) UHD Graphics 630`, then passed. The Intel image smoke and transfer
fixture also exited zero with final pixel `4290510847`. The three temporary
registry entries were restored to their previous values in a `finally` block.
This validates the exact GPU tile implementation on both installed OpenGL
adapters, not merely a driver-selection path.

## 2026-07-18 performance matrix and non-blocking presentation baseline

`tests/gfx3/run-performance-matrix.ps1` now builds the unchanged primitive and
OMA workloads against gfxlib2, forced gfxlib3 OpenGL, and forced gfxlib3 Vulkan.
Each workload forces ordered completion before it records a duration; it does
not report command-queue time as rendering time. The first recorded desktop
matrix after adding the runner was:

```text
primitive gfxlib2:       PSET 0.00590 s, LINE 0.02221 s, PAINT 0.06687 s
primitive gfxlib3 GL:    PSET 0.29906 s, LINE 0.22824 s, PAINT 8.93389 s
primitive gfxlib3 Vulkan:PSET 0.38232 s, LINE 2.26443 s, PAINT 13.68169 s
OMA 30 frames gfxlib2:   0.01880 s
OMA 30 frames OpenGL:    0.77339 s
OMA 30 frames Vulkan:    0.76070 s
```

This is a deliberately failing performance baseline. It proves that gfxlib3's
correct GPU routes are not yet fast enough for the required drop-in advantage,
especially for thousands of small primitive commands and repeated PAINT.
`docs/gfxlib3/performance.md` is the authoritative command-family benchmark
manifest and optimization backlog.

The first latency repairs make `SCREENSET` and visible `SCREENCOPY` enqueue an
ordered presentation without blocking the BASIC thread. `SCREENSYNC` remains
the explicit completion operation. WGL and EGL request swap interval zero, and
Vulkan prefers IMMEDIATE presentation when the driver offers it. The renderer
and OpenGL ordered transparent-PUT batch now admit 1,024 commands, while PSET
stages up to 65,536 points in one bounded GPU command. The existing explicit
page-flip smoke completed with exit zero under both forced desktop OpenGL and
Vulkan after the change.

The Android ARM64 PIC archives were rebuilt from the same source with NDK
27.2.12479018 and packaged through the checkout's Android wrapper. An earlier
run of `org.freebasic.gfx3perf.renderer` logged `FREEBASIC_ANDROID_EXIT:0`.
The later fresh-package retest described below instead exits before native
startup, so that earlier result is historical evidence only, not current
Android validation. The API 24 Adreno 306 exposes OpenGL ES 3.0 and no usable
Vulkan feature, so it can provide GLES coverage only, never Android Vulkan
validation.

## 2026-07-18 cooperative filled-ellipse verification and Android retest

Opaque filled ellipses now run as one 64-lane workgroup on both desktop
backends. One lane preserves the Float64 midpoint state machine and all lanes
cooperatively cover each resulting span. The established
`circle-compat-smoke.bas` image hash, `6BDC39D7`, passed with forced OpenGL and
forced Vulkan on the RTX 2060. The primitive fixture's filled-circle section
measured 0.04563 seconds on OpenGL and 0.08264 seconds on Vulkan, compared
with the earlier approximately 0.61 and 0.45 second serial-shader results.

The supplied `C:\Nextcloud\Android adb\platform-tools\adb.exe` was used to
retest the physically attached AGM A8. A new primitive benchmark package and
a newly packaged copy of the established Android renderer smoke package both
installed, then repeatedly exited cleanly with process status 1 before native
startup. Neither native crashes nor `FREEBASIC_ANDROID_EXIT` records appeared.
This demonstrates a current wrapper/package startup regression shared by the
known-good smoke fixture, rather than a gfxlib3 primitive failure. Each test
package was force-stopped; Android performance validation is blocked until the
NativeActivity startup regression is repaired.

## 2026-07-18 PSET outer-lock fast path

The completed-work PSET profile isolated a redundant lock pair in every public
`fb_GfxPset` call. That entry point already holds `FB_GRAPHICS_LOCK`, which
serializes public graphics calls, page selection, and mode teardown. Its normal
screen route now validates the active mode under that outer lock instead of
taking the mode mutex again for every point. Internal callers that do not have
the outer-lock guarantee continue through the mutex-protected route.

The focused primitive fixture improved PSET from 0.29906 to 0.02792 seconds on
forced OpenGL and from 0.38232 to 0.02265 seconds on forced Vulkan. The public
gfxlib3 `-mt` API smoke exited zero after the change. This is material CPU
submission-overhead reduction, but it is still slower than the 0.00590-second
gfxlib2 memory-buffer baseline and does not remove the LINE, rectangle, text,
and PAINT batching work.

## 2026-07-18 PSET shadow ordering and alpha repair

The PSET compatibility fast path retains a CPU shadow while a screen access
lock is active. It now applies gfxlib2's primitive-alpha equation to that
shadow instead of copying a translucent source unchanged. A dedicated ordered
shadow commit was added for GPU operations that must read the prior PSET
result, notably PAINT. It avoids uploading the complete shadow on every PSET,
which would defeat the hot-path improvement.

`alpha-primitives-smoke.bas` passed when explicitly selecting OpenGL and RTX
Vulkan. This covers alpha PSET, alpha LINE/BF, alpha PAINT, alpha CLS,
disabling and re-enabling alpha primitives, and GPU-surface primitive checks.
The complete primitive fixture remained at 0.03237 seconds for PSET on OpenGL
and 0.03903 seconds on RTX Vulkan, while its ordered final readback passed.

## 2026-07-18 ordered Vulkan LINE batch

Consecutive lines against one GPU surface now use a bounded 64-command Vulkan
batch. The batch records one dispatch per line in original Basic order and
inserts a shader memory barrier between dispatches, because overlapping lines
and alpha primitives observe the destination pixel. This reduces submission
overhead without changing the primitive math or allowing destination-read
commands to reorder.

With the RTX 2060 selected explicitly, `primitive-benchmark.bas` measured
the LINE section at 0.16766 seconds, down from the preceding approximately
3.07-second result. The forced-Vulkan line-pattern, command-compatibility, and
alpha-primitives smokes all exited zero on the RTX 2060 and Intel UHD 630.
The normal and `-mt` gfxlib3 archives both rebuilt successfully; the normal
archive's RTX alpha-primitives smoke also exited zero.

## 2026-07-18 GPU scanline PAINT optimization

The desktop PAINT shaders now use a scanline seed queue. Each queued pixel
expands one horizontal non-border span, marks that span in the GPU-resident
visited map, and enqueues eligible pixels on the adjacent rows. The algorithm
preserves four-neighbour connectivity and the existing alpha and pattern write
rules, while a plain enclosed region now has approximately one queued seed per
row rather than one per pixel. CPU screen staging is still absent from the
GPU-surface path.

With the RTX 2060, the forced primitive fixture measured PAINT at 0.75213
seconds on OpenGL and 2.25265 seconds on Vulkan, down from the preceding
approximately 9.04 and 13.70 second pixel-FIFO measurements. Forced Intel
UHD 630 Vulkan measured 2.11353 seconds and passed alpha primitives. The
alpha, patterned, pattern-depth, and large renderer-only surface PAINT smokes
passed under both forced desktop backends. The normal archive passed the
pattern-depth smoke with both RTX Vulkan and OpenGL; both libraries rebuilt.

The OpenGL render-thread interop smoke reported NVIDIA GeForce RTX 2060 under
the ordinary Windows preference and Intel UHD 630 after a temporary
per-application power-saving preference. Both passed, and the prior registry
value was restored immediately. This rechecks the revised OpenGL shader on
both installed adapters.

## 2026-07-18 Android GLES physical-device revalidation

The attached device was queried through
`C:\Nextcloud\Android adb\platform-tools\adb.exe`: it reports model A8,
OpenGL ES 3.0, and no Vulkan feature. It therefore validates the Android GLES
backend only; Android Vulkan remains untested pending a Vulkan-capable device.

The package wrapper was configured to link the checkout's
`lib/freebasic/android-aarch64/libfbgfx3mtpic.a`, rebuilt with NDK
27.2.12479018. This is required because the installed package root lacked the
gfxlib3 PIC archive and failed before native startup with an unresolved
`fb_GfxScreenRes` symbol.

`android-renderer-smoke.bas` installed, started its native program, passed its
PSET/POINT and page-display checks, and remained alive in the intentional
success-display interval. `android-primitive-benchmark.bas` installed and
exited with `FREEBASIC_ANDROID_EXIT:0` after batched PSET, LINE, filled-box,
filled-circle, PAINT, text, transparent PUT, and ordered-readback coverage.
A physical `adb screencap` during the smoke interval showed page 1's dark
inner fill and border, rather than page 0's circle and cross, confirming the
visible page transition as well as command completion.

The run exposed and repaired a GLES command-state defect: `gles_points()` did
not set `operation_type` to the shared shader's POINT value. A point command
could inherit an earlier primitive type and leave its target pixel unchanged.
The backend now explicitly sets that uniform and only enables its point alpha
snapshot branch when a submitted point requires alpha blending.

## 2026-07-18 Transfer snapshot and PRESET verification

The normal desktop gfxlib3 archive was rebuilt after the transfer changes.
`image-smoke.bas` exited zero with both the forced OpenGL backend and forced
Vulkan backend, including every built-in PUT mode.  The desktop performance
matrix then completed under gfxlib2, gfxlib3 OpenGL, and gfxlib3 Vulkan with
the same final ordered-pixel result (`4290510847`) for every transfer run.

The new screen GET path downloads the active GPU page once into the existing
compatibility shadow and services subsequent GET rectangles from that coherent
snapshot.  On the current desktop, the 512-GET section fell from roughly
0.87 seconds to 0.038 seconds under OpenGL and from roughly 0.59 seconds to
0.091 seconds under Vulkan.  PRESET now uses the ordered raster batch for a
distinct source surface; this preserves the source inversion rule and avoids
one compute dispatch per sprite.

The ARM64 Android gfxlib3 PIC archive was rebuilt against the checkout and
`transfer-benchmark.bas` was packaged, installed, and run on the attached A8
device.  Its native process reported `FREEBASIC_ANDROID_EXIT:0`.  This
validates GLES PSET, PRESET, AND, OR, XOR, TRANS, ALPHA, BLEND, ADD, GET, and
the direct CPU FB.IMAGE edit invalidation check.  Unrelated log messages from
the separately running `net.fbxl.oma.openstunts` process were left untouched.

The later 8,192-entry queue configuration was rebuilt into the same ARM64 PIC
archive. `android-renderer-smoke.bas` was packaged under an isolated temporary
application ID, launched on the A8, and its own process (`13283`) reported
`FREEBASIC_ANDROID_EXIT:0` after the intentional success-display interval.
The process-local log capture avoids attributing the other application's
concurrent gfxlib3 messages to this test.

## 2026-07-18 Ordered OpenGL logical PUT verification

The desktop normal and `-mt` gfxlib3 archives rebuilt after the OpenGL logical
operation batch addition. The forced-OpenGL `image-smoke.bas` exited zero,
covering PSET, PRESET, AND, OR, XOR, TRANS, ALPHA, BLEND, ADD, clipping,
depth conversion, and source-image lifetime behavior. The transfer fixture
also exited zero with its existing final source-edit result of `4290510847`.

The Windows Graphics preference was temporarily set only for fresh temporary
smoke executables and restored in a `finally` cleanup. The OpenGL interop
fixture reported `Intel`, `Intel(R) UHD Graphics 630`; the same Intel context
passed `image-smoke.bas` and the transfer fixture, including AND, OR, and XOR.
Its respective timings were 0.065, 0.066, and 0.063 seconds and its final
pixel was `4290510847`.

The forced-Vulkan image and transfer fixtures also passed after this desktop
rebuild on device index 0 (RTX 2060) and device index 1 (Intel UHD 630). The
RTX transfer result was `4290510847`, with AND/OR/XOR at 0.073/0.070/0.073
seconds; Intel produced the same result at 0.087/0.090/0.087 seconds.

## 2026-07-18 SCREENRES presentation warm-up verification

`mode_init()` now performs one synchronous present after selecting page zero
and before exposing a successful screen to the caller. This establishes the
native presentation state at `SCREENRES` rather than delaying it until a
user's first draw command. The normal desktop archive was forcibly rebuilt.
Forced OpenGL `image-smoke.bas`, `transfer-benchmark.bas`, and the new
`image-cache-benchmark.bas` all exited zero. The transfer fixture retained
its expected final pixel `4290510847`.

Forced Vulkan `image-smoke.bas` also exited zero. Its image-cache fixture
reported a cold PSET time of 0.00982 seconds and the expected final pixel
`2152765148`, confirming that the common initialization change preserves the
Vulkan ordering and visible-page contract.

## 2026-07-18 OpenGL asynchronous page-present coalescing verification

The OpenGL executor now coalesces only non-final asynchronous PRESENT commands
for the currently visible page inside one ordered renderer drain. It retains
the final page state and performs the normal end-of-batch swap; a synchronous
PRESENT remains an isolated completion command and calls the platform swap
directly. The normal archive was forcibly rebuilt after this change.

Forced OpenGL `screen-state-benchmark.bas` exited zero with final pixel
`4279511295`, and `page-flip-presentation-smoke.bas` exited zero. This covers
`SCREENSET`, `SCREENCOPY`, selected-page POINT readback, and the explicit
`SCREENSYNC` completion boundary after the presentation optimization.

## 2026-07-18 Android SCREENRES warm-up verification

The ARM64 threaded PIC archive was rebuilt with Android NDK 27.2.12479018 and
placed in an isolated package runtime directory, leaving the installed Android
toolchain untouched. A fresh `android-renderer-smoke.bas` APK built with the
checkout wrapper and that archive was installed under
`org.freebasic.gfx3screenreswarm` on the attached AGM A8. Its own process
`16254` logged `FREEBASIC_ANDROID_EXIT:0` after the normal screen, primitive,
page-selection, and `SCREENSYNC` checks. This validates the shared initial
presentation change on physical OpenGL ES 3.0 hardware.

The same current desktop archive rebuilt `image-smoke.bas` and
`page-flip-presentation-smoke.bas` with forced Vulkan. Both fixtures exited
zero with `FBGFX3_VULKAN_DEVICE_INDEX=0` (RTX 2060) and again with index 1
(Intel UHD 630). This rechecks the shared `SCREENRES` initial presentation and
page-selection contract on both Vulkan adapters.

## 2026-07-18 OpenGL PAINT scratch-buffer reuse verification

The normal desktop archive was forcibly rebuilt after making OpenGL retain its
paint scanline-queue buffer across commands. The forced OpenGL primitive
fixture completed with final pixel `4278190080`; `paint-pattern-smoke.bas` and
`paint-large-gpu-surface-smoke.bas` both exited zero. These checks cover solid
and patterned fill behavior plus a large GPU-only target while exercising the
new persistent allocation lifetime.

A temporary 256-lane and then 1024-lane compute flood-fill implementation
passed the pattern and large-target smokes, but was slower than the retained
scanline queue, so it was removed. The restored implementation again passed
the pattern smoke and produced primitive final pixel `4278190080` with PAINT
at 0.50935 seconds. With Windows graphics preference temporarily set to the
Intel adapter and restored afterwards, `gl-interop-smoke.bas` reported
`Intel(R) UHD Graphics 630` and both it and `paint-pattern-smoke.bas` exited
zero.

The final normal and `-mt` desktop archives rebuilt cleanly after completing
the per-page SCREENLOCK snapshot allocation and shutdown ownership. Normal,
`-mt`, and large GPU-surface PAINT smokes all exited zero. A freshly linked
forced-Vulkan page-flip smoke also exited zero on device index 0 (RTX 2060)
and device index 1 (Intel UHD 630).

## 2026-07-18 Deferred SCREENLOCK verification

The normal and threaded desktop archives rebuilt after deferring outer
SCREENUNLOCK uploads to an ordered GPU consumer. `screen-state-benchmark.bas`
now uses `SCREENSYNC` after its direct CPU writes. It passed against gfxlib2,
forced OpenGL, and forced Vulkan with final pixel `4279511295`. The measured
completed lock sections were 0.01356, 0.04834, and 0.05616 seconds
respectively. The threaded forced-OpenGL fixture also passed at 0.03181
seconds.

`screenptr-nested-lock-smoke.bas` passed forced OpenGL and forced Vulkan on
both device index 0 (RTX 2060) and device index 1 (Intel UHD 630). This covers
the required outer-lock write, nested dirty range, GPU draw while locked, and
ordered POINT readback cases.

The ARM64 threaded PIC archive rebuilt successfully with NDK 27.2.12479018.
Two fresh isolated-runtime APKs then started on the attached AGM A8 but did
not create a native window before Android's activity timeout, including a
renderer-only control APK. They were force-stopped. This is a packaging or
device-launch blocker rather than a successful Android validation, so no
Android performance claim is recorded for this change.

## 2026-07-18 Vulkan ordered POINTS batch verification

The Vulkan backend now batches up to the runtime's 256-descriptor capacity for
adjacent opaque `POINTS` commands. The runtime keeps each source command as a
separate compute dispatch and emits a storage write-to-read barrier between
dispatches, so overlapping arc pixels retain BASIC's FIFO overwrite result.
The obsolete backend overlap-bounds fallback was removed rather than relaxed:
it was unreachable whenever two commands met the ordered-batch contract.

The normal desktop archive rebuilt cleanly. `arc-benchmark.bas` with forced
Vulkan completed 4,096 arcs with final pixel `4278190080` in 0.07465 seconds
on device index 0 (RTX 2060) and 0.11635 seconds on device index 1 (Intel UHD
630). `circle-compat-smoke.bas` retained hash `6BDC39D7` on both adapters.
The forced Vulkan primitive fixture also completed on both adapters with final
pixel `4278190080`, covering the shared opaque POINTS path used by text and
other point streams.

## 2026-07-18 Isolated PAINT benchmark verification

`paint-benchmark.bas` was added to distinguish screen PAINT from the mixed
primitive fixture. It draws one bordered 1024 by 768 region, completes the
first PAINT with `POINT`, repeats the compatible operation fifteen times, and
checks both the final interior and the unchanged border. The desktop matrix
runner now builds it against gfxlib2, forced OpenGL, and forced Vulkan.

The initial gfxlib2 run reported first/repeated timings of 0.00877 and
0.09549 seconds. Forced Vulkan completed with the established first, final,
and border pixels on device index 0 (RTX 2060) at 0.20593 and 0.82249 seconds,
and device index 1 (Intel UHD 630) at 0.22490 and 0.79828 seconds. These are
completed-work measurements, not queue-only timings. They also confirm that
transfer-capable screen PAINT currently uses its documented CPU flood and
dirty GPU upload compatibility path; no false claim of an all-GPU screen fill
is made from this result.

The page shadow is now retained across consecutive compatible PAINT calls and
committed only when a GPU consumer needs it. The same mode owns a reusable
visited map and FIFO queue while its mutex is held. A warm forced-Vulkan RTX
run retained all three expected pixels at 0.15821 seconds for the first fill
and 0.37293 seconds for the remaining fifteen; forced OpenGL retained them at
0.18076 and 0.38511 seconds. `command-compat-smoke.bas` also exited zero on
the forced Vulkan RTX path after the ordering change.

## 2026-07-18 Desktop GPU adapter verification

The installed Vulkan loader reported two devices in stable loader order:
device index 0 is the NVIDIA GeForce RTX 2060 (`10de:1f11`) and device index 1
is the Intel UHD Graphics 630 (`8086:3e9b`). The normal adapter policy still
selects the strongest compatible device. For reproducible diagnostics,
`FBGFX3_VULKAN_DEVICE_INDEX` was set to each index before the renderer opened.

The current normal archive compiled `command-compat-smoke.bas` and
`draw-benchmark.bas` with forced Vulkan. The command compatibility smoke
exited zero on both forced devices. DRAW retained exact pixel `4280202480` on
both, completing in 0.80530 seconds on the RTX and 1.21723 seconds on Intel.
This proves renderer initialization, presentation, queued shader work, and an
ordered readback on each GPU, rather than merely proving that Vulkan enumerates
them.

The same current archive compiled forced-OpenGL `gl-interop-smoke.bas`. It
reported `vendor=NVIDIA Corporation` and
`renderer=NVIDIA GeForce RTX 2060/PCIe/SSE2`, then printed its pass marker.
WGL has no portable per-device selector; it follows Windows' per-executable
graphics preference. The preceding PAINT scratch-buffer verification records
the corresponding Intel UHD 630 OpenGL smoke after selecting that preference.

## 2026-07-18 Ordered asynchronous submission batching verification

The shared context now stores a bounded 1,024-command asynchronous FIFO under
its own submission mutex. It drains that FIFO atomically into the renderer
queue before a synchronous command, which preserves the caller-visible result
and resource-lifetime ordering while avoiding one queue lock per sprite.
Renderer commands remain distinct and retain their original FIFO sequence.

The normal archive rebuilt from every gfxlib3 source with `-Werror`. Forced
OpenGL and forced Vulkan OMA runs with copy disabled and one final readback
returned pixel zero in 0.19244 and 0.23334 seconds respectively. The normal
page-copy variant also retained pixel zero. `command-compat-smoke.bas` exited
zero through forced OpenGL and forced Vulkan, covering PAINT, recursive DRAW,
text, CPU image targets, and BMP round-trip behavior across the new ordering
boundary. The `-mt` archive rebuilt and its forced OpenGL command smoke plus
forced Vulkan OMA workload both exited zero.

## 2026-07-18 Android submission-batching regression

The current Android ARM64 threaded position-independent archive, including
the shared asynchronous submission FIFO, was staged in an isolated package
runtime and used to rebuild `android-renderer-smoke.bas`. The fresh package
`org.freebasic.gfx3submitbatch` was launched on the connected AGM A8, serial
`b857d433`, running Android 7.0 on an Adreno 306 OpenGL ES 3.0 driver.

The device has no Vulkan HAL, so the automatic Vulkan probe correctly fell
back to the EGL/GLES backend. The app's own process logged
`FREEBASIC_ANDROID_EXIT:0` after its ten-second visible-frame hold. A capture
taken while the process was foregrounded visibly read `GFX3 ANDROID PASS:
OPENGL ES 3.0` and showed the GPU-rendered page border. This covers the
current archive's renderer selection, primitives, ordered POINT readback,
`SCREENSET`, `SCREENSYNC`, and NativeActivity presentation on physical GLES
hardware. It is not Android Vulkan hardware validation.

The same current desktop archive rebuilt `command-compat-smoke.bas` and
`page-flip-presentation-smoke.bas` with forced Vulkan. Both fixtures exited
zero with `FBGFX3_VULKAN_DEVICE_INDEX=0` (NVIDIA GeForce RTX 2060) and again
with index 1 (Intel UHD 630). The check includes queued primitive work,
ordered compatibility commands, logical-page selection, and presentation on
each Vulkan adapter after submission batching.

## 2026-07-18 PUT and GET graphics-lock fast path

The public GET and PUT wrappers now flush caller-local PSET work through a
graphics-lock-aware compatibility helper and no longer take `mode->mutex`
again while the runtime's `FB_GRAPHICS_LOCK` already serializes the public
mode state. The normal Win64 archive rebuilt from every gfxlib3 source. The
forced OpenGL and RTX Vulkan `command-compat-smoke.bas`,
`page-flip-presentation-smoke.bas`, and `image-smoke.bas` fixtures all exited
zero. These cover pending PSET ordering before image operations, CPU-image
GET/PUT compatibility, page selection, presentation, recursive DRAW, text,
PAINT, and image-file round trips.

The Android ARM64 threaded PIC archive rebuilt with NDK 27.2.12479018 and was
SHA-256 matched into the package runtime. A freshly packaged
`android-primitive-benchmark.bas` process completed with
`FREEBASIC_ANDROID_EXIT:0`; its ordered final pixel was `4278190080`.
`oma-sprite-benchmark.bas` then completed from the same archive with final
pixel zero and `FREEBASIC_ANDROID_EXIT:0` in 1.17121 seconds. This confirms
the changed GET/PUT front end on the physical Adreno 306 OpenGL ES 3.0 path.

The Win64 `-mt` archive also rebuilt from the changed source. Fresh
multithreaded forced-OpenGL and forced-RTX-Vulkan `command-compat-smoke.bas`
executables both exited zero, covering the same ordered image and primitive
contract while the renderer owns its dedicated GPU thread.

The same Android archive was packaged with `transfer-benchmark.bas`, which
exercises PSET, PRESET, AND, OR, XOR, TRANS, ALPHA, explicit BLEND, ADD, GET,
and a direct CPU FB.IMAGE mutation/cache-invalidation check. The physical
process reported final pixel `4285267712` and `FREEBASIC_ANDROID_EXIT:0`.
This is a current-device result for every built-in non-custom PUT mode and
GET, not merely the common transparent-sprite path.

## 2026-07-18 GLES direct-source PUT verification

The GLES blend shader now binds a distinct source surface directly and passes
its origin as a uniform. It continues to snapshot both source and destination
when a PUT reads and writes the same surface, which avoids framebuffer texture
feedback and preserves overlapping self-copy ordering. Fixed blit uniform
locations are cached at link time. The ARM64 archive rebuilt with NDK 27.2,
was SHA-256 matched into the isolated package runtime, and then ran
`transfer-benchmark.bas` on serial `b857d433`.

The physical Android 7 Adreno 306 GLES process reported final pixel
`4285267712` and `FREEBASIC_ANDROID_EXIT:0` after every standard PUT mode,
GET, and the CPU-image cache invalidation check. Destination-reading mode
timings fell materially while preserving that exact final pixel. The expanded
`gpu-surface-smoke.bas` then uploaded four distinct pixels and self-blitted
the first three one pixel right. Its Android process exited zero after proving
the expected immutable-snapshot result `1, 1, 2, 3`.

## 2026-07-18 Vulkan reusable PUT-record verification

The Vulkan runtime now holds a mapped ordered-PUT record buffer in each of its
three submission slots and reuses it only after the corresponding fence
signals. The normal Win64 archive rebuilt with the current sources, including
the GLES compile unit, with the normal warning policy. `command-compat-smoke`
then exited zero with `FBGFX3_VULKAN_DEVICE_INDEX=0` on the RTX 2060 and again
with index 1 on the Intel UHD 630. The threaded archive also rebuilt and the
RTX forced-Vulkan command smoke exited zero.

The RTX isolated OMA run completed 30 frames with final pixel zero in 0.15795
seconds. The current forced-Vulkan transfer fixture reported final pixel
`4290510847` after every standard PUT mode and GET. These checks exercise the
reused records, distinct-source batch path, ordering barriers, readback, and
both available desktop Vulkan adapters.

## 2026-07-18 final GPU-only PAINT regression

The large PAINT smoke's no-define path now requests normal renderer
auto-selection rather than `GFX_NULL`. That matters on Android: `GFX_NULL`
would exercise the CPU reference backend even though the device's selected
driver is OpenGL ES 3.0. The current package used normal GLES selection and
reported `FREEBASIC_ANDROID_EXIT:0` after both 80 by 80 fills and the 80 by
120, 4,720-pixel serpentine wall case. The forced OpenGL and forced Vulkan
Win64 builds of the same fixture also exited zero.

The GLES batch convergence fix was tested with the ARM64 threaded archive
rebuilt by NDK 27.2. It retains all PAINT work, mask expansion, batch-state
copy, comparison, and final composite in GPU resources. The test deliberately
allows only render-target and transfer-source access on the target surface,
so a CPU download-and-reupload workaround cannot satisfy it.

After the final archive rebuild, the isolated
`org.freebasic.gfx3paintlargefinal` package again logged
`FREEBASIC_ANDROID_EXIT:0` on the attached Android device.

## 2026-07-18 stable solid-image PUT verification

The CPU-image cache records a full-image uniform native color alongside its
existing mutation snapshot. The desktop compute-only specialization was tested
with the expanded `image-smoke.bas`: a solid image first performs PSET, then
the same cached image performs AND against an initialized destination. Forced
OpenGL and forced RTX Vulkan both exited zero, proving that the shortcut did
not leave a later source-reading mode with an uninitialized texture.

The Win64 threaded archive rebuilt from the same source and the expanded image
smoke exited zero through both forced OpenGL and forced RTX Vulkan.

The ARM64 GLES archive rebuilt successfully. The connected Android handset ran
the normal OMA fixture with final pixel zero and `FREEBASIC_ANDROID_EXIT:0` in
1.22822 seconds. An initial unguarded rectangle route measured 3.95849 seconds
there, so the current compute-capability gate intentionally leaves GLES on its
correctly batched shader BLIT implementation.

After the current full ARM64 archive rebuild, the isolated
`org.freebasic.gfx3paintsmallfinal` GLES PAINT package also logged
`FREEBASIC_ANDROID_EXIT:0` on the attached handset.

## 2026-07-18 physical GLES batched-PUT cache verification

The GLES ordered PUT-batch shader now resolves its five immutable uniform
locations during backend initialization. The hot batch uses those cached
locations and does not repeat `glGetUniformLocation` for every group of
sprites. Program creation treats a missing location as backend initialization
failure, preserving the existing shader-contract check.

The current ARM64 threaded-PIC archive rebuilt with NDK 27.2.12479018 and its
SHA-256 matched runtime copy packaged `oma-sprite-benchmark.bas` as
`org.freebasic.gfx3.glesbatchcache`. On the attached Android 7 AGM A8, serial
`b857d433`, its Adreno 306 GLES 3.0 backend reported 30 frames, final pixel
zero, `1.138641769997776` seconds, and `FREEBASIC_ANDROID_EXIT:0`.

The same matched archive packaged the expanded `image-smoke.bas` as
`org.freebasic.gfx3.glesbatchimage`. That physical application exited with
`FREEBASIC_ANDROID_EXIT:0`, including the solid-image PSET followed by cached
source AND regression. Its freshly cleared log contained no synchronous
surface-download failure.

## 2026-07-18 exact OpenGL filled-circle batch verification

The OpenGL backend now batches adjacent opaque filled ellipse commands with a
winner/resolve compute pair. The selection shader retains the original
midpoint span algorithm and the resolve shader applies the last command index
at every overlapping pixel. Outlines, arcs, and alpha ellipses remain on the
existing exact per-command shader.

The normal Win64 archive rebuilt with warnings enabled. Forced OpenGL
`circle-compat-smoke.bas` reported the unchanged expected full-screen hash
`6BDC39D7`; the same source linked with gfxlib2 reported `6BDC39D7` as well.
The forced-OpenGL primitive benchmark retained final pixel `4278190080` and
measured the CIRCLE section at 0.00584 and 0.00832 seconds. The fresh gfxlib2
reference retained the same final pixel and measured 0.00579 seconds.

The rebuilt Win64 threaded archive produced the same forced-OpenGL hash. The
current forced RTX Vulkan fixture also retained `6BDC39D7`, confirming the
shared public CIRCLE contract remains identical while Vulkan batching is still
separate optimization work.

## 2026-07-18 ordered Vulkan ellipse batch verification

Vulkan now records adjacent ellipse commands in one submission while retaining
one exact midpoint dispatch and explicit compute dependency per command. This
removes the per-ellipse allocation and queue-submit cycle without allowing
overlapping CIRCLE commands to race.

The current forced RTX Vulkan primitive fixture retained final pixel
`4278190080` and measured CIRCLE at 0.05639 seconds. The forced Vulkan circle
fixture retained hash `6BDC39D7` on both `FBGFX3_VULKAN_DEVICE_INDEX=0` (RTX
2060) and index 1 (Intel UHD 630). Both normal and threaded Win64 archives
rebuilt successfully; the threaded forced-Vulkan fixture retained that hash on
both adapters as well. The threaded Intel primitive run retained final pixel
`4278190080` and measured CIRCLE at 0.13455 seconds. This is a verified
adapter-specific baseline, not evidence that the integrated Vulkan path has
reached the requested gfxlib2 lead.

The ARM64 threaded-PIC archive also rebuilt from this source, including the
non-Vulkan fallback stub. The matched current archive packaged
`circle-compat-smoke.bas` as `org.freebasic.gfx3.ellipsebatchandroid`; the
connected Android 7 Adreno 306 device reported hash `6BDC39D7` and
`FREEBASIC_ANDROID_EXIT:0`.

## 2026-07-18 owned-image cache generation verification

`image-smoke.bas` now covers both cache coherency contracts. It first uploads
an image, mutates it with an image-target PSET, and confirms a second PUT
uses the changed pixel. It also obtains an `IMAGEINFO` pixel pointer, writes
through that public pointer, and confirms the next PUT refreshes the cached
GPU image. Forced desktop OpenGL and forced Vulkan both exited zero with the
expanded smoke.

The ARM64 threaded-PIC archive rebuilt with NDK 27.2.12479018. The current
`org.freebasic.gfx3imagecachefinal` package ran the expanded smoke on the
attached AGM A8 Android 7 handset, Adreno 306 GLES 3.0, and logged
`FREEBASIC_ANDROID_EXIT:0`. This validates the mobile renderer's safe
snapshot fallback as well as the owned-image generation fast path.

`bload-bitfields-smoke.bas` now caches the first BMP, BLOADs a replacement
image, and then PUTs that replacement to the screen. Forced OpenGL and forced
Vulkan both passed the expanded test, confirming BMP BLOAD advances the same
owned-image generation rather than leaving a stale cache texture.
The isolated `org.freebasic.gfx3bloadcachefinal` Android package also logged
`FREEBASIC_ANDROID_EXIT:0` on the attached Adreno 306 GLES device.

## 2026-07-18 physical GLES exact filled-circle batch verification

The GLES backend now converts a compatible opaque filled midpoint ellipse into
one instanced raster draw. The CPU supplies only the exact legacy span list;
the GPU rasterizes every span into the resident integer surface. The fixed
staging array is used only when both radii are at most 256 pixels, which bounds
the sequence to 1,025 spans. Alpha and outline cases retain the older ordered
path.

The ARM64 threaded-PIC archive rebuilt successfully with NDK 27.2.12479018.
The matched `org.freebasic.gfx3.currentprimitive` package ran on the attached
Android 7 AGM A8, serial `b857d433`, Adreno 306 GLES 3.0. It reported final
pixel `4278190080` and CIRCLE time `0.01243135402910411`; the process exited
normally after emitting the benchmark result. The preceding archive measured
`0.3584842700` seconds for that same section.

The same rebuilt archive packaged `circle-compat-smoke.bas` as
`org.freebasic.gfx3.ellipsebatchandroid`. The physical device reported the
expected full-screen hash `6BDC39D7` and `FREEBASIC_ANDROID_EXIT:0`. This
checks full circles, aspect-ratio ellipses, fills, and arcs, including cases
which correctly remain on the ordered GLES route.

`primitive-benchmark.bas` now accepts `GFX2_REFERENCE` on Android, preventing
its normal gfxlib3 selection define so the identical source can provide a
direct gfxlib2 device baseline. The current reference package measured CIRCLE
at `0.3747826560` seconds with final pixel `4278190080`.

## 2026-07-18 Vulkan same-colour rectangle batch verification

The editable Vulkan rectangle shader was regenerated into the checked SPIR-V
header with glslangValidator and spirv-opt. Its y workgroup now selects one
rectangle command; the legacy one-rectangle dispatch remains y=0. The normal
Win64 archive rebuilt with warnings enabled and the threaded archive rebuilt
from the same source.

Forced Vulkan `circle-compat-smoke.bas` retained hash `6BDC39D7` on both RTX
2060 and Intel UHD 630. The primitive fixture retained final pixel
`4278190080` on both adapters. RTX OMA samples measured 0.25073, 0.25155, and
0.27837 seconds; Intel samples were 0.36403, 0.39252, and 0.40778 seconds.

The Android ARM64 threaded-PIC archive rebuilt successfully with the Vulkan
fallback stub. The attached Adreno 306 device remains GLES-only, so it cannot
execute this Vulkan path.

## 2026-07-18 GPU-surface benchmark verification

The new fixture compiled and completed on desktop OpenGL, RTX Vulkan, and
Intel Vulkan. It returned pixel `4278654257` on all three. The attached AGM A8
Android 7 device completed the physical GLES package with
`FREEBASIC_ANDROID_EXIT:0`, pixel `4278654257`, and recorded create 0.02730,
clear 0.00940, upload 0.01434, download 0.03807, map 0.03638, blit 0.00225,
and present 1.04049 seconds at its reduced workload.

## 2026-07-18 ordered tile PUT and reusable download verification

The normal Win64 archive rebuilt with the generalized OpenGL PUT tile shader.
Forced OpenGL `image-smoke.bas` exited zero, and all transfer benchmark modes
retained final pixel `4290510847`. Repeated completed fixture samples measured
PSET at 0.02889 to 0.03604 seconds, TRANS at 0.01906 to 0.02162 seconds, and
the 512-GET section at 0.00209 to 0.00388 seconds.

The Vulkan download cache rebuilt in the same archive. Forced Vulkan
`image-smoke.bas` exited zero with `FBGFX3_VULKAN_DEVICE_INDEX=0` (RTX 2060)
and index 1 (Intel UHD 630). The forced RTX transfer fixture retained pixel
`4290510847` on all three samples, with GET at 0.09216, 0.09771, and 0.08468
seconds. These runs verify cached staging lifetime and adapter selection; they
do not establish a Vulkan lead over gfxlib2 for synchronous GET.

## 2026-07-18 cached POINT and extended PSET stream verification

`point-cache-smoke.bas` passed on forced desktop OpenGL and Vulkan after the
per-page ordered POINT cache was added. The full one-run desktop performance
matrix also completed with every fixture returning its expected result pixel,
including image, file, screen-state, GPU-surface, and OMA workloads.

The PSET stream bound was then raised to 131,072 points. Three forced desktop
OpenGL and Vulkan primitive samples retained pixel `4278190080`; OpenGL PSET
times were 0.01655, 0.01403, and 0.01136 seconds, while Vulkan times were
0.02004, 0.01790, and 0.01373 seconds. The current Android ARM64 archive
packaged and ran the complete 40,000-PSET primitive fixture on the connected
AGM A8, Adreno 306 GLES 3.0 device. It logged PSET time `0.0419296869658865`,
final pixel `4278190080`, and `FREEBASIC_ANDROID_EXIT:0`.

The current threaded Win64 archive also passed `image-smoke.bas` on forced
OpenGL and forced RTX Vulkan. The rebuilt Android ARM64 threaded-PIC archive
was packaged as `org.freebasic.gfx3.tilecurrent` and explicitly started by
component on the attached AGM A8, Android 7, Adreno 306 GLES 3.0 device. It
reported `FREEBASIC_ANDROID_EXIT:0`. The physical device has no Vulkan HAL, so
this is a current GLES build and compatibility check rather than Vulkan test.

## 2026-07-18 queue-aligned Vulkan PUT verification

The normal Win64 archive rebuilt after the Vulkan ordered PUT capacity increased
from 256 to 1,024 entries. Forced RTX transfer-benchmark samples retained final
pixel `4290510847` for every PUT mode and GET. Forced RTX OMA samples retained
pixel zero at 0.09085, 0.09071, and 0.10131 seconds; forced Intel samples
retained pixel zero at 0.14168, 0.15302, and 0.11991 seconds.

The matching Android ARM64 threaded-PIC archive was packaged as
`org.freebasic.gfx3.queuecurrent` and ran `image-smoke.bas` on the attached
Adreno 306 GLES 3.0 device with `FREEBASIC_ANDROID_EXIT:0`.

## 2026-07-18 opaque PAINT span verification

The opaque PAINT span and visited-run changes passed forced OpenGL and Vulkan
pattern, depth, and large GPU-surface smoke fixtures. The current Android
ARM64 archive completed `paint-pattern-depth-smoke.bas` on the attached AGM
A8, Adreno 306 GLES 3.0 device with `FREEBASIC_ANDROID_EXIT:0`.

A final one-run desktop matrix completed after the change. Every registered
gfxlib2, forced OpenGL, and forced Vulkan fixture returned its expected result
pixel, including OMA, file/row, image-cache, screen-state, and GPU-surface
workloads.

## 2026-07-18 Vulkan tile replay verification

The generated Vulkan tile shader and runtime pipeline rebuilt successfully.
Forced `transfer-benchmark.bas` retained final pixel `4290510847` on RTX 2060
and Intel UHD 630. Forced `image-smoke.bas` exited zero on both adapters.
The 30-frame forced OMA fixture retained final pixel zero on RTX at 0.10492
seconds and Intel at 0.12581 seconds.

The threaded Win64 tile path retained transfer pixel `4290510847` on the RTX.
The rebuilt Android ARM64 threaded-PIC archive was packaged as
`org.freebasic.gfx3.vktilecurrent` and the attached Adreno 306 GLES device
reported `FREEBASIC_ANDROID_EXIT:0` for `image-smoke.bas`.

## 2026-07-18 Vulkan mixed-colour filled rectangle verification

Forced primitive-benchmark runs retained final pixel `4278190080` on RTX 2060
and Intel UHD 630 after the public mixed-colour BOX BF batch was routed through
ordered tile replay. Forced `image-smoke.bas` exited zero on both adapters.

## 2026-07-18 Vulkan opaque filled CIRCLE verification

The Win64 gfxlib3 archive rebuilt with the generated winner and resolve shaders.
Forced RTX 2060 Vulkan `primitive-benchmark.bas` completed with final pixel
`4278190080`; its CIRCLE section measured 0.00537 seconds. Forced Intel UHD
630 Vulkan completed the same fixture with final pixel `4278190080` and a
0.21329-second CIRCLE section through the portable ordered GPU shader.

The CIRCLE compatibility fixture hashes 7,680 synchronous `POINT` readbacks.
It exceeded the 34-second bounded runner after this focused change, so it is
not recorded as a completed full-fixture verification here. The previously
recorded image-smoke result remains the latest completed image/API smoke;
the readback-heavy hash should be rerun with a longer dedicated timeout.

## 2026-07-18 producer-side rectangle packet verification

The Win64 gfxlib3 archives rebuilt after the bounded opaque-rectangle packet
was added to the context protocol. `small-filled-rectangle-batch-smoke.bas`,
which issues 30,720 opaque `LINE ... , BF` rectangles, exited zero with forced
OpenGL and forced Vulkan. The normal 30-frame OMA fixture retained final pixel
zero at 0.0522152 seconds on forced OpenGL and 0.0491441 seconds on forced
Vulkan.

The Vulkan packet's one-item ordering boundary was then checked with the full
`image-smoke.bas` and `put-depth-conversion-smoke.bas` fixtures. Both now exit
zero on forced Vulkan, proving that a filled rectangle followed by a regular
non-uniform or RGB565-converted CPU-image `PUT` retains a valid GPU command
path. Forced OpenGL `image-smoke.bas` also exits zero from the same archive.

The current threaded Android ARM64 archive was packaged as
`org.freebasic.gfx3.packetfinal` and run on the connected AGM A8, Android 7,
Adreno 306 GLES 3.0 device. It reported 0.5917512499727309 seconds, final
pixel zero, and `FREEBASIC_ANDROID_EXIT:0`. This is a GLES compatibility
result only: the device does not expose a Vulkan HAL, and GLES intentionally
does not use the desktop rectangle packet. The same archive also completed
the normal `image-smoke.bas` as `org.freebasic.gfx3.packetimage` with
`FREEBASIC_ANDROID_EXIT:0`.

## 2026-07-19 Vulkan PSET PUT verification

The new focused transfer-path fixture reproduced the prior PSET stall at one
operation and at batch size 256 on forced RTX Vulkan. Regenerating the Vulkan
shaders after the PSET/PRESET early-return change made both cases complete.
The full 4,096-operation path and the complete all-mode transfer fixture then
completed on RTX and Intel UHD 630 with final pixel `4290510847`. Forced
OpenGL completed the same all-mode fixture with that pixel. `image-smoke.bas`,
which includes a public image PSET PUT and direct writable-image invalidation,
exited zero on forced RTX Vulkan, Intel Vulkan, and OpenGL.

## 2026-07-19 compatible BLIT packet verification

The Win64 archive rebuilt after the new bounded producer `BLITS` command was
added. Forced OpenGL and forced Vulkan each completed `transfer-path-benchmark`
with its expected pixel `2152765148`, and `image-smoke.bas` exited zero on both
backends. The full all-mode transfer fixture retained final pixel `4290510847`
on both backends; it also verifies direct writable CPU-image invalidation after
the packet-compatible PSET sequence. The normal forced OMA fixture retained
pixel zero on both desktop backends.

The rebuilt Android ARM64 threaded-PIC archive was installed as
`org.freebasic.gfx3.blitpacket`. The connected AGM A8, Android 7, Adreno 306
GLES 3.0 device completed the unchanged 30-frame OMA fixture in
0.6018389579840004 seconds with final pixel zero and
`FREEBASIC_ANDROID_EXIT:0`. This device exposes no Vulkan HAL, and its GLES
backend intentionally does not form the desktop-only `BLITS` packet.

The same rebuilt archive was then installed as `org.freebasic.gfx3.blitpages`
and ran `screen-state-benchmark.bas` on that device. It retained the expected
page/palette/lock pixel `4279511103` and reported `FREEBASIC_ANDROID_EXIT:0`.
This specifically covers the `SCREENCOPY`, `SCREENSET`, and ordered readback
path after the desktop packet changes.

## 2026-07-19 Android primitive baseline and renderer investigation

`primitive-benchmark.bas` was packaged with `GFX2_REFERENCE` and the Android
wrapper was corrected so that this definition suppresses its conditional
gfxlib3 marker. The resulting gfxlib2 APK contains no gfxlib3 context symbol.
With the keyboard button hidden to avoid an unrelated Android 7 Java text
widget failure, the AGM A8 reported these gfxlib2 section times: clear
0.1594373440, PSET 0.02403499995, LINE 0.01011249999, BOX BF
0.01321296900, CIRCLE 0.00315682299, PAINT 0.03114901105, DRAW STRING
0.01074499998, and PUT TRANS 0.00833385397 seconds. Its final pixel was
`4278190080`.

The matching GLES gfxlib3 baseline reported final pixel `4278190080` but was
slower for the same sections, especially clear, LINE, and BOX BF. Investigation
showed that GLES sent each of those commands through a separate fullscreen
primitive shader draw and repeated uniform-symbol lookups. This identifies the
next safe performance work: an explicit instanced GPU geometry path, rather
than relying on old-driver-sensitive clear optimizations.

An experimental `glClearBufferuiv` path wedged the connected Android 7 Adreno
306 driver during screen initialization. It was removed immediately and is not
part of the renderer. An initialization-time primitive uniform cache and a
clear-folding experiment were also removed after their test APKs hung on this
driver while a previously installed APK continued to work. Therefore no
post-change Android timing is claimed yet; the Android performance validation
must resume after the device or test environment is restored.

## 2026-07-19 GLES opaque filled rectangle batch verification

The Android ARM64 threaded-PIC archive rebuilt after the GLES instanced
rectangle path was added. `primitive-benchmark.bas` installed as
`org.freebasic.gfx3.primitivecompare8` completed on the connected AGM A8,
Android 7, Adreno 306 GLES 3.0 device with final pixel `4278190080` and
`FREEBASIC_ANDROID_EXIT:0`.

The unchanged 1,000 public `LINE ... , BF` commands measured
0.0099301040 seconds. This improves the prior gfxlib3 baseline of
0.1744890620 seconds by about 17.6 times and is faster than the verified
gfxlib2 reference time of 0.0132129690 seconds. The path batches only
consecutive rectangles that are filled, opaque, use one target, and share one
clip. It writes ordered instanced GPU quads to the integer surface texture;
outlined, alpha-blended, and differently clipped rectangles retain the exact
primitive fallback.

## 2026-07-19 GLES exact line point-batch verification

`primitive-benchmark.bas` installed as
`org.freebasic.gfx3.primitivecompare10` completed on the same Android GLES
device with final pixel `4278190080` and `FREEBASIC_ANDROID_EXIT:0`. The 1,000
ordinary LINE operations measured 0.0898726560 seconds, down from the original
0.4002541660-second GLES path. This is a 4.45-times improvement, but remains
slower than the 0.0101125000-second gfxlib2 reference, so it is not claimed as
performance parity.

The batch generates the legacy Bresenham coordinate sequence in the GLES
vertex stage and emits one one-pixel GPU point per covered coordinate, including
the original 16-bit style-mask rule. It retains the individual fallback for
alpha lines, mixed clips, and pathological lines exceeding 4,096 pixels. This
removes the former fullscreen draw per line without moving line rasterization
onto the CPU.

The same rebuilt archive also packaged `api-smoke.bas` as
`org.freebasic.gfx3.linebatchapi`; the Android log recorded
`FREEBASIC_ANDROID_EXIT:0`. The packaging wrapper itself timed out after the
application had already exited, so the log marker, not the wrapper status, is
the authoritative completion result for that run.

## 2026-07-19 GLES opaque filled circle span-run verification

The rebuilt archive installed as `org.freebasic.gfx3.primitivecompare12`
completed `primitive-benchmark.bas` on the attached Android device with final
pixel `4278190080` and `FREEBASIC_ANDROID_EXIT:0`. Its 64 public filled CIRCLE
operations measured 0.0066860940 seconds, improving the previous 0.0105338020
seconds by about 1.58 times. gfxlib2's measured reference remains faster at
0.0031568230 seconds, so this is an implementation improvement, not a parity
claim.

Consecutive opaque, filled, small ellipses now append their exact midpoint
spans in command order and issue one GLES instanced span draw. Alpha, outline,
large-radius, and mixed-clip ellipses keep the existing compatibility path.
The same device once more hung when a consecutive-clear collapse was enabled;
that experiment was removed, and each public CLS continues to execute through
the normal GPU primitive path.

## 2026-07-19 desktop dense PSET verification

The new public `pset-benchmark.bas` compiled once with gfxlib2 and once with
each forced gfxlib3 desktop renderer. Its 200,000 ordered changing-colour
PSET operations completed with final `POINT` value `4278190080` in all four
runs. The timings were 0.3890753 seconds for gfxlib2, 0.1326832 for OpenGL,
0.1807450 for Vulkan on the GeForce RTX 2060, and 0.0825465 for Vulkan on the
Intel integrated GPU. This verifies that the benchmark used the public ABI and
that the accumulated GPU point work is visible at the required readback
ordering boundary.

The identical Android fixture was then packaged under two isolated application
IDs using the same device and target size. gfxlib2 completed its 40,000 points
in 0.1162800 seconds; the fresh `libfbgfx3mtpic.a` GLES package completed in
0.0948091 seconds. Both reported final pixel `4290789568` and
`FREEBASIC_ANDROID_EXIT:0`. The two packages prevent output from an older
installed APK being attributed to the rebuilt renderer.

## 2026-07-19 Android page-flip recheck

`android-renderer-smoke.bas` was packaged from the rebuilt ARM64 archive as
`org.freebasic.gfx3.pageflipcurrent`, then started after dismissing the device
lock screen. A capture taken during its success-display interval showed the
expected displayed page 1: the dark inner fill with its light cyan border.
It did not show page 0's circle and cross. The isolated activity subsequently
reported `FREEBASIC_ANDROID_EXIT:0`. This rechecks the visible `SCREENSET 1,
1` transition on the connected GLES device; it does not substitute for a
Vulkan Android page-flip test, because that device has no Vulkan HAL.

## 2026-07-19 coordinate-state benchmark verification

`coordinate-state-benchmark.bas` compiled and completed with gfxlib2, forced
OpenGL, forced RTX Vulkan, and forced Intel Vulkan. All four desktop runs
reported PMAP sum `163328` and final VIEW pixel `4294966777`. The new fixture
uses only the public VIEW, WINDOW, and PMAP statements.

It was also packaged twice for the connected A8 under isolated gfxlib2 and
gfxlib3 application IDs. The packages identify themselves in their native log
before reporting timings. gfxlib2 reported VIEW 0.304476198 seconds; gfxlib3
GLES reported 0.303535364 seconds. Both reported PMAP sum `20416`, final pixel
`4282367417`, and `FREEBASIC_ANDROID_EXIT:0`.

## 2026-07-21 compact glyph and cooperative PAINT verification

The final Win64 archives were relinked into fresh command-compatibility,
console, console-font, PAINT pattern, PAINT depth, large GPU-surface PAINT, and
alpha-primitive executables. Forced OpenGL passed all seven checks. Forced
Vulkan passed the same seven checks independently with device index 0, the
NVIDIA GeForce RTX 2060, and device index 1, the Intel UHD Graphics 630. Both
Vulkan runs identified themselves as `Vulkan compute`; the alpha test did not
silently fall back to OpenGL. The earlier 162-build gfxlib example parity sweep
continues to cover the unchanged public programs, while this fresh matrix
targets every command path modified by the current shader pass.

All nine OMA entry points were rebuilt with `-gfx3 -mt` against the final
archive from their normal source trees. Arkanoid, Behold, Demolition Derby,
kinematics, Quest for a King, Rambo vs Kitty Cat, Star Phalanx, and Tamper each
remained alive through a three-second asset-directory startup check. Duel used
`SD_TEST_BOT=1` and remained alive for fifteen seconds. The final five-run
30-frame OMA benchmark retained pixel zero with medians of 0.06227 seconds on
OpenGL, 0.06313 on RTX Vulkan, and 0.05898 on Intel Vulkan.

The Android ARM64 threaded-PIC archive was rebuilt after the desktop shader
changes and packaged into isolated renderer and primitive applications. Both
installed and completed on the connected Android 7 AGM A8 with Adreno 306 GLES
3.0, reporting `FREEBASIC_ANDROID_EXIT:0`. The primitive package retained final
pixel `4278190080`; its recorded sections were PSET 0.06511, LINE 0.06620, BOX
BF 0.02809, CIRCLE 0.02720, PAINT 0.09550, DRAW STRING 0.05350, and PUT TRANS
0.06225 seconds. The device exposes no Vulkan HAL, so this proves the automatic
GLES fallback and current ARM64 archive, not Android Vulkan support.

## 2026-07-21 GPU asset and projective transform verification

The public extension header now exports 16 aliases. `Gfx3SurfaceLoad` decodes a
BLOAD-compatible bitmap into temporary staging pixels, uploads it once, and
returns an opaque GPU-authoritative source. Ordinary PUT recognizes that
descriptor. `Gfx3SurfaceBlitScaled`, `Gfx3SurfaceBlitRotated`, and
`Gfx3SurfaceMode7` all submit the same bounded inverse-projective command. The
front end calculates one matrix per operation; backend shader lanes calculate
source coordinates, wrapping, filtering, transparency, and PUT results per
destination pixel.

Fresh Win64 normal, multithreaded, PIC, and multithreaded-PIC archives built
without warnings promoted to errors. The public export audit parsed all 59
FBCALL declarations, 29 runtime hooks, and all 16 gfxlib3 extension aliases,
then printed `GFX3_PUBLIC_EXPORT_AUDIT_PASS`.

`gpu-transform-smoke.bas` now covers nearest scale, one exact linear sample,
90-degree pivot rotation, repeating Mode 7 projection, an opaque transformed
surface used by ordinary PUT, and two adjacent overlapping transforms whose
second operation must win. The last case exercises the batch route rather than
ending every operation with a readback. Zero dimensions, zero rotation scale,
zero camera height, and an out-of-range source rectangle are also required to
fail without damaging the renderer. Fresh runs exited zero through:

- Null reference
- OpenGL 4.3 compute
- Vulkan compute device 0, NVIDIA GeForce RTX 2060
- Vulkan compute device 1, Intel UHD Graphics 630

`gpu-asset-smoke.bas` also exited zero through the same four routes. Current
GPU-surface, CPU-image PUT, and page-flip presentation fixtures passed Null
where applicable, OpenGL, and both Vulkan devices. The page checks prove that
the new command and batching changes did not reintroduce the earlier work-page
or presentation ordering fault.

The completed-work transform benchmark retained final pixel `4281135214` on
all desktop GPU routes. Its current raw samples were:

| Backend | 1,500 scales | 750 rotations | 200 Mode 7 planes |
| --- | ---: | ---: | ---: |
| OpenGL 4.3 compute | 0.08433 s | 0.04076 s | 0.01252 s |
| Vulkan, RTX 2060 | 0.03014 s | 0.01917 s | 0.01453 s |
| Vulkan, Intel UHD 630 | 0.03014 s | 0.01450 s | 0.01406 s |

Vulkan records adjacent operations and their barriers into one command-buffer
submission instead of submitting and waiting per transform. The first RTX
post-change sample reduced scale from about 0.0810 to 0.01763 seconds and
rotation from about 0.0575 to 0.01056 seconds. Later raw values vary with GPU
clock and desktop load, but retain the batched architecture and exact pixel.
gfxlib2 has no scale, pivot rotation, filtering, or Mode 7 API with the same
contract, so no synthetic gfxlib2 number is presented as a direct speedup.

All 36 C files also passed a strict Linux syntax compile on the `.99` Ubuntu
host with multithreading and PIC enabled. The embedded GLES single-transform
and instanced-batch vertex/fragment pairs compile and link as OpenGL ES 3.0
through `check-gles-transform-shaders.ps1`. The Android API-24 NDK rebuilt the
actual ARM64 threaded-PIC archive; the official library and both packaging
copies have SHA-256:

```text
33369A6B8B5E1DC7793D53CE0A421ACA510D8871A1F8A2070C352DED27319E0A
```

The earlier physical Adreno 306 single-transform fixture and reduced-count
benchmark both exited zero. The current batch-aware exact package and benchmark
are `obj/android-gfx3-device/gpu-transform-current.apk` and
`obj/android-gfx3-device/gpu-transform-benchmark.apk`. During this final pass
the phone stopped enumerating at the Windows USB layer after an ADB transport
reset, so these two rebuilt packages could not receive their final physical
run. This is recorded as a device-verification gap, not treated as a pass. The
exact package now includes adjacent overlapping operations specifically so the
next device run will prove instancing and ordering together.

## 2026-07-21 corrected ordinary sprite benchmark

`oma-sprite-benchmark.bas` now constructs a non-uniform 13 by 16 RGB565 sprite
instead of a uniform image that gfxlib3 could legally collapse to a rectangle.
The isolated build omitted page copy and performed one final ordered `POINT`
from a pixel written by the sprite stream. Every desktop route completed
30,720 unscaled `PUT TRANS` operations and returned pixel `3784439`.

Seven-run medians were 0.0121904 seconds for gfxlib2 DirectX, 0.0109006 for
gfxlib3 OpenGL, 0.0158551 for Vulkan on device 0, the NVIDIA RTX 2060, and
0.0284729 for Vulkan on device 1, the Intel UHD 630. Fresh `image-smoke.bas`
executables then exited zero on OpenGL and independently on both Vulkan
devices.

`gpu-sprite-benchmark.bas` uploaded the identical native RGB565 pattern once.
Its direct GPU-surface medians were 0.0101348 seconds on OpenGL, 0.0099030 on
RTX Vulkan, and 0.0291048 on Intel Vulkan, again with pixel `3784439`. Public
PUT from the same GPU source measured 0.0129068, 0.0143825, and 0.0304430
seconds respectively. These measurements deliberately contain no scaling or
rotation.

The Win64 normal and multithreaded archives rebuilt successfully after adding
fenced per-submission Vulkan tile buffers. The physical GLES rerun of the
current `BLITS` packet is recorded below.

## 2026-07-21 extended plain-sprite qualification

The corrected fixture now accepts `OMA_BENCHMARK_FRAME_COUNT` so a longer
sample can reduce the effect of desktop GPU clock and scheduling changes. The
qualification build used 300 frames, omitted `SCREENCOPY`, warmed the image
cache before timing, and ended with one ordered `POINT`. It therefore measured
307,200 unscaled, unrotated `PUT TRANS` calls after gfxlib3 had uploaded the
source texture. Seven alternating desktop runs all returned pixel `3784439`.

After the renderer pass, the medians were 0.1184215 seconds for gfxlib2
DirectX, 0.0994626 seconds for gfxlib3 OpenGL, 0.0965940 seconds for Vulkan on
the RTX 2060, and 0.1359875 seconds for Vulkan on the Intel UHD 630. Their
corresponding throughputs were 2.594, 3.089, 3.180, and 2.259 million blits per
second. OpenGL and RTX Vulkan are 19.1 and 22.6 percent faster than gfxlib2.
Intel Vulkan improved by about 70 percent over its preceding 1.33-million
blit/s result, but remains 12.9 percent behind gfxlib2.

NVIDIA uses two shader passes for these destination-independent batches. One
invocation per source pixel atomically records the last BASIC command reaching
each destination pixel, and a resolve pass performs the final write. Intel uses
compact 16 by 16 tile records with separate 8, RGB565, and 32-bit TRANS
pipelines. A 16,384-command packet was removed after it caused severe tail
latency on both adapters; the retained producer and Vulkan limit is 8,192.

Fresh `image-smoke.bas` runs exited zero on both Vulkan devices. The current
`trans-batch-depth-smoke.bas` also exited zero at 8, 16, and 32-bit depths on
both devices, first with the normal archive and again with the multithreaded
archive. This covers the specialized sprite routes and the general PUT
fallbacks modified by the pass.

The attached AGM A8, serial `b857d433`, enumerated normally and selected its
Adreno 306 OpenGL ES 3.0 renderer. The ARM64 multithreaded PIC gfxlib3 archive
was rebuilt with NDK 27.2.12479018 and matched the staged packaging copy at
SHA-256 `8C2C637DE1F25F489F22EA160055BE8481604A8D51F5F7BF73901BFA9916CDF8`.
Fresh packages `org.freebasic.gfx2.plainsprite300` and
`org.freebasic.gfx3.plainspritecurrent` were installed rather than reusing an old
APK. Five alternating runs all returned pixel `3784439` and
`FREEBASIC_ANDROID_EXIT:0`.

Android medians were 1.823304 seconds for gfxlib2 and 1.477863 seconds for
gfxlib3 GLES, or about 0.1685 and 0.2079 million blits per second. gfxlib3 is
therefore 23.4 percent faster by completed-work throughput. Its producer
portion was 0.360330 seconds, followed by about 1.117533 seconds of ordered GPU
completion. The device also compiled all three new depth-specific GLES TRANS
programs and passed `trans-batch-depth-smoke.bas` with
`FREEBASIC_ANDROID_EXIT:0`.

## 2026-07-22 GPU-target clipping boundary

`put-clipping-smoke.bas` covers all four screen corners, a smaller VIEW SCREEN
rectangle, and a fully offscreen integer-limit sentinel. It exited zero with
gfxlib2, forced desktop OpenGL, Vulkan device 0 (NVIDIA GeForce RTX 2060), and
Vulkan device 1 (Intel UHD Graphics 630). The test compares the native RGB
portion because gfxlib2 and Vulkan POINT omit stored alpha while desktop
OpenGL retains it.

After the Vulkan sparse-selector change, `image-smoke.bas`,
`put-depth-conversion-smoke.bas`, and `trans-batch-depth-smoke.bas` were rebuilt
and exited zero independently on both Vulkan adapters. The clipping smoke was
also rebuilt and passed on both adapters. These cover the specialized TRANS
paths, every supported logical depth, generic PUT modes, CUSTOM fallback, and
the integer-limit command boundary. The generated SPIR-V header exactly
matched a clean regeneration at SHA-256
`DB106A5C8A7095F68844DE0D64E3D46FD473FFCDE231509B039125E6F889ADE8`.

The Win64 normal and multithreaded gfxlib3 archives rebuilt successfully. The
final Android ARM64 multithreaded PIC archive and staged package copy match at
SHA-256 `0BA074A77FF1B8FF63744CECF9EC02542624F6559051CE5A1970FD075E03A58F`.
Fresh APKs were then produced from that exact staged archive:

- `org.freebasic.gfx3.putclipsmoke`
- `org.freebasic.gfx3.putclipcurrent`
- `org.freebasic.gfx3.plainspriteclipcurrent`

All three were installed on physical device `b857d433`, AGM A8 with Adreno 306
OpenGL ES 3.0, and reported `FREEBASIC_ANDROID_EXIT:0`. The smoke completed all
assertions. The clipping benchmark and ordinary sprite benchmark both returned
pixel `3784439`. The final ordinary sprite run completed in 1.522110 seconds;
the final clipping run submitted in 0.401530 seconds and drained in 0.526562
seconds. The handset still exposes no Vulkan HAL, so Android Vulkan remains
unavailable rather than untested.

## 2026-07-22 renderer overlap qualification

The common context now hands each full backend-sized `BLITS` packet to the
renderer immediately. The GLES backend follows each asynchronous fence with a
nonblocking `glFlush`. A live thread sample before the GLES change showed the
BASIC producer active on one native thread and the renderer in
`adreno_drawctxt_wait` on another. This excluded the command queue and the
FreeBASIC graphics lock as the cause, and identified driver submission as the
remaining offload boundary.

`sprite-offload-benchmark.bas` was added to the performance matrix for gfxlib2,
forced OpenGL, and forced Vulkan. The fixture reports submission, completion,
a 0.250-second independent CPU phase, residual completion, iteration count,
and ordered final pixel. Nine rotated desktop rounds all returned pixel
`3784439`. Completed medians were 0.097461 seconds for gfxlib2, 0.057167 for
OpenGL, 0.062524 for Vulkan device 0 (NVIDIA GeForce RTX 2060), and 0.089560
for Vulkan device 1 (Intel UHD Graphics 630). Residual waits after CPU work
were 0.001897, 0.001205, and 0.002076 seconds for the three GPU routes.

Fresh `image-smoke.bas` executables exited zero with forced OpenGL and with
both Vulkan devices. Fresh multithreaded `put-clipping-smoke.bas` executables
also exited zero with OpenGL, RTX Vulkan, and Intel Vulkan. This covers both
normal and multithreaded archives and the image and clipping paths changed by
the submission work.

The connected physical device remained serial `b857d433`, AGM A8 with Adreno
306 OpenGL ES 3.0 and no Vulkan HAL. These fresh packages all reported
`FREEBASIC_ANDROID_EXIT:0`:

- `org.freebasic.gfx3.spriteoffloadflush`
- `org.freebasic.gfx3.spriteoffloadlongflush`
- `org.freebasic.gfx3.putclipflush`
- `org.freebasic.gfx3.putclipsmokeflush`

Seven alternating ordinary-sprite runs used
`org.freebasic.gfx2.spriteoffload` as the reference. Median completed times
were 1.833228 seconds for gfxlib2 and 1.103041 seconds for gfxlib3 GLES.
gfxlib3 submitted in 0.379419 seconds, and its residual completion fell from
0.721045 to 0.468407 seconds while the application performed 0.250 seconds of
independent work. The CPU iteration medians were 12,034,048 for gfxlib2 and
11,988,992 for gfxlib3. Five clipped-sprite runs produced a 0.543765-second
gfxlib3 median, and the Android clipping smoke passed every boundary assertion.

A diagnostic 2,000-frame build submitted 2,048,000 sprites in each measured
phase and exited zero. Its baseline completed in 7.049880 seconds; its overlap
phase completed in 7.044606 seconds with 4.114113 seconds left after the CPU
interval. This rechecks sustained packet and fence operation beyond the normal
benchmark length.

The final archives have these SHA-256 values:

| Archive | SHA-256 |
| --- | --- |
| Win64 `libfbgfx3.a` | `2A75B9948424AB7C3A994DC2BB28AEBB01DDDC626633E4497CEC60C02F52276E` |
| Win64 `libfbgfx3mt.a` | `91584BD283B82B7126F3682AB9106DEFF0F5FF4E1F5DC0171F510C9C9BA155E1` |
| Android ARM64 `libfbgfx3mtpic.a` | `2D96FE52B6BB62BDCE487EAB796C8F14A0FF5E66249B4281D4BCD09892081519` |

The Android archive and its staged packaging copy match exactly. The aggregate
Windows `gfxlib3` make goal encountered the local prerequisite check for a
missing `rsync`, after the normal archive had already linked. Building the
normal and multithreaded archive targets directly completed successfully; the
missing host tool did not affect either archive or the tests above.

## 2026-07-22 public-query and compatibility-state optimization

The repeatable matrix now includes `palette-family-benchmark.bas` and
`control-query-benchmark.bas`. `coordinate-state-benchmark.bas` gained an
independent POINTCOORD section, and `screen-state-benchmark.bas` gained an
independent FLIP section. The public coverage ledger maps display, drawing,
image, input, console, and gfxlib3 extension families to a timing fixture or an
explicit correctness-only boundary. The current public-export audit finds all
59 declared graphics entries, 29 runtime graphics hooks, and 16 gfxlib3
extension aliases in the rebuilt archive and reports
`GFX3_PUBLIC_EXPORT_AUDIT_PASS`.

A complete `run-performance-matrix.ps1 -Runs 1` build-and-run sweep then
completed all 62 gfxlib2/OpenGL/Vulkan workload variants and emitted 242 timing
medians. Every shared correctness value matched: mode, console, primitive,
PSET, PAINT, arc, transfer, DRAW, page, palette, coordinate, image-cache,
file-row, ordinary sprite, clipped sprite, and offload pixels or checksums.
The GPU-only surface and transform pairs also matched between OpenGL and
Vulkan. This one-sample sweep is a coverage gate; the multi-run figures below
are used for performance conclusions.

The first control-query run exposed GPU barriers in input operations that only
need CPU snapshots. The initial fix introduced an `INPUT_POLL` protocol
command which avoided the GPU sequence but still made every empty SCREENEVENT
wait for the window thread. The 2026-07-27 pass superseded that intermediate
form: SCREENEVENT now matches gfxlib2 by reading only the already published
event ring. The independent 10 ms platform pump owns native event publication,
and an activity generation avoids redundant pumps during ordinary renderer
work. SETMOUSE publishes its accepted state asynchronously; touch and other
input reads inspect their mutex-protected snapshots directly. Win32 also
caches joystick capabilities and limits XInput refresh to one poll per 8 ms.

The actual `input-smoke.bas` executable then exited zero with OpenGL and Vulkan
in normal and `-mt` builds. It covers immediate PostMessage key, mouse, wheel,
focus, close, SETMOUSE, touch fallback, native handle, and KEY_QUIT behavior.
The intermediate three-run SCREENEVENT medians for 1,024 calls were 0.077129
seconds on OpenGL, 0.068737 on Vulkan device 0 (RTX 2060), and 0.062471 on
Vulkan device 1 (Intel UHD 630). The earlier full-barrier OpenGL run was
0.519124 seconds. The final queue-only 4,096-call medians are recorded in the
2026-07-27 checkpoint: 0.00895 seconds on OpenGL and 0.00906 on gfxlib2, with
Vulkan normally between 0.0087 and 0.012 seconds.
OpenGL's focused SETMOUSE, absent GETJOYSTICK, and GETTOUCHCOUNT results are now
0.000048, 0.007778, and 0.003227 seconds respectively, down from 0.034452,
2.874266, and 0.520736 seconds.

WINDOW, PMAP, and POINTCOORD now use the existing public graphics lock instead
of taking an additional mode mutex around caller-local state. The focused
fixture retained PMAP sum `163328`, cursor sum `1720320`, and final pixel
`4280832120` through gfxlib2, OpenGL, RTX Vulkan, and Intel Vulkan. OpenGL's
WINDOW, PMAP, and POINTCOORD sections measured 0.000079, 0.000048, and 0.000300
seconds, compared with 0.002234, 0.003214, and 0.030636 before the change. RTX
Vulkan measured 0.000029, 0.000026, and 0.000171 seconds.

The page compatibility layer no longer sends a redundant PRESENT after
PAGE_SET or a BLIT into the visible surface. Each backend already marks those
operations dirty and presents the final surface at the end of the ordered
drain. `page-flip-presentation-smoke.bas` exited zero on desktop OpenGL and
Vulkan. The same source was freshly packaged as
`com.freebasic.gfx3pagecurrent` and reported `FREEBASIC_ANDROID_EXIT:0` on the
connected Android device.

The ARM64 archive was rebuilt with NDK 27.2.12479018 and staged into the Android
packaging runtime. A fresh `control-query-benchmark.bas` package selected GLES
and exited zero. Its 256 SCREENEVENT calls fell from 0.011402 to 0.000129
seconds after the direct Android snapshot change. GETMOUSE measured 0.000217
seconds and the touch families measured 0.000191 to 0.000230 seconds. The same
gfxlib2 source measured 0.178643 seconds for GETMOUSE and 0.086920 to 0.153761
seconds for touch. A fresh `android-input-smoke.bas` package received an ADB
lowercase A, printed `GFX3_ANDROID_INPUT_PASS a`, and exited zero. Device
`b857d433` remains the API 24 AGM A8 with Adreno 306 OpenGL ES 3.0 and no Vulkan
HAL, so mobile Vulkan remains unavailable on this hardware.

Final archive hashes for this pass are:

| Archive | SHA-256 |
| --- | --- |
| Win64 `libfbgfx3.a` | `1875E061588C7FFDE6DAD21752FB30CB05067FFAEFDBE31B590D3F4E7C43BE83` |
| Win64 `libfbgfx3mt.a` | `AA55F2C6060193A6F6B504FC2F9C83AA1E47E5A9D3BEA96FF2F7012874B005E7` |
| Android ARM64 `libfbgfx3mtpic.a` | `A79BC35F9C902162CF9E01C00C5B9C86323C5318FB88AE9EC0C37C92039BE320` |

The Android archive and staged packaging copy match exactly.

The same source was copied into an isolated tree on the Linux x86-64 host at
192.168.250.99 and rebuilt with `make -B -j4 gfxlib3`. GCC compiled the X11,
OpenGL, GLES, Vulkan, input, renderer, and compatibility sources with the
project's warning, hardening, and implicit-declaration error flags. All four
archive variants completed successfully, and each contains 36 object members:

| Linux x86-64 archive | SHA-256 |
| --- | --- |
| `libfbgfx3.a` | `2FAA77DC93A2DF0AFF91E25E9698407A6AF5769B6F5AC501460980FB782020E9` |
| `libfbgfx3pic.a` | `C49654B74D04E1EF0D28EC0C04F64AB112F4A9E4A7B388865DF82F3D6AF0D1CD` |
| `libfbgfx3mt.a` | `623EEA0CC372863E38CA962ECFE51186A78307C510B15D65A848668D6B380331` |
| `libfbgfx3mtpic.a` | `F0D097DD14C8C5C54F047E6C644EF197F09693FB5E4FC1961EF47B98AAA5ABB3` |

## 2026-07-22 page-copy throughput and offload pass

The screen-state fixture now uses two distinct immutable source pages and a
third visible destination. It reports BASIC producer time before SCREENSYNC
and completed time after that explicit boundary for both SCREENCOPY and FLIP.
This separates application-thread offload from end-to-end GPU throughput.

Visible SCREENSET, SCREENCOPY, and FLIP submit pending packets to the renderer
without waiting. OpenGL batches complete page copies through
`glCopyImageSubData`; Vulkan batches them as `vkCmdCopyBuffer` operations with
batch-level memory dependencies. Both paths propagate exact content tokens and
remove only copies proven dead by later operations in the same ordered page
run. Page-only renderer drains defer the native swap until SCREENSYNC, a
platform poll, or a later non-page command. GLES shares the presentation rule
while retaining its shader copy implementation.

Seven-run medians for 256 alternating page requests were:

| Backend and adapter | Page submit | Page complete | FLIP submit | FLIP complete |
| --- | ---: | ---: | ---: | ---: |
| OpenGL | 0.0043508 s | 0.0214914 s | 0.0035900 s | 0.0124198 s |
| Vulkan, NVIDIA RTX 2060 | 0.0043895 s | 0.0093913 s | 0.0044528 s | 0.0086162 s |
| Vulkan, Intel UHD 630 | 0.0032280 s | 0.0124838 s | 0.0034048 s | 0.0083304 s |

The stable gfxlib2 FLIP reference measured 0.0054706 seconds to submit and
0.0156030 seconds through completion. gfxlib2 page-copy samples were bimodal
under the desktop compositor, with a 0.360212-second submit median and a low
0.011865-second sample, so no precise page-copy ratio is inferred from them.
Every desktop route returned pixel `4283458815`.

The partial-VIEW page-copy smoke exits zero through desktop OpenGL, Vulkan
device 0, and Vulkan device 1. The same test was packaged as
`com.freebasic.gfx3pagecurrent`, installed on physical Android device
`b857d433`, and reported `FREEBASIC_ANDROID_EXIT:0`. This proves the full-copy
optimization still routes a clipped page copy through the general shader path.

The current Android screen-state package selected OpenGL ES 3.0 and reported:

| Operation, 64 requests | Producer | Explicit completion |
| --- | ---: | ---: |
| SCREENCOPY | 0.000741615 s | 0.107299375 s |
| FLIP | 0.000752032 s | 0.103285157 s |

Its final lock-test pixel was `4283458623`, the expected value after the
Android fixture's 64 byte writes, and the process reported
`FREEBASIC_ANDROID_EXIT:0`. The device remains an API 24 AGM A8 with an Adreno
306 and no Vulkan HAL, so this is GLES evidence only.

The final public-export audit at this page-batching checkpoint again reports 59 public graphics declarations,
29 runtime graphics hooks, and 16 gfxlib3 extension aliases, ending in
`GFX3_PUBLIC_EXPORT_AUDIT_PASS`. A complete one-run performance matrix built
and ran all 62 gfxlib2/OpenGL/Vulkan variants. Its sources expose 248 timing
fields after adding the separate page and FLIP producer values. Shared final
pixels, counts, and checksums matched. Final page samples from that coverage
run were:

| Backend | Page submit | Page complete | FLIP submit | FLIP complete |
| --- | ---: | ---: | ---: | ---: |
| gfxlib2 | 0.3887447 s | 0.4090777 s | 0.0088918 s | 0.0322157 s |
| OpenGL | 0.0036243 s | 0.0221072 s | 0.0040233 s | 0.0098131 s |
| Vulkan | 0.0066239 s | 0.0092403 s | 0.0027964 s | 0.0199052 s |

The page/input smoke executables were then rebuilt from the final archives.
Page presentation exited zero under OpenGL and both Vulkan adapters. The full
Win32 input injection smoke exited zero under OpenGL and both Vulkan adapters
in normal and multithreaded builds.

All 36 gfxlib3 source objects were rebuilt for every Win64 archive variant and
for Android ARM64 MT/PIC. The checkpoint archive hashes were:

| Archive | SHA-256 |
| --- | --- |
| Win64 `libfbgfx3.a` | `64EFD15936DE7FEDB7FB4120E8441CC703258E2EA9C18A181B10FBF365732F46` |
| Win64 `libfbgfx3pic.a` | `64EFD15936DE7FEDB7FB4120E8441CC703258E2EA9C18A181B10FBF365732F46` |
| Win64 `libfbgfx3mt.a` | `8E16117D966CADB47808512815AEF1C4BF7E5974C680E74124322CD2DF463FE1` |
| Win64 `libfbgfx3mtpic.a` | `8E16117D966CADB47808512815AEF1C4BF7E5974C680E74124322CD2DF463FE1` |
| Android ARM64 `libfbgfx3mtpic.a` | `9AF0ABED787417F305503CFCDA1CC63426D103E01BDB2727A1116B774240E2CC` |

The Android archive and packaging-stage copy match exactly and each contains
36 object members.

The checkpoint source subset was also rebuilt on Linux x86-64 at
192.168.250.99 with `make -B -j4 gfxlib3`. All four archives contain 36 object
members:

| Linux x86-64 archive | SHA-256 |
| --- | --- |
| `libfbgfx3.a` | `23E6B1AA2B9B9F7FDCE121FBBA4A08B3268B788F82E809598D5C2CFE87C0A821` |
| `libfbgfx3pic.a` | `8D081D4C4E7F1B8D74A801EC7B8F5CF22CCC797F5E6276DD959BF7BD5853FCC1` |
| `libfbgfx3mt.a` | `3059C4E5C6B2AD0C0EC30AE896C8C103007B751456679350597707508C4A8869` |
| `libfbgfx3mtpic.a` | `E5267A4265B14B79C73508867C73125E98FCCFD2CC08BD34F1FEBEA3D9C5D7F1` |

The verified remote staging tree was removed after hashing. It was temporary
and is not recoverable.

## 2026-07-22 multi-dispatch PAINT and offload verification

The desktop PAINT selector now sends bounded normal screen pages to OpenGL or
Vulkan compute instead of treating transfer capability as a reason to download
them. CPU images and normal GLES pages retain their documented compatibility
routes. GPU-only GLES surfaces retain fragment-mask ping-pong.

The desktop shader uses four globally ordered phases. Candidate bounds are
discovered by one workgroup. OpenGL 16 by 16 or Vulkan 16 by 8 workgroups then
verify every candidate interior and adjacent perimeter pixel. A storage barrier
precedes the parallel fill. The final one-workgroup dispatch runs exact scanline
topology only after rejection. The Vulkan phase word and bounds live after the
device-local visited map and queue. `vkCmdFillBuffer` phase transitions have
explicit transfer-to-compute and compute-to-transfer dependencies, and each
fenced submission slot owns its command and scratch allocations.

The renderer may execute only the final command in an adjacent run of solid,
opaque, non-border recolours with the same target, seed, clip, and border. The
predicate also requires strictly increasing command sequence numbers. Pattern,
alpha, border-coloured, and incompatible fills cannot be combined. The new
`paint-coalescing-smoke.bas` verifies both the allowed case and a
border-coloured topology barrier on screen and GPU-only surfaces.

`vulkan_paint.comp` compiled for Vulkan 1.0 with `glslangValidator`, all embedded
modules were regenerated, and all four Win64 archives rebuilt with warnings
enabled. The following freshly linked fixtures exited zero under desktop
OpenGL, Vulkan device 0, and Vulkan device 1:

- `paint-coalescing-smoke.bas`;
- `paint-pattern-smoke.bas`;
- `paint-pattern-depth-smoke.bas`;
- `paint-large-gpu-surface-smoke.bas`;
- `alpha-primitives-smoke.bas`.

Device 0 was the NVIDIA GeForce RTX 2060 and device 1 was the Intel UHD
Graphics 630. Windows per-executable graphics preference was then applied to a
fresh OpenGL adapter probe, coalescing smoke, and benchmark. The probe reported
`vendor=Intel renderer=Intel(R) UHD Graphics 630`, both correctness programs
exited zero, and every temporary registry value was restored or removed in the
cleanup block. No test-specific GPU preference remained.

The final desktop PAINT medians in seconds were:

| Backend and adapter | First complete | Repeated submit | Repeated complete |
| --- | ---: | ---: | ---: |
| OpenGL, RTX 2060 | 0.0134757 | 0.0002159 | 0.0039004 |
| Vulkan, RTX 2060 | 0.0044376 | 0.0003678 | 0.0021139 |
| Vulkan, Intel UHD 630 | 0.0154313 | 0.0002053 | 0.0066986 |
| OpenGL, Intel UHD 630 | 0.0143262 | 0.0001951 | 0.0090450 |

The stable preceding gfxlib2 seven-run reference was 0.0058297 seconds for the
first fill, 0.0802542 seconds for repeated submission, and 0.0802549 seconds
through completion. The repeated workload contains fifteen adjacent recolours,
so its completed result intentionally includes exact elimination of fourteen
unobservable intermediate colours. The first-fill column remains the check on
one independently observable operation.

The final one-run public performance matrix completed all 62 variants and
produced 251 raw timing records plus 251 corresponding median records. PAINT
returned first pixel `4280226057`, final pixel `4291850288`, and border pixel
`4278190335` through gfxlib2, OpenGL, and Vulkan. Its operation timings were:

| Backend | First complete | Repeated submit | Repeated complete |
| --- | ---: | ---: | ---: |
| gfxlib2 | 0.0070439 | 0.8676802 | 0.8676820 |
| OpenGL | 0.0102596 | 0.0002861 | 0.0062746 |
| Vulkan, RTX 2060 | 0.0046925 | 0.0002051 | 0.0024496 |

The gfxlib2 repeated value is a known host-scheduling outlier and is retained
as raw evidence, not used for the primary ratio. The complete matrix log is
`.codex-tmp-gfx3-paint/matrix-multi-final.log` in the working tree.

The Android ARM64 MT/PIC archive was rebuilt with NDK 27.2.12479018. A fresh
`org.freebasic.gfx3.paintoffloadfinal` package selected OpenGL ES 3.0 on
physical device `b857d433`, returned the expected first, final, and border
pixels, and reported `FREEBASIC_ANDROID_EXIT:0`. Five-run medians for its three
recolours were 0.0166655 seconds to submit and 0.0455994 seconds through
completion. The paired gfxlib2 values were 0.1466764 and 0.1467031 seconds. A
fresh GPU-only `org.freebasic.gfx3.paintcoalescefinal` package also reported
`FREEBASIC_ANDROID_EXIT:0`. The device remains an API 24 AGM A8 with an Adreno
306 and no Vulkan HAL, so it supplies GLES rather than Vulkan evidence.

Every final local archive contains 36 object members:

| Archive | SHA-256 |
| --- | --- |
| Win64 `libfbgfx3.a` | `6A856385080C7B934A06288AB0AA588912826250BB49ECAE3AB735105865750C` |
| Win64 `libfbgfx3pic.a` | `6A856385080C7B934A06288AB0AA588912826250BB49ECAE3AB735105865750C` |
| Win64 `libfbgfx3mt.a` | `9E692DAC0A645C8F483B1E478ED931042C9A339C8F5315310B021D85611BA692` |
| Win64 `libfbgfx3mtpic.a` | `9E692DAC0A645C8F483B1E478ED931042C9A339C8F5315310B021D85611BA692` |
| Android ARM64 `libfbgfx3mtpic.a` | `E2E89278C52C57ACB20B1A31B57585BA2A1F0CED33ECC0F94914F73579BBF5C6` |

The final source subset also rebuilt on Linux x86-64 at 192.168.250.99. Each
archive contains 36 members:

| Linux x86-64 archive | SHA-256 |
| --- | --- |
| `libfbgfx3.a` | `3862E4DA5B6DEFD195A495D61E8ED9D9FC6A163CDDAEF3249AF7105D575C9857` |
| `libfbgfx3pic.a` | `A09152919A3A59C35CA484BD6BB4DD291504CE38C37E37D1AFC6772DE6A65DF6` |
| `libfbgfx3mt.a` | `6457E032B1F5C4AD5CDB95032A26093DC9EC09BDFC164D54A87095D846BFE96C` |
| `libfbgfx3mtpic.a` | `9BE86DC120BFFCA174C52DC03624235CB23351424DF4716FC05EF10EBE4F18CA` |

The remote path was created with `mktemp`, checked against the expected
`/tmp/gfx3-paint-final.*` form, resolved before deletion, and removed after the
hash and member checks. It was temporary and is not recoverable.

## 2026-07-22 complete OMA desktop and AGM A8 delivery

The complete active OMA set in this tree was built from unchanged game source
with public gfxlib3 selection. Support directories, generated Android staging
trees, and the separate DOS packages are not independent entry points. The
nine delivered programs are:

| Program | Source entry point |
| --- | --- |
| Arkanoid | `OMA/ArkanoidTest/ArkanoidTest.bas` |
| Behold | `OMA/Behold/Behold.bas` |
| Demolition Derby | `OMA/DemolitionDerby/main.bas` |
| Duel 999 | `OMA/duel999/SD_Main.bas` |
| Kinematics | `OMA/kinematics/kinematic_man_two_bodies_self_collision_friction.bas` |
| Quest for a King | `OMA/QuestForAKing-Win32-1.5/src/win11.bas` |
| Rambo vs Kitty Cat | `OMA/RamboVsKittyCat-Win32-0.1/killquest.bas` |
| Star Phalanx | `OMA/StarPhalanx-win32-0.5/entryv2.bas` |
| Open Market | `OMA/Tamper/tamper/src/openmarket_bootstrap.bas` |

### Build matrix

All nine programs built for each requested target:

| Target | Count | Selection and build notes |
| --- | ---: | --- |
| Windows x86-64 | 9 | Current checkout compiler, `-gfx3 -mt -arch x86_64` |
| Windows x86 | 9 | Current compiler lowered each source with `-gfx3 -mt -arch 686`; the installed Win32 compiler supplied only the stable process entry point and Win32 system link |
| Android ARM64 | 9 | Current `fbc-android -gfx3 -mt --target android-aarch64` wrapper and final threaded PIC archive |

The installed standalone Win32 compiler predates the public `-gfx3` switch.
It was not allowed to recompile the game code or select gfxlib2. Each game was
compiled to a PE32 object by the current gfxlib3-aware compiler. A comment-only
FreeBASIC main module then selected the 32-bit runtime entry point while the
link explicitly selected the current `libfbgfx3mt.a`. Every delivered Win32
file reports PE machine `0x014c`.

The final archive hashes are:

| Archive | SHA-256 |
| --- | --- |
| Win64 `libfbgfx3.a` | `FFC985884A434C06B670340A8B57FEF74684F334CAD983141AA5CAA07C7B51D9` |
| Win64 `libfbgfx3mt.a` | `109A4A90C1DF47127DF1B109A6139F4B964D52F3FD2DCCFEDFCE4C13894EDEF4` |
| Win32 `libfbgfx3.a` | `B43BD04A70E90D1BD08507D0BA8C036DB23B7092B88B9CCEF536A04FBE9B614A` |
| Win32 `libfbgfx3mt.a` | `871769BEE4B820DFF4188CA993DEBC95883FBAE9CB3925153CB33810A6918DAF` |
| Android ARM64 `libfbgfx3mtpic.a` | `56503A36FD59CDE2A2D9919BE7DD191B57E969625B6A73EBC705301643A35CA0` |

The Android archive and its isolated packaging-stage copy have the same hash.
The deliverable directories contain nine final Win64 executables, nine final
Win32 executables, and nine final APKs. Diagnostic and gfxlib2 reference APKs
are retained beside the Android artifacts but are not part of that count.

### Windows launch matrix

Each automatic-backend executable was started from its real game asset
directory and observed for six seconds. A passing launch had to remain live,
own a nonzero graphics window, answer the Windows responsiveness query, and
produce no fatal gfxlib3 log. All 18 launches passed:

| Program | Win64 automatic | Win32 automatic |
| --- | --- | --- |
| Arkanoid | Pass | Pass |
| Behold | Pass | Pass |
| Demolition Derby | Pass | Pass |
| Duel 999 | Pass | Pass |
| Kinematics | Pass | Pass |
| Quest for a King | Pass | Pass |
| Rambo vs Kitty Cat | Pass | Pass |
| Star Phalanx | Pass | Pass |
| Open Market | Pass | Pass |

Both automatic sets selected the desktop OpenGL gfxlib3 backend. The recorded
context was OpenGL 4.3 on NVIDIA driver 595.79.

Every Win64 game was then launched for five seconds with Vulkan requested and
`FBGFX3_VULKAN_DEVICE_INDEX` set first to 0 and then to 1. All 18 additional
launches remained live with responsive nonzero graphics windows and no fatal
log. Vulkan loader probes recorded the actual logical-device creation:

| Device index | Physical adapter | Driver module |
| ---: | --- | --- |
| 0 | NVIDIA GeForce RTX 2060 | `nvoglv64.dll` |
| 1 | Intel(R) UHD Graphics 630 | `igvk64.dll` |

The per-title standard error logs contain no OpenGL initialization message in
the Vulkan runs. The loader probe logs show `vkCreateDevice` selecting the
listed adapter and driver for each diagnostic index.

### Physical AGM A8 installation

Device `b857d433` identifies itself as an AGM A8 running Android API 24. It has
an Adreno 306 and exposes no Vulkan package-manager feature or Vulkan HAL.
gfxlib3 therefore follows its normal ordered backend policy and selects
OpenGL ES 3.0. This device supplies physical GLES evidence, not Android Vulkan
evidence.

The final installed packages are:

| Program | Package |
| --- | --- |
| Arkanoid | `org.freebasic.oma.gfx3.arkanoid` |
| Behold | `org.freebasic.oma.gfx3.behold` |
| Demolition Derby | `org.freebasic.oma.gfx3.demolitionderby` |
| Duel 999 | `org.freebasic.oma.gfx3.duel999` |
| Kinematics | `org.freebasic.oma.gfx3.kinematics` |
| Quest for a King | `org.freebasic.oma.gfx3.qfak` |
| Rambo vs Kitty Cat | `org.freebasic.oma.gfx3.rambo` |
| Star Phalanx | `org.freebasic.oma.gfx3.starphalanx` |
| Open Market | `org.freebasic.oma.gfx3.openmarket` |

Each package was launched from a cleared logcat, remained live during its
observation interval, and produced no FATAL EXCEPTION, native fatal signal,
SIGSEGV, ANR, or nonzero FreeBASIC exit marker. Final screenshots show the
following first useful states:

| Program | Observed state |
| --- | --- |
| Arkanoid | Bricks, ball, paddle, score, and lives |
| Behold | Title/menu and Android touch controls |
| Demolition Derby | Setup screen and Android touch controls |
| Duel 999 | Server/join setup with correct RGB565 colours and touch controls |
| Kinematics | Animated figures and physics geometry |
| Quest for a King | Mostly black initial state with Android touch controls |
| Rambo vs Kitty Cat | Active game scene |
| Star Phalanx | Main menu |
| Open Market | Title screen |

Quest for a King's initial state was checked against a separately packaged
gfxlib2 reference on the same device. The reference also showed the mostly
black startup state with only thin content and controls, so this is not a
gfxlib3-only failure. The two builds enter their unusual startup flow at
different timings after injected input, so no pixel-identical post-input claim
is made.

Temporary trace and gfxlib2 reference packages used during diagnosis were
uninstalled. The device now retains only the nine final gfxlib3 OMA package
identifiers listed above. The reference APKs and screenshots remain in the
local artifact tree as evidence.

### OMA synchronization failure and fix

Demolition Derby and Duel 999 exposed an important CPU/GPU boundary rather
than a slow primitive shader. Both games contain legacy software-style loops
that read a destination pixel with POINT, calculate a new colour on the CPU,
and immediately write the same coordinate with PSET. Treating every POINT as
an isolated result barrier caused thousands of alternating `READ_PIXEL` and
`POINTS` packets. The renderer and shaders were fast, but the application
forced one GPU round trip per pixel.

gfxlib3 now detects exact repeated POINT/PSET pairs at one coordinate. After
two pairs it obtains one coherent page shadow, services subsequent dependent
reads and writes there, and uploads the accumulated page once at the next GPU
ordering boundary. The route supports 32-bit pages and packed RGB565 pages,
including legacy depth 15 modes normalized to RGB565. Ordinary PSET, LINE,
CIRCLE, boxes, PUT, text, clipping, page movement, and presentation remain GPU
commands. The compatibility route activates only after the program itself
introduces the CPU data dependency.

On the AGM A8, Demolition Derby changed from a black screen at 15 seconds and
first useful rendering around 30 to 45 seconds to a rendered setup by five
seconds. Duel 999 previously remained black for minutes while its trace showed
thousands of alternating read and write packets. The final RGB565-aware build
rendered its full setup by eight seconds. Approximately four seconds of each
cold launch is GLES and application initialization on this old device, so
these are observed startup bounds rather than isolated shader benchmarks.

Page selection was also made an explicit asynchronous frame boundary.
Selecting a visible page or copying to it stages PRESENT immediately, submits
the renderer batch, and lets the backend drain page work without forcing the
BASIC thread to wait for presentation. SCREENSYNC remains the explicit
completion boundary. This restores OMA page flipping while preserving the
ordered GPU-copy and batching design.

A fresh `page-flip-presentation-smoke.bas` executable was linked from the same
final Win64 archive after the OMA builds. It exited zero through OpenGL, Vulkan
device 0, and Vulkan device 1. The test checks visible and active page changes,
full and clipped SCREENCOPY pixels, and graphical-console cell page copying.

### Resizable logical SCREEN verification

`tests/gfx3/resizable-screen-smoke.bas` opens two 32-bit pages with
`FB.GFX_RESIZABLE` and resizes the real Win32 client. It checks the completed
`EVENT_WINDOW_RESIZE` dimensions, `SCREENINFO`, `GET_SCREEN_SIZE`, pitch,
per-page overlap preservation, and black expansion pixels. It then publishes
a second resize during `SCREENLOCK`, proves `SCREENPTR` remains stable until
unlock, verifies the deferred migration, shrinks both pages, and finally
maximizes the window. A fullscreen plus resizable request must fail.

Fresh Win64 and Win32 executables passed through gfxlib2 GDI, gfxlib3 OpenGL,
and gfxlib3 Vulkan. These runs exercise software-page replacement in gfxlib2
and GPU clear/blit page migration in gfxlib3.

The same source tree was staged on the Linux x86-64 `.99` host and both
libraries were rebuilt natively. `resizable-screen-x11-smoke.bas` then exited
zero through gfxlib2 X11, gfxlib3 OpenGL, and gfxlib3 Vulkan in one 1024 by 768
Xvfb display managed by Openbox. It requests client resizes through a second
X11 connection and verifies events, queries, two-page growth and shrink, black
expansion pixels, and deferred replacement during `SCREENLOCK`.

## 2026-07-27 library-first renderer and input checkpoint

This pass changed no OMA game source. The games were used only as representative
library workloads. Production changes were confined to gfxlib3's event queue,
idle pump, draw-state lookup, FB.IMAGE residency lookup, OpenGL fence policy,
Vulkan presentation lifecycle, and automatic backend order.

Desktop automatic selection now tries Vulkan before OpenGL. The freshly linked
public selection smoke reported `Vulkan compute` without `FBGFX` or a renderer
flag. The strict header-independent Vulkan bootstrap opened the runtime twice
and reported:

```text
Vulkan bootstrap: version 1.4, devices 2, selected 0 (10de:1f11), queue 0
```

With `FBGFX3_VULKAN_DEVICE_INDEX=1`, both opens instead reported selected
device 1, `8086:3e9b`. These identify the NVIDIA GeForce RTX 2060 and Intel UHD
Graphics 630 respectively. The default therefore follows the ranked
discrete-adapter policy instead of depending on loader enumeration order.

The following freshly linked Win64 `-mt` fixtures all exited zero through
forced OpenGL, Vulkan device 0, and Vulkan device 1:

- `renderer-selection-smoke.bas`
- `input-smoke.bas`
- `image-smoke.bas`
- `heterogeneous-blit-smoke.bas`
- `page-flip-presentation-smoke.bas`
- `point-cache-smoke.bas`
- `screenptr-nested-lock-smoke.bas`
- `resizable-screen-smoke.bas`
- `presentation-smoke.bas`
- `alpha-primitives-smoke.bas`
- `api-smoke.bas`

This is 33 backend-specific runs. It covers the queue-only SCREENEVENT contract,
synthetic key/mouse event delivery, stable and heterogeneous image residency,
visible and active page changes, POINT ordering, nested CPU page ownership,
native resize migration, alpha primitives, and general API behavior. The
OpenGL control-sequence change therefore did not weaken a barrier, readback, or
resource-lifetime boundary.

The final empty-event fixture made 4,096 SCREENEVENT calls. The previous
gfxlib3 renderer-round-trip form required approximately 0.328 seconds.
Five-run medians after the queue-only change were 0.00895 seconds on gfxlib3
OpenGL and 0.00906 seconds on gfxlib2; Vulkan samples were normally 0.0087 to
0.012 seconds. This matches gfxlib2's observable event-queue behavior and
removes thousands of unnecessary synchronous renderer commands from games.

The source rebuilt as Android ARM64 threaded PIC with NDK 27.2.12479018.
After the final idle-generation correction, new renderer, input, and touch
APKs were packaged from that exact archive and installed on device `b857d433`,
the API 24 AGM A8 with Adreno 306 and no Vulkan HAL. Isolated launches produced:

| Fixture | Archive | Physical result |
| --- | --- | --- |
| automatic renderer and primitives | final | Vulkan probing fell back to OpenGL ES; exit zero |
| native key input | final | ADB key A reached INKEY; `GFX3_ANDROID_INPUT_PASS a` and exit zero |
| native touch | final | held ADB contact reached GETTOUCH and hit testing; `GFX3_ANDROID_TOUCH_PASS` and exit zero |
| image residency | preceding renderer archive | expanded image mutation/readback smoke exited zero |
| ordinary OMA sprite stream | preceding renderer archive | 1.110213 seconds, final pixel `3784439`, exit zero |

The final renderer fixture reported `FREEBASIC_ANDROID_EXIT:0` after its
ten-second visible-frame interval. The final input and touch processes emitted
their PASS markers and `FREEBASIC_ANDROID_EXIT:0`. The last source change only
prevents a pure internal platform-poll command from counting as rendering
activity; it does not alter image, blit, shader, or residency code. The image
and sprite results are identified separately so the records do not imply that
those two unchanged paths were repackaged after that scheduling-only change.
The final threaded ARM64 archive SHA-256 is
`66709C80EDE9F2C8CFC324D0DA2E076E5772D6B3BF0CB189EE7DC96B53DB32C6`.

An unrelated installed OpenStunts process initially emitted GLES out-of-memory
messages while the first package batch ran. It had a different package and
process ID. Android also briefly resumed that old activity after some fixtures
exited. Result attribution therefore uses each package's process ID and
FreeBASIC exit marker, not an unfiltered device-wide log. The three final
fixture processes contain no fatal exception, native fatal signal, SIGSEGV,
or ANR.

The final Win32 input fixture explicitly separates synthetic WM_KEYDOWN and
WM_CHAR injection because PostMessage does not update the physical keyboard
state used by TranslateMessage. It passed three OpenGL runs, five RTX Vulkan
runs, and three Intel Vulkan runs after the idle-pump change. Fresh five-run
empty-SCREENEVENT medians were 0.00878 seconds for gfxlib2, 0.00942 seconds for
gfxlib3 OpenGL, and 0.00979 seconds for automatic RTX Vulkan. The difference
is within the observed host scheduling variation and remains approximately
35 times faster than the old synchronous renderer-round-trip implementation.

The focused desktop sprite comparison in `performance.md` is the authoritative
current result: gfxlib3 completed the identical 61,441-sprite stream 4.37 times
faster on OpenGL, 4.94 times faster on RTX Vulkan, and 2.21 times faster on
Intel Vulkan than gfxlib2. All four routes returned the same final pixel.

## 2026-07-27 large-image cache and Vulkan submission checkpoint

Profiling an unchanged OMA workload identified repeated whole-image hashing for
a roughly 3 MiB mutable FB.IMAGE. The exact-snapshot cache now accepts entries
through 16 MiB under a 64 MiB desktop or 24 MiB Android total budget. The new
`large-image-cache-smoke.bas` exposes the writable IMAGEINFO pointer, repeats
stable PUT operations, changes one pixel directly, and requires the following
PUT and POINT to observe that edit. The freshly linked fixture exited zero
through OpenGL, RTX Vulkan, Intel Vulkan, and automatic Android GLES.

The Vulkan submission runtime now has six resource slots. At most three
runtime operations share one command buffer and fence, so two independent
groups can overlap. The strict `vulkan-bootstrap.c` test requires the first
three operations to produce one `vkQueueSubmit`, starts a fourth operation
without waiting, checks targeted sequence retirement, and then requires the
second and final submit. It was compiled with:

```text
gcc -Wall -Wextra -Werror -fno-strict-aliasing
```

The final source passed two complete open/submit/close lifecycles on each
installed Vulkan adapter:

```text
device 0: 10de:1f11, NVIDIA GeForce RTX 2060
device 1: 8086:3e9b, Intel UHD Graphics 630
```

The final page-flip and graphical-console-page regression also exited zero on
both adapters. Before the final wait/close ordering cleanup, the same six-slot
implementation passed ten high-risk BASIC fixtures on each Vulkan adapter:
API, image, large image, page flip, presentation, POINT cache, nested
SCREENPTR lock, heterogeneous blit, pending POINT ordering, and Vulkan API.
Eight representative OpenGL fixtures passed, and the strict visible Vulkan
presentation program passed on both adapters. The final cleanup was then
rechecked by the strict double-lifecycle bootstrap and page-flip test above.

Runtime shutdown telemetry confirmed that this is driver-level batching, not
only producer queuing. One unchanged Rambo run completed 1,164 runtime
operations with 459 actual `vkQueueSubmit` calls; disabling the batch required
1,534 submissions for 1,534 operations. Two clean paired 15-second game runs
measured about 7.9 percent less total process CPU and 35.4 percent less
main/game-thread CPU than gfxlib2. The longer 614,401-sprite fixture retained
the exact final pixel and measured approximately 6.17 times gfxlib2 throughput
on RTX OpenGL, 5.91 times on RTX Vulkan, and 2.48 times on Intel Vulkan.

Both Android ARM64 PIC archives rebuilt from the final source with NDK
27.2.12479018. The final threaded archive is:

```text
libfbgfx3mtpic.a  56496A8E82943CA725B083F700C5424A88C306EF2649FEBE5F9AD38C40EE7A2E
```

`android-final-source2-renderer.apk` was packaged from that exact archive,
installed as `net.fbxl.gfx3renderer.finalsource2` on connected device
`b857d433`, and launched. The AGM A8 reported no Vulkan HAL, selected its
Adreno 306 GLES driver, completed the renderer fixture, and logged
`FREEBASIC_ANDROID_EXIT:0`.

Immediately before the final Vulkan-only wait/close cleanup, fresh Android
renderer, large-image, native key, and held-touch packages all exited zero on
the same device. The OMA-style A/B packages returned the same final pixel
`3784439`; gfxlib3 GLES completed in 0.843750 seconds and gfxlib2 in 1.217575
seconds. The final source change affects only Vulkan submission flushing, a
path this no-Vulkan device cannot enter. The exact final archive was still
repackaged and run as the renderer proof above.

The final Win64 archive hashes are:

```text
libfbgfx3.a    F0D0FA0D4C87E42907C1EBCDFEB996811F84E0ACBECEC9060E420F39032BA81E
libfbgfx3mt.a  20F02CB6D08FC830180A0229B83F5BBB24E1805F4347940F08A00637C4BF510D
```

No OMA game source was changed for these results.

## 2026-07-27 OMA profiler regression pass

The opt-in renderer profile was used with unchanged OMA programs to identify
GPU resource churn, CPU/GPU synchronization, and command-stream density. The
resulting image-cache, atlas, deferred-shadow, full-clear-shadow, and
single-pixel locking changes were made in gfxlib3. No game source was changed
for these measurements.

Fresh multithreaded Win64 executables passed the following checks:

- `image-smoke.bas` on OpenGL, Vulkan device 0, and Vulkan device 1
- `point-cache-smoke.bas` on OpenGL, Vulkan device 0, and Vulkan device 1
- `screenptr-nested-lock-smoke.bas` on OpenGL and Vulkan
- `input-smoke.bas` on OpenGL and Vulkan

The image test covers cached uploads, direct CPU image mutation, built-in PUT
modes, and the custom-blender synchronization boundary. The POINT and
SCREENPTR tests cover PSET, PUT, CLS, direct locked writes, nested locks, and
locked read-modify-write ordering. The input test injects Win32 messages and
requires SCREENEVENT to observe a message posted immediately after an empty
poll.

A one-millisecond empty-poll coalescing experiment failed that immediate input
test with exit status 3 and was removed. The final input implementation only
avoids a native poll when an event is already present in the published queue.
Fresh OpenGL and Vulkan input executables then both exited zero.

The current Vulkan archive also incorporates per-swapchain-image
render-finished semaphores. A submission fence alone does not prove that
presentation has consumed its wait semaphore; reacquiring the same swapchain
image supplies the required reuse boundary on Vulkan 1.0. Fresh image and
POINT fixtures exited zero on the NVIDIA GeForce RTX 2060 at device index 0
and the Intel UHD Graphics 630 at device index 1.

The new `locked-point-pset-benchmark.bas` returned the same final pixel through
gfxlib2, OpenGL, and both Vulkan devices. Five-run medians were 0.577231
seconds for gfxlib2, 0.126480 for OpenGL, 0.118154 for RTX Vulkan, and 0.152001
for Intel Vulkan.

The unchanged Rambo source now holds its persistent sprites in four atlas
surfaces. After 32 warm-up cell uploads, steady-state profiles report zero
resource creation, destruction, upload, and download. OpenGL presents 28 to 29
frames per second against the game's explicit 30 Hz clock while processing
approximately 15,400 sprite blits per second.

## Ten-game OMA and oversized-sheet rerun, 2026-07-28

`wide-image-cache-smoke.bas` creates a 10,960 by 40 RGB565 image, PUTs 40 by
40 rectangles from both ends, modifies one cached rectangle through the
IMAGEINFO pointer, and requires the next PUT to return the new pixel. Current
Win64 and Win32 executables exit zero through:

- OpenGL on the NVIDIA desktop driver
- Vulkan device 0, NVIDIA GeForce RTX 2060
- Vulkan device 1, Intel UHD Graphics 630

The Vulkan initialization log names the selected adapter and its vendor and
device IDs, proving the override did not silently reopen the default GPU.

The current ARM64 threaded-PIC gfxlib3 archive was rebuilt with NDK
27.2.12479018. A fresh Android package of the same wide-image test exited zero
on device `b857d433`, the API 24 AGM A8 with Adreno 306 OpenGL ES 3.0. The
device exposes no Vulkan HAL, so automatic selection correctly attempts Vulkan
and falls through to GLES rather than claiming an Android Vulkan test.

The original ten-program OMA matrix was then rebuilt from unchanged game
source, installed with its stable validation package names, launched, driven
into the live renderer paths where applicable, and checked against
process-scoped fatal and gfxlib3 diagnostics. All ten remained resumed and
passed with zero diagnostics:

1. Arkanoid
2. Behold
3. Demolition Derby
4. Duel 999
5. Kinematics
6. Nietzsche Special Edition
7. Quest for a King
8. Rambo vs Kitty Cat
9. Star Phalanx
10. Open Market

QFAK previously failed when SURFACE_CREATE tried to allocate its complete
10,600-pixel-wide sprite sheet on GLES. The corrected package reaches the
active castle scene and dialogue through cached 40 by 40 GPU regions. The
standalone test independently covers the wider 10,960-pixel background strip
and direct CPU mutation.

A new unchanged-source Win64 OpenGL profile also built and ran those original
ten titles successfully. The final active intervals contained no gfxlib3
errors or warnings, no uploads or downloads, no READ_PIXEL commands, and no
SURFACE_DOWNLOAD commands. The result files are:

- `.codex-tmp-gfx3-build/oma-all-region-cache-win64-opengl/oma-profile-20260728-024101.json`
- `.codex-tmp-gfx3-build/oma-all-region-cache-win64-opengl/oma-profile-20260728-024101.csv`
- `.codex-tmp-gfx3-build/oma-all-final-win32-opengl/oma-profile-20260728-031345.json`
- `.codex-tmp-gfx3-build/oma-all-final-win32-opengl/oma-profile-20260728-031345.csv`
- `.codex-tmp-gfx3-build/android-oma-all-current/final-all-device.stdout.log`
- `.codex-tmp-gfx3-build/android-oma-all-current/device-qfak-final-current.png`

The Win32 pair is a separate exact-source 32-bit build and live profile of all
ten programs. Every graphics window remained responsive and its renderer log
contained zero gfxlib3 error or warning records.

## Four playable OMA additions and one support tree, 2026-07-28

Five directories were added to `OMA`, but they represent four games:

1. `Scorched Earth` contains OpenHostility.
2. `TurboTrek` contains TurboTrek.
3. `vtrek` contains vtrek.
4. `WSR5_3` contains OpenWallStreet.
5. `Scorched` contains an isolated Dolphin emulator configuration, cache, and
   Wii user-data tree for OpenHostility validation. It has no FreeBASIC game
   source or independent executable.

`tests/gfx3/profile-oma-games.ps1` now models all fourteen playable OMA
projects. The four additions include their complete source lists, include
directories, compiler dialect switches, working directories, asset paths, and
gameplay automation. Both 64-bit and 32-bit Windows matrices build gfxlib2 and
gfxlib3 from the same game source.

### Deterministic application checks

Before live profiling, the projects completed their available deterministic
checks:

- OpenHostility passed `--self-test` and `--alpha-loop-test`.
- TurboTrek completed its bounded startup smoke.
- vtrek passed `--render-test` and 7,333 self-test assertions with no failure.
- OpenWallStreet passed 781 self-tests and generated its UI snapshots without
  a failure.

An extended OpenHostility automation run found an application-side null
pointer defect in `ScrollbarWidgetValue`. FreeBASIC's ordinary `Or` evaluates
both operands, so `widget = 0 Or widget->data = 0` dereferenced `widget` even
when it was null. The checks are now separate. Two gfxlib2 and two gfxlib3
Win64 live samples then completed, and freshly compiled Win32 gfxlib2 and
gfxlib3 self-tests both exited zero.

OpenHostility's Unix asset search also required DOS archive and hidden
attributes when calling `Dir`. Android classifies packaged regular files with
the archive attribute and classifies a full `./name` path as hidden. The
central file lookup now requests both attributes and retains exact path
matching. Its Android package also defines the existing Android target and
touch symbols so path joining uses `/`.

### Library corrections driven by these games

The new application set added or extended three focused regressions:

- `alpha-points-extension-smoke.bas` verifies logical VIEW/WINDOW mapping,
  repeated alpha coordinates, and stable ordered point layers.
- `screenptr-nested-lock-smoke.bas` now writes through `SCREENPTR` without
  `SCREENLOCK` and requires a following POINT to observe the write.
- `page-flip-presentation-smoke.bas` verifies selected work and visible pages,
  complete and clipped copies, and console page state.

Every fixture was freshly linked and exited zero through Null, OpenGL, RTX
Vulkan device 0, and Intel Vulkan device 1 on both Win64 and Win32. This is 24
final-source backend-specific runs. The Win32 public extension declarations
also use the platform ABI macro: stdcall on 32-bit Windows and cdecl elsewhere.
This fixed real Win32 stack corruption while retaining cdecl render callbacks.

`Gfx3DrawPoints` now feeds the compatibility point accumulator instead of
forcing one renderer command per API call. Repeated mapped coordinates still
receive successive GPU layers, while adjacent independent strings can share a
packet. Clipping remains a renderer operation. TurboTrek and OpenWallStreet
use this call for their antialiased font masks rather than issuing one public
PSET per covered sample.

`fb_GfxScreenPtr` now marks the returned page shadow dirty even outside a
graphics lock. The API returns writable storage, and a caller may retain that
address and write later, so the library must upload it before the next GPU
consumer. The Android version of the unlocked-pointer regression selected
Adreno GLES, observed the direct write, and logged
`FREEBASIC_ANDROID_EXIT:0`.

TurboTrek's omaGUI backend previously requested the default one page and then
called `ScreenCopy`. Its work and visible page were both page zero, making the
copy a no-op and exposing partially drawn frames to the Android compositor.
The backend now requests two pages, selects work page 1 and visible page 0,
and retains a one-page fallback. Six captures one second apart, followed by
three captures from the final archive, were byte-for-byte identical:

```text
983EDB30692A89168DAE6F886CA4E69A4792B9759BF9C612125423B1FF94898E
```

The complete menu, descriptions, object counts, and controls are present in
that frame. This is a real GPU page copy and asynchronous presentation, not
the previous same-page no-op.

### Desktop renderer and adapter checks

Fresh Win64 Vulkan profiles opened and ran all four games on both installed
adapters. Process-scoped logs identify:

```text
device 0: 10de:1f11, NVIDIA GeForce RTX 2060
device 1: 8086:3e9b, Intel UHD Graphics 630
```

All applications remained responsive and produced live screenshots through
each adapter. The paired final OpenGL results are recorded in
`performance.md`; they show meaningful main-thread reductions in TurboTrek,
vtrek, and OpenWallStreet, but not a universal whole-process win.

The current-library result files are:

- `.codex-tmp-gfx3-build/oma-new-five-openhostility-fixed-win64/oma-profile-20260728-070511.json`
- `.codex-tmp-gfx3-build/oma-new-five-final-rest-win64-opengl/oma-profile-progress.json`
- `.codex-tmp-gfx3-build/oma-new-five-openwallstreet-final-win64/oma-profile-20260728-071003.json`
- `.codex-tmp-gfx3-build/oma-new-five-final-win32-opengl/oma-profile-20260728-065955.json`
- `.codex-tmp-gfx3-build/oma-new-five-win64-vulkan-rtx/oma-profile-20260728-050041.json`
- `.codex-tmp-gfx3-build/oma-new-five-win64-vulkan-intel/oma-profile-20260728-050226.json`

One combined Win64 sweep is intentionally not listed as a pass: it exposed the
OpenHostility null check above and stopped before the later games. The
isolated corrected reruns are the authoritative results.

The Win32 profile preceded only the final OpenHostility null-guard source
correction. Both Win32 renderer variants were rebuilt after that edit and
their deterministic self-tests exited zero. The retained live profile did not
enter the corrected branch.

### Physical Android checks

All four final AArch64 APKs were linked from the current threaded PIC archive,
installed on device `b857d433`, and launched:

| Game | Physical result |
| --- | --- |
| OpenHostility | Live ICE001 terrain, tanks, HUD, and initialization dialog; process remained alive |
| TurboTrek | Complete menu; repeated captures were byte-identical |
| vtrek | Enter key advanced from setup to the live quadrant and sector view |
| OpenWallStreet | Complete main menu and descriptive panel; process remained alive |

The AGM A8 is API 24 with an Adreno 306 OpenGL ES 3.0 driver and no Vulkan HAL
or Vulkan feature. Every package attempted Vulkan, received the expected
unsupported result `-6`, and selected GLES. This verifies automatic fallback;
it does not claim Android Vulkan coverage. The final logs contain no fatal
exception, native fatal signal, or ANR.

The final device evidence includes:

- `.codex-tmp-gfx3-build/oma-new-five-android/device/openhostility-gfx3-final-source.png`
- `.codex-tmp-gfx3-build/oma-new-five-android/device/turbotrek-gfx3-current-1.png`
- `.codex-tmp-gfx3-build/oma-new-five-android/device/vtrek-gfx3-current-game.png`
- `.codex-tmp-gfx3-build/oma-new-five-android/device/openwallstreet-gfx3-current.png`

### Final archive and APK identities

```text
Win64 libfbgfx3.a             2D7BF52C69399D6D0D90151695F4C25ADAB23AE29772BFE577B6849F3D2DA49E
Win64 libfbgfx3mt.a           250B6F4FB3B8148CC222760985BD1A38EF310BD1475640133928B83567421B7B
Win32 libfbgfx3.a             0673BE9D29596232FAC94BB5C0870A91F98F71CAE24A8C78197A901F2356A013
Win32 libfbgfx3mt.a           59B4829BDB913D3B1F34C7C07F89A156EFEEF241EAD661FFCE25711B9917F277
Android libfbgfx3pic.a        1DD1C8457F45DAC5ACBBDDF67C37BDA5B740B08D381A03CBD938A9CE40FB417F
Android libfbgfx3mtpic.a      A26C95CD30FD64830B6494C0BC141DD831A9D33BED6F200A2C4C1E91C7018441
OpenHostility APK             7E1C2F038E81A7C45276A0FDC37E99A6279B62353DBA87CF56B4E363CC071B33
TurboTrek APK                 D67264622E8F48F2E1606CD4699EA42664B2C493C5CAAAD4A4EC7FB9F7BBA89D
vtrek APK                     B811F5F89AD4C3035B2CEF0808F96A63A83EA53ED74542102CF363EADB2912F3
OpenWallStreet APK            206015B5D03E1800B9719CDE2CE44F869B793FD73148CEA694122E31220CD1FB
```

## 2026-07-28 OpenSlicks qualification

OpenSlicks was built from `E:\openSlicks` with gfxlib2 and gfxlib3 instead of
using a reduced benchmark. Its deterministic suite passed 2,872 assertions
with zero failures in each of these current-library configurations:

- Win64 gfxlib2
- Win64 gfxlib3 OpenGL
- Win32 gfxlib2
- Win32 gfxlib3 OpenGL
- Win64 gfxlib3 Vulkan on device 0 and device 1

The Win32 executables use a 32 MiB linker stack reservation. Large persistent
game and test records now use static storage rather than placing nearly 1 MiB
of long-lived state beside helper scratch arrays on the 32-bit stack.

Live race coverage passed on:

| Target | Backend | Selected device | Result |
| --- | --- | --- | --- |
| Win64 | OpenGL | desktop OpenGL driver | Correct frame, about 125 presents/s |
| Win64 | Vulkan device 0 | NVIDIA GeForce RTX 2060, vendor 10de, device 1f11 | Correct frame, about 274 presents/s |
| Win64 | Vulkan device 1 | Intel UHD Graphics 630, vendor 8086, device 3e9b | Correct frame, about 125 presents/s |
| Win32 | OpenGL | desktop OpenGL driver | Correct frame, deterministic suite passed |
| Win32 | Vulkan device 0 | NVIDIA GeForce RTX 2060 | Correct frame |
| Win32 | Vulkan device 1 | Intel UHD Graphics 630 | Correct frame |
| Android AArch64 | automatic | Adreno 306 OpenGL ES 3.0 | Correct race and touch overlay, about 44 to 48 presents/s |

The AGM A8 reports no Vulkan HAL. Automatic selection attempts Vulkan, receives
the documented unsupported result `-6`, and initializes GLES. This is fallback
coverage and must not be presented as Android Vulkan coverage.

The final Android race renderer intervals show zero upload, zero download,
zero completion, and zero wait after the one-time 0.244 MiB track upload.
The final screenshot is
`.codex-tmp-gfx3-build/openslicks-android/device-race-packed.png` and the
process-filtered device record is
`.codex-tmp-gfx3-build/openslicks-android/device-race-packed.log`.

The independent `gpu-surface-smoke.bas` Android package also exits zero after
automatic GLES fallback and proves that a NULL destination reaches the current
screen page with the exact expected POINT value.

The final OpenSlicks Android APK SHA-256 is
`4AC033EB50D9738B951A9B6DEB2431A0D143221E76ED3131FFD62F1A3AF61D6B`.
The independent Android surface-smoke APK SHA-256 is
`6D440E4F4AEA003CB1933F1B819B004A056BA680C44779DAA5F855AC9D19222E`.

## 2026-07-28 fixed-screen maximize parity

gfxlib3 now follows gfxlib2's fixed logical maximize behavior. A normal framed
mode retains its logical dimensions, pitch, pages, and console while the
native client grows. The shared presentation layout chooses the largest
whole-number scale, centers the image on black, and converts mouse coordinates
in both directions.

OpenGL applies the layout with `glViewport` and clears only the native bars.
Vulkan passes the exact presentation rectangle to the compute shader, which
writes black outside it and performs nearest-neighbor integer source mapping
inside it. Neither backend resizes or transfers the logical GPU page.

`fixed-screen-maximize-smoke.bas` passed the following current-library matrix:

| Executable ABI | Runtime/backend | Physical adapter | Result |
| --- | --- | --- | --- |
| Win64 | gfxlib2 | gfxlib2 display driver | Exit 0 |
| Win64 | gfxlib3 OpenGL | desktop OpenGL driver | Exit 0 |
| Win64 | gfxlib3 Vulkan | NVIDIA GeForce RTX 2060 | Exit 0 |
| Win64 | gfxlib3 Vulkan | Intel UHD Graphics 630 | Exit 0 |
| Win32 | gfxlib2 | gfxlib2 display driver | Exit 0 |
| Win32 | gfxlib3 OpenGL | desktop OpenGL driver | Exit 0 |
| Win32 | gfxlib3 Vulkan | NVIDIA GeForce RTX 2060 | Exit 0 |
| Win32 | gfxlib3 Vulkan | Intel UHD Graphics 630 | Exit 0 |

On the 1366 by 696 native client used for these runs, every fixed 160 by 120
case reported the expected 800 by 600 image at 283,48 with scale 5. The smoke
captured black bars and exact red, green, and background pixels, verified
unchanged logical queries and pitch, rejected resize events, checked native
and logical mouse mapping, and restored the original 160 by 120 client.

The separate `GFX_RESIZABLE` smoke also exited zero in the same eight-case
matrix. This proves fixed maximize did not accidentally convert the existing
resizable contract into presentation scaling.

The strict header-only layout fixture passed both normal and
`GFXLIB_NEVERSCALE` builds with `-Wall -Wextra -Werror`. The standalone Vulkan
presentation fixture compiled from only `vulkan-presentation.c` and
`gfx3_vulkan.c` with `-Wall -Wextra -Werror -fno-strict-aliasing`, then exited
zero on both physical Vulkan adapters.

The shared code rebuilt into Android AArch64 normal and multithreaded PIC
archives. Fresh renderer-selection, point-cache, and page-flip APKs were
installed on the physical AGM A8. Automatic selection chose OpenGL ES 3.0 and
all three logged `FREEBASIC_ANDROID_EXIT:0`. Android has no desktop maximize
operation; these runs prove that the shared presentation and `SCREENSYNC`
changes did not regress its GLES renderer.

The X11 implementation uses the same layout and coordinate conversions and
updates its view dimensions from `ConfigureNotify`. A new live Openbox run on
the protected .99 host remains pending. The new
`fixed-screen-maximize-x11-smoke.bas` passes the FreeBASIC Linux cross-target
front end, but this session has no SSH agent, private key, or protected
password file. The supplied password was not placed in a command transcript.

## 2026-07-28 newJRPG qualification

The production JRPG editor and player build under gfxlib2 and gfxlib3 on
Win64, and under gfxlib3 on Win32. Eleven graphics-focused engine tests compile
for both Win64 runtimes. All eleven gfx3 tests pass on OpenGL, Vulkan RTX 2060,
and Vulkan Intel UHD 630. The first ten also pass in the same three-backend
Win32 matrix; the long Win32 graphical battle was compiled but not executed.

The complete graphical battle passed in 102.06 seconds on gfxlib2, 100.46 on
gfx3 OpenGL, 88.67 on RTX Vulkan, and 96.96 on Intel Vulkan. Five focused
gfxlib3 rectangle, primitive-order, point-order, page-flip, and clipping
regressions also pass on all three desktop renderer selections.

The asset-independent JRPG renderer smoke was packaged from the current
Android AArch64 archive and installed on the AGM A8. Automatic selection
received Vulkan unsupported `-6`, selected OpenGL ES 3.0, passed the complete
pixel assertion set, logged `runtime renderer smoke OK`, and exited with
`FREEBASIC_ANDROID_EXIT:0`.

The detailed test list, Vulkan repair, performance interpretation, omaNet
Win32 repair, Android build command, limitations, and current artifact hashes
are in [`newjrpg-qualification.md`](newjrpg-qualification.md).

## 2026-07-29 Vulkan mixed primitive tile coalescing

The Vulkan renderer now accepts rectangles in its adjacent opaque primitive
packet. Ellipse-free point, line, and rectangle packets use one ordered 16 by
16 tile replay dispatch. Rectangle-free packets retain the compact atomic
winner path, now driven by a useful-workgroup table. A packet containing both a
rectangle and midpoint ellipse retains the established exact type-specific
fallback.

The Win64 normal and multithreaded archives rebuilt with all 21 Vulkan shader
sources regenerated successfully. Six focused executables were relinked and
passed on both Vulkan adapters:

- `mixed-rectangle-primitive-order-smoke.bas`
- `mixed-primitive-order-smoke.bas`
- `small-filled-rectangle-batch-smoke.bas`
- `pending-points-order-smoke.bas`
- `page-flip-presentation-smoke.bas`
- `put-clipping-smoke.bas`

The new mixed-rectangle fixture checks overlapping PSET, styled LINE, BOX BF,
outline BOX, clipping, submission-slot reuse, and the rectangle-to-ellipse
fallback boundary. It also compiled as Win32 and exited zero on both the
NVIDIA GeForce RTX 2060 and Intel UHD Graphics 630.

The unchanged newJRPG editor source was relinked against the final archive.
Five steady RTX samples produced a median 8.245 ms Vulkan execution per page;
six Intel samples produced 8.226 ms. The immediately preceding atomic
rectangle-winner build measured about 14.0 and 15.4 ms respectively. The
standard primitive fixture retained final pixel `4278190080`; its three-run
filled-box medians were 0.01719 seconds on RTX and 0.01961 seconds on Intel.

The revised QFAK graphical battle exited zero with all of its own assertions.
Its profile established that expensive intervals are dominated by synchronous
pixel reads and timed engine updates rather than the changed primitive packet,
so its variable wall time is retained as correctness evidence only.

Win32 normal and multithreaded archives and Android AArch64 normal and
multithreaded PIC archives rebuilt successfully. The attached AGM A8 selects
GLES because it exposes no Vulkan HAL; this Vulkan-only optimization therefore
has no physical Android execution path to qualify on that device.

<!-- end of verification.md -->
