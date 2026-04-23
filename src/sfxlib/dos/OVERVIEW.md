# DOS Folder Guide

This folder is the start of the DOS backend work.

The goal here is simple: let `sfxlib` talk to the kind of hardware a DOS FreeBASIC-style runtime would actually expect to find, especially Sound Blaster audio and MPU-401 style MIDI through the `BLASTER` environment settings.

## `fb_sfx_msdos.h`

Shared DOS header.

What it does:
- defines the parsed DOS sound-card settings
- gives the DOS source files one shared config structure
- declares the `BLASTER` parser helper

## `sfx_driver_sb.c`

Sound Blaster playback driver.

What it does:
- reads the `BLASTER` environment string
- pulls out the base port, IRQ, DMA channels, MPU port, and card type
- resets the DSP
- sends mixed audio out through a simple direct-DAC playback path
- exports the DOS driver list

Current limitation:
- this direct-DAC path is synchronous and does not yet provide a background pump for unattended streaming commands; future DOS work should move to a DMA/IRQ-driven feed model instead of copying the threaded Unix/macOS approach

Main routines:
- `fb_sfxMsdosParseBlaster()`
- `msdos_sb_init()`
- `msdos_sb_exit()`
- `msdos_sb_write()`
- `fb_sfxDriverSoundBlaster`
- `__fb_sfx_drivers_list`

## `sfx_driver_pcspk.c`

PC speaker fallback playback driver.

What it does:
- does not require `BLASTER`
- uses PIT channel 2 one-shot pulses as a crude PCM-style fallback
- keeps DOS audio working when no Sound Blaster is configured

Main routines:
- `msdos_pcspk_init()`
- `msdos_pcspk_exit()`
- `msdos_pcspk_write()`
- `fb_sfxDriverPcSpeaker`

## `sfx_midi_sb.c`

DOS MIDI backend.

What it does:
- uses the `P` value from `BLASTER` when available
- opens an MPU-401 style UART port
- sends short MIDI messages out to the device

Main routines:
- `fb_sfxMidiDriverOpen()`
- `fb_sfxMidiDriverClose()`
- `fb_sfxMidiDriverSend()`
