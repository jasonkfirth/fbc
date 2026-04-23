# Win32 Folder Guide

This folder contains the Windows backends.

The shared runtime builds audio data, and these files hand that data to Windows APIs.

## Files In This Folder

### `fb_sfx_win32.h`
Shared Windows backend header.

Job:
- defines common Win32 backend state
- gives the Windows files one shared set of declarations
- helps WinMM and WASAPI agree on formats and state

Main routines and data:
- Win32 backend state structure
- helper declarations used by the Windows drivers

### `sfx_driver_winmm.c`
Classic Windows multimedia driver.

Job:
- opens a `waveOut` playback device
- converts floating-point mix data into 16-bit PCM
- hands audio to the older, dependable WinMM API

Main routines:
- `winmm_init()` opens and prepares the WinMM device
- `winmm_exit()` closes it and frees buffers
- `winmm_write()` converts and submits audio frames
- `fb_sfxDriverWinMM` exports the driver record

### `sfx_driver_wasapi.c`
Modern Windows audio driver using WASAPI.

Job:
- creates a WASAPI audio client
- opens the default render endpoint
- exports a driver record for the runtime to try

Main routines:
- `wasapi_init()` starts the WASAPI client
- `wasapi_exit()` shuts it down
- `wasapi_write()` accepts mixed frames for output
- `fb_sfxDriverWASAPI` exports the driver record
- `__fb_sfx_drivers_list` registers the available Windows drivers

### `sfx_capture_wasapi.c`
Windows capture support using WASAPI.

Job:
- opens a capture endpoint
- stops and tears capture down cleanly
- provides a read path for recorded samples

Main routines:
- `fb_sfxCaptureStart()` starts Windows capture
- `fb_sfxCaptureStop()` stops Windows capture
- `fb_sfxCaptureRead()` reads captured frames
- `fb_sfxCaptureActive()` reports whether capture is active

## Good Starter Tasks Here

- adjust sample format handling
- improve device selection
- expand capture support
- tighten WinMM or WASAPI buffer behavior

If you are new, `sfx_driver_winmm.c` is usually the easier Windows file to read first.
