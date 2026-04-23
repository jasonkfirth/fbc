# Haiku Folder Guide

This folder contains the Haiku backend work.

Haiku uses its own Media Kit APIs, so this is where `sfxlib` talks to that side of the system. The files here are a mix of support code and the main Haiku driver.

## Files In This Folder

### `fb_sfx_haiku.h`
Shared Haiku backend header.

Job:
- defines the Haiku backend state
- declares playback and capture entry points
- keeps the C and C++ Haiku files on the same contract

Main routines:
- `fb_sfxHaikuInit()`, `fb_sfxHaikuExit()`
- `fb_sfxHaikuWrite()`
- `fb_sfxHaikuCaptureInit()`
- `fb_sfxHaikuCaptureStart()`, `fb_sfxHaikuCaptureStop()`
- `fb_sfxHaikuCaptureRead()`
- `fb_sfxHaikuRunning()`
- `fb_sfxHaikuInitDebug()`, `fb_sfxHaikuDebugEnabled()`

### `haiku_audio_init.cpp`
Media Kit setup and teardown support.

Job:
- prepares Haiku audio objects
- sets default backend values
- handles setup and cleanup around Media Kit use

Main routines:
- backend initialization helpers
- backend cleanup helpers

### `haiku_audio_stream.cpp`
Stream-oriented playback support for Haiku.

Job:
- manages audio streaming details on top of Haiku APIs
- helps move mixed frames into a Haiku playback path

Main routines:
- stream setup helpers
- stream write/callback helpers
- stop/cleanup helpers

### `sfx_driver_haiku.cpp`
Main Haiku playback driver.

Job:
- owns the exported Haiku driver record
- starts and stops the Haiku backend
- accepts mixed audio from the shared runtime

Main routines:
- `fb_sfxHaikuInitDebug()`
- `fb_sfxHaikuDebugEnabled()`
- `fb_sfxHaikuInit()`
- `fb_sfxHaikuExit()`
- `fb_sfxHaikuWrite()`
- `fb_sfxDriverHaiku` exports the driver record

### `sfx_capture_haiku.cpp`
Haiku capture support.

Job:
- manages recorded input on Haiku
- starts and stops capture
- reads captured frames for the shared runtime

Main routines:
- `fb_sfxHaikuCaptureInit()`
- `fb_sfxHaikuCaptureStart()`
- `fb_sfxHaikuCaptureStop()`
- `fb_sfxHaikuCaptureRead()`

### `sfx_midi_haiku.cpp`
Haiku MIDI backend.

Job:
- opens a Haiku MIDI synth object
- maps short MIDI messages into Haiku MIDI calls
- gives the shared MIDI layer a Haiku-specific send path

Main routines:
- `fb_sfxMidiDriverOpen()`
- `fb_sfxMidiDriverClose()`
- `fb_sfxMidiDriverSend()`

## Good Starter Tasks Here

- improve comments around Media Kit flow
- make playback and capture setup easier to follow
- keep the exported driver/capture entry points small and obvious

If you are new, start with `fb_sfx_haiku.h` and `sfx_driver_haiku.cpp`, then read the two `haiku_audio_*.cpp` support files after that.
