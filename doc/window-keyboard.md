<!-- Project: FreeBASIC graphics; File: window-keyboard.md.
Purpose: Document usable window metrics and application-controlled soft keyboards.
Responsibilities: Units, platform behavior, threading and runnable examples.
This file does not define widget layout or promise that an IME request is accepted. -->

# Window metrics and software keyboards

Include `fbgfx.bi`, or `fbgfx3.bi` for gfxlib3. These are `ScreenControl`
operations, so no compiler parser changes are required. Link graphics and runtime
archives built from the same source revision. New headers alone do not update an
installed SDK, and mixing objects from different internal graphics layouts is unsafe.

| Operation | Parameters |
| --- | --- |
| `GET_WINDOW_METRICS` (19) | Usable width, height, layout DPI, clockwise rotation in degrees |
| `GET_SOFT_KEYBOARD` (20) | Observed visibility, availability; remaining results zero |
| `SET_SOFT_KEYBOARD` (119) | Nonzero requests show; zero requests hide |
| `SET_SOFT_KEYBOARD_REGION` (120) | Editable rectangle: x, y, width, height, in canvas pixels |
| `CLEAR_SOFT_KEYBOARD_REGIONS` (201) | Clear the browser's editable-region list |

Use native `Integer` outputs, or the existing 32/64-bit `ScreenControl`
overloads. Setters do not overwrite their arguments. Visibility is `-1` for
observed visible, `0` for observed hidden, and `-2` when observation is unavailable.
Availability reports whether this backend has a software-keyboard bridge; it
does not indicate that the user has selected an IME or disconnected a hardware keyboard.

`GET_SCREEN_SIZE` retains the BASIC drawable size. `ScreenInfo` retains its
backend display-information role. Window metrics describe the usable host area, which can differ
from that drawable after rotation, resizing or IME appearance. Applications
choose when to resize or scale their drawable. Invalid or unavailable geometry
returns zero dimensions. Do not allocate a framebuffer before bounding its size.

## Browser

Window dimensions are CSS pixels. `visualViewport` accounts for browser zoom
and, where exposed by the browser, the keyboard-reduced visible area. An optional
`Module.fbWindowElement` restricts the result to a containing element; otherwise
the browser window is used. Layout DPI is `96 * devicePixelRatio`. CSS pixels
already incorporate device density: do not multiply CSS layout sizes by this
ratio a second time. Rotation uses `screen.orientation.angle`, with the older
`window.orientation` fallback. Desktop `ScreenInfo` queries now return the
browser's actual screen dimensions instead of a fixed 512 by 512 result.

Mobile browsers require an editable DOM target and user activation. The bridge
creates one unobtrusive textarea, handles committed IME input, and feeds bounded
UTF-8 bytes through the ordinary runtime key/event queues. Intermediate
composition events are not submitted. A single commit is limited to 128 UTF-8
bytes, preventing a paste from overflowing the runtime's 256-entry key queue.
Larger commits are declined as a whole. Desktop browsers without touch input
retain normal canvas keyboard handling and treat show requests as no-ops.

A polling application cannot manufacture user activation after a tap. Before
input, clear and republish its eligible, clipped text-field rectangles. The
bridge keeps at most 256 rectangles. On the trusted canvas pointer event it can
focus the textarea immediately. With no rectangles, an earlier show request is
retried on the next canvas tap. An explicit hide also releases DOM focus.

The VirtualKeyboard API reports actual geometry when available. Other browsers
use viewport occlusion while the bridge owns focus. Focus without reliable
occlusion reports unknown, since it may mean a hardware keyboard is attached.
See the [VirtualKeyboard specification](https://www.w3.org/TR/virtual-keyboard/).

## Android

Both gfxlibs use the NativeActivity Java template's layout observations. Sizes
are native pixels; layout density is Android `densityDpi`, with **160 DPI** as
the device-independent layout baseline. Rotation is relative to the device's
natural orientation. Window measurements are separate from the requested
graphics mode, including while the keyboard is animating.

Show and hide are explicit, asynchronous requests, independent of the optional
KB overlay button. The Java template marshals its bridge to the UI thread.
API 30 and newer use IME visibility from window insets; older releases can
observe an occluding keyboard but report unknown for a focused floating IME.
This also observes Back dismissal instead of leaving a requested-show flag set
forever. See [Android WindowInsets](https://developer.android.com/reference/android/view/WindowInsets).

The activity handles density, smallest-screen-size and screen-layout changes
alongside rotation. A density change can also switch Android's normal/large
screen class; omitting that flag destroys a running BASIC session.

The packaged activity and native runtime must both contain this change to
publish metrics. An older activity leaves dimensions unavailable. Native
callback snapshots are synchronized independently of graphics locks. Call
controls on the application thread; do not use them from a render callback.

The activity owns one input view. Its composing buffer is submitted only when
the IME commits or finishes composition; intermediate prefixes are not typed
again. Native fallback input is disabled while that managed view owns input.
Existing Android text-to-key translation limits still apply; this API does not
add a general Unicode textbox implementation.

Mobile INKEY rings hold 255 queued keys rather than the desktop's 15. An IME
can submit a whole word before BASIC next polls input; the old ring silently
discarded its prefix. The mobile SCREENEVENT ring holds 511 events, allowing
press/release pairs and pointer events around a commit. Android and browser
bridges decline commits longer than 128 characters/UTF-8 bytes respectively
as a whole, instead of forwarding a truncated suffix. These remain bounded
queues; applications must continue draining input regularly.

When an application recreates its drawable to match the measured usable area,
both Android renderers present that page in the usable top-left viewport and
map input through the same rectangle. An independently sized graphics mode
retains its existing scaling behavior. Initial touch coordinates reach the
event queue before button presses, including the first contact without a
preceding mouse move.

## Desktop fallback

Native desktop keyboard setters and region operations are harmless no-ops;
keyboard visibility and availability return zero. Metrics use the current
drawable, or desktop dimensions before a mode exists. Windows queries the
window's DPI, falling back to its device context on older releases. Other
desktop backends currently report nominal 96 DPI and zero rotation.

The Android packager accepts `--link-object FILE` for application-owned native
prerequisites with exactly one selected ABI. It rejects unresolved application
symbols at native link time, keeps same-named BASIC modules separate, and drains
compiler children before removing scratch after a failure. The corresponding
Templari client requires Android API 24 for its Bionic lockf-based preferences.

`tests/gfx/mobile-controls.bas` records a bounded show/hide cycle and observed
metrics in `mobile-controls.log`. The Templari checkout additionally contains
`tests/test_mobile_browser.cjs` and `tests/test_responsive_display_gui.bas` for
IME composition, user-gesture regions and responsive-dialog regression checks.

<!-- end of window-keyboard.md -->
