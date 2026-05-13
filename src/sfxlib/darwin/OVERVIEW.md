# Darwin Folder Guide

This folder is for the macOS backend.

The shared runtime makes sound data. The Darwin files are the part that hand that sound to macOS.

## Files In This Folder

### `fb_sfx_darwin.h`
Shared macOS backend header.

Job:
- keeps the common Darwin/CoreAudio state in one place
- gives macOS driver files a shared set of declarations
- makes it easier to grow the Apple side later

Main routines and data:
- `FB_SFX_DARWIN_STATE`
- `fb_sfxDarwinInit()`
- `fb_sfxDarwinExit()`
- `fb_sfxDarwinWrite()`
- `fb_sfxDarwinRunning()`

### `sfx_driver_coreaudio.c`
CoreAudio playback driver.

Job:
- opens a CoreAudio output queue
- creates a few rotating buffers
- enumerates and selects CoreAudio output devices
- accepts mixed float samples from sfxlib
- sends those samples to macOS for playback

Main routines:
- `fb_sfxDarwinInit()` starts the queue
- `fb_sfxDarwinActivate()` enables the background feed worker used by continuous playback commands
- `fb_sfxDarwinDeactivate()` stops that background feed worker
- `fb_sfxDarwinExit()` shuts it down and frees buffers
- `fb_sfxDarwinWrite()` copies frames into the next queue buffer
- `darwin_device_list()` reports CoreAudio output endpoints
- `darwin_device_select()` rebuilds the queue on the requested endpoint
- `fb_sfxDriverCoreAudio` exports the driver record

Design note:
- CoreAudio queue buffers do not pull fresh mixed audio on their own in this backend, so a small worker thread now calls `fb_sfxUpdate()` for unattended playback in the same spirit as the Linux fix
- The Darwin driver registry also exposes the shared null driver, giving tests
  and headless/QEMU runs a deterministic audio path without opening CoreAudio

### `sfx_capture_coreaudio.c`
CoreAudio capture driver.

Job:
- opens the default CoreAudio input queue
- receives native interleaved 16-bit PCM buffers
- writes captured frames into the shared sfxlib capture ring
- treats missing devices or denied microphone permission as a normal start failure

Main routines:
- `fb_sfxPlatformCaptureStart()`
- `fb_sfxPlatformCaptureStop()`
- `fb_sfxPlatformCaptureRead()`

### `sfx_midi_coremidi.c`
CoreMIDI backend.

Job:
- opens a CoreMIDI client and output port
- picks a destination endpoint
- sends short MIDI messages from the common MIDI layer

Main routines:
- `fb_sfxMidiDriverOpen()`
- `fb_sfxMidiDriverClose()`
- `fb_sfxMidiDriverSend()`

## Good Starter Tasks Here

- tighten timing and latency control
- add an AudioUnit path later if we want a more advanced driver
