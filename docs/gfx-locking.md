<!--
Project: FreeBASIC runtime and graphics
File: gfx-locking.md
Purpose: Explain the runtime-hook and graphics-mode lock contract.
Responsibilities: Document callback, allocation and shutdown ownership.
This file intentionally does NOT promise arbitrary user callbacks are thread safe.
-->

# Runtime hooks and graphics mode lifetime

When a path needs both locks, acquire `FB_LOCK()` before
`FB_GRAPHICS_LOCK()` and release them in reverse order. These locks are
recursive on the native MT runtimes. Taking the runtime lock again is safe
only if this thread already owns it before entering graphics.

The global runtime lock protects the hook table and console state. The graphics
lock protects the active mode pointer and its storage. Driver locks protect
presentation access; the event mutex protects input queues. The string lock
protects descriptors and temporary allocation. A drawing-only operation can
use the graphics lock alone, but must not then enter a runtime hook which
acquires `FB_LOCK()`.

## The input and mode-change path

1. `fb_Inkey()` takes the runtime lock before selecting and calling the input
   hook. `fb_GfxInkey()` then takes graphics, polls the driver and consumes a key
   under driver synchronization. `fb_hMakeInkeyStr()` allocates the returned
   temporary string under the string allocator's own lock.
2. `fb_GfxScreen()`, `fb_GfxScreenRes()` and `fb_GfxScreenQB()` take runtime
   before graphics. The internal `set_mode()` keeps those outer locks across
   driver shutdown, old storage release, new allocation and hook publication.
   Readers cannot enter an old hook while the replacement is half installed.
3. Allocation failure releases any partial mode and clears the hooks before
   the outer locks are released. Every public return path balances both locks.
4. The graphics exit callback takes the same lock pair before testing the mode
   pointer. It stops the native driver before releasing mode storage, event
   synchronization and copied drop payloads. Repeated exit calls are harmless.
   `fb_End()` invokes graphics cleanup before runtime global destruction;
   Windows loader-lock rules prohibit moving driver-thread joins into a DLL
   global destructor.

Previously the screen entry points took graphics first and then entered the
runtime lock inside `set_mode()`. An input thread could own runtime and wait
for graphics while the mode thread owned graphics and waited for runtime.
The WinCE screen replacement follows the same corrected entry order.

The legacy LZW encoder and decoder share `fb_lzw_entry`. They now protect that
table with graphics synchronization, including off-screen calls. They must not
take the runtime lock from an image operation already holding graphics.

## Driver and caller obligations

A native producer may post input under its driver/event synchronization. It
must not call runtime hooks while holding those locks. In particular, a worker
being joined by mode teardown cannot wait for runtime or graphics, since the
teardown thread owns them. Stop and join the producer before freeing queues.

`SCREENLOCK` deliberately keeps graphics and framebuffer access locked across
user statements. Keep that interval bounded. Do not change modes, call runtime
console/input hooks or wait for another thread that needs those services while
holding it. A borrowed `SCREENPTR` does not extend a mode's lifetime.

Application callbacks invoked during drawing have the same restriction. Copy
data out and perform runtime work after graphics is released when the work
needs the global lock. String allocation alone uses the separate string lock;
it does not establish ownership of the runtime hook table.

## Executable regression

`make gfx-locking-test` runs
[tests/gfxlib2/locking.c](../tests/gfxlib2/locking.c) through the
[native runner](../build_scripts/test-gfx-locking.py). It links the current
screen and codec implementations against native MT runtime/graphics archives.
Linker wrappers track lock ownership per thread and reject reverse acquisition.

The test arranges a real two-thread input/mode interleave, exercises a polling
callback and temporary-string allocation, checks an injected allocation failure,
round-trips the shared codec with graphics held, and checks orderly repeated
shutdown. It uses the null driver and does not qualify visible presentation,
physical input or every platform driver's worker lifecycle. The runner's
`--control-source` option can verify that an older screen implementation is
rejected by the same regression.

<!-- end of gfx-locking.md -->
