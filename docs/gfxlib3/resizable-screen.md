# FreeBASIC resizable SCREEN contract

## Purpose

This document defines the opt-in resizable window form shared by gfxlib2 and
gfxlib3. It covers the public flag, completed-resize event, framebuffer
lifetime, page preservation, synchronization rules, and backend support.

It does not change the default behavior of `SCREEN` or `SCREENRES`. Programs
which do not request the new flag retain a fixed logical framebuffer.

## Fixed-screen maximize is a separate contract

An ordinary framed fixed-screen window can be maximized without
`GFX_RESIZABLE`. This follows gfxlib2's existing behavior:

- the BASIC-visible width, height, pitch, pages, and console stay unchanged;
- no `EVENT_WINDOW_RESIZE` is posted;
- the largest whole-number scale which fits the native client is used;
- the scaled page is centered and unused native pixels are black; and
- mouse coordinates are converted between the native client and the logical
  framebuffer.

For a logical size `Lw` by `Lh` and native client `Cw` by `Ch`, the scale is:

```text
max(1, min(Cw / Lw, Ch / Lh))
```

The divisions are integer divisions. A 160 by 120 page in a 1366 by 696
client therefore becomes 800 by 600 at native position 283,48, with scale 5.
When a client is smaller than the logical page, scale 1 is retained and the
page is centered with clipped edges, matching gfxlib2.

gfxlib3 performs this presentation entirely after the logical page has been
rendered. OpenGL uses a scaled viewport and a GPU clear for the bars. Vulkan's
presentation compute shader maps invocations inside the presentation
rectangle to logical pixels and writes black outside it. Neither path
reallocates a page, uploads it again, downloads it, or asks the CPU to scale
pixels. `GFXLIB_NEVERSCALE` retains the existing one-to-one presentation
policy.

This fixed-screen behavior must not be confused with the opt-in form described
below. `GFX_RESIZABLE` changes the logical framebuffer itself to match the
client and reports the completed change to the program.

## Opening a resizable window

The existing flags argument accepts `fb.GFX_RESIZABLE`:

```freebasic
#include once "fbgfx.bi"

screenres 640, 480, 32, 2, fb.GFX_RESIZABLE
```

The standard-mode form accepts the same flag:

```freebasic
screen 18, 32, , fb.GFX_RESIZABLE
```

No source define is needed for gfxlib2. A gfxlib3 program selects gfxlib3 in
the usual way and uses the same call:

```freebasic
#define __FB_GFXLIB3__
#include once "fbgfx.bi"

screenres 640, 480, 32, 2, fb.GFX_RESIZABLE
```

`GFX_RESIZABLE` cannot be combined with `GFX_FULLSCREEN`, `GFX_NO_FRAME`, or
`GFX_SHAPED`. Those modes do not have an ordinary framed desktop window to
resize. An invalid combination makes mode creation fail.

## Completed-resize event

Dragging a window edge or using the operating system's maximize and restore
controls changes the logical framebuffer to the new client dimensions. When
that operation has completed, `SCREENEVENT` returns
`fb.EVENT_WINDOW_RESIZE`. The dimensions are available as `event.width` and
`event.height`:

```freebasic
dim event as fb.EVENT

while screenevent( @event )
	if event.type = fb.EVENT_WINDOW_RESIZE then
		print "new framebuffer:"; event.width; "x"; event.height
	end if
wend
```

The event is posted after all pages and the graphical console have migrated.
Drawing and queries performed after receiving it therefore see the new mode,
not an intermediate window-system size.

The named `width` and `height` fields share the existing `x` and `y` event
storage. This keeps the C and FreeBASIC event ABI size unchanged.

## Querying the current dimensions

The following existing APIs report the current logical size after a resize:

```freebasic
dim as integer width, height, depth, bytes_per_pixel, pitch, refresh
dim as string driver

screeninfo width, height, depth, bytes_per_pixel, pitch, refresh, driver
screencontrol fb.GET_SCREEN_SIZE, width, height
```

`SCREENINFO` also reports the new pitch. A program should query these values
again after every resize event instead of retaining the dimensions or pitch
from mode creation.

## Framebuffer and page behavior

Every logical screen page changes size, including pages which are not visible
or active. The overlapping top-left rectangle is preserved on each page.
Pixels exposed by an expansion are initialized to black. Content outside a
smaller new size is discarded.

The visible and active page numbers remain valid. Page flipping and
`SCREENSET` continue to refer to the corresponding resized pages.

The graphical text console changes its row and column capacity to match the
new pixel dimensions and current font. Cells in the overlapping top-left
region are preserved; new cells are blank.

## Direct-memory synchronization

`SCREENPTR` points into size-dependent storage. A pointer obtained before a
completed resize is invalid after the resize event and must not be retained.

While `SCREENLOCK` is active, a pending native resize is deferred. The mapped
pointer and pitch remain stable until the matching `SCREENUNLOCK`. The next
graphics synchronization boundary applies the latest coalesced window size
and posts one completed-resize event.

This rule prevents the window thread from freeing memory while BASIC code has
direct access to it. It also avoids holding a renderer or window lock during a
possibly large allocation.

## Renderer implementation

gfxlib2 allocates replacement software pages before changing the live mode.
It copies the overlap for every page, changes the display driver's backing
bitmap or image while holding the driver lock, then atomically publishes the
new pages. An allocation or driver failure leaves the old mode usable.

gfxlib3 creates new GPU render targets, clears them on the GPU, and copies each
old page's overlap with an ordered GPU blit. The visible replacement page is
presented before the old GPU pages are retired. CPU shadows are uploaded only
when prior direct-memory access made them authoritative. A resize therefore
does not normally download screen pixels to system memory.

The native window thread only publishes the newest client dimensions. The
BASIC thread applies the resize at a graphics API boundary. This lock ordering
keeps input and window dispatch responsive and avoids a window-thread versus
renderer-thread deadlock.

## Platform and backend support

| Runtime | Platform/backend | Status |
| --- | --- | --- |
| gfxlib2 | Win32 GDI | Supported and verified |
| gfxlib2 | X11 software driver | Supported and verified under Xvfb/Openbox |
| gfxlib2 | Explicit legacy OpenGL driver | Not supported for this flag; automatic selection skips it |
| gfxlib3 | Win32 OpenGL | Supported and verified |
| gfxlib3 | Win32 Vulkan | Supported and verified |
| gfxlib3 | X11 OpenGL and Vulkan | Supported and verified under Xvfb/Openbox |
| gfxlib3 | Android | Rejected; NativeActivity does not create a resizable desktop window |

The verified Windows matrix covers Win32 and Win64 executables, two logical
pages, growth and shrink, resize deferral during `SCREENLOCK`, maximize, event
and query dimensions, preserved pixels, black expansion pixels, and invalid
flag combinations. The Linux x86-64 matrix repeats deterministic X11 growth
and shrink, page migration, queries, events, and locking through gfxlib2 X11
plus gfxlib3 OpenGL and Vulkan.

<!-- end of resizable-screen.md -->
