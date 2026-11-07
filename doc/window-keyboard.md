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
| `GET_SOFT_KEYBOARD` (20) | Observed visibility, availability, known keyboard top and height in drawable pixels; zero height means bounds are unavailable |
| `GET_TOUCH_CAPABILITY` (21) | Touch capability in parameter 1: present `-1`, absent `0`, unknown `-2` |
| `GET_DEVICE_CLASS` (22) | `GFX_DEVICE_CLASS_UNKNOWN`, `GFX_DEVICE_CLASS_DESKTOP`, or `GFX_DEVICE_CLASS_HANDHELD` |
| `SET_SOFT_KEYBOARD` (119) | Nonzero requests show; zero requests hide |
| `SET_SOFT_KEYBOARD_REGION` (120) | Editable rectangle: x, y, width, height, in canvas pixels |
| `CLEAR_SOFT_KEYBOARD_REGIONS` (201) | Clear the browser's editable-region list |

Use native `Integer` outputs, or the existing 32/64-bit `ScreenControl`
overloads. Setters do not overwrite their arguments. Visibility is `-1` for
observed visible, `0` for hidden or not enabled for this surface, and `-2`
when observation is unavailable.
Availability reports whether this backend has a software-keyboard bridge; it
does not indicate that the user has selected an IME or disconnected a hardware keyboard.

The optional top/height outputs describe an input panel overlay, without changing
the drawable dimensions returned by `GET_WINDOW_METRICS`. Convert them to your
layout units before panning a focused editor. Check for a positive height first;
older runtimes and backends without native panel geometry return zero.

On LuneOS, gfxlib2 uses the legacy `text_model_factory` and `wl_webos_shell`
interfaces exposed by the compositor. The optional installed
`libwayland-webos-client.so.1` supplies their wire metadata. Requests and events
are matched by name and signature because LuneOS's shell event order differs
from other webOS releases with the same advertised version. Unknown layouts
retain the ordinary Wayland fallback. The driver supplies the launcher's app ID,
handles native client-size and close events, and activates the IME after its
surface receives keyboard focus. Committed text and editing keys enter the
normal graphics input queue. The observed input-panel rectangle supplies the
optional keyboard bounds; this is independent of physical device rotation.
Linux Wayland reports touch from `wl_seat` capabilities. Linux X11 reports
unknown until it has a touch-device query; the value is separate from keyboard
bridge availability so a desktop text-input protocol does not imply a handset.
Linux recognizes an administrator's `/etc/machine-info` chassis value,
mobile-specific session labels, the standard device-tree `chassis-type`, and
SMBIOS handheld/tablet chassis types. Known session labels include Phosh,
Lomiri, Ubuntu Touch, Plasma Mobile, GNOME Mobile, SXMO and Sailfish. KDE's
`QT_QUICK_CONTROLS_MOBILE=true` and `PLASMA_PLATFORM` phone, handset and tablet
markers are also recognized. An explicit machine-info chassis takes precedence
over session and firmware detection. A Mir socket alone does not identify a
handheld, since Mir also supports desktop sessions. Set
`FBC_DEVICE_CLASS=desktop|handheld|mobile|tablet|unknown` to override all of
these sources when an integrator has more reliable information.

`GET_SCREEN_SIZE` retains the BASIC drawable size. `ScreenInfo` retains its
backend display-information role. Window metrics report the host area or
drawable supplied by the backend. The reported size can change after rotation,
resizing or IME appearance, but it may still include space covered by a keyboard
overlay. Applications choose when to resize or scale their drawable. Invalid or
unavailable geometry returns zero dimensions. Do not allocate a framebuffer
before bounding its size.

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

The Android, browser, and Linux INKEY rings hold 255 queued keys rather than
the smaller desktop queue. An IME
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

## Linux handheld sessions

Linux graphics tries the native Wayland driver before X11, including when an
XWayland display is also available. The Wayland driver creates an xdg-shell
toplevel and presents the software framebuffer through `wl_shm`. If no usable
Wayland compositor is present, driver selection continues to X11 and then
fbdev. X11 remains the normal desktop path on Midcom.
On a detected handheld, both Wayland backends request a maximized toplevel for
resizable windows. The compositor can then adjust the usable area for rotation
and the on-screen keyboard. Fixed-size windows keep their requested behavior,
desktop windows are not automatically maximized, and an explicit fullscreen
request retains precedence.
Lomiri-launched applications with a `DESKTOP_FILE_HINT` use native Wayland
when both Wayland and Xwayland are available. An unregistered Mir shell with
an X display retains the Xwayland fallback because Lomiri may reject its
Wayland connection.

The software driver services input and presentation on a window thread, so a
program does not need an input query to show a drawn frame. It reuses at most
two current buffers and keeps a bounded number of older buffers until the
compositor releases them. Changed rows are tracked separately for each buffer.
Wayland frame callbacks pace submissions according to the compositor; buffer
release remains a separate condition for safely reusing the underlying memory.
A resized buffer can replace the old one while a frame callback is pending,
because the compositor may be waiting for the new size before presenting it.
Input and keyboard-region queries can run inside `SCREENLOCK`; presentation
waits until `SCREENUNLOCK` so those queries cannot show a partly drawn frame.
The gfxlib3 EGL capability probe checks libraries without opening a Wayland
connection. Window creation checks the compositor and initializes EGL; this
keeps a temporary probe session from taking the application's Lomiri focus.
The display-metrics probe keeps a connection, handles output updates, and
reconnects after a broken connection rather than opening a new session on each
query. When a Wayland window opens, it adopts that connection after the probe
releases its protocol objects. The probe waits until the window is registered
before it can reconnect. This keeps Lomiri's process focus check from selecting
an earlier metrics-only session. Wayland windows also use the launcher-provided
desktop identity when available.

The display probe reports logical output dimensions, scale, DPI, transform,
touch capability, and text-input-v3 availability before a graphics mode exists.
When available, xdg-output supplies the logical dimensions, including fractional
compositor scaling. Otherwise the probe uses the output mode and integer buffer
scale. Layout DPI is estimated from those dimensions and the physical panel size;
an unavailable or implausible physical size falls back to 96 DPI.
The rotation value follows the transform advertised in `wl_output.geometry`;
it is not a direct reading from an orientation sensor. For example, the
Ubuntu Touch development VM's nested compositor reported a normal Wayland
transform while its Mir output was inverted. Applications should use the
current drawable dimensions for layout even when the reported rotation is zero.
The window driver binds xdg-shell, `wl_seat`, touch, and text-input-v2 or
text-input-v3 when the compositor offers them. Mir-based Ubuntu Touch releases
can expose v2; newer compositors commonly expose v3. Touch contacts enter
gfxlib's mouse and touch paths; editable regions are sent as text-input cursor
rectangles. A focused editable field requests the compositor's configured
keyboard, and committed UTF-8 text is posted through the normal SCREENEVENT
queue. Characters representable by the legacy byte-oriented input API are
also posted to INKEY; other Unicode characters remain available as events.
Each commit is bounded to 255 code points. Composing preedit is not displayed
by these backends; they wait for the input method's committed text. Physical
`wl_keyboard` events are translated with xkbcommon when that runtime library
is present, including compositor-configured key repeat and virtual keyboards
that remap control keys. Mapped physical keys also report Unicode values above
255 through `SCREENEVENT` without passing them to the byte-oriented `INKEY`
queue. The backend still reports hardware keys by scancode
if xkbcommon is unavailable. These queues are bounded; applications must drain
input regularly.
[Mir added text-input-v2 support for Qt clients](https://discourse.ubuntu.com/t/mir-release-2-7-0/26850).

Text-input-v3 commits and deletion requests remain pending until `done`, then
the deletion is queued before the replacement text. Closing or leaving the
focused surface discards its pending edit. The legacy input interface does
not provide surrounding text or selection state: its before-cursor backspace
fallback cannot exactly represent UTF-8 byte counts, and after-cursor deletion
is not supported. Input methods requiring those edits are not fully supported.
On gfxlib2, a focused editor's keyboard request is retained across a temporary
surface focus change; an explicit hide cancels the request even while the
surface is unfocused. Re-registering an unchanged text region does not resend
it to the compositor. Both Wayland drivers commit a disable before restoring a
requested v3 text input after focus enters the surface. This clears stale
input-method state seen on Phosh without forcing the session keyboard visible.
If a virtual keyboard sends a printable XKB key with no legacy scan-code
mapping, its text is still delivered, including repeats, without inventing a
scan code. Applications that consume only byte-oriented INKEY input cannot
receive every Unicode character. omaGUI's native Linux input path consumes
Unicode text from SCREENEVENT in order, while keeping the existing public
byte-oriented key records. Preedit and surrounding-text replacement remain
subject to the limitations above. Its bundled non-DOS font fallback covers
common Latin-1 characters; additional scripts need a font pack loaded by the
application.

Plasma Mobile, GNOME mobile, and Phosh are handled through these Wayland
protocols rather than shell-name checks. The compositor owns keyboard policy
and may resize the toplevel when its keyboard appears; gfxlib applies that
configure as a framebuffer resize. Generic Wayland has no standard
keyboard-visibility query.
Where Phosh exposes `sm.puri.OSK0`, gfxlib uses it to report observed
visibility. It also reads KWin's `org.kde.kwin.VirtualKeyboard` visibility
property when available. The KWin query uses the optional GIO runtime and
does not require the `gdbus` command. These queries run asynchronously, so a
first call can report unknown visibility while discovery is in progress, and
an observed change can take about a second to reach the cache. Fallback show
and hide requests invalidate older observations. Mir/Lomiri sessions can use an
optional Maliit peer bridge; it loads GIO at runtime, includes the active
cursor region in show requests, and passes
committed text and editing keys to the graphics input queue. If Lomiri reports
that a new Wayland window is not focused yet, a bounded worker retries the
show request; hiding the keyboard or closing the window cancels it. If the
Maliit peer disconnects, the bridge retires that connection and a later
keyboard request can reconnect. A request interrupted by the disconnect may
fail and needs to be made again. One request worker handles Linux session
keyboard show and hide, keeping a slow service off the application thread.
Newer requests replace queued ones, and closing a mode cancels its pending show.
After an in-flight show returns, the bridge attempts a hide if the mode has
closed. Because the legacy key queue cannot represent uncommitted composition,
the bridge asks Maliit to disable prediction
for the focused field. This permits direct committed text on keyboards that
honor the hint; it does not provide preedit display, candidate replacement,
or full composition support for keyboards that require them. Maliit may
report visibility through input-method area updates; otherwise visibility while
a show request is active stays unknown. Text-input-v3 advertisement means the
compositor accepts text-input clients, not that an OSK is installed or enabled.
A tested Ubuntu Touch 20.04 development VM paired a Maliit server expecting
`com.canonical.Unity.FocusInfo` with a Lomiri session exposing
`com.lomiri.Shell.FocusInfo`. Its keyboard appeared in a test with a temporary
focus-service proxy, keyboard-policy adjustments, and library-path shims.
A reported bridge does not repair that session mismatch, and gfxlib does not
install a session-wide proxy or change the user's keyboard settings.
The UBports focal repository also contains a Maliit build using the newer
focus-service name. Both graphics libraries passed the same GUI text-entry
test with that server temporarily mounted at its normal executable path,
without the focus proxy. The VM still needed its keyboard-policy and
graphics-library adjustments for that test.
[Maliit's Lomiri focus patch](https://sources.debian.org/patches/maliit-framework/2.3.0-11/2001-obey-lomiri-focus.patch/)
documents the upstream focus-service rename.
A compositor can overlay its keyboard without resizing the drawable; in that
case `GET_WINDOW_METRICS` still includes the covered area. When a keyboard is
visible, or its visibility is unknown on a handheld, omaGUI uses a stable
pre-keyboard viewport to recognize a height-only resize. Otherwise it moves the
focused field toward the top as a conservative fallback and restores its
position when the keyboard closes. A small height reduction may only reflect
shell controls, so omaGUI still uses that fallback when the reduction is less
than a quarter of the stable viewport height. This does not provide an exact
occlusion rectangle and cannot guarantee visibility under every overlay.

gfxlib3's Linux query API uses the same Wayland output probe for screen size,
DPI, rotation, and touch capability, including before a mode is opened. Its
Linux renderer creates compositor-managed EGL windows, prefers GLES, and applies
compositor resize events. Wayland presentation requests a swap interval of one
so the compositor can pace buffer reuse, including with software EGL drivers.
When a resize arrives during `SCREENLOCK`, gfxlib3 keeps the latest request
queued until the outer `SCREENUNLOCK` releases the framebuffer. The next
graphics operation or `SCREENEVENT` query can then apply it.
It handles Wayland pointer, physical keyboard and touch
events, and binds text-input-v2/v3 when available. Committed UTF-8 reaches the
event queue as code points and the bounded key queue when byte-sized;
text-region and soft-keyboard requests go to the compositor. With text-input-v3,
visibility is zero while this surface is unfocused or has no keyboard request.
After the request is enabled, visibility remains unknown unless the session or
input method reports it. This is not a system-wide keyboard visibility query.

On Mesa systems, `LIBGL_ALWAYS_SOFTWARE=true` requests a CPU renderer for
gfxlib3. This can help isolate GPU-driver failures, though software rendering
can be slower and still requires a renderer with the needed GLES features. See
the [Mesa environment-variable reference](https://docs.mesa3d.org/envvars.html).

The gfxlib2 and gfxlib3 Wayland paths are still being qualified on physical handheld sessions.
gfxlib2 touch tracking reports up to 16 active contacts, matching gfxlib's Android
contact limit. The first contact also drives the legacy mouse path; additional
contacts do not change or release that pointer. gfxlib3 publishes up to 16 contacts
through its touch input state. Contacts above either library's limit are ignored
until a slot becomes free. Hardware keyboard and IME input are handled, and
`wl_pointer` provides mouse motion, three buttons, and discrete wheel events in
both libraries. Pointer warping and clipping remain unavailable through the standard
Wayland path. Desktop X11 continues through the existing X11 backend.

References: [Wayland text-input protocol](https://wayland.app/protocols/text-input-unstable-v3),
[GNOME on-screen keyboard behavior](https://help.gnome.org/gnome-help/keyboard-osk.html.en),
[KDE Plasma Keyboard](https://github.com/KDE/plasma-keyboard).

## Desktop fallback

On X11, both graphics libraries accept a single Unicode scalar returned by
`XLookupString` in a UTF-8 locale and publish it through `SCREENEVENT`.
Values through 255 remain available through the legacy `INKEY` interface;
larger values are available as events only. Keyboard mapping notifications
refresh Xlib's lookup table when the desktop layout changes. This path does
not implement an X11 input-method preedit display or multi-character commits.

Native desktop backends without a keyboard bridge ignore show, hide and region
requests and report zero availability. Visibility is zero when the backend
knows the keyboard is hidden, or `-2` when it cannot observe one. Metrics use
the current drawable, or desktop dimensions before a mode exists. Windows
queries the window's DPI, falling back to its device context on older releases.
Other desktop backends currently report nominal 96 DPI and zero rotation.

The Android packager accepts `--link-object FILE` for application-owned native
prerequisites with exactly one selected ABI. It rejects unresolved application
symbols at native link time, keeps same-named BASIC modules separate, and drains
compiler children before removing scratch after a failure. The corresponding
Templari client requires Android API 24 for its Bionic lockf-based preferences.

`tests/gfx/mobile-controls.bas` and `tests/gfx3/mobile-controls-smoke.bas`
record device and touch capability, window metrics and a bounded show/hide cycle
in `mobile-controls.log` and `mobile-controls-gfx3.log`, respectively. These
fixtures do not synthesize touch or focus an editable field, so their logs do not
establish text entry or visible keyboard operation. On Linux,
`sh tests/gfx/linux-maliit-tests.sh` checks the Maliit bridge, input events,
reconnection and shutdown using a private bus; it requires GIO development files
and does not exercise a desktop keyboard. `tests/gfx3/linux-mobile-mode-race.bas`
also exercises real mode replacement alongside keyboard controls on an isolated
X11 display. The Templari checkout also
contains `tests/test_mobile_browser.cjs` and
`tests/test_responsive_display_gui.bas` for IME composition, user-gesture
regions and responsive-dialog regression checks.

## 2026-11-02 mobile application qualification

The current libraries were rebuilt in private focal/glibc and native
postmarketOS 26.06 AArch64/musl trees for Templari. Both complete 286-unit clients
displayed and returned zero after a compositor close, confirming GUI and I/O
worker shutdown. Installed compilers and SDK archives were not replaced.

Twelve native C fixtures passed on each target with warnings treated as errors,
covering device classification, display-cache lifetime, text commits and bounds,
cursor regions, window policy, buffer presentation and both touch backends.
Lomiri displayed both omaGUI renderers and accepted keyboard text. A native
touchscreen fixture also reached the software driver's logical pointer and
touch paths. Its read-only field dismissed the editor's keyboard request.

On the physical phone, gfxlib2 reported 148 DPI and observed the actual OSK.
Touching its `a` key changed the editor from `abc` to `abca`; Enter hid the OSK
and restored its viewport. Rotating with the keyboard open reported 90 degrees,
preserved focus and resized the framebuffer; returning to portrait restored
zero degrees. The phone's original transform and scale were restored.

gfxlib3 also displayed on the phone, using a private header adapter for the
installed 1.20.1 compiler's missing `fbgfx3?` selector. That adapter selected the
MT archive explicitly; it was not installed as a public header. Phone gfxlib3
input/rotation was not qualified by that display-only run. The focal VM's
Maliit session mismatch described above still prevents claiming a visible OSK
there. Templari's `docs/mobile-linux.md` records the recipes and remaining limits.

<!-- end of window-keyboard.md -->
