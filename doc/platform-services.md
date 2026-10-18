<!-- FreeBASIC runtime and graphics: platform-services.md.
Purpose: Document portable notifications, cooperative polling and native drops.
Responsibilities: Public contracts, platform prerequisites and useful examples.
This document does not define an application network client or background service. -->

# Notifications, idle polling and received drops

These APIs are available through `fbnotify.bi`, `fbidle.bi` and `fbdrop.bi` in
the `fb` dialect. They use the existing compiler's external runtime ABI; no
new keywords or compiler replacement are required. Build the updated runtime
and graphics libraries before linking applications against these headers.
An installed older archive will not gain the entry points by including a BI.
The ordinary Make source graph selects the common C units and exactly one
platform adapter by filename, including the MT and PIC variants.

## Passive notifications

`FBC.notifyCreate(app_name, context)` initializes an initially zero `Any Ptr`.
Use `notifySend(context, title, body)` to post or replace that context's most
recent notification. `notifyClose` withdraws it, and `notifyDestroy(context)`
releases the native resources and clears the pointer. Destroying a zero context
is harmless. Never reuse a copied handle after its context was destroyed.

Every operation on one context belongs to the thread that created it. A wrong
thread returns `FB.SERVICE_WRONG_THREAD`; it does not destroy the context.
Contexts are independent, but the application must serialize each context's
lifetime. `notifyPoll` pumps only the private Windows notification window.
It preserves application input and `WM_QUIT`. On Android it checks permission
and activity lifetime; on Unix it checks context ownership.

Arguments are counted standard UTF-8 strings on every target, including
Windows. The application name, title and body accept at most 127, 63 and 255
bytes respectively, excluding their terminators. Application name and title
must be nonempty. Malformed UTF-8, embedded NUL and oversized arguments return
`SERVICE_INVALID` without a native post. Limits are deliberately shared across
platforms, so a post does not silently truncate when moved to Windows.
Unix notification bodies are escaped when the desktop service advertises
markup support; plain-text servers receive the original text. Android uses
ordinary text rather than HTML.

| Result | Meaning |
| --- | --- |
| `FB.SERVICE_OK` | Operation accepted. A send does not prove that a person saw it. |
| `FB.SERVICE_UNAVAILABLE` | Optional library, notification server or activity is absent. |
| `FB.SERVICE_INVALID` | Bad argument, size, UTF-8 or context output. |
| `FB.SERVICE_FAILED` | Allocation or native operation failed. |
| `FB.SERVICE_DENIED` | Android permission or notification/channel policy blocks posting. |
| `FB.SERVICE_WRONG_THREAD` | Context belongs to another thread. |

These results do not change BASIC's `ERR`. A context may be created before the
desktop notification server becomes available; the send result is authoritative.
Do Not Disturb, session policy and user preferences can suppress an accepted
post. The service never substitutes a modal message box.

| Platform | Adapter and prerequisites |
| --- | --- |
| Windows | `Shell_NotifyIconW`, a hidden owner-thread window and a shell icon retained until close/destroy. `shell32.dll` is loaded from its absolute system path. No additional import library is required. |
| Linux | Optional `libnotify.so.4`, GLib and a desktop session providing `org.freedesktop.Notifications` on D-Bus. Works independently of the application's X11 or Wayland renderer. No shell command is executed. |
| FreeBSD, NetBSD, OpenBSD, DragonFly | The same optional libnotify/session-bus adapter. A console-only installation can correctly return unavailable. |
| Android | JNI bridge plus the current `src/tools/android` NativeActivity/Java template and notification manifest permission. The existing APK builder installs these templates. Android 8+ gets a persistent `freebasic.messages` channel. Android 13+ also requires the user's `POST_NOTIFICATIONS` grant. |
| Other targets | Linkable API returning unavailable; there is no fake console notification. |

On Android, `notifyPermission(context, 0)` queries permission. Passing `1`
requests it on the Java UI thread and returns denied while the request is
pending. Request in response to an application action, then query while the
user answers. A settings-level block or blocked channel remains denied.
Android displays the APK label as its application identity. Native contexts
retain Java global references safely and release them on destruction. JNI
strings use UTF-16 conversion, including supplementary characters, rather than
JNI's incompatible modified UTF-8 encoding.

An Android worker that creates notification contexts needs at least one MiB
of stack: use `ThreadCreate(@worker, user, 1024 * 1024)`. BASIC's smaller
default worker stack is sufficient for polling alone, but some Android Java
VMs abort while attaching such a thread. Notification creation checks the
native stack first and returns unavailable when it is too small. The APK
template already gives the main BASIC program thread sufficient stack.

Applications mixing direct libnotify calls with this API must initialize
libnotify first and keep that initialization alive until these contexts are
destroyed. libnotify has process-global initialization; the adapter counts its
own context references and never uninitializes an externally initialized copy.
Native libnotify operations are serialized across contexts. The BI also links
the Unix pthread dependency for callers which do not otherwise use `-mt`.

See [notify.bas](../examples/services/notify.bas) for creation, permission,
posting, polling and teardown.

The native APIs and policies are described by
[Microsoft's shell documentation](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shell_notifyiconw),
the [desktop notification specification](https://specifications.freedesktop.org/notification/latest-single/)
and [Android's notification permission documentation](https://developer.android.com/develop/ui/views/notifications/notification-permission).

## A small cooperative idle loop

`FB.IDLE_STATE` is a zero-initialized, 32-byte caller-owned record.
`FBC.idleInit(state, interval_ms)` accepts 1 through 86,400,000 milliseconds.
The first callback is due one interval later. `idleStep(state, callback, user,
maximum_wait_ms)` waits up to the next deadline or the specified wait limit,
then invokes at most one due callback. The wait limit accepts 0 through 60,000
milliseconds; zero is a nonblocking step for an existing application loop.
The default wait limit is 1,000 milliseconds.

The callback runs on the caller's thread, without a runtime/graphics lock.
It returns nonzero to continue or zero to stop. `idleStop(state)` also stops
the loop and may be called by the callback. Test `state.running` in the caller.
Do not reinitialize or recursively step the same state from its callback.
Only the owner thread may read or write the state. Start with a new zeroed
state for a new owner thread.

Windows uses the performance counter. Linux, Android and supporting BSD/Unix
targets use `CLOCK_MONOTONIC`; legacy targets use their runtime timer and rearm
on clock regression. Slow callbacks and suspend/resume coalesce missed ticks,
so reopening a laptop never triggers a burst of catch-up network requests.
Waits do not wake immediately because a console key remains pending.

The wait bound covers sleeping, not callback duration. Keep callbacks short
and use bounded network requests. For a GUI, call `idleStep` between event
processing iterations with a small wait limit. For separate work, use
`ThreadCreate`, publish results under a mutex, and join before freeing state.
Notification contexts remain on their own owner thread.

[idle.bas](../examples/services/idle.bas) shows the finite caller-thread loop.
[idle-worker.bas](../examples/services/idle-worker.bas) shows a separate worker
with a synchronized stop request and a join before cleanup.

An idle loop runs while the operating system schedules the process. It does
not install a daemon, autostart entry, Android foreground service, wake lock or
push transport. Android can suspend or kill an activity process. A client
requiring checks while suspended must use an appropriate Android job/service
or push mechanism. Android documents those choices in its
[background work overview](https://developer.android.com/develop/background-work/background-tasks).

## Native file and text drops

Include `fbdrop.bi`, open a graphics window, then call
`FBC.screenDropEnable()`. The result is `FB.DROP_OK` when that active driver
supports reception and `DROP_UNAVAILABLE` otherwise. Call again after each
mode change. The same BASIC source works with default gfxlib2 and `-gfx3`.

A received drop is an ordinary `SCREENEVENT` with `type = FB.EVENT_DROP`.
The record remains 20 bytes and contains no pointers:

| Field | Meaning |
| --- | --- |
| `drop_id` | Positive identifier valid within this active mode. |
| `drop_count` | Number of items, 1 through `FB.DROP_MAX_ITEMS` (64). |
| `drop_kind` | `FB.DROP_FILES` (1) or `FB.DROP_TEXT` (2). |

Retrieve each zero-based item using `FBC.screenDropGet(drop_id, index, text)`.
Success replaces `text` with its own UTF-8 copy. The native buffer is already
released, and subsequent drops cannot invalidate the returned BASIC string.
Failed identifier/index lookups preserve the output. An allocation failure can
leave the output empty and returns `DROP_FAILED`.

Each mode retains eight payloads with a maximum of 65,536 bytes per payload,
including all item terminators. The ninth drop replaces the first payload.
Read the payload before eight newer drops, disabling reception or changing
modes. A discarded identifier returns `DROP_EXPIRED`; an invalid index returns
`DROP_INVALID`. Peeking at `SCREENEVENT` does not consume the event or payload.
Event queue overflow follows the existing graphics event policy. Payload
retention remains bounded even if the application stops consuming events.

Reception is opt-in. Disabling it clears retained payloads. A mode teardown
stops native producers before releasing payloads and input locks. Input locks
protect queue metadata; BASIC string assignment runs after releasing them.
There are no per-pixel allocations or notification calls in rendering paths.

| Driver | Native behavior |
| --- | --- |
| gfxlib2 Windows and gfxlib3 Windows | Shell file drops (`WM_DROPFILES`), including multiple UTF-16 filenames converted to UTF-8. The transferred shell handle is released even for a disabled or rejected drop. OLE text/object dragging is not implemented. |
| gfxlib2 X11 on Linux/BSD and gfxlib3 Linux X11 | Xdnd versions 3 through 5, copy action, local `text/uri-list`, `UTF8_STRING` and `text/plain;charset=utf-8`. The receiver negotiates types, returns Status/Finished, and abandons an unresponsive selection after five seconds. |
| Other drivers, including Android and direct DRM | Explicit unavailable result. The API and event layout remain portable. gfxlib3 has no native BSD window adapter yet. |

X11 accepts local absolute file URIs, including the `localhost` form, and
decodes percent escapes. Remote hosts, non-file schemes, URI queries/fragments,
embedded NUL, control bytes and malformed UTF-8 are rejected atomically.
Oversized and `INCR` streaming selections are refused rather than buffered
without a bound. A destroyed source window does not terminate the application.
The adapter traps only its own source-request X error serial and forwards
unrelated errors to the application's previous Xlib handler. As with other
Xlib users, applications replacing that process-global handler must coordinate
their handler changes with native event processing.

The X11 receiver negotiates copy when a source initially suggests move or link;
both replies report copy, and no source deletion is requested. Ask, private and
custom actions are refused. This follows the target fallback described in the
[Xdnd protocol](https://wiki.freedesktop.org/www/Specifications/XDND/).

Dropped paths and text are data. No file is opened, URI fetched, command
executed or authorization inferred. The application still decides which files
it will accept and how to handle links or untrusted post text.

[drag-drop.bas](../examples/graphics/drag-drop.bas) is a complete receiver with
capability checking, multiple items, close/Escape handling and copied strings.

## Checked randomization examples

The existing `fbrandom.bi` API and its provider/failure contract are documented
in [secure-random.md](secure-random.md). The new
[secure-random.bas](../examples/services/secure-random.bas) demonstrates a
complete 32-byte request, checked refusal, hexadecimal encoding and buffer
clearing. It reports only the encoded length.
[secure-range.bas](../examples/services/secure-range.bas) demonstrates unbiased
bounded selection with rejection sampling and bounded retries. Neither example
uses or seeds `RND`, and neither silently substitutes a weaker generator.

After building and installing the updated runtime, headers and graphics
archives through the normal Make workflow, these examples build with:

```sh
fbc -mt examples/services/notify.bas
fbc -mt examples/services/idle.bas
fbc -mt examples/services/idle-worker.bas
fbc -mt examples/services/secure-random.bas
fbc -mt examples/services/secure-range.bas
fbc -mt examples/graphics/drag-drop.bas
fbc -mt -gfx3 examples/graphics/drag-drop.bas
```

Use the updated Android APK builder, templates and target runtime for service
examples on a device. For example, `fbc-android --target android-aarch64 -mt
examples/services/notify.bas` builds the ARM64 notification example. Desktop
drag reception is selected by the active graphics driver, not an APK setting.

## Focused verification

`build_scripts/test-platform-services.py` builds deterministic contract tests,
native service objects, all service examples and the native notification test
against an explicitly selected existing SDK. `test-notify-session.py` adds an
isolated D-Bus server and verifies exact Unicode payloads, literal markup,
replacement IDs, refused inputs and close requests.

`build_scripts/test-gfx-drop.py` queries the Make-selected source graph and
builds private graphics archives with at most two compiler jobs. Windows tests
deliver shell-format drops through real native window procedures.
`test-x11-drop.py` uses a private X server and a separate Xdnd sender for files,
text, extended type offers, remote/invalid URIs, oversized/INCR payloads,
supported protocol versions, move-to-copy fallback, unsupported actions,
destroyed sources and timeouts.

`tests/services/android-poll.bas` is a finite native device fixture. Build it
with the updated Android runtime and the current APK template, using package
`org.freebasic.servicetest`. It tests a rejected small-stack worker, then posts
from a correctly sized worker every 1.5 seconds for 20 checks. Press Home after
the first checks and reopen the same activity before completion. Records use
the `FBCServicesTest` logcat tag. Blocking notifications for this test package
exercises an explicit denied result and cleanup. The random-provider check
accepts checked refusal and verifies that the complete 32-byte span was wiped.

The focused qualification on 2026-10-17 covered native notifications and idle
loops on Windows, Debian Linux and FreeBSD, both Windows graphics receivers,
28 Linux Xdnd cases across gfxlib2/gfxlib3 and 14 FreeBSD gfxlib2 Xdnd cases.
Physical A8 (Android 7/API 24) and Galaxy S10 (Android 12/API 31) checks covered
Home/background posts, resume, all 20 worker callbacks and notification close.
The S10 used the persistent notification channel and provided system random
bytes. The A8 correctly refused randomness on its older kernel and wiped the
requested span. Both devices rejected undersized notification worker stacks.
Blocking the A8's application policy and the S10's application and individual
channel settings produced explicit denied results. The test settings were
restored before the final allowed-delivery checks.

Strict semantic lint accepted all eight desktop BASIC roots and the Android
device root, with zero errors and zero operational errors. Two desktop
warnings describe the deliberate 20-byte event ABI assertion. Six Android
warnings describe the runtime callback convention, explicit Android log-library
binding and the test's intentional program termination through the APK loader.

These are focused API and native-protocol checks. They do not prove physical
dragging with every desktop/file manager, human notification visibility,
Android 13+ permission prompts, Doze, process restart or whole-project
regression/release qualification. Background device checks cover a running
process while its activity is stopped; they do not install an OS job. Receipts
record actual exits, source/tool hashes and these boundaries. Private test
archives do not replace the installed SDK, and no Templari integration is made.

<!-- end of platform-services.md -->
