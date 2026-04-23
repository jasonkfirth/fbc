# Unix Folder Guide

This folder holds shared Unix backend support.

It is not a full driver folder like `win32`. It is more of a common utility area for Unix-like targets.

## Files In This Folder

### `fb_sfx_unix.h`
Shared Unix backend header.

Job:
- defines common Unix backend state
- declares basic lifecycle and debug helpers
- gives Unix-side code one shared contract

Main routines:
- `fb_sfxUnixInit()`
- `fb_sfxUnixExit()`
- `fb_sfxUnixWrite()`
- `fb_sfxUnixRunning()`
- `fb_sfxUnixInitDebug()`
- `fb_sfxUnixDebugEnabled()`

### `sfx_unix.c`
Shared Unix backend implementation.

Job:
- owns the common Unix backend state
- provides simple startup and shutdown helpers
- gives Unix-family drivers a common write/debug base

Main routines:
- `fb_sfxUnixInit()`
- `fb_sfxUnixExit()`
- `fb_sfxUnixWrite()`
- `fb_sfxUnixRunning()`
- `fb_sfxUnixInitDebug()`
- `fb_sfxUnixDebugEnabled()`

### `sfx_midi_bsd.c`
Unix/BSD-style MIDI backend.

Job:
- tries common MIDI device nodes like `/dev/midi`
- writes short MIDI messages to the active device
- gives BSD-like targets a real MIDI path

Main routines:
- `fb_sfxMidiDriverOpen()`
- `fb_sfxMidiDriverClose()`
- `fb_sfxMidiDriverSend()`

## Why This Folder Exists

This folder exists because Unix-like targets often need a little shared plumbing even when the final driver code lives elsewhere.

If you are new, this is a good place to read because it is smaller and more about state and helpers than the main runtime code.
