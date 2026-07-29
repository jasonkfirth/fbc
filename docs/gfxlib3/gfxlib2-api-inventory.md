# gfxlib2 API and implementation inventory

## Scope and sources

This inventory is the compatibility contract for gfxlib3. It was derived from:

- `src/compiler/rtl-gfx.bas`, which registers graphics runtime intrinsics.
- `src/compiler/parser-quirk-gfx.bas`, which lowers graphics statements.
- `src/gfxlib2/fb_gfx.h`, which declares the C ABI and driver interface.
- `src/rtlib/fb_gfx_private.h`, which defines the shared context and PUT ABI.
- `inc/fbgfx.bi`, which exposes constants, events, and the FB.IMAGE layout.
- `tests/command-sweep/gfxlib-command-sweep.bas` and `tests/qb`, which exercise
  the FreeBASIC and QB language surfaces.
- The common and platform-specific source files under `src/gfxlib2`.

There are four contracts, not one:

1. The BASIC language commands and functions.
2. The C symbols emitted or referenced by the compiler.
3. The graphics-mode hooks installed into the general runtime.
4. The platform driver callback table.

gfxlib3 must preserve the first three. It replaces the fourth with a larger
platform and GPU backend split.

## BASIC language surface

### Display, pages, and synchronization

| BASIC API | Runtime entry point | Current implementation |
| --- | --- | --- |
| `SCREEN` | `fb_GfxScreen`, `fb_GfxScreenQB` | `gfx_screen.c` selects a mode, allocates CPU pages, installs runtime hooks, and starts a driver. |
| `SCREENRES` | `fb_GfxScreenRes` | `gfx_screen.c` validates an arbitrary size/depth and follows the same setup path. |
| `SCREENINFO` | `fb_GfxScreenInfo32`, `fb_GfxScreenInfo64` | `gfx_screeninfo.c` returns logical mode and driver information. |
| `SCREENLIST` | `fb_GfxScreenList` | `gfx_screenlist.c` merges and sorts driver mode lists. |
| `SCREENSET` | `fb_GfxPageSet` | `gfx_page.c` changes work and visible pages. |
| `FLIP`, `SCREENCOPY` | `fb_GfxFlip` | `gfx_page.c` copies or asks an OpenGL driver to flip. |
| `SCREENLOCK` | `fb_GfxLock` | `gfx_access.c` reference-counts the driver lock. |
| `SCREENUNLOCK` | `fb_GfxUnlock` | `gfx_access.c` marks a line range dirty and releases the driver lock. |
| `SCREENPTR` | `fb_GfxScreenPtr` | `gfx_access.c` returns the current CPU work-page pointer. |
| `SCREENSYNC` | `fb_GfxWaitVSync` | `gfx_vsync.c` delegates to the driver. |
| `WINDOWTITLE` | `fb_GfxSetWindowTitle` | `gfx_screen.c` stores a bounded title and delegates to the driver. |
| `SCREENCONTROL` | `fb_GfxControl_s`, `fb_GfxControl_i32`, `fb_GfxControl_i64` | `gfx_control.c` implements string and integer getters, setters, and event polling. |
| `SCREENEVENT` | `fb_GfxEvent` | `gfx_event.c` consumes the protected event ring. |
| `SCREENGLPROC` | `fb_GfxGetGLProcAddress` | `gfx_opengl.c` exposes a driver GL procedure only in OpenGL mode. |

### Drawing state and primitives

| BASIC API | Runtime entry point | Current implementation |
| --- | --- | --- |
| `PSET`, `PRESET` | `fb_GfxPset` | `gfx_pset.c` transforms, clips, and writes one CPU pixel. |
| `POINT` | `fb_GfxPoint` | `gfx_point.c` synchronously reads one CPU pixel. |
| `POINTCOORD` | `fb_GfxCursor` | `gfx_pmap.c` returns the current graphics pen position. |
| `PMAP` | `fb_GfxPMap` | `gfx_pmap.c` maps physical and logical window coordinates. |
| `LINE`, `LINE ... B`, `LINE ... BF` | `fb_GfxLine` | `gfx_line.c` draws styled CPU lines; boxes are delegated to `gfx_box.c`. |
| `CIRCLE` | `fb_GfxEllipse` | `gfx_circle.c` rasterizes circles, ellipses, arcs, and filled ellipses on the CPU. |
| `PAINT` | `fb_GfxPaint` | `gfx_paint.c` builds flood-fill spans on the CPU, then paints them. |
| `DRAW` | `fb_GfxDraw` | `gfx_draw.c` parses the QB DRAW command language and calls primitive routines. |
| `DRAW STRING` | `fb_GfxDrawString` | `gfx_drawstring.c` draws the built-in font or image-based custom fonts. |
| `VIEW` | `fb_GfxView` | `gfx_view.c` stores the clip rectangle and optional fill/border. |
| `WINDOW` | `fb_GfxWindow` | `gfx_window.c` stores logical coordinate mapping and Y direction. |
| `PALETTE` | `fb_GfxPalette` | `gfx_palette.c` updates emulated and device palettes. |
| `PALETTE USING` | `fb_GfxPaletteUsing`, `fb_GfxPaletteUsing64` | The two width variants update all palette entries. |
| `PALETTE GET` | `fb_GfxPaletteGet`, `fb_GfxPaletteGet64` | The two width variants read one palette entry. |
| `PALETTE GET USING` | `fb_GfxPaletteGetUsing`, `fb_GfxPaletteGetUsing64` | The two width variants copy the complete palette out. |

`RGB` and `RGBA` are compiler/runtime color helpers rather than gfxlib2 drawing
entry points. Their numeric results still form part of graphics compatibility.

### Images, transfers, and files

| BASIC API | Runtime entry point | Current implementation |
| --- | --- | --- |
| `IMAGECREATE` | `fb_GfxImageCreate`, `fb_GfxImageCreateQB` | `gfx_image.c` allocates aligned CPU memory and writes a new or QB-compatible header. |
| `IMAGEDESTROY` | `fb_GfxImageDestroy` | `gfx_image.c` releases the original allocation stored before the aligned header. |
| `IMAGEINFO` | `fb_GfxImageInfo32`, `fb_GfxImageInfo64` | `gfx_image_info.c` validates old/new headers and returns dimensions and pixel memory. |
| `GET` | `fb_GfxGet`, `fb_GfxGetQB` | `gfx_get.c` copies a clipped screen/image rectangle to an image or QB array. |
| `PUT` | `fb_GfxPut` and a selected `fb_hPut*` routine | `gfx_put.c` clips the source and destination and dispatches one of ten blend modes. |
| `BLOAD` | `fb_GfxBload`, `fb_GfxBloadQB` | `gfx_bload.c` identifies BMP plus QB and FreeBASIC raw BSAVE blocks, then performs palette and target conversion. |
| `BSAVE` | `fb_GfxBsave`, `fb_GfxBsaveEx` | `gfx_bsave.c` writes BMP or legacy data according to arguments and dialect. |
| `IMAGECONVERTROW` | `fb_GfxImageConvertRow` | `gfx_image_convert.c` converts 8, 16, 24, and 32-bit row formats. |

The standard new-style FB.IMAGE header is 32 bytes and has type value `7`, a
bytes-per-pixel field, width, height, pitch, a currently unused `tex` word, and
eight reserved bytes. `inc/fbgfx.bi` exposes the last twelve bytes as reserved.
Existing programs may read the header or directly access the following pixels,
so gfxlib3 cannot silently turn an ordinary FB.IMAGE into an opaque GPU object.

### Input

| BASIC API | Runtime entry point | Current implementation |
| --- | --- | --- |
| `MULTIKEY` | runtime hook `fb_GfxMultikey` | Reads the 128-entry key-state table maintained by the platform driver. |
| `GETMOUSE`, `SETMOUSE` | hooks `fb_GfxGetMouse`, `fb_GfxSetMouse` | Delegate to optional driver callbacks and adjust scanline scaling. |
| `GETJOYSTICK` | `fb_GfxGetJoystick` | Platform implementations or stubs report buttons and eight axes. |
| `GETXPAD` | `fb_GfxGetXPad` | Platform implementations or a HID fallback report modern gamepad state. |
| `GETTOUCHCOUNT`, `GETTOUCH` | `fb_GfxGetTouchCount`, `fb_GfxGetTouch` | Use native driver callbacks or a mouse-as-touch fallback. |
| `GETTOUCHHIT` | `fb_GfxGetTouchHit`, `fb_GfxGetTouchHitCircle` | Hit-test the current touch snapshot. |
| QB `STICK`, `STRIG` | `fb_GfxStickQB`, `fb_GfxStrigQB` | Translate joystick state to QB-compatible results. |

### General runtime APIs redirected in graphics mode

`gfx_screen.c` installs these 29 hooks when a graphics mode opens. They are as
important to compatibility as the graphics-only statements.

| Area | Hook entry points |
| --- | --- |
| Keyboard | `fb_GfxInkey`, `fb_GfxGetkey`, `fb_GfxKeyHit`, `fb_GfxMultikey` |
| Text screen state | `fb_GfxColor`, `fb_GfxClear`, `fb_GfxWidth`, `fb_GfxLocateRaw`, `fb_GfxLocate`, `fb_GfxGetX`, `fb_GfxGetY`, `fb_GfxGetXY`, `fb_GfxGetSize`, `fb_GfxReadXY` |
| Text output/input | `fb_GfxPrintBuffer`, `fb_GfxPrintBufferWstr`, `fb_GfxPrintBufferEx`, `fb_GfxPrintBufferWstrEx`, `fb_GfxReadStr`, `fb_GfxLineInput`, `fb_GfxLineInputWstr` |
| Mouse | `fb_GfxGetMouse`, `fb_GfxSetMouse` |
| Miscellaneous runtime | `fb_GfxOut`, `fb_GfxIn`, `fb_GfxSleep`, `fb_GfxIsRedir` |
| Pages | `fb_GfxPageCopy`, `fb_GfxPageSet` |

The graphical console keeps a parallel character-cell array for every page.
`PRINT`, scrolling, `CLS`, `LOCATE`, and `SCREEN(row, col)` must update or read
both pixels and character cells.

## C ABI emitted or referenced by the compiler

`fb_gfx.h` declares 58 unique public `FBCALL` symbols in its main public block.
`fb_GfxImageConvertRow` is a 59th public `FBCALL` symbol declared with the image
conversion helpers. The list is:

```text
fb_GfxBload                 fb_GfxBloadQB
fb_GfxBsave                 fb_GfxBsaveEx
fb_GfxControl_i             fb_GfxControl_i32
fb_GfxControl_i64           fb_GfxControl_s
fb_GfxCursor                fb_GfxDraw
fb_GfxDrawString            fb_GfxEllipse
fb_GfxEvent                 fb_GfxFlip
fb_GfxGet                   fb_GfxGetGLProcAddress
fb_GfxGetJoystick           fb_GfxGetQB
fb_GfxGetTouch              fb_GfxGetTouchCount
fb_GfxGetTouchHit           fb_GfxGetTouchHitCircle
fb_GfxGetXPad               fb_GfxImageConvertRow
fb_GfxImageCreate           fb_GfxImageCreateQB
fb_GfxImageDestroy          fb_GfxImageInfo
fb_GfxImageInfo32           fb_GfxImageInfo64
fb_GfxLine                  fb_GfxLock
fb_GfxPaint                 fb_GfxPalette
fb_GfxPaletteGet            fb_GfxPaletteGet64
fb_GfxPaletteGetUsing       fb_GfxPaletteGetUsing64
fb_GfxPaletteUsing          fb_GfxPaletteUsing64
fb_GfxPMap                  fb_GfxPoint
fb_GfxPset                  fb_GfxPut
fb_GfxScreen                fb_GfxScreenInfo
fb_GfxScreenInfo32          fb_GfxScreenInfo64
fb_GfxScreenList            fb_GfxScreenPtr
fb_GfxScreenQB              fb_GfxScreenRes
fb_GfxSetWindowTitle        fb_GfxStickQB
fb_GfxStrigQB               fb_GfxUnlock
fb_GfxView                  fb_GfxWaitVSync
fb_GfxWindow
```

The compiler also references all ten PUT implementations by address:

```text
fb_hPutTrans    fb_hPutPSet     fb_hPutPReset   fb_hPutAnd
fb_hPutOr       fb_hPutXor      fb_hPutAlpha    fb_hPutAdd
fb_hPutCustom   fb_hPutBlend
```

`fb_hEncode` and `fb_hDecode` are exported compression helpers used by legacy
graphics data. Eleven `fb_image_convert_*` row routines form an internal C
conversion interface. gfxlib3 may share compatible utility code, but its
archive must satisfy every compiler-referenced symbol.

### Win64 archive ABI audit, 2026-07-17

The current archive audit used `nm -g --defined-only` on the actual Win64
`libfbgfx.a` and `libfbgfx3.a` outputs. The gfxlib2 archive defines 122 names
with the `fb_Gfx`, `fb_hPut`, `fb_hPixel`, or `fb_image_convert` prefixes; the
gfxlib3 archive defines 258. Every compiler-emitted public `fb_Gfx*` entry and
every ten selected PUT implementation names listed above is present in
gfxlib3.

The 22 gfxlib2-only names are not missing language ABI hooks. They divide into
three private implementation groups:

- Context, raster, and CPU memory helpers: `fb_GFXCTX_Destructor`,
  `fb_GfxDrawLine`, `fb_hPixelCpy`, and `fb_hPixelSet`.
- Obsolete Win32 driver records: `fb_gfxDriverD2D`, `fb_gfxDriverDirectDraw`,
  `fb_gfxDriverGDI`, and `fb_gfxDriverOpenGL`.
- gfxlib2-internal specialized PUT and row converters: `fb_hPutOrC`,
  `fb_hPutPSetC`, `fb_hPutTrans1C`, plus the eleven
  `fb_image_convert_8to*`, `24*to*`, and `32*to*` helper variants.

gfxlib3 replaces those CPU-only details with its own compatibility layer and
GPU backend operations. They are not compiler call targets and must not be
reintroduced as an accidental route into gfxlib2 state.

## Shared context and state

### Process-wide `FBGFX`

The single `__fb_gfx` object owns mode dimensions, depth, CPU pages, visible
page, framebuffer pointer, palettes, dirty scanlines, driver, console font and
cells, keyboard state, event queue, flags, and the nested screen lock count.

### Per-calling-thread `FB_GFXCTX`

The runtime TLS context owns the work page, row-pointer table, current target,
VIEW rectangle, WINDOW mapping, pen position, foreground/background colors,
pixel access functions, PUT function table, and context flags. Its `id` is
compared with the current screen id so stale thread-local state is rebuilt after
a mode change.

### Locking

- `FB_GRAPHICS_LOCK` protects high-level graphics state.
- `DRIVER_LOCK` calls `fb_GfxLock`, which reference-counts the selected
  driver's lock.
- The event ring has a separate mutex.
- Platform drivers commonly have their own window/update thread and mutex.

The API appears synchronous because primitives finish their CPU writes before
returning. Presentation is often already asynchronous.

## Driver callback API

The `GFXDRIVER` table has 16 callbacks:

```text
init             exit             lock             unlock
set_palette      wait_vsync       get_mouse        get_touch_count
get_touch        set_mouse        set_window_title set_window_pos
fetch_modes      flip             poll_events      update
```

The common core allocates and draws into CPU pages. A normal driver converts or
copies dirty rows to the display. The existing OpenGL driver is still a CPU
renderer: `gfx_opengl.c` uploads the complete CPU framebuffer with
`glTexSubImage2D` and draws one textured quad. It does not execute LINE, CIRCLE,
PUT, text, or PAINT on the GPU.

Driver families currently present include:

- Windows: Direct2D, DirectDraw, GDI, and OpenGL.
- Unix/X11: X11 image, OpenGL, and Linux fbdev where enabled.
- Darwin, Haiku, Android, JavaScript/WebGL, DOS/VESA/VGA, Wii, Xbox, NuttX, and
  the null driver.

This breadth is intentionally not a gfxlib3 requirement.

## Current rendering flow

```text
BASIC statement
  -> compiler lowering
  -> fb_Gfx* entry point
  -> graphics lock and TLS context preparation
  -> coordinate transform and clipping on CPU
  -> CPU pixel loop or memory copy into a work page/image
  -> dirty scanline marks
  -> platform update thread
  -> conversion/upload/blit to the display
```

The main optimization boundary for gfxlib3 is the CPU pixel loop and the later
full framebuffer upload. Parsing, validation, coordinate policy, event
translation, and QB compatibility should remain host-side unless moving them
has a demonstrated benefit.

<!-- end of gfxlib2-api-inventory.md -->
