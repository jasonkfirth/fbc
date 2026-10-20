<!--
Project: FreeBASIC source documentation
File: source-code/sound.md
Purpose: Explain the sound library's source groups and audio execution paths.
Responsibilities: Map commands, synthesis, timing, buffers and platform drivers.
This file intentionally does NOT promise device support from source presence.
-->

# Sound source tree

[src/sfxlib](../../src/sfxlib/) implements BASIC sound commands, software
synthesis, sample playback, MIDI, capture and native audio output. Shared
implementation files live at the library root. Platform audio code lives in
directories such as `linux`, `win32`, `darwin`, `haiku`, `dos` and `unix`;
architecture directories contain accelerated kernels.

```text
sfxlib/
    fb_sfx*.h               Runtime, driver, mixer, buffer and capture contracts
    sfx_*_cmd.c             Command-facing operations
    sfx_sound.c             SOUND dispatch and tone voice setup
    sfx_play.c              PLAY music-string behavior
    sfx_init.c              Runtime initialization
    sfx_shutdown.c          Orderly shutdown
    sfx_core.c              Core lifecycle, update/pumping and driver coordination
    sfx_vars.c              Shared runtime context and defaults
    sfx_voice.c             Voice state and allocation
    sfx_mixer.c             Mixing active voices into output frames
    sfx_oscillator.c         Waveform generation
    sfx_buffer.c            Mix-buffer support
    sfx_ringbuffer.c        Frame storage and producer/consumer positions
    sfx_capture_*.c         Capture commands, data handling and saving
    sfx_music_*.c           Music asset and playback operations
    sfx_midi_*.c            MIDI control and software FM support
    sfx_thread.c            Runtime and driver-I/O locking
    <OS>/ unix/             Native output, capture, MIDI and scheduling adapters
    x86/ x86_64/ arm/ aarch64/
    third_party/            Imported codec or supporting implementation code
```

## From a BASIC command to samples

The compiler's
[parser-quirk-sfx.bas](../../src/compiler/parser/intrinsics/parser-quirk-sfx.bas)
recognizes sound syntax.
[runtime/rtl-sfx.bas](../../src/compiler/runtime/rtl-sfx.bas) declares the
runtime calls and constructs their typed arguments.

For the classic two-argument `SOUND`, the runtime declaration selects
`fb_sfxSoundLegacy2()` in [sfx_sound.c](../../src/sfxlib/sfx_sound.c). That path
preserves the legacy duration unit. The extended channel operation initializes
the runtime as needed, validates the command and queues a voice. Voice setup
assigns the channel, frequency, waveform, level and lifetime.

The [mixer](../../src/sfxlib/sfx_mixer.c) combines active voices into audio
frames. [sfx_core.c](../../src/sfxlib/sfx_core.c) coordinates updates and driver
writes, while the selected platform driver submits audio through its native
API. Caller-clocked raw output has a separate path and must preserve the
caller's frame count and ordering.

Keep frames, samples and bytes distinct. A frame contains one sample per
channel, and buffer sizes must account for both channel count and sample
storage. A duration measured in legacy timer ticks cannot be passed directly
as an output-frame count. The active output sample rate determines the audio
clock used by synthesis.

## Interfaces and state ownership

| Contract | What to inspect |
| --- | --- |
| [fb_sfx.h](../../src/sfxlib/fb_sfx.h) | Context, channels, voices and public entry points |
| [fb_sfx_internal.h](../../src/sfxlib/fb_sfx_internal.h) | Shared implementation helpers and runtime coordination |
| [fb_sfx_driver.h](../../src/sfxlib/fb_sfx_driver.h) | Native callbacks, driver capabilities and timing expectations |
| [fb_sfx_buffer.h](../../src/sfxlib/fb_sfx_buffer.h) | Buffer dimensions, storage and transfer interfaces |
| [fb_sfx_mixer.h](../../src/sfxlib/fb_sfx_mixer.h) | Mixer lifecycle and voice processing |
| [fb_sfx_capture.h](../../src/sfxlib/fb_sfx_capture.h) | Capture lifetime and transfer operations |

The driver contract distinguishes background progress from blocking writes.
Those flags affect who advances playback and whether a command must wait.
MIDI and capture support must be traced through their actual interfaces;
having a platform implementation file does not justify advertising every
capability on the output driver.

[sfx_thread.c](../../src/sfxlib/sfx_thread.c) centralizes locking. Runtime state
and driver I/O have separate coordination requirements, including DOS scheduler
constraints. Check callback and worker lifetimes before changing shutdown:
a worker must stop using a buffer before that buffer or its driver is freed.
Likewise, do not hold a lock across a device wait without checking the platform
path's lock contract.

## Source selection and useful regressions

[mk/source-graph.mk](../../mk/source-graph.mk) selects shared, Unix, target and
architecture sources and applies optional backend filters. C and C++ files
must also have unique selected object basenames. Imported code under
`third_party` follows its integration path rather than an unrestricted
recursive source search.

Use [tests/sfx](../../tests/sfx/) for focused command and mixer regressions.
[render-tone.bas](../../tests/sfx/render-tone.bas) and its
[multi-module definition](../../tests/sfx/render-tone.bmk) are useful starting
points for generated audio; [state.bas](../../tests/sfx/state.bas) exercises
runtime state. A successful null-driver or captured-output check establishes
different behavior from audible playback through a physical device.

The [sound examples guide](../../examples/sfxlib/README.md) connects command
families to runnable programs. The existing
[root folder reference](../../src/sfxlib/OVERVIEW.md) provides a longer file
catalog; use current declarations and implementations for exact signatures.

<!-- end of source-code/sound.md -->
