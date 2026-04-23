/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: fb_sfx_internal.h

    Purpose:

        Internal declarations used by the FreeBASIC sound subsystem.

        This header provides shared internal APIs used by the various
        sfxlib modules.  It exposes mixer helpers, buffer management,
        voice management, and debugging infrastructure.

    Responsibilities:

        • declare internal helper functions
        • define internal constants
        • provide subsystem utility macros
        • expose internal mixer and buffer interfaces

    This file intentionally does NOT contain:

        • the public runtime API
        • platform driver implementations
        • command layer logic

        Public definitions are located in:

            fb_sfx.h
*/

#ifndef __FB_SFX_INTERNAL_H__
#define __FB_SFX_INTERNAL_H__

#include "fb_sfx.h"
#include "fb_sfx_driver.h"
#include "fb_sfx_buffer.h"
#include "fb_sfx_capture.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


/* ------------------------------------------------------------------------- */
/* Debug system                                                              */
/* ------------------------------------------------------------------------- */

/*
    Debug logging

    Debug output is centralized through these macros so that it can be
    enabled or disabled globally without modifying individual modules.

    The implementation resides in sfx_debug.c.
*/

void fb_sfxDebugInit(void);
int  fb_sfxDebugEnabled(void);
void fb_sfxDebugLog(const char *fmt, ...);


/*
    Debug macro used throughout the subsystem.

    Using a macro avoids unnecessary function calls when debugging
    is disabled.
*/

#define SFX_DEBUG(...) \
    do { if (fb_sfxDebugEnabled()) fb_sfxDebugLog(__VA_ARGS__); } while (0)



/* ------------------------------------------------------------------------- */
/* Runtime state helpers                                                     */
/* ------------------------------------------------------------------------- */

/*
    Runtime sanity check

    Many functions assume that the sound subsystem has been
    successfully initialized.  This helper prevents accidental
    use of the system before initialization.
*/

int fb_sfxEnsureInitialized(void);
int fb_sfxEnsureInit(void);

/*
    Internal threading policy

    sfxlib uses worker threads for subsystems such as MIDI playback.
    That requirement is independent from whether the BASIC program
    itself was built with FreeBASIC's public multithreading switch.

    Therefore the sound runtime enables its own internal locking and
    worker-thread support on platforms that provide a native threading
    API.  Only targets that genuinely lack such support stay single-
    threaded here.
*/

#if defined(_WIN32) || \
    defined(__linux__) || \
    defined(__ANDROID__) || \
    defined(__CYGWIN__) || \
    defined(__APPLE__) || \
    defined(__FreeBSD__) || \
    defined(__NetBSD__) || \
    defined(__OpenBSD__) || \
    defined(__DragonFly__) || \
    defined(__sun) || \
    defined(__HAIKU__)
#define FB_SFX_MT_ENABLED 1
#else
#define FB_SFX_MT_ENABLED 0
#endif

void fb_sfxRuntimeLockInit(void);
void fb_sfxRuntimeLockShutdown(void);
void fb_sfxRuntimeLock(void);
void fb_sfxRuntimeUnlock(void);
const FB_SFX_DRIVER *fb_sfxDriverDetachLocked(const FB_SFX_DRIVER *expected_driver);
void fb_sfxDriverExitUnlocked(const FB_SFX_DRIVER *driver);



/* ------------------------------------------------------------------------- */
/* Mixer subsystem                                                           */
/* ------------------------------------------------------------------------- */

/*
    Mixer architecture

    The mixer is responsible for combining all active voices
    into a single output buffer.

    Drivers do not generate audio directly.

    Instead they request audio frames from the mixer and send
    the resulting samples to the operating system audio API.
*/

void fb_sfxMixerInit(void);
void fb_sfxMixerShutdown(void);
void fb_sfxMixerProcess(int frames);
void fb_sfxMixerStopChannel(int channel);
void fb_sfxMixerStopAll(void);


/*
    Voice management
*/

FB_SFXVOICE *fb_sfxVoiceAlloc(void);
void fb_sfxVoiceFree(FB_SFXVOICE *voice);

void fb_sfxVoiceStopChannel(int channel);



/* ------------------------------------------------------------------------- */
/* Channel helpers                                                           */
/* ------------------------------------------------------------------------- */

FB_SFXCHANNEL *fb_sfxChannelGet(int channel);
void fb_sfxChannelInit(void);
void fb_sfxChannelCmd(int channel);
int fb_sfxChannelCmdGet(void);
void fb_sfxChannelCmdReset(void);
int fb_sfxResolveChannel(int channel);
void fb_sfxChannelSetVolume(int channel, float volume);
float fb_sfxChannelGetVolume(int channel);
void fb_sfxChannelSetPan(int channel, float pan);
float fb_sfxChannelGetPan(int channel);
void fb_sfxChannelMute(int channel, int state);
int fb_sfxChannelMuted(int channel);
void fb_sfxChannelSetInstrument(int channel, int instrument);
int fb_sfxChannelGetInstrument(int channel);
void fb_sfxChannelReset(int channel);
void fb_sfxChannelResetAll(void);
void fb_sfxVolume(float level);
float fb_sfxVolumeGet(void);
void fb_sfxVolumeChannel(int channel, float level);
float fb_sfxVolumeChannelGet(int channel);
void fb_sfxVolumeReset(void);
void fb_sfxBalance(float position);
float fb_sfxBalanceGet(void);
void fb_sfxBalanceReset(void);
void fb_sfxPan(int channel, float position);
float fb_sfxPanGet(int channel);
void fb_sfxPanReset(void);



/* ------------------------------------------------------------------------- */
/* Buffer utilities                                                          */
/* ------------------------------------------------------------------------- */

/*
    Buffer architecture

    The mixer writes audio frames into a ring buffer owned
    by the runtime.

    Drivers consume frames from this buffer and deliver them
    to the platform audio system.
*/

int  fb_sfxBufferInit(int frames);
void fb_sfxBufferShutdown(void);
void fb_sfxBufferClear(void);
float *fb_sfxBufferPtr(void);
int  fb_sfxBufferFrames(void);
int  fb_sfxBufferSamples(void);
void fb_sfxBufferWrite(int index, float value);
float fb_sfxBufferRead(int index);



/* ------------------------------------------------------------------------- */
/* Ring buffer helpers                                                       */
/* ------------------------------------------------------------------------- */

int fb_sfxRingBufferAvailable(const FB_SFX_RINGBUFFER *rb);
int fb_sfxRingBufferFree(const FB_SFX_RINGBUFFER *rb);



/* ------------------------------------------------------------------------- */
/* Capture subsystem                                                         */
/* ------------------------------------------------------------------------- */

/*
    Audio capture

    The capture subsystem allows recording audio from:

        • microphone input
        • line input
        • system mixer (loopback)

    Captured samples are stored in a runtime-owned buffer so
    that BASIC programs can read them deterministically.
*/

void fb_sfxCaptureInit(void);
void fb_sfxCaptureShutdown(void);

int fb_sfxCaptureWrite(const short *samples, int frames);
int fb_sfxCaptureRead(float *samples, int frames);



/* ------------------------------------------------------------------------- */
/* Driver selection                                                          */
/* ------------------------------------------------------------------------- */

/*
    Driver initialization

    During runtime initialization the library attempts to
    initialize drivers from the driver list until one
    successfully starts.
*/

int fb_sfxDriverInit(void);
void fb_sfxDriverShutdown(void);



/* ------------------------------------------------------------------------- */
/* Utility helpers                                                           */
/* ------------------------------------------------------------------------- */

/*
    Safe memory allocation helpers.

    These wrappers provide consistent error handling for
    memory allocation across the library.
*/

void *fb_sfxAlloc(size_t size);
void  fb_sfxFree(void *ptr);



/* ------------------------------------------------------------------------- */
/* Sample conversion helpers                                                 */
/* ------------------------------------------------------------------------- */

/*
    Drivers often require specific sample formats.

    These helpers convert the internal mixer format into
    platform-friendly formats.
*/

void fb_sfxConvertFloatToS16(
    const float *src,
    short *dst,
    int frames
);

void fb_sfxConvertFloatToS32(
    const float *src,
    int *dst,
    int frames
);



/* ------------------------------------------------------------------------- */
/* Safety helpers                                                            */
/* ------------------------------------------------------------------------- */

/*
    Clamp sample value to valid audio range.

    Internal mixer samples should remain within [-1.0, 1.0].
*/

float fb_sfxClampSample(float v);
float fb_sfxOscillatorSample(FB_SFXVOICE *voice);
float fb_sfxWaveformSample(FB_SFXVOICE *voice);

void fb_sfxVoiceInit(FB_SFXVOICE *voice);
void fb_sfxVoiceSetWaveform(FB_SFXVOICE *voice, int waveform);
void fb_sfxVoiceSetFrequency(FB_SFXVOICE *voice, int freq);
void fb_sfxVoiceSetEnvelope(FB_SFXVOICE *voice, int id);
void fb_sfxVoiceStopType(int voice_type);
void fb_sfxVoiceStopTypeChannel(int voice_type, int channel);
void fb_sfxVoicePauseType(int voice_type);
void fb_sfxVoicePauseTypeChannel(int voice_type, int channel);
void fb_sfxVoiceResumeType(int voice_type);
void fb_sfxVoiceResumeTypeChannel(int voice_type, int channel);
int fb_sfxVoiceStatusType(int voice_type);
int fb_sfxVoiceStatusTypeChannel(int voice_type, int channel);
void fb_sfxEnvelopeInit(void);
void fb_sfxEnvelopeDefine(int id, float attack, float decay, float sustain, float release);
void fb_sfxEnvelopeRelease(FB_SFXVOICE *voice);
float fb_sfxEnvelopeProcess(FB_SFXVOICE *voice, float dt);
void fb_sfxWaveCmd(int id, int waveform_type);
int fb_sfxWaveDefined(int id);
void fb_sfxWaveCmdReset(void);
void fb_sfxEnvelopeCmd(int id, float attack, float decay, float sustain, float release);
int fb_sfxEnvelopeDefined(int id);
void fb_sfxEnvelopeCmdReset(void);
void fb_sfxInstrumentDefine(int id, int wave_id, int env_id);
void fb_sfxInstrumentAssign(int channel, int instrument_id);
int fb_sfxInstrumentDefined(int id);
void fb_sfxInstrumentReset(void);
void fb_sfxInstrumentApply(FB_SFXVOICE *voice, int channel, int default_wave, int default_env);
void fb_sfxMixBufferInit(void);
void fb_sfxMixBufferShutdown(void);
int fb_sfxMixBufferWrite(const float *samples, int frames);
int fb_sfxMixBufferRead(float *samples, int frames);
int fb_sfxDecodeFile(const char *filename,
                     float **samples,
                     int *frames,
                     int *channels,
                     int *sample_rate);
int fb_sfxAllocContext(void);
void fb_sfxFreeContext(void);
int fb_sfxInitCore(void);
void fb_sfxExitCore(void);
void fb_sfxUpdate(int frames);
void fb_sfxBeep(void);
void fb_sfxBeepEx(int frequency, float duration);
void fb_sfxSound(int frequency, float duration);
void fb_sfxSoundChannel(int channel, int frequency, float duration, float volume);
void fb_sfxSoundStop(void);
void fb_sfxSoundStopChannel(int channel);
void fb_sfxTone(int channel, int frequency, float duration);
void fb_sfxToneStop(int channel);
void fb_sfxToneStopAll(void);
void fb_sfxNoise(int channel, float duration, float volume);
void fb_sfxNoiseStop(int channel);
void fb_sfxNoiseStopAll(void);
void fb_sfxNote(const char *note, int octave, float duration);
void fb_sfxNoteChannel(int channel, const char *note, int octave, float duration);
void fb_sfxPlay(const char *music);
void fb_sfxPlayChannel(int channel, const char *music);
void fb_sfxPlayStop(void);
void fb_sfxPlayPause(void);
void fb_sfxPlayResume(void);
int fb_sfxPlayStatus(void);
void fb_sfxTempo(int bpm);
int fb_sfxTempoGet(void);
void fb_sfxOctave(int octave);
int fb_sfxOctaveGet(void);
void fb_sfxVoice(int instrument);
int fb_sfxVoiceGet(void);
void fb_sfxRest(float duration);
void fb_sfxRestChannel(int channel, float duration);
int fb_sfxMusicLoad(const char *filename);
void fb_sfxMusicUnload(int id);
void fb_sfxMusicPlay(int id);
int fb_sfxMusicPlayCmd(int id);
int fb_sfxMusicPlayFile(const char *filename);
void fb_sfxMusicLoop(int id);
int fb_sfxMusicLoopCmd(int id);
int fb_sfxMusicLoopFile(const char *filename);
void fb_sfxMusicPause(void);
void fb_sfxMusicPauseId(int id);
void fb_sfxMusicResume(void);
void fb_sfxMusicResumeId(int id);
void fb_sfxMusicStop(void);
void fb_sfxMusicStopId(int id);
int fb_sfxMusicStatus(void);
int fb_sfxMusicCurrent(void);
long fb_sfxMusicPosition(void);
void fb_sfxSfxLoad(int id, const char *filename);
void fb_sfxSfxUnload(int id);
void fb_sfxSfxPlay(int id);
void fb_sfxSfxPlayChannel(int channel, int id);
void fb_sfxSfxLoop(int id);
void fb_sfxSfxLoopChannel(int channel, int id);
void fb_sfxSfxStop(int id);
void fb_sfxSfxStopChannel(int channel);
void fb_sfxSfxStopAll(void);
void fb_sfxSfxPause(int id);
void fb_sfxSfxPauseChannel(int channel);
void fb_sfxSfxPauseAll(void);
void fb_sfxSfxResume(int id);
void fb_sfxSfxResumeChannel(int channel);
void fb_sfxSfxResumeAll(void);
int fb_sfxSfxStatus(int id);
int fb_sfxSfxStatusChannel(int channel);
int fb_sfxSfxAnyActive(void);
int fb_sfxAudioPlay(const char *filename);
int fb_sfxAudioLoop(const char *filename);
int fb_sfxAudioFeed(float *buffer, int frames);
void fb_sfxAudioPause(void);
void fb_sfxAudioResume(void);
void fb_sfxAudioStop(void);
int fb_sfxAudioStatus(void);
int fb_sfxAudioIsPlaying(void);
int fb_sfxAudioIsPaused(void);
int fb_sfxStreamOpen(const char *filename);
int fb_sfxStreamPlay(void);
void fb_sfxStreamPause(void);
void fb_sfxStreamResume(void);
void fb_sfxStreamStop(void);
void fb_sfxStreamStopInternal(void);
int fb_sfxStreamFeed(float *buffer, int frames);
int fb_sfxStreamSeek(long position);
long fb_sfxStreamPosition(void);
int fb_sfxStreamRewind(void);
int fb_sfxStreamIsOpen(void);
int fb_sfxStreamPlaying(void);
int fb_sfxStreamPaused(void);
int fb_sfxMidiOpen(int device);
int fb_sfxMidiClose(void);
int fb_sfxMidiPlay(const char *filename);
int fb_sfxMidiStop(void);
int fb_sfxMidiPause(void);
int fb_sfxMidiResume(void);
int fb_sfxMidiIsOpen(void);
int fb_sfxMidiPlaying(void);
int fb_sfxMidiPaused(void);
void fb_sfxMidiPauseReset(void);
void fb_sfxMidiStopInternal(void);
void fb_sfxMidiJoinWorker(void);
int fb_sfxMidiDriverOpen(int device);
void fb_sfxMidiDriverClose(void);
int fb_sfxMidiDriverSend(unsigned char status,
                         unsigned char data1,
                         unsigned char data2);
int fb_sfxMidiSend(unsigned char status,
                   unsigned char data1,
                   unsigned char data2);
int fb_sfxCaptureStart(void);
void fb_sfxCaptureStop(void);
void fb_sfxCapturePause(void);
void fb_sfxCaptureResume(void);
int fb_sfxCaptureStatus(void);
int fb_sfxCaptureAvailable(void);
int fb_sfxCaptureReadSamples(float *buffer, int frames);
int fb_sfxCaptureSave(const char *filename, int seconds);
int fb_sfxCaptureSaveCmd(const char *filename);
int fb_sfxDeviceCount(void);
const char *fb_sfxDeviceName(int id);
void fb_sfxDeviceList(void);
void fb_sfxDeviceInfo(int id);
void fb_sfxDeviceInfoCurrent(void);
int fb_sfxDeviceSelect(int id);
int fb_sfxDeviceCurrent(void);
int fb_sfxVoiceActiveCount(void);
int fb_sfxPlatformCaptureStart(void);
void fb_sfxPlatformCaptureStop(void);
int fb_sfxPlatformCaptureRead(float *buffer, int frames);



#ifdef __cplusplus
}
#endif

#endif

/* end of fb_sfx_internal.h */
