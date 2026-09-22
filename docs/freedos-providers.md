# Native FreeDOS threads, sound, and networking

DOS also supports [Intel ICH AC'97 playback](msdos-ac97.md), with Sound Blaster
first when `BLASTER` describes responding hardware. The AC'97 backend requires
actual PCI/codec discovery and works in both the default and threaded profiles.

FreeBASIC XL has an optional preemptive DOS thread provider, selected by
`DOS_THREAD_PROVIDER=pdmlwp` when building the runtime and
`-target dos -dos-threads pdmlwp` when compiling and linking a program.
It uses repaired PDMLWP sources, DJGPP, and a DPMI host. Tests on September 12,
2026 established involuntary thread preemption and background Sound Blaster
and PC speaker playback in DOSBox-X, including a booted FreeDOS 1.4 guest with
CWSDPMI r7.

This is shared-memory concurrency on **one CPU**, not multicore execution or
a complete POSIX pthread implementation. The default DOS runtime keeps its
thread stubs and TCP-disabled policy. Ordinary `-mt` alone is still rejected
on DOS. QB and previous FB programs do not acquire a provider dependency.

## Build and use

Use a DJGPP toolchain and a compiler rebuilt from this source tree. With the
cross tools on PATH and the project's normal cross-build tool/prefix settings:

```sh
make TARGET_TRIPLET=i586-pc-msdosdjgpp DOS_THREAD_PROVIDER=pdmlwp rtlib sfxlib
fbc -target dos -dos-threads pdmlwp program.bas
```

In PowerShell, quote make assignments, for example
`'TARGET_TRIPLET=i586-pc-msdosdjgpp' 'DOS_THREAD_PROVIDER=pdmlwp'`.
The build produces `libfbmt.a`, `libsfxmt.a`, and `libfbpdmlwp.a` in the
DOS runtime directory; ordinary `libfb.a`/`libsfx.a` remain separate.
Provider objects use a separate `dos/pdmlwp` build directory. Link threaded
programs with the provider option even when using separately compiled objects.

The compiler selects MT archives and adds the scheduler and DJGPP linker
wrappers automatically. It rejects non-DOS use, unknown providers, and
`-fpu sse`. `ThreadCall` remains unsupported because this provider has no
libffi ABI. No pthread header ABI is added. DOS cross-linking also no longer
inherits a host compiler's nonexistent DOS `-lsocket` dependency.

To build just the scheduler archive without installing anything:

```sh
python build_scripts/build-dos-pdmlwp.py --cc i586-pc-msdosdjgpp-gcc \
  --ar i586-pc-msdosdjgpp-ar --output build/dos-provider/libfbpdmlwp.a
```

See [the library's provenance and credit file](../contrib/dos/pdmlwp/README.fbxl.md)
for its separate license and redistribution conditions. Retain application
relinkable objects and link instructions as required by that license.
Runtime installation copies the library source, notices and standalone builder
beside the archive in pdmlwp-source. An isolated installation and rebuild from
that installed source were verified.

## Runtime behavior and boundaries

* RTC IRQ8 drives scheduling at 128 Hz. IRQ0/PIT remains available at its
  normal rate for clocks, graphics, and PC speaker use. Initialization refuses
  to take over an enabled periodic RTC. Teardown restores RTC/PIC state and
  the previous handler.
* DOS implements ThreadCreate, ThreadWait, ThreadDetach, ThreadSelf, recursive
  mutexes, and queued condition signal/broadcast/wait. Workers have separate
  TLS tables and locked stacks of at least 256 KiB. A cleanup thread releases
  exited stacks. Condition waits require the mutex to be held exactly once.
  Contended mutexes queue their waiters and reserve ownership on release, so
  a busy worker cannot repeatedly reacquire ahead of a waiting shutdown thread.
  Condition waits release through the same ownership queue.
* Scheduler sleep deadlines use RTC ticks, not `ftime()` or DOS calendar calls
  inside the dispatcher. At 128 Hz the tick interval is 7.8125 ms. Requests
  round upward and allow for the partly elapsed current tick. Long waits split
  into intervals shorter than half the unsigned clock range, handling wrap.
  The speaker's faster interrupt rate is divided before advancing this clock.
* Context switches preserve registers, segment registers, flags, x87 state,
  errno and __dpmi_error. They do not preserve XMM/AVX state. The compiler
  rejects SSE; threaded DOS sound disables its SIMD tiers. Foreign code must
  also avoid live SSE/AVX state.
* Runtime critical sections prevent task switches while keeping hardware IRQs
  enabled. Public mutex/condition waits allow other runnable threads to proceed.
  Blocking DOS calls and long runtime critical sections delay other threads.
* Linker wrappers serialize malloc/calloc/realloc/free, uclock(), and DJGPP's
  __dpmi_int real-mode bridge. delay() uses scheduler/PIT timing instead of BIOS RTC waits.
  GNU ld handles COFF's leading underscore itself: use `--wrap=malloc`, not
  `--wrap=_malloc`.
* This does not make every external C library reentrant. Serialize foreign
  stdio, signal state, direct DOS calls, library globals and callbacks against
  other threads. Do not replace the scheduler's SIGILL handler or periodic RTC
  settings. Do not block on thread operations inside runtime critical sections.
  ISR callbacks must not allocate, mix sound, or wait on mutexes.

Rebuild the DOS runtime, gfxlib2, sfxlib and provider archives together after
changing private thread structures. Sfxlib contains a static private mutex;
mixing an older sfxlib archive with a larger runtime mutex corrupts adjacent
state. Public BASIC thread/mutex handles remain opaque and unchanged.

The SuperDuel investigation adds an integration probe which renders 200 frames
while a busy floating-point worker runs, then acquires its mutex and joins it.
`tests/dos-providers/build-freebasic-probes.py --graphics` builds it for both
GAS and GCC, requiring `libfbgfxmt.a` in the supplied library directory. See
[the SuperDuel report](msdos-superduel.md) for the DOSBox-X and booted FreeDOS
results, IRQ selector fixes, and optional Watt-32 TCP changes.

FreeDOS/DJGPP still need a DPMI host. JEMM is an expanded memory manager/V86
monitor, not a DPMI host. The separate `DOS_DPMI_YIELD=YesPlease` profile
releases a virtual-machine time slice; it supplies no FreeBASIC scheduler.
CWSDPMI r7 is the verified host here. HDPMI, other memory managers and physical
machines need their own tests.

## Sound Blaster integration

When the provider is available, sfxlib starts a FreeBASIC worker after mixer
initialization. It feeds bounded PCM blocks while the application runs,
including while the foreground makes no sound or scheduling calls. Foreground
playback uses the existing driver mutex. Normal and abbreviated shutdown
stop/join the worker before freeing DMA and mixer buffers.

The Sound Blaster ISR acknowledges the DSP and publishes completion only.
Mixing and allocation stay in the worker. IRQ5/IRQ7 DMA8 playback waits for
completion with a timeout, so a missing IRQ cannot hang shutdown indefinitely.
If IRQ installation fails, writes retain synchronous timing. If thread
creation fails, the cooperative audio hook remains available. Driver waits
bypass that hook to avoid recursive mixing in this fallback.

DMA remains **single-cycle 8-bit mono**, with stereo mixed down before transfer.
This is not an auto-init DMA ring and can have gaps between blocks. The change
establishes concurrent feeding and completion handling, not gapless playback
or Windows/Linux audio latency.

Integration tests found failures that simple thread creation would miss:

1. Interrupted movedata() can leave ES pointing at a DMA selector. Before
   entering C, the scheduler now establishes ES=DS and clears DF, then restores
   the interrupted thread's selectors and flags on return.
2. DJGPP delay() uses BIOS INT 15h/AH=86h, which changes RTC periodic timing.
   A second DSP reset stopped preemption. The wrapper avoids that BIOS call.
3. A FreeDOS disk operation and a worker's BIOS clock call could reuse DJGPP's
   real-mode stack concurrently. Serializing __dpmi_int removed the real-mode
   callback protection faults during WAV saving.
4. A TIMER loop with active audio intermittently stalled after all application
   workers had joined. DOS TIMER now locks the complete gettimeofday operation,
   including shared DJGPP clock conversion state. The scheduler originally
   used ftime here too; sleep now uses RTC ticks. Locking just the BIOS bridge
   was insufficient. Three fresh FreeDOS runs after this
   repair completed in 36.595, 35.379 and 29.723 seconds under the same watchdog.

## PC speaker integration

The DOS worker also feeds PCSpeaker. With the provider available, it prepares
one-bit samples in a 1,024-byte queue. A small locked interrupt consumer outputs
one sample per RTC tick at **8,192 Hz**, even while foreground computation or a
runtime critical section prevents the mixer worker from running. Scheduling
still occurs at 128 Hz, once per 64 sample ticks. The extra ticks are acknowledged
in protected mode; the original-rate ticks continue to chain to the old handler.
The consumer uses its own locked 4 KiB stack and performs no allocation, mixing,
floating-point operations, DOS calls, or thread waits.

This does not take over IRQ0 or change the PIT rate. DJGPP's existing uclock()
implementation still initializes its usual PIT counting mode. The shared RTC
interface refuses a second consumer and refuses an enabled alarm, update IRQ,
or square-wave output. Removing the speaker consumer restores the scheduler's
128 Hz rate; runtime teardown restores the original RTC state. Speaker shutdown
restores only port 61h's speaker-control bits, preserving other control changes.

The queue holds up to 125 ms of audio. The worker waits for space with a bounded
progress timeout, while foreground SOUND/PLAY waits for its submitted tail to
finish. Empty queues output silence and can recover when refilled. Long IRQ
blackouts still lose sample timing; a worker blocked longer than the queue's
coverage can underrun. This is a **mono, one-bit, bandwidth-limited** output path.
Multiple sfxlib voices mix before quantization, so chords and background effects
are possible, but amplitude detail and volume fidelity remain limited. It does
not turn the speaker into a Sound Blaster or provide hard real-time guarantees.

Use the existing backend selector before initializing sound:

```basic
environ "SFXLIB_DRIVER=PCSpeaker"
sound 0, 440, 2.0, 0.5
sound 1, 660, 2.0, 0.3
```

Build with `-target dos -dos-threads pdmlwp`. The runnable
[three-voice demo](../tests/dos-providers/pcspeaker-demo.bas) performs foreground
computation without sleep, yield, or sound-pump calls. If the provider or fast
RTC consumer is unavailable, the speaker retains synchronous writes; a worker
can still feed that path if threading is available, but foreground time slices
interrupt its bit-banged waveform. Default DOS and QB keep their existing
foreground playback and require no scheduler archive.

Two further runtime issues surfaced during this work:

* Concurrent DJGPP uclock() calls could mistake an older, preempted BIOS tick
  read for midnight. A FreeDOS probe measured a **86,400.024641-second** jump
  while TIMER advanced 0.05 seconds. The optional profile now wraps complete
  uclock() calls, including latch reads and rollover bookkeeping.
* DJGPP can defer the scheduler's synthetic signal. The dispatcher rechecks
  scheduling exclusion before switching threads, and the outermost runtime
  unlock services a deferred tick. This avoids suspending a critical section
  and prevents frequent short clock/heap locks from starving other workers.

The RTC rate and interrupt restrictions follow the
[DS12885/DS12887 data sheet](https://www.analog.com/en/ds12885/datasheet.html)
and [DJGPP interrupt guidance](https://www.delorie.com/djgpp/v2faq/faq18_9.html).
The clock race was checked against
[DJGPP's uclock source](https://raw.githubusercontent.com/jwt27/djgpp-cvs/master/src/libc/pc_hw/timer/uclock.c)
and the actual linked library's disassembly.

## Tests and reproducibility

[The probe instructions](../tests/dos-providers/README.md) describe the builders
and bounded DOSBox-X harness. Each run uses a fresh directory, requires guest
completion and every requested PASS marker, and retains logs/configuration.
Booted FreeDOS uses a disposable copy of the official floppy. No production
disk is exposed.

Local tools: build-djgpp v3.4/GCC 12.2.0; DOSBox-X 2026.03.29, Pentium,
normal CPU core, fixed 50,000 cycles, 32 MiB RAM; CWSDPMI r7; FreeDOS kernel
2043 and FreeCOM 0.86. Focused checks include:

| Check | What it establishes |
| --- | --- |
| Plain DJGPP timer | Ten asynchronous timer signals while the main loop spins |
| Runtime handoff | Two CPU-bound workers progress without voluntary yields; x87, errno, TLS and identities survive switches |
| Synchronization/heap | Contended mutex, condition broadcast, 4,000 allocation/reallocation iterations, join and detach |
| RTC sleep | Deadline rounding and unsigned counter wrap checked against BIOS time |
| Graphics/threads | 200 VESA frames with a busy x87 worker and mutex/join shutdown; GAS, GCC, banked VESA and booted FreeDOS |
| Background sound | Three start/stop cycles, Sound Blaster completion IRQs, foreground CPU progress and saved PCM |
| Clocked PC speaker | Three cycles, about 410 samples in a 50 ms runtime lock, queue starvation/recovery, and RTC/speaker restoration |
| Threads with speaker IRQs | The same x87, errno, TLS, synchronization, allocation, join and detach checks under the 8192 Hz consumer |
| Three-voice BASIC demo | Concurrent foreground computation and mixed speaker output through the compiler-selected provider |
| FreeBASIC program | Compiler-selected provider, concurrent dynamic strings/error contexts, joins and sound cleanup on END |
| Default DOS QB | 16-bit INTEGER, strings, foreground SOUND and PLAY without a provider |
| Unavailable scheduler | Existing RTC ownership is respected; foreground Sound Blaster audio still completes |

The FreeBASIC program was built with both GAS and GCC backends. The emulator
recording contains 3.791 seconds of non-silent hardware output; accepted mixer
captures contain 0.783 seconds each. Windows single/MT sound archives and the
optional DOS gfxlib2 MT archive also build. Compiler option checks and the
make structure test pass. fblint reports no errors in the changed FreeBASIC
files; existing driver advice and expected DOS-provider test warnings remain.

On September 14, 2026, the DOS fbcunit executable passed in DOSBox-X in all
three supported runtime profiles. The default archive reported 1,140,024
passed, 0 failed, total 2,411. The PDMLWP profile reported 1,140,026 passed,
0 failed, total 2,411. The combined PDMLWP and Watt-32 profile reported
1,140,024 passed, 0 failed, total 2,411. The differing passed count is retained
as emitted by fbcunit; all 2,411 reported tests completed without a failure.
The provider-aware
`tests/file/tcp.bmk` was also compiled with `-mt -dos-threads pdmlwp -l watt`
and run separately. It completed with status 0 against the DOSBox-X NE2000
packet driver and its SLIRP backend. That test is silent unless an assertion
fails.

The full fbctests and Exampleageddon platform matrix was not rerun beyond
these DOS provider profiles. These checks do not establish every graphics,
network, DPMI-host, or CPU combination passing.

The speaker integration probe passed three cycles on booted FreeDOS in 67.994
host seconds. Its phase log and strict sample interval are separate, so FAT
logging time is not counted as playback time. The harness permits a bounded
120-second run for this profile. The emulator capture after the scheduler
repairs contains 5.996 seconds of non-silent speaker output; the two mixer
captures contain 0.750 seconds each. This measures functionality, not fidelity
on a physical speaker. C probe builds retain the existing unused-parameter
warning in fb_unicode.h. Lint of the new BASIC demo and changed compiler
include reports zero errors and 12 advisory warnings: target-specific audio
and environment use, intentional integer note arithmetic/width, and compiler
symbols supplied by the including source.

After these repairs, the FreeDOS runtime-with-speaker test, Sound Blaster
regression, default QB speaker test, and GAS language probe all passed. The
GCC language probe and three-voice demo also passed in the built-in DOS shell.
Both unavailable-RTC speaker paths produced non-silent emulator captures.
Symbol checks confirm the new uclock wrapper is linked into provider programs
and absent from the default QB executable. The rebuilt Windows compiler passes
all eight provider option checks, and the DOS-host compiler source passes its
emit-only check.

## TCP and alternatives

The independent `DOS_TCP_PROVIDER=watt32` profile supplies the guarded IPv4
adapter in src/rtlib/dev_tcp.c. It requires matching Watt-32 headers,
libwatt.a, a packet driver and WATTCP.CFG. It does not itself enable threads.
Missing-packet-driver initialization was tested as a recoverable error.
The [SuperDuel investigation](msdos-superduel.md) tested actual TCP traffic
and found initialization conflicts with the scheduler's SIGILL handler and
the graphics PIT clock. The adapter preserves the exception handlers and
initializes Watt's clock before graphics. Serialized socket operations and
buffered reads now pass 8 KiB exchanges with graphics and a worker, including
fragmented replies and a peer closing immediately after its final bytes.
SuperDuel records gameplay and returns cleanly to DOS with either an external
Windows server or its own DOS server thread. The report distinguishes these
DOSBox-X internal-DOS game runs from the booted FreeDOS graphics/thread probe.
PDMLWP's LWP_TCP means the cleanup thread, not networking.

The published [Watt-32 DJGPP package](https://www.delorie.com/pub/djgpp/current/v2tk/wat3211b.zip)
contains a 2,251,960-byte archive. Its sock_init() macro passes three size
arguments, including sizeof(time_t); an inspected sezero checkout uses two.
Pair headers and archive. The published headers omit shutdown constants
defined internally as 1 and 2; the DOS adapter uses those values locally.
Watt-32's neterr object intentionally replaces DJGPP's `strerror()` so socket
errno values have messages. When a DOS link includes `-l watt` or `#inclib
"watt"`, the compiler detects that final library and passes GNU ld's
`--allow-multiple-definition` for this documented pair only. The option is not
used by default DOS links or by unrelated library duplicates.

[FSU Pthreads](https://arcb.csc.ncsu.edu/~mueller/pthreads/) remains experimental.
The published DJGPP libgthreads.a has round-robin scheduling disabled. The
retained patch repairs dormant RR code and modern compiler build issues, but
RR handoff still timed out. FSU is not selected. GNU Pth is cooperative and
does not meet the no-yield test. HX remains a separate Win32 deployment route.

Earlier QEMU probes timed out even on the plain DJGPP timer. They did not
establish that native preemption was impossible. Passing DOSBox-X controls
and integration tests supersede that blocker; the QEMU setup was not repaired.

Primary sources and package provenance:

* [PDMLWP package](https://ftp5.gwdg.de/pub/msdos/gcc/djgpp/v2tk/pdmlwp03.zip).
* [DJGPP timer documentation](https://www.delorie.com/djgpp/doc/libc/libc_702.html).
* DJGPP [gettimeofday source](https://raw.githubusercontent.com/jwt27/djgpp-cvs/master/src/libc/dos/dos/gettimeo.c)
  and [ftime source](https://raw.githubusercontent.com/jwt27/djgpp-cvs/master/src/libc/dos/sys/timeb/ftime.c),
  cross-checked against the linked DJGPP archive's disassembly.
* [DOSBox-X FreeDOS guide](https://dosbox-x.com/wiki/Guide%3AInstalling-FreeDOS)
  and [supported commands](https://github.com/joncampbell123/dosbox-x/wiki/DOSBox%E2%80%90X%E2%80%99s-Supported-Commands).
* [FreeDOS downloads](https://www.freedos.org/download/) and
  [JEMM documentation](https://github.com/Baron-von-Riedesel/Jemm).
* [DJGPP cross-toolchain release](https://github.com/andrewwutw/build-djgpp/releases/tag/v3.4).

<!-- end of freedos-providers.md -->
