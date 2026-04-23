#include <string.h>

#include "fb_sfx.h"
#include "fb_sfx_buffer.h"
#include "fb_sfx_capture.h"

#if defined(__GNUC__)
#define SFX_WEAK __attribute__((weak))
#else
#define SFX_WEAK
#endif

static int fb_sfxCaptureStoredSamples(const FB_SFXCAPTURE *cap)
{
    if (cap->write_pos >= cap->read_pos)
        return cap->write_pos - cap->read_pos;

    return FB_SFX_CAPTURE_BUFFER - (cap->read_pos - cap->write_pos);
}

static int fb_sfxCaptureFreeSamples(const FB_SFXCAPTURE *cap)
{
    return (FB_SFX_CAPTURE_BUFFER - 1) - fb_sfxCaptureStoredSamples(cap);
}

void fb_sfxCaptureInit(void)
{
    if (!__fb_sfx)
        return;

    memset(&__fb_sfx->capture, 0, sizeof(__fb_sfx->capture));
    __fb_sfx->capture.rate = __fb_sfx->samplerate > 0 ? __fb_sfx->samplerate : FB_SFX_DEFAULT_RATE;
    __fb_sfx->capture.channels = __fb_sfx->output_channels > 0 ? __fb_sfx->output_channels : FB_SFX_DEFAULT_CHANNELS;
}

void fb_sfxCaptureShutdown(void)
{
    if (!__fb_sfx)
        return;

    memset(&__fb_sfx->capture, 0, sizeof(__fb_sfx->capture));
}

void fb_sfxCapturePause(void)
{
    if (!__fb_sfx)
        return;

    __fb_sfx->capture.enabled = FB_SFX_CAPTURE_PAUSED;
}

void fb_sfxCaptureResume(void)
{
    if (!__fb_sfx)
        return;

    __fb_sfx->capture.enabled = FB_SFX_CAPTURE_RUNNING;
}

int fb_sfxCaptureStatus(void)
{
    if (!__fb_sfx)
        return FB_SFX_CAPTURE_STOPPED;

    return __fb_sfx->capture.enabled;
}

void fb_sfxCaptureBufferInit(void)
{
    fb_sfxCaptureInit();
}

void fb_sfxCaptureBufferShutdown(void)
{
    fb_sfxCaptureShutdown();
}

int fb_sfxCaptureBufferWrite(const short *samples, int frames)
{
    FB_SFXCAPTURE *cap;
    int channels;
    int total_samples;
    int writable_samples;
    int frames_to_write;
    int i;

    if (!__fb_sfx || !samples || frames <= 0)
        return 0;

    cap = &__fb_sfx->capture;
    channels = cap->channels > 0 ? cap->channels : FB_SFX_DEFAULT_CHANNELS;
    total_samples = frames * channels;
    writable_samples = fb_sfxCaptureFreeSamples(cap);

    if (writable_samples <= 0)
        return 0;

    if (total_samples > writable_samples)
        total_samples = writable_samples - (writable_samples % channels);

    frames_to_write = total_samples / channels;

    for (i = 0; i < total_samples; ++i) {
        cap->buffer[cap->write_pos] = samples[i];
        cap->write_pos = (cap->write_pos + 1) % FB_SFX_CAPTURE_BUFFER;
    }

    return frames_to_write;
}

int fb_sfxCaptureBufferRead(short *samples, int frames)
{
    FB_SFXCAPTURE *cap;
    int channels;
    int total_samples;
    int readable_samples;
    int frames_to_read;
    int i;

    if (!__fb_sfx || !samples || frames <= 0)
        return 0;

    cap = &__fb_sfx->capture;
    channels = cap->channels > 0 ? cap->channels : FB_SFX_DEFAULT_CHANNELS;
    total_samples = frames * channels;
    readable_samples = fb_sfxCaptureStoredSamples(cap);

    if (readable_samples <= 0)
        return 0;

    if (total_samples > readable_samples)
        total_samples = readable_samples - (readable_samples % channels);

    frames_to_read = total_samples / channels;

    for (i = 0; i < total_samples; ++i) {
        samples[i] = cap->buffer[cap->read_pos];
        cap->read_pos = (cap->read_pos + 1) % FB_SFX_CAPTURE_BUFFER;
    }

    return frames_to_read;
}

int fb_sfxCaptureWrite(const short *samples, int frames)
{
    return fb_sfxCaptureBufferWrite(samples, frames);
}

int fb_sfxCaptureRead(float *samples, int frames)
{
    FB_SFXCAPTURE *cap;
    int channels;
    int total_samples;
    int readable_samples;
    int frames_to_read;
    int i;

    if (!__fb_sfx || !samples || frames <= 0)
        return 0;

    cap = &__fb_sfx->capture;
    channels = cap->channels > 0 ? cap->channels : FB_SFX_DEFAULT_CHANNELS;
    total_samples = frames * channels;
    readable_samples = fb_sfxCaptureStoredSamples(cap);

    if (readable_samples <= 0)
        return 0;

    if (total_samples > readable_samples)
        total_samples = readable_samples - (readable_samples % channels);

    frames_to_read = total_samples / channels;

    for (i = 0; i < total_samples; ++i) {
        samples[i] = (float)cap->buffer[cap->read_pos] / 32768.0f;
        cap->read_pos = (cap->read_pos + 1) % FB_SFX_CAPTURE_BUFFER;
    }

    return frames_to_read;
}

void fb_sfxCaptureBufferClear(void)
{
    if (!__fb_sfx)
        return;

    __fb_sfx->capture.read_pos = 0;
    __fb_sfx->capture.write_pos = 0;
    memset(__fb_sfx->capture.buffer, 0, sizeof(__fb_sfx->capture.buffer));
}

int fb_sfxCaptureAvailable(void)
{
    FB_SFXCAPTURE *cap;
    int channels;

    if (!__fb_sfx)
        return 0;

    cap = &__fb_sfx->capture;
    channels = cap->channels > 0 ? cap->channels : FB_SFX_DEFAULT_CHANNELS;

    return fb_sfxCaptureStoredSamples(cap) / channels;
}

int SFX_WEAK fb_sfxPlatformCaptureStart(void)
{
    return 0;
}

void SFX_WEAK fb_sfxPlatformCaptureStop(void)
{
}

int SFX_WEAK fb_sfxPlatformCaptureRead(float *buffer, int frames)
{
    (void)buffer;
    (void)frames;
    return 0;
}
