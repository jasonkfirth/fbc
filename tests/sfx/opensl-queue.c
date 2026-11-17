/*
    Project: FreeBASIC sound library
    File: opensl-queue.c
    Purpose: Exercise production OpenSL buffer ownership without audio hardware.
    Responsibilities: Model retained PCM pointers, FIFO completion and enqueue
        failures using the NDK's real OpenSL interface declarations.
    This file intentionally does NOT contain synthesis or an Android emulator.
*/
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* The host compiler does not implement Android availability attributes. */
#define SL_API_DEPRECATED(level)
#include "../../src/sfxlib/android/sfx_driver_opensl.c"

/* The fake engine retains pointers just as OpenSL does; it does not copy PCM. */
static const short *pending[FB_SFX_OPENSL_BUFFERS];
static short expected[FB_SFX_OPENSL_BUFFERS][8];
static int pending_count;
static int reject_next;
static int complete_in_enqueue;

static void verify_pending(void)
{
    int i;
    for (i = 0; i < pending_count; ++i)
        assert(memcmp(pending[i], expected[i], sizeof(expected[i])) == 0);
}

static void complete_one(void)
{
    int i;
    assert(pending_count > 0);
    verify_pending();
    for (i = 1; i < pending_count; ++i) {
        pending[i - 1] = pending[i];
        memcpy(expected[i - 1], expected[i], sizeof(expected[i]));
    }
    --pending_count;
    opensl_callback(NULL, NULL);
}

static SLresult fake_enqueue(SLAndroidSimpleBufferQueueItf self,
                            const void *data, SLuint32 bytes)
{
    (void)self;
    verify_pending();
    if (reject_next) {
        reject_next = 0;
        return SL_RESULT_RESOURCE_ERROR;
    }
    assert(pending_count < FB_SFX_OPENSL_BUFFERS);
    assert(bytes == sizeof(expected[0]));
    pending[pending_count] = data;
    memcpy(expected[pending_count], data, bytes);
    ++pending_count;
    /* Completion may beat the producer's return from Enqueue. */
    if (complete_in_enqueue)
        complete_one();
    return SL_RESULT_SUCCESS;
}

static const struct SLAndroidSimpleBufferQueueItf_ fake_queue = {
    .Enqueue = fake_enqueue
};
static const struct SLAndroidSimpleBufferQueueItf_ *fake_queue_ptr = &fake_queue;

/* Only external boundaries are stubbed; reservation/write/callback are real. */
const SLInterfaceID SL_IID_ENGINE = NULL;
const SLInterfaceID SL_IID_PLAY = NULL;
const SLInterfaceID SL_IID_ANDROIDSIMPLEBUFFERQUEUE = NULL;
SLresult slCreateEngine(SLObjectItf *object, SLuint32 count,
    const SLEngineOption *options, SLuint32 interfaces,
    const SLInterfaceID *ids, const SLboolean *required)
{
    (void)object; (void)count; (void)options;
    (void)interfaces; (void)ids; (void)required;
    return SL_RESULT_RESOURCE_ERROR;
}
int fb_hAndroidSfxIsRunning(void) { return 1; }
int fb_hAndroidSfxInWorker(void) { return 0; }
int fb_hAndroidSfxActivate(int rate, int channels, int frames)
{
    (void)rate; (void)channels; (void)frames;
    return 0;
}
void fb_hAndroidSfxDeactivate(void) {}
void fb_sfxDriverDiagnostics(const char *name, const float *pcm, int n, int ch)
{
    (void)name; (void)pcm; (void)n; (void)ch;
}
void fb_sfxConvertFloatToS16(const float *src, short *dst, int count)
{
    int i;
    for (i = 0; i < count; ++i)
        dst[i] = (short)(src[i] * 1000);
}

int main(void)
{
    int iteration;
    int i;
    float pcm[8];
    assert(allocate_buffers(4, 2) == 0);
    initialized = 1;
    queue = &fake_queue_ptr;

    for (iteration = 0; iteration < 10000; ++iteration) {
        for (i = 0; i < 8; ++i)
            pcm[i] = (float)((iteration % 20) + i) / 100;
        if (pending_count == FB_SFX_OPENSL_BUFFERS) {
            assert(opensl_write(pcm, 4) == 0);
            complete_one();
        }
        if (iteration % 17 == 0) {
            reject_next = 1;
            assert(opensl_write(pcm, 4) == -1);
            verify_pending();
        }
        complete_in_enqueue = iteration % 11 == 0;
        assert(opensl_write(pcm, 4) == 4);
        verify_pending();
        if (iteration % 7 == 0 && pending_count > 0)
            complete_one();
    }
    while (pending_count > 0)
        complete_one();
    /* A final shutdown callback must not disturb the next initialization. */
    shutting_down = 1;
    pthread_mutex_lock(&buffer_mutex);
    release_all_buffers_locked();
    pthread_mutex_unlock(&buffer_mutex);
    opensl_callback(NULL, NULL);
    for (i = 0; i < FB_SFX_OPENSL_BUFFERS; ++i)
        assert(buffers[i].in_use == 0);
    free_buffers();
    assert(allocate_buffers(4, 2) == 0);
    shutting_down = 0;
    complete_in_enqueue = 0;
    assert(opensl_write(pcm, 4) == 4);
    assert(pending[0] == buffers[0].samples);
    complete_one();
    free_buffers();
    puts("OPENSL_QUEUE_OWNERSHIP=PASS (10000 submissions, failures, early callbacks, restart)");
    return 0;
}

/* end of opensl-queue.c */
