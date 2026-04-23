# Linux Folder Guide

This folder contains the Linux backends.

The shared code mixes audio, and these files pass that audio to Linux sound systems.

## Files In This Folder

### `fb_sfx_linux.h`
Shared Linux backend header.

Job:
- defines the common Linux backend state
- gives Linux driver files the same shared data
- declares a few Linux helper routines

Main routines:
- `fb_sfxLinuxInit()`
- `fb_sfxLinuxExit()`
- `fb_sfxLinuxWrite()`
- `fb_sfxLinuxRunning()`
- `fb_sfxLinuxInitDebug()`
- `fb_sfxLinuxDebugEnabled()`

### `sfx_driver_alsa.c`
Playback driver for ALSA.

Job:
- opens an ALSA playback device
- sets sample rate, channel count, and buffers
- writes mixed audio frames to ALSA

Main routines:
- `alsa_driver_init()` starts the ALSA playback device
- `alsa_driver_exit()` shuts it down
- `alsa_driver_write()` sends frames to ALSA
- `fb_sfxDriverAlsa` exports the driver record

### `sfx_capture_alsa.c`
Capture driver for ALSA input.

Job:
- opens an ALSA capture device
- reads recorded frames from Linux audio input
- feeds those frames into the library capture path

Main routines:
- ALSA capture startup helpers
- `fb_sfxCaptureRead()` for reading capture frames
- stop/cleanup helpers for capture shutdown

### `sfx_driver_pulse.c`
Playback driver for PulseAudio.

Job:
- opens a PulseAudio playback stream
- writes mixed samples to the Pulse server
- gives the runtime another Linux backend option

Main routines:
- Pulse driver init helper
- Pulse driver exit helper
- Pulse write helper
- `fb_sfxDriverPulse` exports the driver record

### `sfx_midi_alsa.c`
Linux MIDI backend.

Job:
- opens an ALSA raw MIDI output
- sends short MIDI messages from the common MIDI layer
- gives Linux its own platform MIDI implementation

Main routines:
- `fb_sfxMidiDriverOpen()`
- `fb_sfxMidiDriverClose()`
- `fb_sfxMidiDriverSend()`

## Good Starter Tasks Here

- clean up Linux device setup code
- add clearer error messages
- improve fallback order between Linux backends
- expand capture features

If you are new, `fb_sfx_linux.h` and `sfx_driver_alsa.c` are the best places to start.
