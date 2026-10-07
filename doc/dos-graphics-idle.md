<!--
    FreeBASIC DOS graphics, dos-graphics-idle.md
    Purpose: Describe the opt-in GUI idle wait and DOS timer build profile.
    Responsibilities: Explain timing, ownership, build caching and qualification.
    This file does not define a general sleep API or a DOS thread scheduler.
-->

# DOS graphics idle profile

`DOS_GFX_LOW_POWER=YesPlease` builds gfxlib2 with a minimum 120 Hz PIT timer.
The selected frequency is a multiple of the requested screen refresh rate.
At 60 Hz it uses two interrupts per frame. The default retains its 1 kHz
timer. This option is independent of DOS threading, TCP and DPMI-yield policy.

A content-checked stamp rebuilds `dos/gfx_dos.o` when this option changes.
It reuses the other objects and refreshes the affected archive. Switching
back to the default does not require a second runtime tree or a clean build.

Lowering the timer rate alone is insufficient. `__dpmi_yield()` returns at
the next available interrupt, which can make a polling application run faster
and consume more CPU. A qualified DOSBox-X GUI can pair the profile with
`fb_GfxDosIdle(int milliseconds)`. omaGUI selects that helper with
`OMAGUI_DOSBOX_X_TIMER_IDLE`; the helper must exist in the linked DOS library.
This flag does not enable the helper in ordinary DOS builds.

The helper belongs to the foreground GUI thread. Release graphics locks
before calling it, and keep screen lifecycle operations on that same thread.
Nonpositive waits return immediately. Requests above one second are capped.
Calls made under a graphics lock or inside the display interrupt return
without yielding. Without an active graphics timer, it yields once and uses
the existing runtime delay. It is a graphics polling wait, not a replacement
for the runtime's audio-aware `SLEEP` path.

IRQ0 advances a monotonic unsigned counter in PIT input cycles. A subtraction
handles counter rollover; the bounded interval is much shorter than the wrap
period. One additional timer period accounts for the partial first period.
The helper therefore waits at least the requested guest interval and can
overshoot by up to two timer periods, plus interrupt and host scheduling
latency. It does not poll `TIMER`, DJGPP `uclock`, BIOS ticks or the frame
update counter, which the renderer consumes. It leaves BIOS tick accumulation
and the ordinary DOS sleep implementation in place.

DOSBox-X qualification uses `dos idle api=true`. The timed 120 Hz prototype
passed Tiko's edit and File-menu actions and reduced measured idle host CPU
from approximately 0.54 to 0.36 cores. The final source passed 113 retained
rendering checks and bounded waits including a locked call and an oversized
request. Both timer profiles compile. Repeated builds and switching profiles
were checked with one private object/archive. These results do not establish
physical DOS compatibility or native Windows responsiveness.

## Software cursor damage

The DOS framebuffer holds application pixels between refreshes. IRQ0 borrows
those pixels to draw the software cursor, publishes them, then restores the
background. Restoring that temporary cursor must not create damage for the
next refresh. A stationary pointer now leaves video memory alone until the
application draws, the cursor moves or changes visibility, or its palette
changes. Mouse events and the display tick continue to run on idle refreshes.

When the cursor moves or hides, its previous rows are marked for publication
from the current framebuffer. Restoring an old saved background at that point
would overwrite application drawing. Any real damage redraws a visible cursor
before publication because banked VESA drivers can copy every row between the
first and last dirty rows. IRQ0 owns the last published position; its state,
code and cursor storage remain locked for interrupt use.

The canonical DOS implementation passed 43 cursor checks at 8, 16 and 32 bits
with both timer profiles. All 25 captured region hashes matched the original
path, including bank-span damage, clipping, palette changes and drawing below
the pointer. Stationary updates fell from seven per 100 ms to zero. One
four-second graphics-only comparison measured 0.146 CPU cores for the original
path and 0.086 for the retained cursor. Tiko's complete-loop CPU comparison
remained inconclusive, so these figures do not establish an editor speedup or
native responsiveness. Physical DOS hardware has not been qualified.

## Deferred refresh requests

IRQ0 keeps at most one pending display refresh while `SCREENLOCK` prevents
publication. Missed refreshes cannot display intermediate frames because those
frames are no longer available. Replaying the old backlog delayed the next
lock after a long draw and could eventually overflow the signed counter.
The pending counter now saturates at the positive refresh interval established
by mode initialization. BIOS time and the foreground idle clock still advance
for every interrupt, independently of display publication.

The focused nested-lock test held the framebuffer for 100 ms. With the 120 Hz
profile, the old counter reached 14 ticks against a two-tick refresh interval;
the bounded implementation stopped at two. Both timer profiles passed 55
checks at 8, 16 and 32 bits, with all 25 cursor region hashes unchanged.
A private matched 20-edit Tiko comparison took approximately 0.83 guest
seconds before coalescing and 0.66 after it. These quantized DOS timings do not
prove native responsiveness. Host marker creation times were reused between
runs and were excluded from the performance evidence.

<!-- end of dos-graphics-idle.md -->
