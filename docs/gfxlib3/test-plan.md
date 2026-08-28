# gfxlib3 test plan

## Principle

gfxlib3 compatibility is judged by observable FreeBASIC behavior, not by
whether a shader produced a plausible picture. Tests compare exact pixels,
return values, error codes, events, image layouts, and synchronization effects.

## Test layers

### 1. Compiler and link selection

- Build a graphics program without the define and prove it links only gfxlib2.
- Package `android-gfx2-default-smoke.bas` without a define, `fbgfx3.bi`, or
  `-gfx3`. Require its compile-time marker check and GFX_NULL PSET/POINT round
  trip to pass on device, proving Android packaging has not changed gfxlib2's
  default selection.
- Build the same program with `__FB_GFXLIB3__` before `fbgfx.bi` and prove it
  links only the correct gfxlib3 archive variant.
- Run `tests/gfx3/audit-public-exports.ps1` against built Win64 archives. It
  extracts the current public `fb_Gfx*` and selected `fb_hPut*` declarations,
  plus the runtime graphics hooks, from gfxlib2's `fb_gfx.h` and requires both
  archives to define every symbol.
  Pass explicit archive paths for the Android `libfbgfxmtpic.a` and
  `libfbgfx3mtpic.a` pair after an arm64 build.
- Build `compiler-gfx3-option-smoke.bas` with `-gfx3`, then link its object
  with the normal `compiler-gfx3-option-main.bas` module. Verify that the
  option pre-includes the public source selector, the final link selects only
  gfxlib3 through
  object metadata, and the separate executable exits zero.
- Build `driver-selection-precedence-smoke.bas` with `-gfx3`. Its deliberate
  unconditional bare `#define __FB_GFXLIB3__` must remain source-compatible
  with the option. Run the same source against gfxlib2 and gfxlib3 normal and
  `-mt`; require an explicit `SCREENCONTROL SET_DRIVER_NAME, "nUlL"` to win
  over `FBGFX=Vulkan`, then require clearing the stored name to let
  `FBGFX=NuLl` select the Null renderer. Package the gfxlib3 form on Android
  and require the same pass marker and clean NativeActivity exit.
- Build `selection-gfx3-intrinsic.bas` with `-gfx3`. It deliberately has no
  `fbgfx.bi` include and retains the bare source define. Require a link only
  against gfxlib3 and a successful GFX_NULL PSET/POINT round trip; this proves
  intrinsic callback selection observes the option preinclude as well. Package
  the same source through `fbc-android -gfx3` and require a clean NativeActivity
  exit, proving the older-cross-compiler translation preserves the same route.
- Package `android-gfx3-option-smoke.bas` with `fbc-android -gfx3` and no
  source define. Verify the connected device reports the selected GPU renderer
  and `FREEBASIC_ANDROID_EXIT:0`; this proves the packaging wrapper also
  selects the required multithreaded gfxlib3 runtime archive.
- Package `renderer-selection-smoke.bas` through the same Android helper with
  its source-level `__FB_GFXLIB3__` define and again with `-gfx3`. Require
  `GET_DRIVER_NAME` to report a GPU backend and the exact PSET/POINT check to
  pass in both forms. This guards the older-cross-compiler option translation
  and the direct source-selection path independently.
- Package `android-gfx3-extension-header-smoke.bas` with no explicit define or
  `-gfx3`. It includes `fbgfx3.bi` directly, so it verifies the wrapper selects
  gfxlib3 and overlays the extension header before creating, reading, and
  destroying an opaque GPU surface.
- Repeat for normal and `-mt` builds on every host. Repeat for `-pic` and
  `-mt -pic` only on targets that support position-independent executables;
  the Windows compiler deliberately rejects that mode.
- Build the modified compiler on each supported host platform and inspect its
  real verbose link line, rather than assuming the Windows selection result is
  portable.
- Run the compiler RTL prototype tests so no ABI signature drifts.
- Run `screenlist-smoke.bas` without opening a renderer. Verify positive depth
  calls restart the standard-mode iterator, zero resumes it, entries remain
  sorted, 15/16 and 24/32 requests preserve their compatible fallback, and an
  unsupported depth clears the pending list. On Win32 additionally run
  `screenlist-win32-smoke.bas` and require the current native DEVMODE to occur
  exactly once in the public list. On an X11/RandR desktop, run
  `screenlist-x11-smoke.bas` and require the current root-window dimensions to
  occur exactly once in the public 32-bit list. Independently compile
  `x11-screenlist-live.c` with `-Wall -Wextra -Werror` against the current
  `gfx3_screenlist.c` and run it under a live X server or Xvfb. Require native
  8/16/32 lists, 15/16 and 24/32 aliases, sorted unique values, unsupported
  depth cleanup, and a clean close after the optional RandR library is loaded.
- Run `renderer-selection-smoke.bas` without `FBGFX`, with a valid alternate
  GPU name, and with an unknown name. Verify default order, requested priority,
  full fallback, and the actual `GET_DRIVER_NAME` result. The strict C
  selection test must also verify gfxlib2's named-driver sequence: the named
  backend once, then the complete backend order including that backend, then
  the same two passes with the fullscreen bit inverted. An unknown name must
  skip the dedicated attempt and retain the complete-order fallback; explicit
  Null must remain a single non-windowed attempt.
- On a physical Android device without Vulkan, package
  `android-named-renderer-fallback-smoke.bas`. Set only `FBGFX=Vulkan`, not
  `FB.GFX_VULKAN`, and require the named preference to fall through to the
  actual OpenGL ES renderer with exact POINT readback.
- Run `vulkan-adapter-ranking.c` with strict warnings. It must preserve the
  Float64 exact-ellipse preference, rank discrete, integrated, virtual, CPU,
  and unknown Vulkan adapters in that order, and prefer graphics-plus-compute
  queues over compute-only queues. The real Vulkan smoke must still open a
  logical device, proving selection falls through from policy to creation.
- Before opening any graphics mode, run `screeninfo-desktop-win32-smoke.bas`
  through gfxlib2 and gfxlib3. Require the active DEVMODE width, height, depth,
  and refresh, with zero framebuffer bpp/pitch and an empty driver name.
- On Android, package `screeninfo-desktop-smoke.bas` with `-gfx3` and require
  positive NativeActivity window dimensions and depth, with zero framebuffer
  bpp/pitch and an empty driver before the renderer is selected.
- Run `screen-refresh-smoke.bas` under gfxlib2 and gfxlib3. A windowed or null
  mode may receive a positive SCREENRES refresh request, but both SCREENINFO
  and GET_SCREEN_REFRESH must report zero because neither driver changes or
  publishes a physical display mode.
- Run `depth-normalization-smoke.bas` through GFX_NULL, forced OpenGL, forced
  Vulkan, automatic selection, and Android GLES. Verify 15-bit SCREENRES
  reports 16-bit RGB565 storage, 24-bit reports 32-bit storage, and POINT
  returns the same normalized colours as gfxlib2.
- On Android, build through `fbc-android` and verify that gfxlib3 selects the
  thread-safe runtime even when the BASIC source contains no thread calls.
- On an Android device with no Vulkan feature, capture the automatic probe.
  Require the normal no-HAL fallback to GLES and a clean FreeBASIC exit, but
  reject an `invalid vkGetInstanceProcAddr(VK_NULL_HANDLE,
  "vkEnumerateInstanceVersion")` diagnostic from the optional version query.

### 2. Common infrastructure

- Command size, type, payload, and arithmetic validation.
- Multi-producer ordering with sequence rollover protection.
- Queue full/empty transitions, spurious wakeups, shutdown while empty, and
  shutdown with pending commands.
- Non-blocking queue drains and bounded asynchronous render-command batches;
  completion-bearing commands, shutdown, and interop callbacks must remain
  boundaries.
- Resource handle generation, stale handles, reference overflow, deferred
  destruction, and allocation failures.
- Fence completion, renderer failure wakeup, probe rejection before backend
  state exists, and init rejection after a backend has claimed partial state.
  In both startup-failure paths the common renderer must leave no thread,
  command-queue storage, or resource-registry storage behind.
- Run the real OpenGL backend through a test platform which allocates a context
  and rejects the first required function resolver call. Require one platform
  destroy, one attempted load, no published renderer storage, and then run the
  normal real OpenGL smoke through the ordinary platform adapter.

The current `tests/gfx3/infrastructure.c` executable covers these checks with
strict warnings. It also passes commands through the dedicated render thread
to the reference backend and verifies initial colors, clipped clears, clipped
point batches, solid lines, patterned lines, BOX/BF, overlapping surface PUT,
all non-custom built-in PUT modes, out-of-range reads, logical depth masks, and
ordered surface destruction.

`tests/gfx3/vulkan-bootstrap.c` compiles without Vulkan headers, an import
library, or the common threaded command module. On Win32 and Linux it opens the
real loader twice, creates and tears down an instance and logical device each
time, verifies compute queue selection, reuses a fenced command buffer, checks
GPU fill and storage-buffer compute output, then checks exact device-local
surface clear and pitched transfer pixels including untouched row padding.
`tests/gfx3/vulkan-adapter-ranking.c` is the hardware-independent companion
for the physical-device policy. It checks the deterministic score ordering;
`vulkan_runtime_open_internal()` then attempts actual logical-device creation
in that order and continues after a failed creation.

The infrastructure executable also instantiates the Vulkan backend through the
common render thread. It checks generation-tagged device-local surfaces,
logical depth masks, clear, point, styled-line, BOX/BF, transfer, readback,
page selection, present-command acceptance, and destruction. The line and box
stream, every built-in PUT mode, and an overlapping same-surface PUT are
downloaded and compared byte for byte with the null reference backend.
`tests/gfx3/vulkan-api-smoke.bas` proves the same explicit `FB.GFX_VULKAN`
route through ordinary SCREENRES, PSET, POINT, LINE, BF, CIRCLE, CPU-image
upload, PUT, visible-page writes, and orderly window teardown in normal and
`-mt` builds.

The current six-slot implementation is covered by the standalone bootstrap.
The first three asynchronous empty operations must share one actual
`vkQueueSubmit`; a fourth operation starts a second command-buffer group
without waiting for the first. The runtime must therefore report four retained
operation slots, one queue submission, and no completed sequence. The same
test tags those operations as sequences 10 through 40. A wait through 20 must
complete the first group while retaining sequence 40, and a wait through 40
must submit and release the second group. The exact submission counter must
advance from one to two. The test then verifies GPU fill, compute,
device-local clear, and pitched transfer after the ring has been exercised.
The strict Win32 presentation test verifies repeated acquire, present, and
resize using the slot-local acquire and swapchain-image render-finished
semaphores.

The bootstrap also calls the non-blocking submission poll while later tagged
slots may still be active. It verifies that active and completed fences can be
observed without a queue-wide wait. It then queues three point streams through
different slots before readback. That verifies descriptor-set updates do not
redirect an earlier dispatch and that each stream's deferred host-visible
staging buffer remains valid through its fence. The renderer lifecycle regression holds backend
startup, queues asynchronous work, a synchronous SURFACE DESTROY, and later
asynchronous work, then proves all three commands share one batch while the
destroy completion waits only its own sequence. Together with the Vulkan
targeted-wait bootstrap, this proves the end-to-end destruction rule without
waiting for a later submission.

Repeat this matrix through double and triple swapchains, resize/recreation,
mode replacement, and a forced device-loss cleanup path.

`tests/gfx3/vulkan-presentation.c` is the strict Win32 and X11 WSI acceptance
test. It creates an 8 by 8 native client and a windowed header-independent
Vulkan runtime, then captures displayed client pixels after GPU presentation
through Win32 client pixels or `XGetImage`. Exact quadrants prove 1, 2, 4, and
8-bit index masking and palette lookup, RGB565 expansion, and 32-bit color
conversion. It repeats presentation on each surface, resizes the client to 16
by 12, presents again, and verifies both the scaled displayed pixels and the
recreated swapchain extent. The test requires at least two swapchain images and
runs with `-Wall -Wextra -Werror`. The X11 form is repeated to catch drivers
that keep an old swapchain valid after a client resize.

On Win32 or X11 the infrastructure executable attempts a real OpenGL 4.3 core
context. When available, it verifies GPU-resident clear, points, styled lines,
BOX/BF, and every non-custom built-in PUT mode by readback, then compares varied
clipped and reversed GPU lines and boxes against the null backend pixel by
pixel. It then uses the Vulkan presentation command's native-pixel keyboard
rectangle and active state, capturing an interior blue compositor pixel. An
unsupported OpenGL implementation is reported as a skip rather than
silently selecting the CPU backend.

On Android, `android-renderer-smoke.bas` must run without a renderer flag. On a
device without Vulkan it must report OpenGL ES 3.0, pass exact primitive
readback, visibly present its result, and exit zero. Repeat launch without
reinstalling to catch runtime archive, NativeActivity, EGL, and render-thread
lifetime errors. A separate Vulkan-capable Android device must run a Vulkan
variant of that primitive/readback fixture with `FB.GFX_VULKAN`, report Vulkan
as its selected driver, survive resize and close, and cover the already
implemented Vulkan KB presentation overlay on the physical device before
Android Vulkan WSI can be marked verified.

`android-input-smoke.bas` waits a bounded interval for a lowercase A through
ordinary INKEY. Run it once with an Android key event and once with text input,
then require `GFX3_ANDROID_INPUT_PASS a`, exit zero, and no unresolved Java
native-method diagnostic in logcat.

`android-keyboard-overlay-smoke.bas` is the physical KB-control acceptance
fixture. Start it with the normal keyboard-button manifest policy, confirm no
keyboard is visible at launch, tap the upper-right native-pixel KB target, and
require the system keyboard plus the active blue GPU control. Tap it again and
require both to return to their inactive state. The fixture rejects a leaked
mouse-button event from either KB tap; complete it with a lowercase A committed
from the software keyboard to verify the Java IME bridge and clean exit.

`gl-control-smoke.bas` forces the desktop OpenGL backend, then validates
`SCREENCONTROL` colour-component totals, depth/stencil/accumulation/sample
non-negativity, the legacy OpenGL-2D default values, and safe extension-string
delivery. Run it in normal and `-mt` modes. Package the same source on Android
to verify the GLES snapshot is captured on the renderer thread and survives
the NativeActivity lifecycle.

`gl-set-control-smoke.bas` is the non-driver-specific companion. Run it once
against gfxlib2 and once with `-gfx3`. It sets each SET_GL_* pixel-format
request and requires the matching GET_GL_* value before a graphics mode and
while GFX_NULL is active. It also locks in the legacy inactive GET_GL_2D_MODE
zero and GET_GL_SCALE one results. Do not treat a passing request round trip as
proof that gfxlib3 recreates an active GPU context: format negotiation belongs
to mode initialization, and the active OpenGL/GLES snapshot is covered by
`gl-control-smoke.bas`. After a query-runtime change, rebuild the Android ARM64
threaded-PIC archive, stage it in the package runtime, and package
`gl-control-smoke.bas` with `-gfx3` for a physical GLES snapshot check.

`mmx-control-smoke.bas` is the legacy CPU-acceleration companion. Run it
against gfxlib2 on the current architecture and against gfxlib3 in normal and
`-mt` configurations. gfxlib3 must accept SET_X86_MMX_ENABLED before and after
GFX_NULL without changing GET_X86_MMX_ENABLED from false, because no CPU MMX
blitter exists in its renderer.

`gl-interop-smoke.bas` forces desktop OpenGL and verifies that SCREENGLPROC is
NULL on the BASIC thread, then queues `FB.Gfx3RunOnRenderThread`. Its callback
must resolve and call `glGetString` while the renderer owns the context, and
SCREENGLPROC must return to NULL after the callback. Repeat in normal and
`-mt` builds, then package it on Android to prove GLES resolver ownership.
Callbacks must never invoke ordinary graphics APIs, so the fixture confines
itself to a context query and caller-owned result fields. The current desktop
OpenGL and physical Android GLES runs pass. It then opens a Null mode and
requires the callback request to fail without calling user code, proving an
unsupported interop request does not poison the render queue or mode shutdown.

`mode-lifecycle-smoke.bas` repeatedly replaces active public modes while
varying dimensions and page counts. It alternates replacement with explicit,
idempotent `SCREEN 0`, checks exact primitive readback after every open, and
proves that a rejected zero-width request leaves the current valid mode usable.
Run a long headless sequence, focused runs through each desktop GPU backend,
and repeated automatic OpenGL ES sequences on Android. This is the bounded
regression gate; the thousands-of-opens longevity run and per-stage fault
injection remain separate release work.

`android-lifecycle-stress.bas` is the physical Android complement. Package it
for the connected device and cold-launch it at least twice. Each process must
complete 256 automatic GPU mode opens, exact PSET/POINT readbacks, variable
page counts, and double-close teardown before emitting its GLES pass marker
and `FREEBASIC_ANDROID_EXIT:0`.

`tests/gfx3/image-smoke.bas` and `command-compat-smoke.bas` run through the
reference, OpenGL, and Vulkan backends. Together they cover CPU images, GET,
all PUT modes including a custom callback, PAINT, DRAW, canonical text, and a
BMP round trip.

`tests/gfx3/paint-pattern-smoke.bas` first runs unchanged through gfxlib2,
then runs gfxlib3 through Null, OpenGL, Vulkan, and the threaded desktop
backends, plus physical Android GLES. It fills a nonzero VIEW with a distinct
8 by 8 32-bit tile and verifies that the tile stays anchored to absolute target
coordinates instead of the staging rectangle.

`tests/gfx3/paint-pattern-depth-smoke.bas` applies that same target-origin
check to the native 8-bit indexed, 16-bit RGB565, and 32-bit pattern byte
layouts. It compares the observed 16-bit POINT expansion with gfxlib2's
historical RGB565 result before exercising the desktop and Android matrices.

`tests/gfx3/bload-raw-smoke.bas` saves and restores a seven-byte explicit
memory block, then saves and restores a 16 by 16 32-bit GPU-backed screen
page. It proves the renderer download before BSAVE and upload after BLOAD are
ordered correctly. It runs through Null, OpenGL, Vulkan, automatic, and `-mt`
desktop paths plus Android GLES. The current gfxlib2 Linux Null driver faults
on its own raw screen restore, so the shared `.99` baseline is limited to the
explicit-memory block.

`tests/gfx3/bsave-format-smoke.bas` saves indexed, RGB565, and 32-bit GPU
pages to BMP, including the public 24-bit override. It reloads each output to
a 32-bit GPU page and checks palette BGR ordering and true-colour conversion
through the same backend and Android matrix.

`tests/gfx3/png-screen-smoke.bas` saves and reloads RGBA and indexed Null
GPU pages. It checks exact alpha, index, palette, and case-insensitive filename
handling. The unchanged `tests/gfx/png-roundtrip.bas` compiled with `-gfx3`
adds CPU-image RGB565, forced true-colour, and damaged-CRC coverage.

`tests/gfx3/console-depth-smoke.bas` writes graphical-console cells in 8-bit
and RGB565 modes. It verifies SCREEN's character result, the packed indexed
foreground/background attribute, and expanded RGB565 foreground/background
values through the same backend and Android matrix.

`tests/gfx3/bload-bitfields-smoke.bas` writes a one-pixel 16-bit RGB565
Windows `BI_BITFIELDS` BMP and a one-pixel 32-bit RGBA bitfields BMP with an
alpha DWORD after the standard 40-byte header. It requires exact 32-bit green
and RGBA conversions. These are focused parser checks for the checked
contiguous-mask path; externally produced BMP corpus coverage remains future
work.

`tests/gfx3/bload-v4v5-bitfields-smoke.bas` writes the matching one-pixel
RGBA bitfield payload under the 56-byte Windows V3, 108-byte V4, and 124-byte
V5 headers. It proves the masks are read from the extended header rather than
mistaken for palette data, and is run against gfxlib2 plus gfxlib3 Null,
OpenGL, Vulkan, automatic `-mt`, and Android GLES.

`tests/gfx3/bload-os2-core-smoke.bas` writes a one-pixel 4-bit OS/2 V1
`BITMAPCOREHEADER` BMP. Its indexed high nibble and 3-byte BGR palette must
produce exact RGB(128,64,32) in both a 32-bit image and the active screen
page. This verifies the core-header decoder-to-renderer path without claiming
RLE or later OS/2 support.

`tests/gfx3/bload-indexed-smoke.bas` writes Windows 1-bit and 4-bit BMPs.
It verifies the high source bit, high source nibble, correctly sized BGRX
palettes, and exact palette-to-32-bit conversion through every renderer.

`tests/gfx3/bload-rle-smoke.bas` writes Windows RLE8 and RLE4 files. It
verifies encoded and absolute RLE8 records, end-of-line/end-of-bitmap records,
bottom-up rows, an in-bounds delta move, and packed RLE4 absolute indexes.
It repeats exact pixels through CPU-image and active-screen targets, proving
the decoded rows traverse the renderer upload path. It also verifies an
over-wide encoded run is rejected without modifying either target. Larger
malformed-stream coverage belongs in a later corpus.

`tests/gfx3/alpha-primitives-smoke.bas` is the exact alpha-primitive gate. It
first proves the disabled solid-write path, then enables the SCREEN flag and
compares PSET, LINE, BF, solid PAINT, CPU-image targets, GPU-only surface
targets, CLS, and runtime disable/re-enable behavior against gfxlib2's
256-divisor primitive formula. Compile it once against gfxlib2 with
`-d GFX2_REFERENCE`, then run gfxlib3 through Null, OpenGL, Vulkan, automatic
selection, and Android GLES. Pattern PAINT and PUT ALPHA are intentionally
separate tests because they use different gfxlib2 semantics.

`tests/gfx3/opengl-gpu-surface-state-smoke.bas` is the readback-hazard
regression companion. It opens an ordinary OpenGL mode without alpha, replaces
it with an alpha mode, then stages PSET/POINT, LINE, BF, PAINT, and CPU-image
operations before creating a GPU-only surface. Each stage must still allow an
opaque BF on that independent target. This guards the required detachment of
the reusable read framebuffer before a later writable-image compute dispatch.

`tests/gfx3/circle-compat-smoke.bas` is the CIRCLE conformance fixture. It
uses a fixed opaque 96 by 80 scene of full circles, aspect 0.5 and 2.0
ellipses, filled circles, and ordinary and wrapped arcs. The FNV-style logical
pixel hash is first established through gfxlib2, then required unchanged on
gfxlib3 Null, OpenGL, Vulkan, and Android GLES. Randomized arc/aspect
differential testing remains a later expansion.

`tests/gfx3/custom-font-smoke.bas` is the custom `DRAW STRING` fixture. It
constructs the documented byte header inside a 32-bit `FB.IMAGE`, supplies
two three-pixel glyphs, and verifies the gfxlib2 magenta TRANS key, PSET
copying, and unsupported-character font-height advance through both TRANS and
PSET on screen and an image. It hashes screen and CPU-image output, exercises a gfxlib3 GPU-only
target, and requires the
gfx2-derived hashes through gfxlib3 Null, OpenGL, Vulkan, automatic selection,
and Android GLES. Custom blender callback matrices and full 8/16-bit font
coverage remain separate work.

`tests/gfx3/presentation-smoke.bas` opens visible Win32 OpenGL modes at 8, 16,
and 32 bits. It checks logical pixels, indexed palette presentation, explicit
synchronized PRESENT, visible-page changes, and native window-title delivery.

`tests/gfx3/gpu-surface-smoke.bas` runs through the null and real OpenGL
backends, and now also runs through Vulkan in normal and `-mt` builds. It keeps
pixels in opaque renderer surfaces while checking explicit
transfers, info, clear, GPU blit, direct presentation, standard primitive and
circle/arc targets, PAINT, DRAW, text, GET, CPU-image PUT, destruction, and
mode-owned cleanup. The Vulkan PRESENT command is ordered and accepted, but
the dedicated captured-pixel test owns exact visible conversion coverage.
It also creates one surface for each individual usage role and proves that
download accepts only `TRANSFER_SOURCE`, upload accepts only
`TRANSFER_DESTINATION`, drawing accepts only `RENDER_TARGET`, and GPU blit or
presentation source use accepts only `SAMPLED`. The same transfer-only surface
also proves that PAINT and PUT CUSTOM do not use their CPU compatibility
barriers to bypass `RENDER_TARGET`. Its scoped-map portion proves exact
downloaded pixels, partial writable-map preservation, rejection of ordinary
surface calls and destruction while mapped, capability rejection, and forced
staging release at mode shutdown.
It also maps a 2 by 2 upload rectangle, checks its tight pitch and all four
pixels, commits an independent 1 by 1 rectangle, and rejects an out-of-bounds
rectangle before staging allocation.

`tests/gfx3/gpu-surface-map-depth-smoke.bas` separately verifies that scoped
maps use one byte per native indexed pixel and two bytes per RGB565 pixel. It
modifies only the first and final pixel through the map, then compares raw GPU
downloads after unmap. This keeps staging-layout proof independent of POINT's
public indexed and RGB565 colour conversion.

`tests/gfx3/console-smoke.bas` runs in normal and `-mt` builds through the null,
OpenGL, and visibly presented Vulkan backends. It checks
ordinary PRINT redirection, exact glyph/background pixels,
character and color reads, WIDTH, LOCATE/POS/CSRLIN, independent page cells,
GPU scrolling, CLS, and mode shutdown hook removal.

`tests/gfx3/console-font-smoke.bas` is the focused standard-mode grid oracle.
It verifies SCREEN 9's 80 by 25 8 by 14 EGA console, SCREEN 11's 80 by 30 8
by 16 VGA console, valid WIDTH changes to 8 by 8 and back to 8 by 16, an
invalid non-canonical WIDTH request, and LOCATE plus SCREEN(row, column)
character reads after each switch. The gfxlib2 result on `.99` is compared with
Null, OpenGL, Vulkan, automatic, and multithreaded gfxlib3 desktop runs, then
with the physical Android GLES fallback.

The image and command-compatibility smokes also run through Vulkan in normal
and `-mt` builds. Together they cover GET, all built-in PUT modes, the CPU
custom-blender barrier, PAINT, DRAW, glyph batches, and a BMP save/load round
trip above device-local logical pages.

`tests/gfx3/input-smoke.bas` runs normal and `-mt` Win32 OpenGL and Vulkan
executables. It finds the real native window, injects deterministic Win32
messages, and checks
press/repeat/release events, MULTIKEY, CP437 INKEY, GETKEY, key-buffer status,
mouse movement and buttons, both wheel axes, SETMOUSE, focus transitions,
SCREENEVENT peeking, close delivery, and KEY_QUIT. It also sleeps after posting
WM_CLOSE and checks native visibility before making another graphics/input
call, proving the renderer's idle poll producer dispatched the message. Native
SCREENCONTROL checks cover the HWND, null Win32 display handle, desktop size,
and ordered outer-window position get/set.

`tests/gfx3/input-x11-smoke.bas` runs normal and `-mt` X11 OpenGL and Vulkan
executables. It uses a second Display connection to generate key, motion,
button, wheel, focus, and close input without violating render-thread ownership
of the platform connection. It verifies five-button mapping, rejection of an
unknown native button, key state, native Display/Window publication, and the
release and restoration of pointer confinement across focus changes. The test
dynamically opens the server's `libXtst.so.6`; gfxlib3 itself does not depend on
Xtst.

`tests/gfx3/line-input-smoke.bas` runs normal and `-mt` Win32 OpenGL
executables. It injects `WM_CHAR` and editing-key messages, inserts a character
in the middle of a narrow line, verifies byte-to-wide LINE INPUT conversion,
calls standard-input reading through the installed `readstrproc`, and checks
the resulting graphical character cells.

`tests/gfx3/vga-port-smoke.bas` runs normal and `-mt` executables through null
and OpenGL backends. It writes and reads sequential six-bit RGB components via
the VGA DAC ports, checks the expanded palette values, and verifies that status
port 0x3DA returns the retrace bit after synchronized presentation. The input
smoke separately proves that graphics-aware SLEEP wakes on a pending key and
does not consume it.

The unchanged `tests/gfx/getjoystick.bas`, `getxpad.bas`, and `gettouch.bas`
programs are also built with gfxlib3 selected and run directly.
`tests/gfx3/xinput-fallback-smoke.bas` opens a real Win32 GPU mode, allows the
render thread to complete an XInput poll, and checks the complete missing-device
GETJOYSTICK and GETXPAD contract. On Win32, GETJOYSTICK deliberately executes
the separate lazy WinMM query while GETXPAD observes the renderer-owned XInput
snapshot. The Win32 input smoke adds mouse-as-touch
contact, coordinate, rectangle, circle, and release checks.
`tests/gfx3/android-touch-smoke.bas` waits for a held device contact,
then validates native GETTOUCH coordinates, ID, and rectangular hit testing.
The Android adapter accepts bounded snapshots of up to 16 contacts; a physical
multi-contact gesture remains a manual device-matrix test. `tests/gfx3/qb-input-smoke.bas`
opens QB SCREEN 13 and verifies all STICK/STRIG no-controller results in normal
and `-mt` builds. `tests/gfx3/android-gamepad-smoke.bas` is the installed-APK
link and no-controller gate for the Android gamepad callbacks. With a paired
controller, repeat it while moving both sticks, triggers, d-pad, and every
mapped button, then compare GETJOYSTICK and GETXPAD values with gfxlib2.

`tests/gfx3/qb-screen-smoke.bas` opens QB SCREEN 7 with its third SCREEN
argument, writes an isolated pixel, and switches the generic work and visible
pages through its public ABI declaration. It proves the historical QB wrapper
mapping that turns SCREEN's visible argument into the active drawing page,
then verifies that pages zero and one retain independent PSET values. Desktop
builds select Null, OpenGL, or Vulkan with compile-time switches; Android
leaves the renderer request unset and validates the automatic GLES fallback.

Pitched two-row transfers include padding sentinels. The tests prove upload
uses only the active source pixels and download leaves destination padding
untouched in the reference, OpenGL, and Vulkan backends.

The typed context/surface layer is exercised separately from raw command tests.
Its write calls are intentionally asynchronous; a later read, download, flush,
or destroy proves that queue ordering makes all earlier work visible.

### 3. Null/reference backend

The null backend is the deterministic reference and headless route. Run:

- `tests/command-sweep/gfxlib-command-sweep.bas`.
- `tests/qb/gfxlib-command-sweep.bas`.
- Runtime-capable tests under `tests/gfx`.
- Focused pixel fixtures for clipping, relative coordinates, VIEW, WINDOW,
  styles, arcs, alpha, palettes, all PUT modes, text, and page changes.

The current Win64 `__FB_GFXLIB3__` run of the FB command sweep exits zero. It
therefore provides a broad headless regression gate for the public mode,
image, file, query, lock, palette, input-fallback, and shutdown paths before
interactive GPU tests are considered.

Package the same source with `fbc-android -gfx3` after Android runtime or file
path changes. Its GFX_NULL mode keeps pixel expectations deterministic while
the NativeActivity package still verifies real Android file I/O, lifecycle,
archive selection, and clean shutdown.

The QB sweep should also be packaged with `fbc-android -lang qb -gfx3` after
any Android runtime or selection change. It covers the QB-only parser/runtime
route in a real automatic GLES mode, including the STICK and STRIG fallbacks.

### 4. Backend conformance

For every GPU backend, render the same command stream to an offscreen surface,
read it back once, and compare it with the null/reference backend. A mismatch
reports the first coordinate, expected value, actual value, command sequence,
surface format, and backend.

Required formats are indexed 8-bit logical color, RGB565, and 32-bit ARGB.
1, 2, and 4-bit modes are represented as indexed data but still require their
documented color masks and palettes.

`tests/gfx3/custom-font-depth-smoke.bas` is the focused custom-font depth
fixture. It constructs the documented bitmap font header and native pixel data
at 8, 16, and 32 bits, then checks TRANS, PSET, and unsupported-character
spacing. Its 16-bit assertions use the gfxlib2 POINT RGB565-to-RGB888
expansion, rather than treating the packed font word as the public result.

`tests/gfx3/custom-font-custom-blender-depth-smoke.bas` covers the same
native depths through `DRAW STRING ... CUSTOM`. It requires one callback per
pixel in every supported glyph rectangle, verifies that unsupported characters
remain true no-write gaps, and exercises the screen, CPU-image, and GPU-only
surface routes. The 16-bit result must pass through the ordinary public
RGB-to-RGB565 conversion before POINT expands it.

### 5. Synchronization

- Queue writes followed by `POINT` and prove the read sees the last write.
- Run `screen-state-benchmark.bas` with three pages and require separate
  producer and SCREENSYNC-completed measurements for SCREENSET, SCREENCOPY, and
  FLIP. Alternate two distinct source pages into a third visible page so the
  workload cannot collapse into self-copy or equal-content no-ops.
- Run `page-flip-presentation-smoke.bas` on every GPU backend. In addition to
  complete page copies, set a restricted VIEW and require the pixel inside the
  VIEW to copy while a pixel outside remains unchanged. This proves the
  transfer-engine fast path falls back to the clipped shader path when needed.
- Queue writes followed by `GET`, `BSAVE`, and `SCREENPTR` access.
- Modify CPU image data, PUT it to a GPU target, and verify upload authority.
- Modify a GPU surface, download/map it, and verify readback authority.
- Map an opaque surface, reject concurrent use, update only part of the CPU
  staging image, unmap it, and verify the uploaded result plus unmapping or
  mode-shutdown cleanup.
- Exercise nested `SCREENLOCK` and partial `SCREENUNLOCK` ranges. In
  `screenptr-nested-lock-smoke.bas`, an inner unlock uploads one pointer-written
  row and the existing pointer then writes another row before the outer unlock;
  both rows must be visible through POINT.
- Destroy a surface immediately after its last queued use and prove destruction
  occurs only after the GPU fence.

### 6. Window and input

- Create, resize, minimize, restore, focus, and close the window.
- Run `resizable-screen-smoke.bas` through gfxlib2 and gfxlib3 OpenGL/Vulkan
  as Win32 and Win64 executables. Require the event and dimension queries to
  change only after every software or GPU page has migrated. Require overlap
  preservation, black expansion pixels, a stable `SCREENPTR` during
  `SCREENLOCK`, deferred completion after unlock, and maximize coverage.
- On X11, repeat the resizable smoke through the gfxlib2 software driver and
  gfxlib3 OpenGL/Vulkan under a real window manager. Android must reject the
  flag because it has no resizable desktop window.
- Switch windowed, borderless fullscreen, and no-frame modes repeatedly while
  confirming that the desktop display mode is unchanged.
- On Win32, run `fullscreen-win32-smoke.bas` in forced OpenGL and forced
  Vulkan forms. Require both fullscreen windows to cover the selected monitor
  and both no-frame windows to retain their requested client dimensions.
- On an X11 desktop with a window manager, run `fullscreen-x11-smoke.bas` in
  forced OpenGL and forced Vulkan forms. Require fullscreen client bounds to
  match the current root window and no-frame client bounds to remain 96 by 64.
  The fixture intentionally does not require any particular window-manager
  decoration implementation, and the run must leave the XRandR mode unchanged.
  Repeat normal and `-mt` forms. An isolated Xvfb session with Openbox is an
  accepted noninteractive EWMH test environment.
- Validate logical-to-window mouse scaling and scanline scaling.
- Validate key press, release, repeat, mouse buttons, wheel, enter/leave, focus,
  close, joystick, XPad, and touch translation where supported.
- Poll events from both ordinary and explicit-poll modes.

### 7. Longevity and failure tests

- Open and close modes thousands of times while checking allocations and GPU
  validation output.
- Recreate swapchains during continuous drawing and page changes.
- Inject allocation, shader, device, context, and window creation failures at
  every initialization stage.
- Force render-thread failure with CPU threads waiting on results and prove all
  waiters wake with an error.
- Run with Vulkan validation layers and OpenGL debug output when available.

### 8. GPU-resident PAINT milestone

- Keep the current CPU `FB.IMAGE` flood-fill reference as the behavioral
  oracle. It must remain available because ordinary image pointers are part of
  the existing language contract.
- Compare screen-page and opaque-surface output byte for byte with that oracle
  for every native depth, alpha setting, VIEW rectangle, border color, and
  solid/pattern form.
- Require Vulkan and desktop OpenGL to complete bounded screen and GPU-surface
  fills without a surface download or upload. Verify both the fully checked
  rectangular specialization and the exact scanline fallback.
- Require Android GLES GPU-only surfaces to use shader ping-pong passes, never
  a CPU surface readback. Normal transferable GLES pages deliberately retain a
  cached CPU shadow because a full-screen ES 3.0 frontier can require thousands
  of raster passes. Measure and report that hybrid path separately.
- Exercise a small enclosed region, a region reaching every pixel in the
  clipped VIEW rectangle, and a transfer-destination-free serpentine path
  longer than 4,096 pixels. The last case proves exact four-neighbour
  continuation beyond the original small-target gate.
- Run `paint-coalescing-smoke.bas` to prove adjacent compatible solid recolours
  can collapse and that a border-coloured intermediate operation cannot.
- Report repeated submission and explicit completion independently. Neither a
  short producer duration nor a fast completion alone satisfies the offload
  and throughput requirement.

## Existing suite notes

`tests/gfx` currently contains 46 FreeBASIC files. All now meet their declared
result with gfxlib3 selected. Eight fbcunit sources contribute 464,128 passing
assertions across 19 tests: alpha blender 458,752; LINE/POINT 1,570; DRAW 22;
image expressions 896; palette 1,384; and FB/QB RGB macros 1,504. The three
standalone GETJOYSTICK, GETXPAD, and GETTOUCH programs run successfully. Five
compile-only positive cases compile and all 30 negative cases fail compilation
as expected.

This closes the focused `tests/gfx` directory, not the repository-wide test
matrix. The command sweep covers more public runtime operations in one headless
program and therefore remains a separate required checkpoint.

The current Win64 rerun selects gfxlib3 with `__FB_GFXLIB3__`, links the
multithreaded gfxlib3 archive, and records its fbcunit result as XML. All eight
gfx fbcunit suites pass their 19 tests. The independent non-fbcunit log run
also passes all 38 cases: five expected compile successes, 30 expected compile
failures, and the three standalone joystick, XPad, and touch fallback
programs. The fbcunit console bridge uses FreeBASIC standard output rather
than a CRT-specific `stdout` symbol so the test runner continues to work when
the compiler's MinGW runtime is updated.

For a current-source fbcunit rebuild, run `make -B -f
gfx3/gfx3-unit-tests.mk all` from `tests/`. The wrapper compiles every selected
gfx source with `-gfx3`, links a separate `fbc-tests-gfx3fresh.exe` with only
`-lfbgfx3mt`, and leaves the regular test executable untouched.

For the non-fbcunit log cases, run `bash -lc 'make -B -f
gfx3/gfx3-log-tests.mk all FB_LANG=fb'` from `tests/`. That wrapper uses the
public `-gfx3` compiler option and forwards it, the selected compiler, and C
compiler through recursive `bmk-make.mk` invocations. The option's preinclude
uses the same bare source-level marker as existing gfxlib3 fixtures, so both
ordinary and already-selected sources compile without a macro collision. This
makes runtime selection part of every module compilation, rather than applying
it only to a final link command.

The existing supported headless setup is `SCREENRES(..., FB.GFX_NULL)`. gfxlib3
must keep that precondition for `IMAGECREATE`, `BLOAD`, and related APIs unless
a separately documented extension deliberately adds context-free images.

## Public performance matrix

Run `tests/gfx3/run-performance-matrix.ps1` after a change to command
production, synchronization, renderer batching, shaders, GPU surfaces, input
snapshots, page handling, or mode lifecycle. The default three samples report a
median for every machine-readable timing. A release comparison should rotate
backend order and use at least seven samples when compositor or clock variance
can affect the conclusion.

The matrix now includes focused families for mode lifecycle, console, every
primitive group, PSET, PAINT, arcs, DRAW, transfers, page/lock/FLIP state,
palette set/get/USING forms, display and input queries, VIEW/WINDOW/PMAP and
POINTCOORD, image allocation/cache/file work, ordinary OMA sprites, clipped
sprites, explicit offload overlap, GPU surfaces, and transforms. The exact
public mapping and deliberate correctness-only exclusions live in
`benchmark-coverage.md`.

Treat producer time and completed time as separate results. A producer win
proves lower BASIC-thread cost. A completed-work win proves throughput. The
independent CPU phase in `sprite-offload-benchmark.bas` is required to prove
that queued GPU work progresses concurrently rather than merely being charged
to a later POINT. Input and control queries must not wait for a GPU sequence
unless their documented result actually depends on rendered pixels.

## GPU asset and transform extension tests

The extension has two focused correctness programs:

- `gpu-asset-smoke.bas` creates small BMP and PNG fixtures, loads both with
  `Gfx3SurfaceLoad`, uses the opaque result as an ordinary PUT source, and
  requires exact POINT results. This detects a hidden GPU-to-CPU-to-GPU path as
  well as descriptor/source regressions.
- `gpu-transform-smoke.bas` checks nearest scaling, a known bilinear texel, an
  exact 90-degree pivot rotation, Mode 7 repeat coordinates, and a transform
  whose opaque GPU destination becomes a later ordinary PUT source. It runs
  against the Null reference, desktop OpenGL, each forced Vulkan device, and
  physical Android GLES.

The exact test intentionally submits individual operations. Backend batch
coverage is separate because combining only one command cannot exercise an
adjacency optimizer. `gpu-transform-benchmark.bas` submits long consecutive
runs sharing one source and destination, then downloads one pixel after each
run. It therefore covers Vulkan multi-dispatch submission batching and GLES
instanced transform draws while measuring completed work. Every backend must
print its renderer name, the three elapsed times, and a final pixel. Desktop
OpenGL and both Vulkan adapters must agree on that pixel.

`check-gles-transform-shaders.ps1` extracts the embedded strings from the C
source and asks glslangValidator to compile and link both the single and batch
ES 3.0 program pairs. This catches syntax, version, varying, and interface
errors without duplicating shader text. It complements rather than replaces a
physical driver run.

The exact smoke also requires zero destination size, zero rotation scale, zero
camera height, and an out-of-range source rectangle to return an error while a
later destroy still succeeds.

Additional required negative coverage before the extension ABI is declared
stable:

- invalid and non-finite scale, rotation, camera, horizon, and focal values
- every built-in destination-reading PUT mode through each GPU backend
- self-transform overlap with source regions touching every destination edge
- 8-bit and RGB565 nearest/linear policy and colour conversion
- asset-load error mapping for missing, truncated, and unsupported files
- a GLES batch exact-output fixture with overlapping and clipped instances
- general affine/projective matrix entry point if one is added publicly

## Oversized CPU-image cache coverage

Run `wide-image-cache-smoke.bas` after changing FB.IMAGE metadata, BLOAD, cache
lookup, atlas assignment, surface capability limits, or CPU-image PUT. The test
must force a 10,960-pixel-wide source and sample small rectangles from both
ends. It must then edit one source region through IMAGEINFO and prove that the
next PUT observes the new pixel.

Required backends are Win32 and Win64 OpenGL, both desktop Vulkan adapters, and
physical Android GLES. The Android result is the important maximum-texture
case. A desktop-only pass does not prove region caching because those adapters
can allocate the complete strip.

The OMA matrix must include all fourteen playable programs. `OMA/Scorched` is
Dolphin support data used for Wii validation, not a fifteenth FreeBASIC
program. Save progress after every game so a later interactive or packaging
failure cannot erase preceding evidence. Android diagnostics must be filtered
by the live package process ID; old logcat records from an earlier package are
not test failures.

## OpenSlicks application workload

Use the checked-in profiler after changing screen-surface blits, point
batches, alpha blending, page shadow ownership, or renderer scheduling:

```powershell
.\tests\gfx3\profile-openslicks.ps1 `
    -OpenSlicksRoot E:\openSlicks `
    -Runtimes gfx2,gfx3 `
    -Gfx3Backend OPENGL `
    -Samples 3 `
    -CaptureScreenshots
```

Repeat with `-Gfx3Backend VULKAN -VulkanDeviceIndex 0` and device index 1.
Run both the Win64 compiler and the Win32 compiler. The script must:

- build the game and its deterministic test runner;
- require all 2,872 assertions to pass;
- enter a live race with the same held acceleration input;
- preserve process CPU, main-thread CPU, renderer logs, and screenshots; and
- record the executable exit code rather than inferring success from process
  creation.

The Android package is built with:

```text
bash tests/gfx3/build-openslicks-android.sh
```

Install it on the AGM A8, enter a live race with a held touch rather than a
single-frame tap, and filter logcat by the current package PID. Automatic
selection must attempt Vulkan and select GLES on this device. A valid steady
race has no uploads, downloads, completions, or waits after the initial track
surface upload.

Run `gpu-surface-smoke.bas` independently whenever the NULL-destination
surface path or screen-shadow rules change. The smoke must use POINT after the
blit, so an asynchronous queue bug cannot pass merely by displaying a plausible
frame.

## Fixed-screen maximize coverage

Run `fixed-screen-maximize-smoke.bas` after changing normal window styles,
native size handling, OpenGL presentation, Vulkan presentation, input
coordinates, `SCREENSYNC`, or page ownership.

The required Windows matrix is gfxlib2, gfxlib3 OpenGL, and gfxlib3 Vulkan on
each available physical adapter. Run every case as both a Win32 and Win64
executable. The smoke must verify:

- a normal framed fixed mode has a maximize control but no resize border;
- maximize leaves `SCREENINFO`, pitch, pages, and logical dimensions intact;
- no `EVENT_WINDOW_RESIZE` is posted;
- the largest whole-number scale is used and native bars are black;
- uniquely colored logical corner and centre pixels appear at exact scaled
  positions;
- `SETMOUSE` reaches the centre of the requested scaled logical pixel;
- a native mouse event maps back to the same logical coordinate; and
- restore returns to the original client dimensions.

Also compile and run `presentation-layout.c` with strict warnings, once
normally and once with `GFXLIB_NEVERSCALE`. Compile
`vulkan-presentation.c` with `-Wall -Wextra -Werror
-fno-strict-aliasing` and run it on every Vulkan adapter. This prevents the
shared layout from gaining an accidental platform dependency and verifies the
real swapchain conversion path.

Keep `resizable-screen-smoke.bas` in the same matrix. Fixed maximize and
`GFX_RESIZABLE` intentionally have opposite logical-size behavior, so both
contracts must pass after either implementation changes.

On Linux, run `fixed-screen-maximize-x11-smoke.bas` under an EWMH-capable
window manager such as Openbox, for gfxlib2, OpenGL, and Vulkan. Check the same
logical size, integer layout, bars, and mouse mapping. A compile-only X11
result is not sufficient.

<!-- end of test-plan.md -->
