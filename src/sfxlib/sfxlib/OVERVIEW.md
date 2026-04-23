# sfxlib Root Folder Guide

This folder is the middle of the library.

If you are new, think of it as the shared runtime code. The FreeBASIC compiler side will hook BASIC commands like `PLAY`, `SOUND`, `MUSIC PLAY`, and `CAPTURE START` into the `fb_sfx...` routines implemented here.

You do not need to be an audio specialist to work in this folder. Most files are small, named clearly, and stay close to one job.

## How To Read This Folder

- Files starting with `fb_sfx_*.h` are shared headers. They define the common data types and function declarations used by the rest of the library.
- Files starting with `sfx_*.c` are implementation files. Most of them handle one command family or one small subsystem.
- A good rule of thumb:
  `*_cmd`, `*_play`, `*_pause`, `*_stop`, `*_status`, `*_load` = command-facing entry points
  `core`, `vars`, `init`, `shutdown` = runtime plumbing
  `voice`, `mixer`, `oscillator`, `waveform`, `envelope` = synthesis and playback internals

## Behavior Target

When there is a behavior difference between BASIC dialects, this library should lean toward `QBASIC` / `GW-BASIC` first.

That especially matters for things users will notice right away, such as:
- how `PLAY` reads music strings
- how `BEEP` and `SOUND` feel
- what counts as the default tempo, octave, note length, and blocking behavior

If a command did not really exist in those older Microsoft BASICS, the next best rule is:
- borrow from the closest well-known BASIC that did have it
- if there is no clean precedent, make it act the way a BASIC programmer would naturally expect

For `PLAY`, the plain form should behave like the classic BASIC command first.
The extended channel form is where polyphony-friendly behavior is allowed to be looser, because it is an added library feature rather than a direct clone of old QB syntax.

## Header Files

### `fb_sfx.h`
Main shared header for the whole library.

Main things inside:
- runtime structures such as `FB_SFXCTX`, `FB_SFXVOICE`, and `FB_SFXCHANNEL`
- shared constants for voices, channels, waveforms, envelopes, and assets
- the global runtime pointer `__fb_sfx`
- the public runtime entry points `fb_sfxInit()` and `fb_sfxExit()`

### `fb_sfx_buffer.h`
Defines the mix/capture/ring-buffer support types.

Main routines:
- `fb_sfxRingBufferInit()`, `fb_sfxRingBufferShutdown()`
- `fb_sfxRingBufferWrite()`, `fb_sfxRingBufferRead()`
- `fb_sfxRingBufferAvailable()`, `fb_sfxRingBufferFree()`
- `fb_sfxMixBufferInit()`, `fb_sfxMixBufferShutdown()`
- `fb_sfxCaptureBufferInit()`, `fb_sfxCaptureBufferShutdown()`

### `fb_sfx_capture.h`
Defines the capture subsystem interface.

Main routines:
- `fb_sfxCaptureInit()`, `fb_sfxCaptureShutdown()`
- `fb_sfxCaptureStart()`, `fb_sfxCaptureStop()`
- `fb_sfxCapturePause()`, `fb_sfxCaptureResume()`
- `fb_sfxCaptureStatus()`
- `fb_sfxCaptureWrite()`, `fb_sfxCaptureRead()`
- `fb_sfxCaptureSave()`

### `fb_sfx_driver.h`
Defines the platform driver interface.

Main things inside:
- `SFXDRIVER` / `FB_SFX_DRIVER`
- driver callbacks for `init`, `exit`, `write`, capture, polling, and device management
- the global driver list `__fb_sfx_drivers_list`
- the fallback null driver declaration

### `fb_sfx_internal.h`
Shared helper header used by the implementation files.

Main routines declared here:
- debug helpers like `fb_sfxDebugLog()`
- mixer helpers
- voice helpers
- buffer helpers
- envelope helpers
- a few runtime helpers such as context allocation

### `fb_sfx_mixer.h`
Mixer-specific helper declarations.

Main routines:
- `fb_sfxMixerInit()`, `fb_sfxMixerShutdown()`
- `fb_sfxMixerProcess()`
- `fb_sfxMixerClear()`
- `fb_sfxMixerAllocVoice()`, `fb_sfxMixerFreeVoice()`
- `fb_sfxMixerStopChannel()`, `fb_sfxMixerStopAll()`

## Runtime And Core Files

### `sfx_vars.c`
Owns the global runtime state.

Main routines:
- `fb_sfxDefaultConfig()` sets safe startup defaults
- `fb_sfxAllocContext()` creates the runtime context
- `fb_sfxFreeContext()` frees the runtime context

### `sfx_init.c`
Front door for startup.

Main routines:
- `fb_sfxInit()` prepares the runtime and starts the core
- `fb_sfxEnsureInit()` lazily initializes the system if needed

### `sfx_core.c`
Coordinates the running sound system.

Main routines:
- `fb_sfxInit()` performs subsystem startup work
- `fb_sfxExit()` performs subsystem shutdown work
- `fb_sfxUpdate()` asks the mixer for frames and hands them to the driver
- `fb_sfxDriverInit()` picks and starts a platform driver
- `fb_sfxDriverShutdown()` stops the active driver

### `sfx_shutdown.c`
Shutdown path with more explicit teardown steps.

Main routines:
- `fb_sfxExit()` shuts the runtime down in reverse order
- `fb_sfxAbort()` does a simpler emergency-style shutdown

### `sfx_driver_null.c`
Fallback driver that accepts audio and quietly throws it away.

Main routines:
- `null_driver_init()` marks the driver as active
- `null_driver_shutdown()` marks the driver as stopped
- `null_driver_write()` accepts mixed samples and discards them
- `fb_sfxDriverNull` is the driver record exported to the driver list

## Mixer, Voices, Buffers, And Synthesis

### `sfx_buffer.c`
Owns the main runtime mix buffer.

Main routines:
- `fb_sfxBufferInit()`, `fb_sfxBufferShutdown()`
- `fb_sfxBufferClear()`
- `fb_sfxBufferPtr()`, `fb_sfxBufferFrames()`, `fb_sfxBufferSamples()`
- `fb_sfxBufferWrite()`, `fb_sfxBufferRead()`
- `fb_sfxMixBufferInit()`, `fb_sfxMixBufferShutdown()`
- `fb_sfxMixBufferWrite()`, `fb_sfxMixBufferRead()`

### `sfx_ringbuffer.c`
General-purpose circular buffer support.

Main routines:
- `fb_sfxRingBufferInit()`, `fb_sfxRingBufferShutdown()`
- `fb_sfxRingBufferClear()`
- `fb_sfxRingBufferAvailableRead()`, `fb_sfxRingBufferAvailableWrite()`
- `fb_sfxRingBufferWrite()`, `fb_sfxRingBufferRead()`
- `fb_sfxRingBufferPeek()`

### `sfx_voice.c`
Manages individual voices. A voice is one active sound source.

Main routines:
- `fb_sfxVoiceInit()` resets one voice
- `fb_sfxVoiceAlloc()` finds a free voice slot
- `fb_sfxVoiceFree()` releases a voice
- `fb_sfxVoiceStartTone()` prepares a tone voice
- `fb_sfxVoiceStartSample()` prepares a sample voice
- `fb_sfxVoiceStop()` stops one voice
- `fb_sfxVoiceAdvance()` moves playback forward
- `fb_sfxVoiceActiveCount()` counts active voices

### `sfx_mixer.c`
Combines voices into stereo output.

Main routines:
- `fb_sfxMixerInit()`, `fb_sfxMixerShutdown()`
- `fb_sfxMixerClear()`
- `fb_sfxMixerAllocVoice()`, `fb_sfxMixerFreeVoice()`
- `fb_sfxMixerStopChannel()`, `fb_sfxMixerStopAll()`
- `fb_sfxMixerSetChannelVolume()`, `fb_sfxMixerSetChannelPan()`
- `fb_sfxMixerProcessEnvelope()`
- `fb_sfxMixerOscillator()`
- `fb_sfxMixerClamp()`
- `fb_sfxMixerProcess()`

### `sfx_oscillator.c`
Generates raw waveform samples for a voice.

Main routines:
- `fb_sfxOscillatorReset()`
- `fb_sfxOscillatorSample()`
- `fb_sfxVoiceSetFrequency()`

Private helpers inside:
- phase stepping and phase advance
- sine, square, triangle, saw, and noise sample generators

### `sfx_waveform.c`
Waveform-facing helpers for voices.

Main routines:
- `fb_sfxWaveformSample()` chooses the correct waveform generator
- `fb_sfxVoiceSetWaveform()` stores a waveform on a voice

### `sfx_wave_cmd.c`
Implements the waveform definition table used by BASIC-facing commands.

Main routines:
- `fb_sfxWaveCmd()` defines a waveform slot
- `fb_sfxWaveDefined()` checks whether a slot is defined
- `fb_sfxWaveCmdReset()` clears the waveform table

### `sfx_envelope.c`
Implements ADSR envelope behavior.

Main routines:
- `fb_sfxEnvelopeInit()`
- `fb_sfxEnvelopeDefine()`
- `fb_sfxVoiceSetEnvelope()`
- `fb_sfxEnvelopeRelease()`
- `fb_sfxEnvelopeProcess()`

### `sfx_envelope_cmd.c`
Command-facing envelope management.

Main routines:
- `fb_sfxEnvelopeCmd()` defines an envelope from command-style inputs
- `fb_sfxEnvelopeDefined()` checks whether an envelope exists
- `fb_sfxEnvelopeCmdReset()` clears all envelope definitions

### `sfx_instrument.c`
Stores instruments, which group wave and envelope choices together.

Main routines:
- `fb_sfxInstrumentDefine()`
- `fb_sfxInstrumentAssign()`
- `fb_sfxInstrumentDefined()`
- `fb_sfxInstrumentReset()`

## Channel And Mix Control

### `sfx_channel.c`
Owns per-channel settings such as volume, pan, mute, and instrument.

Main routines:
- `fb_sfxChannelInit()`
- `fb_sfxChannelSetVolume()`, `fb_sfxChannelGetVolume()`
- `fb_sfxChannelSetPan()`, `fb_sfxChannelGetPan()`
- `fb_sfxChannelMute()`, `fb_sfxChannelMuted()`
- `fb_sfxChannelSetInstrument()`, `fb_sfxChannelGetInstrument()`
- `fb_sfxChannelReset()`, `fb_sfxChannelResetAll()`

### `sfx_channel_cmd.c`
Tracks the currently selected channel for commands that omit one.

Main routines:
- `fb_sfxChannelCmd()` sets the active command channel
- `fb_sfxChannelCmdGet()` reads it back
- `fb_sfxChannelCmdReset()` returns to channel 0
- `fb_sfxResolveChannel()` maps "default" command use to a real channel

### `sfx_volume.c`
Implements master and per-channel volume helpers.

Main routines:
- `fb_sfxVolume()`, `fb_sfxVolumeGet()`, `fb_sfxVolumeReset()`
- `fb_sfxVolumeChannel()`, `fb_sfxVolumeChannelGet()`

### `sfx_pan.c`
Implements pan commands.

Main routines:
- `fb_sfxPan()`
- `fb_sfxPanGet()`
- `fb_sfxPanReset()`

### `sfx_balance.c`
Implements left/right balance control.

Main routines:
- `fb_sfxBalance()`
- `fb_sfxBalanceGet()`
- `fb_sfxBalanceReset()`

### `sfx_control.c`
General control helpers that do not belong to just one command family.

Main routines:
- `fb_sfxDeviceList()`
- `fb_sfxDeviceSelect()`
- `fb_sfxSetMasterVolume()`
- `fb_sfxGetMasterVolume()`

## Tone, Note, Play, And Sequencing Commands

### `sfx_beep.c`
Implements `BEEP` style tone generation.

Main routines:
- `fb_sfxBeep()`
- `fb_sfxBeepEx()`

### `sfx_sound.c`
Implements `SOUND`.

Main routines:
- `fb_sfxSound()`
- `fb_sfxSoundChannel()`
- `fb_sfxSoundStop()`
- `fb_sfxSoundStopChannel()`

### `sfx_tone.c`
Implements `TONE`.

Main routines:
- `fb_sfxTone()`
- `fb_sfxToneStop()`
- `fb_sfxToneStopAll()`

### `sfx_noise.c`
Implements noise generation.

Main routines:
- `fb_sfxNoise()`
- `fb_sfxNoiseStop()`
- `fb_sfxNoiseStopAll()`

### `sfx_note.c`
Handles note values and turns them into frequencies.

Main routines:
- `fb_sfxNote()`
- note-to-frequency helper logic lives here too

### `sfx_rest.c`
Implements rests for sequence-style playback.

Main routines:
- `fb_sfxRest()`

### `sfx_tempo.c`
Implements tempo handling.

Main routines:
- `fb_sfxTempo()`
- `fb_sfxTempoGet()`
- `fb_sfxTempoReset()`
- `fb_sfxTempoDuration()` converts note-style lengths into real time

### `sfx_octave.c`
Implements default octave handling.

Main routines:
- `fb_sfxOctave()`
- `fb_sfxOctaveGet()`
- `fb_sfxOctaveReset()`
- helpers to shift octave up and down

### `sfx_play.c`
Parses and runs `PLAY` music strings.

Main routines:
- `fb_sfxPlayString()`
- parser helpers for notes, rests, tempo, octave, articulation, and command state
- plain `PLAY` is meant to follow QB-style behavior first
- channel `PLAY` is the non-blocking extension used for layered playback

## Music, Audio, Stream, MIDI, And SFX Command Families

### `sfx_music_load.c`
Loads and unloads music assets into runtime slots.

Main routines:
- `fb_sfxMusicLoad()`
- `fb_sfxMusicUnload()`

### `sfx_music_play.c`
Starts music playback.

Main routines:
- `fb_sfxMusicPlay()`
- `fb_sfxMusicLoop()`
- `fb_sfxMusicRestart()`

### `sfx_music_pause.c`
Pauses music.

Main routines:
- `fb_sfxMusicPause()`
- `fb_sfxMusicPauseId()`

### `sfx_music_resume.c`
Resumes music.

Main routines:
- `fb_sfxMusicResume()`
- `fb_sfxMusicResumeId()`

### `sfx_music_stop.c`
Stops music.

Main routines:
- `fb_sfxMusicStop()`
- `fb_sfxMusicStopId()`

### `sfx_music_status.c`
Reports music state.

Main routines:
- `fb_sfxMusicStatus()`
- `fb_sfxMusicPlaying()`
- `fb_sfxMusicPosition()`

### `sfx_sfx_load.c`
Loads and unloads sound-effect assets.

Main routines:
- `fb_sfxSfxLoad()`
- `fb_sfxSfxUnload()`

### `sfx_sfx_play.c`
Starts sound-effect playback.

Main routines:
- `fb_sfxSfxPlay()`
- `fb_sfxSfxPlayChannel()`
- `fb_sfxSfxLoop()`
- `fb_sfxSfxLoopChannel()`

### `sfx_sfx_pause.c`
Pauses sound effects.

Main routines:
- `fb_sfxSfxPause()`
- `fb_sfxSfxPauseChannel()`
- `fb_sfxSfxPauseAll()`

### `sfx_sfx_resume.c`
Resumes sound effects.

Main routines:
- `fb_sfxSfxResume()`
- `fb_sfxSfxResumeChannel()`
- `fb_sfxSfxResumeAll()`

### `sfx_sfx_stop.c`
Stops sound effects.

Main routines:
- `fb_sfxSfxStop()`
- `fb_sfxSfxStopChannel()`
- `fb_sfxSfxStopAll()`

### `sfx_sfx_status.c`
Reports sound-effect activity.

Main routines:
- `fb_sfxSfxStatus()`
- `fb_sfxSfxStatusChannel()`
- `fb_sfxSfxAnyActive()`

### `sfx_audio_play.c`
Starts general audio playback from a filename.

Main routines:
- `fb_sfxAudioPlay()`
- `fb_sfxAudioLoop()`

### `sfx_audio_pause.c`
Pauses the general audio player.

Main routines:
- `fb_sfxAudioPause()`

### `sfx_audio_resume.c`
Resumes the general audio player.

Main routines:
- `fb_sfxAudioResume()`

### `sfx_audio_stop.c`
Stops the general audio player.

Main routines:
- `fb_sfxAudioStop()`

### `sfx_audio_status.c`
Reports general audio player state.

Main routines:
- `fb_sfxAudioStatus()`

### `sfx_stream_open.c`
Opens a stream source.

Main routines:
- `fb_sfxStreamOpen()`

### `sfx_stream_play.c`
Starts stream playback.

Main routines:
- `fb_sfxStreamPlay()`

### `sfx_stream_pause.c`
Pauses a stream.

Main routines:
- `fb_sfxStreamPause()`

### `sfx_stream_resume.c`
Resumes a stream.

Main routines:
- `fb_sfxStreamResume()`

### `sfx_stream_seek.c`
Moves a stream to a new position.

Main routines:
- `fb_sfxStreamSeek()`

### `sfx_stream_position.c`
Reports the current stream position.

Main routines:
- `fb_sfxStreamPosition()`

### `sfx_midi_open.c`
Opens a MIDI output device.

Main routines:
- `fb_sfxMidiOpen()`

### `sfx_midi_close.c`
Closes the MIDI device.

Main routines:
- `fb_sfxMidiClose()`

### `sfx_midi_play.c`
Starts MIDI playback.

Main routines:
- `fb_sfxMidiPlay()`

### `sfx_midi_pause.c`
Pauses MIDI playback.

Main routines:
- `fb_sfxMidiPause()`

### `sfx_midi_resume.c`
Resumes MIDI playback.

Main routines:
- `fb_sfxMidiResume()`

### `sfx_midi_stop.c`
Stops MIDI playback.

Main routines:
- `fb_sfxMidiStop()`

### `sfx_midi_send.c`
Sends a raw MIDI message.

Main routines:
- `fb_sfxMidiSend()`

## Capture, Devices, And Utility Commands

### `sfx_capture_start.c`
Starts capture from the command side.

Main routines:
- `fb_sfxCaptureStartCmd()` or command-facing start helpers

### `sfx_capture_stop.c`
Stops capture from the command side.

Main routines:
- `fb_sfxCaptureStopCmd()` or command-facing stop helpers

### `sfx_capture_read.c`
Reads captured frames into a caller buffer.

Main routines:
- `fb_sfxCaptureReadSamples()`
- `fb_sfxCaptureProbe()`

### `sfx_capture_save.c`
Writes captured audio to a WAV file.

Main routines:
- `fb_sfxCaptureSave()`
- `fb_sfxFloatToPCM16()` helper for sample conversion

### `sfx_device_list.c`
Lists available audio drivers/devices.

Main routines:
- `fb_sfxDeviceCount()`
- `fb_sfxDeviceName()`
- `fb_sfxDeviceList()`
- `fb_sfxDeviceFind()`
- `fb_sfxDeviceInfo()`

### `sfx_device_info.c`
Prints or returns driver information.

Main routines:
- `fb_sfxDeviceInfo()`
- `fb_sfxDeviceInfoName()`
- `fb_sfxDeviceValid()`

### `sfx_device_select.c`
Selects and starts one driver/device entry.

Main routines:
- `fb_sfxDeviceSelect()`
- `fb_sfxDeviceCurrent()`
- `fb_sfxDeviceDriver()`
- `fb_sfxDeviceShutdown()`

### `sfx_access.c`
Small access/helper file for simple runtime lookups.

Main routines:
- helper routines that expose runtime state in a simple way

## Final Note

If you are not used to engine code, that is okay. The easiest way to work in this codebase is:

1. pick one command family
2. read the matching `sfx_*.c` file
3. follow the call into `voice`, `mixer`, or `driver` code only if you need more detail

This project is very workable one file at a time.
