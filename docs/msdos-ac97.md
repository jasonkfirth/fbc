# AC'97 playback on DOS

DOS sfxlib includes an `AC97` playback driver in both the ordinary and optional
threaded runtime builds. It provides 48 kHz, signed 16-bit stereo output through
PCI bus-master DMA. No external sound library or TSR is needed; DJGPP still
requires a DPMI host such as CWSDPMI.

## Selection and hardware requirements

Automatic selection is:

1. `SoundBlaster`, when `BLASTER` supplies usable settings and the DSP responds.
2. `AC97`, when a supported PCI controller and primary codec respond.
3. `PCSpeaker`.
4. The null driver.

Setting `BLASTER` does not force use of missing hardware. If its DSP cannot be
found, selection continues to AC'97. The existing `SFXLIB_DRIVER=AC97` override
can request AC'97 explicitly, but it uses the same hardware checks. An
unsuccessful explicit request resumes normal automatic selection.

AC'97 is a codec/link standard, not a common register interface for all PCI
sound cards. This first driver accepts these Intel PCI identities:

| Vendor:device | Controller |
| --- | --- |
| `8086:2415` | 82801AA, ICH |
| `8086:2425` | 82801AB, ICH0 |
| `8086:2445` | 82801BA, ICH2 |
| `8086:2485` | ICH3 |

The controller must have PCI class `040100` and firmware-assigned, aligned,
nonoverlapping I/O BARs for its mixer and bus-master registers. PCI BIOS 2.x
services must be available. The driver checks PCI identity and resources
before accessing device ports, then requires codec-ready status, readable
codec vendor IDs and powered DAC/analog circuitry before enabling playback.
It refuses controllers whose input, output or microphone DMA engine is
already running.

An absent or unrecognized controller, MMIO-only resources, missing PCI BIOS
services, or a nonresponding codec makes initialization fail. There is no blind
I/O probe or port-setting override that bypasses discovery. Later Intel ICH
variants and VIA, SiS, NVIDIA and other layouts are not enabled. Intel HDA and
Ensoniq ES1370 are separate interfaces and must not be mistaken for this driver.

## Playback and threading

The backend uses the mandatory AC'97 rate of 48 kHz and reports that actual
rate to the mixer. It disables variable/double-rate modes rather than assuming
the requested rate was accepted. Stereo input retains both channels; mono
input is duplicated. Float conversion clips out-of-range values and converts
NaN to silence before converting to an integer.

A DOS conventional-memory allocation holds the 32-entry descriptor list and
a PCM buffer of at most 4096 stereo frames. The list and samples are physically
contiguous, aligned and below 1 MiB, so the controller never sees protected-mode
heap addresses. Descriptors contain sample counts, not byte or frame counts.

Each write publishes a descriptor, then waits for the controller's completed
last-buffer status, matching current index and zero remaining samples. Extending
the last-valid index resumes playback without resetting DMA between blocks.
Writes consume playback time, matching sfxlib's blocking-driver contract.
This is block playback, not an uninterrupted circular streaming queue; mixing
and scheduling can leave gaps between blocks.

With `-dos-threads pdmlwp`, the existing audio worker feeds this backend and
yields during completion waits. Foreground computation can continue without
calling an audio pump. The ordinary DOS build retains synchronous writes and
the existing delay/foreground sound path. AC'97 itself does not require the
thread provider, change the compiler ABI, or enable threads in QB programs.

PCI completion interrupts remain disabled. The backend does not install an
exclusive handler on a potentially shared, level-triggered PCI interrupt.
Codec access and reset waits have deadlines; a stalled playback block fails
after one second and enters the core's existing fallback path.

Shutdown stops DMA and disables PCI bus mastering before freeing its physical
buffer, resets the output engine, and restores changed codec/PCI settings.
The sound core joins the worker before driver teardown. If broken hardware
cannot be stopped and PCI BIOS cannot disable mastering, teardown retains the
DOS allocation and quarantines the controller instead of exposing freed memory
to DMA. PCI command writes are checked by readback. A failed stop/reset
does not restore the original bus-master enable bit.

## Build and reproduce

The normal DOS source graph discovers the driver automatically:

```sh
make TARGET_TRIPLET=i586-pc-msdosdjgpp sfxlib
```

For background playback, build matching runtime and sound archives with
`DOS_THREAD_PROVIDER=pdmlwp`; see [FreeDOS providers](freedos-providers.md).
The implementation is in [sfx_driver_ac97.c](../src/sfxlib/dos/sfx_driver_ac97.c).
The integration probe builds against both runtime profiles:

```sh
python tests/dos-providers/build-runtime-probes.py \
  --cc i586-pc-msdosdjgpp-gcc --libdir lib/dos --output build/dos-tests --ac97-only
```

This creates `ac97-default.exe` and threaded `ac97-playback.exe`. Supply a
FreeDOS 1.4 boot floppy containing FreeCOM, CWSDPMI r7 and Python with `pyfatfs`
to the QEMU runner. Every run needs a new output directory.

```sh
python tests/dos-providers/run-qemu-audio.py \
  --qemu /path/to/qemu-system-i386 --boot-template /path/to/x86BOOT.img \
  --cwsdpmi /path/to/CWSDPMI.EXE --program build/dos-tests/ac97-default.exe \
  --work build/dos-tests/ac97-direct --devices ac97 --argument direct \
  --expect "PASS AC97 DMA cycle 3"
python tests/dos-providers/check-ac97-wav.py build/dos-tests/ac97-direct/AC97.wav
```

The runner exposes only a copied floppy, disables networking and records
the command, guest logs, hardware WAVs and outcome JSON. It requires the probe
to return successfully to DOS, all requested result markers and a guest
shutdown event. Timeouts are bounded at 120 seconds maximum.

QEMU 11.0.3's WAV audio backend left RIFF/data lengths zero in this environment,
including after APM shutdown. The runner instead uses monitor `wavcapture`
and `stopcapture` through QMP, with the VM paused at startup and retained after
guest power-off. This explicitly finalizes capture before quitting; it does
not repair files or replace the guest driver. Those monitor commands are
deprecated in QEMU 11, so a future release may require updating the capture
mechanism. The host audio sink is disabled; capture still receives the PCM
read by the emulated sound controller through guest PCI DMA.

Useful matrix options, appended to the common runner arguments:

| Executable | Devices | BLASTER option | Probe argument | Required behavior |
| --- | --- | --- | --- | --- |
| Default | `ac97` | `unset` | `direct` | Three DMA cycles, stereo/mono, BDL wrap |
| Threaded | `ac97` | `unset` | `worker` | Playback during foreground computation |
| Default | `both` | `valid` | `sb` | Sound Blaster wins all three cycles |
| Default | `both` | `unset` or `invalid` | `foreground` | AC'97 wins and remains active |
| Default | `none`, `hda`, or `es1370` | `unset` | `absent` | Init fails; forced request falls back |
| Default | `ac97` | `unset` | `resources` | Invalid BARs rejected; lost DMA grant times out and recovers |

The `resources` probe deliberately changes PCI configuration in its disposable
QEMU guest. Do not use that probe on a working physical machine.

## Verification and limits

Tests on September 12, 2026 used QEMU 11.0.3, its Intel 82801AA emulation,
FreeDOS 1.4 and CWSDPMI r7. Direct DMA completed three cycles with stereo/mono
conversion, writes larger than the staging buffer, descriptor wraparound,
invalid argument checks and repeated shutdown. Independent hardware capture
measured approximately 440 Hz left and 881 Hz right, followed by 47,760 audible
samples duplicated across both channels in the mono cycle.

All selection, worker and fault cases in the table passed. DOSBox-X passed
explicit AC'97 rejection and PC speaker fallback with no supported PCI audio
controller present. The worker test remained on AC'97 and captured audible
hardware output while the main thread computed without pumping audio.
The default QB compatibility probe and FreeBASIC GAS/GCC thread/string/exit
probes were rebuilt against the new archives and passed in the AC'97 guest.

These are emulator results. Physical chipsets, other DPMI hosts, recording,
surround output, variable-rate operation, shared PCI IRQ delivery and later
controller generations have not been established by these tests. The complete
fbctests and Exampleageddon platform matrices were not rerun for this driver.

## Register references

The implementation is original code using the Intel register layout and PCI
BIOS interfaces. These primary sources describe or implement those interfaces:

* [PCI BIOS specification, PCI-SIG catalog](https://pcisig.com/specifications?order=title&sort=asc).
* [Linux Intel8x0 controller identities and registers](https://github.com/torvalds/linux/blob/master/sound/pci/intel8x0.c).
* [QEMU AC'97 controller model](https://github.com/qemu/qemu/blob/v11.0.3/hw/audio/ac97.c).
* [QEMU monitor capture interface](https://github.com/qemu/qemu/blob/v11.0.3/audio/audio-hmp-cmds.c).

<!-- end of msdos-ac97.md -->
