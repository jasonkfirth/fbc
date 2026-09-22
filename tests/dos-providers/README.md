# Native DOS provider probes

The [DOS AC'97 tests](../../docs/msdos-ac97.md) additionally exercise actual PCI
DMA, absent/unsupported hardware, Sound Blaster priority, invalid PCI resources
and timeout recovery. `run-qemu-audio.py` captures emulated AC'97 output in a
booted FreeDOS guest; `check-ac97-wav.py` verifies stereo tones and mono output.
`build-runtime-probes.py --ac97-only` builds just the ordinary and threaded
AC'97 probes against already built archives.

This directory contains integration tests for the optional PDMLWP runtime
and the earlier standalone library experiments. They are separate from
fbctests and Exampleageddon. See [the provider notes](../../docs/freedos-providers.md)
for supported behavior and limits.

## Runtime integration

First build the compiler and DOS archives with `DOS_THREAD_PROVIDER=pdmlwp`.
From the repository root, with appropriate paths for your installation:

```sh
python tests/dos-providers/build-runtime-probes.py \
  --cc i586-pc-msdosdjgpp-gcc --libdir lib/dos --output build/dos-tests
python tests/dos-providers/build-freebasic-probes.py \
  --fbc bin/fbc.exe --cc /path/to/i586-pc-msdosdjgpp-gcc.exe \
  --libdir lib/dos --output build/dos-tests
python tests/dos-providers/run-dosbox-x.py \
  --dosbox /path/to/dosbox-x.exe --cwsdpmi /path/to/CWSDPMI.EXE \
  --program build/dos-tests/rtlib-threads.exe --work build/dos-tests/run-threads \
  --expect "PASS preemption, x87, errno and TLS" \
  --expect "PASS mutex, condition broadcast, allocation, join and detach"
```

The FreeBASIC builder stages both compiler runtime-directory layouts privately.
It builds GAS and GCC backend executables with the provider option and a QB
executable without it. The C builder uses the same linker wrappers as fbc.
`check-options.py --fbc bin/fbc.exe` checks provider acceptance and unsupported
option diagnostics in compiler emit-only mode. The FreeBASIC runtime probe
closes PHASE.TXT after each major stage to locate a watchdog failure without
depending on buffered console output.

Add `--graphics` to the FreeBASIC builder to build `fbgfxthreads-gas.exe` and
`fbgfxthreads-gcc.exe`; the library directory must also contain `libfbgfxmt.a`.
These render 200 frames while a busy floating-point worker runs, then check
mutex handoff and joining. Run with `--expect "PASS graphics and preemptive worker"`.
The regular C thread probe also checks sleep across an unsigned RTC counter
wrap and emits `PASS sleep across RTC clock wrap`. The speaker variant skips
that clock mutation because it would invalidate a live audio worker's deadline.

`freebasic-tcp.bas` and `tcp-echo-peer.py` exercise the separate Watt-32 profile.
See [the SuperDuel instructions](../../docs/msdos-superduel.md) for the matching
library, packet-driver setup, block sizes and fragmented-reply checks.

Run each executable in its own new `--work` directory:

| Program | Required marker(s) |
| --- | --- |
| rtlib-threads.exe | The two markers in the command above |
| sfx-background.exe | `PASS audio cycle 3`, `PASS background audio` |
| pcspeaker-background.exe with `--speaker-only` | `PASS speaker cycle 3`, `PASS clocked background PC speaker` |
| pcspeaker-background.exe with `--speaker-only --argument fallback` | `PASS speaker fallback` |
| pcspeaker-background.exe with `--speaker-only --argument slow` | `PASS speaker fallback` |
| rtlib-threads.exe with `--speaker-only --argument speaker` | The same two runtime markers, with the fast speaker IRQ active |
| pcspeaker-demo.exe with `--speaker-only` | `PASS FreeBASIC PC speaker demo` |
| sfx-background.exe with `--argument fallback` | `PASS unavailable scheduler retains foreground audio` |
| fbthreads-gas.exe / fbthreads-gcc.exe | `PASS FreeBASIC dynamic strings and thread joins`, `PASS FreeBASIC sound exit reached` |
| qb-default.exe | `PASS default DOS QB strings and foreground audio` |

The audio probe demands the SoundBlaster driver and completion IRQs; it cannot
pass by falling back to the null driver. The fallback mode simulates an
existing RTC owner, requires thread initialization to be refused, and checks
that foreground audio still completes. All modes require return to DOS.

Add `--capture` for a DOSBox-X output WAV using its `DX-CAPTURE /A` command.
The sound test also writes AUDIO.WAV and REPEAT.WAV from accepted mixer PCM.
These serve different purposes: only the emulator WAV records the emulated
hardware output. Validate with `check-wav.py` below.

For actual FreeDOS, add `--boot-template /path/to/x86BOOT.img --timeout 60`.
Use the FreeDOS 1.4 floppy edition image. This mode requires Python packages
`pyfatfs==1.1.0`, `fs==2.4.16`, and `setuptools<81` in an isolated environment.
The harness copies the template, installs its own batch/config files, runs the
probe, and exports TXT/WAV results after APM shutdown. It exports SHUTDOWN.COM
from DOSBox-X's built-in drive. Emulator `--capture` is only supported with
the built-in DOS shell. The default watchdog is 30 seconds, maximum 120.
Use `--timeout 120` for the booted FreeDOS integration runs, including the
three-cycle speaker test. The timeout remains a host-side watchdog.

`--speaker-only` disables Sound Blaster emulation, clears BLASTER, and selects
PCSpeaker. The speaker probe checks foreground CPU progress, sample counts,
output during a runtime lock, underrun silence/recovery, foreground sound,
three initialization/shutdown cycles, and speaker/RTC restoration. It closes
PHASE.TXT at each stage so a watchdog failure can be located in a boot image.
`fallback` simulates an existing periodic RTC owner; `slow` keeps threading but
uses a conflicting square-wave output to exercise the old synchronous driver.
Use `--capture` to verify the actual emulated speaker output as well as the
accepted mixer PCM. The one-bit output intentionally differs from that PCM.

```sh
python tests/dos-providers/check-wav.py build/dos-tests/run-audio/AUDIO.WAV \
  build/dos-tests/run-audio/REPEAT.WAV build/dos-tests/run-audio/probe_000.wav
```

## Earlier provider experiments

Use a DJGPP cross compiler and each library's matching headers and archive.
Example shell commands, run from this directory with the indicated variables
set to the extracted provider paths:

```sh
i586-pc-msdosdjgpp-gcc -std=gnu11 -O2 -Wall -Wextra djgpp-timer.c -o TIMER.EXE
i586-pc-msdosdjgpp-gcc -std=gnu11 -O2 -Wall -Wextra \
  -I"$FSU_INCLUDE" fsu-preempt.c "$FSU_ARCHIVE" -o FSUPROBE.EXE
i586-pc-msdosdjgpp-gcc -std=gnu11 -O2 -Wall -Wextra \
  -I"$PDMLWP_INCLUDE" pdmlwp-preempt.c "$PDMLWP_ARCHIVE" -o PDMLWP.EXE
i586-pc-msdosdjgpp-gcc -std=gnu11 -O2 -Wall -Wextra -s \
  -I"$WATT_INCLUDE" watt32-init.c "$WATT_ARCHIVE" -o WATTINIT.EXE
```

`-s` removes debug information from the Watt probe so it fits on the test
floppy. It does not change the program's checks. The compiler used here was
GCC 12.2.0 from build-djgpp v3.4 for Windows.

Boot FreeDOS with a DPMI host and an external watchdog. First run `TIMER.EXE`:
it must receive ten timer signals while its main code spins without yielding.
Do not interpret scheduler timeouts as provider defects if this control fails.

Then run each mode in its own disposable guest:

* `FSUPROBE join`: checks creation, return values, and joins.
* `FSUPROBE rr`: requests round-robin scheduling and performs repeated atomic
  handoffs between two workers. Neither worker calls a library in the loop.
* `FSUPROBE fifo`: a negative control for equal-priority CPU-bound work. A
  timeout is expected without time slicing; it is not an automated test pass.
* `PDMLWP rtc` and `PDMLWP pit`: use the corresponding timer backend. The PIT
  mode changes the timer frequency, so run it only in a disposable guest.
* `WATTINIT`: run without a packet driver. It must report an initialization
  error and continue to print its PASS marker. This checks failure handling,
  not network traffic.

The FSU and PDMLWP probes close `PHASE.TXT` before entering the workload. DOS
may defer directory-size updates until a file is closed, so redirected stdout
alone is unreliable when a watchdog terminates the guest. Successful runs
must have the expected PASS marker; guest startup or a zero emulator exit
status is insufficient. The local investigation kept each boot image, guest
logs, and timeout register/console snapshots under
`build/research/dos-thread-libs`.

## Experimental library patches

`fsu-djgpp-rr.patch` applies to the source tree inside DJGPP's `fpth314s.zip`.
`pdmlwp-lock-addresses.patch` applies to `contrib/pdmlwp` in `pdmlwp03.zip`.
They preserve the initial source repairs. The maintained PDMLWP provider has
additional fixes in `contrib/dos/pdmlwp`; these patches do not replace it.
Both were checked with `git apply --check` against the original packages.

From the repository root, for the local package layout:

```sh
git apply --check --directory=build/research/dos-thread-libs/fpth314s/contrib/pthreads-3.14 \
  tests/dos-providers/fsu-djgpp-rr.patch
git apply --check --directory=build/research/dos-thread-libs/pdmlwp03/contrib/pdmlwp \
  tests/dos-providers/pdmlwp-lock-addresses.patch
```

Apply patches to separate source copies when rebuilding. FSU also needs its
public/internal configuration headers and assembly offsets regenerated for
the chosen options. Run its `config_header` and `get_offsets` generators as
DOS programs; Windows type sizes are not interchangeable. The retained local
`build_fsu_rr.py` and `run_freedos.py` record the exact experimental build and
guest setup. They are local research helpers, not installed build commands.

The source packages contain their original licenses and author notices.
Retain those sources and notices with any redistributed experimental archive.

<!-- end of README.md -->
