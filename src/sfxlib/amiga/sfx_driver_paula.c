/*
    FreeBASIC sound library for classic AmigaOS
    ------------------------------------------

    File: sfx_driver_paula.c

    Purpose:
        Stream mixed sound through Paula when AHI is not installed.

    Responsibilities:
        - allocate one left/right pair through audio.device
        - resample to a supported DMA period and convert to signed eight-bit PCM
        - keep DMA buffers in chip memory and wait for both channel writes
        - stop the shared sound worker before releasing device resources

    This file intentionally does NOT contain:
        - mixer policy, direct hardware register writes, or audio capture

    Paula DMA uses a minimum period of 124 colour clocks. Its clock is
    3546895 Hz on PAL and 3579545 Hz on NTSC. Keep BASIC's requested mixer rate
    and reduce submitted frames using a carried rational phase accumulator.
    Writes use a caller-owned reply port, including calls from the feeder task.
*/

#include "../fb_sfx_driver.h"
#include "../fb_sfx_internal.h"
#include "fb_sfx_amiga.h"

#include <devices/audio.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <graphics/gfxbase.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

static struct MsgPort *paula_port;
static struct IOAudio *paula_allocation;
static signed char *paula_left, *paula_right;
static ULONG paula_capacity, paula_clock;
static ULONG paula_left_mask, paula_right_mask;
static uint64_t paula_phase, paula_step;
static UWORD paula_period;
static int paula_open, paula_channels;
static unsigned int paula_debug_writes;
static int paula_pending;
static signed char paula_pending_left, paula_pending_right;

static int paula_submit(ULONG produced);

static void paula_exit(void)
{
    fb_sfxAmigaWorkerStop();
    /* DMA lengths count words. Preserve an odd final sample, padding its last
       word with silence only when the stream actually ends. */
    if (paula_open && paula_pending) {
        paula_left[0] = paula_pending_left;
        paula_right[0] = paula_pending_right;
        paula_left[1] = paula_right[1] = 0;
        paula_submit(2);
    }
    if (paula_open) CloseDevice(&paula_allocation->ioa_Request);
    if (paula_allocation != NULL) DeleteIORequest(&paula_allocation->ioa_Request);
    if (paula_port != NULL) DeleteMsgPort(paula_port);
    if (paula_left != NULL) FreeMem(paula_left, paula_capacity);
    if (paula_right != NULL) FreeMem(paula_right, paula_capacity);
    paula_open = FALSE;
    paula_allocation = NULL; paula_port = NULL;
    paula_left = paula_right = NULL;
    paula_capacity = 0; paula_phase = 0;
    paula_debug_writes = 0;
    paula_pending = FALSE;
}

static int paula_init(int rate, int channels, int buffer_frames, int flags)
{
    static UBYTE choices[] = {3, 5, 10, 12};
    struct GfxBase *graphics;
    ULONG mask, period;
    (void)flags;
    if (paula_open) return 0;
    if (rate <= 0) rate = FB_SFX_DEFAULT_RATE;
    if (channels != 1 && channels != 2) return -1;
    graphics = (struct GfxBase *)OpenLibrary("graphics.library", 37);
    if (graphics == NULL) return -1;
    paula_clock = graphics->DisplayFlags & PAL ? 3546895UL : 3579545UL;
    CloseLibrary((struct Library *)graphics);
    period = paula_clock / (ULONG)rate;
    if (paula_clock % (ULONG)rate != 0) ++period;
    if (period > 65535UL) return -1;
    if (period < 124UL) period = 124UL;
    paula_period = (UWORD)period;
    paula_step = (uint64_t)(unsigned int)rate * period;
    paula_channels = channels;
    paula_port = CreateMsgPort();
    if (paula_port == NULL) goto fail;
    paula_allocation = (struct IOAudio *)CreateIORequest(paula_port, sizeof(struct IOAudio));
    if (paula_allocation == NULL) goto fail;
    paula_allocation->ioa_Request.io_Message.mn_Node.ln_Pri = 0;
    paula_allocation->ioa_Request.io_Flags = ADIOF_NOWAIT;
    paula_allocation->ioa_Data = choices;
    paula_allocation->ioa_Length = sizeof(choices);
    if (OpenDevice(AUDIONAME, 0, &paula_allocation->ioa_Request, 0) != 0) goto fail;
    paula_open = TRUE;
    mask = (ULONG)(uintptr_t)paula_allocation->ioa_Request.io_Unit;
    paula_left_mask = mask & 1 ? 1 : mask & 8;
    paula_right_mask = mask & 2 ? 2 : mask & 4;
    if (paula_left_mask == 0 || paula_right_mask == 0) goto fail;
    if (fb_sfxAmigaWorkerStart(buffer_frames) != 0) goto fail;
    return 0;
fail:
    paula_exit();
    return -1;
}

static signed char paula_sample(float sample)
{
    /* Comparisons also catch infinity; unordered NaNs become silence before
       any float-to-integer conversion is attempted. */
    if (!(sample >= -1.0f && sample <= 1.0f)) {
        if (sample > 1.0f) sample = 1.0f;
        else if (sample < -1.0f) sample = -1.0f;
        else sample = 0.0f;
    }
    return (signed char)(int)(sample * 127.0f);
}

static int paula_write(const float *samples, int frames)
{
    ULONG produced = 0;
    ULONG capacity;
    int frame;
    if (!paula_open || samples == NULL || frames <= 0 || frames > INT_MAX / paula_channels)
        return -1;
    /* A prior odd write contributes at most one extra byte per channel. */
    capacity = (ULONG)frames + 1;
    if (capacity > paula_capacity) {
        signed char *left = AllocMem(capacity, MEMF_CHIP);
        signed char *right = AllocMem(capacity, MEMF_CHIP);
        if (left == NULL || right == NULL) {
            if (left != NULL) FreeMem(left, capacity);
            if (right != NULL) FreeMem(right, capacity);
            return -1;
        }
        if (paula_left != NULL) FreeMem(paula_left, paula_capacity);
        if (paula_right != NULL) FreeMem(paula_right, paula_capacity);
        paula_left = left; paula_right = right; paula_capacity = capacity;
    }
    if (paula_pending) {
        paula_left[0] = paula_pending_left;
        paula_right[0] = paula_pending_right;
        produced = 1;
    }
    for (frame = 0; frame < frames; ++frame) {
        paula_phase += paula_clock;
        if (paula_phase < paula_step) continue;
        paula_phase -= paula_step;
        paula_left[produced] = paula_sample(samples[frame * paula_channels]);
        paula_right[produced] = paula_sample(samples[frame * paula_channels + (paula_channels == 2)]);
        ++produced;
    }
    paula_pending = (produced & 1) != 0;
    if (paula_pending) {
        --produced;
        paula_pending_left = paula_left[produced];
        paula_pending_right = paula_right[produced];
    }
    if (produced == 0) return frames;
    if (paula_debug_writes < 16)
        SFX_DEBUG("Paula write %u: task=%p input=%d output=%lu", paula_debug_writes,
            FindTask(NULL), frames, (unsigned long)produced);
    return paula_submit(produced) == 0 ? frames : -1;
}

static int paula_submit(ULONG produced)
{
    struct IOAudio requests[2];
    struct MsgPort *reply;
    ULONG offset = 0;
    int channel;
    int result = 0;

    reply = CreateMsgPort();
    if (reply == NULL) return -1;
    while (offset < produced) {
        ULONG length = produced - offset;
        LONG left_error, right_error;
        /* AUDxLEN is a 16-bit word count. Keep each request even and within
           65535 words, including callers with unusually large mix buffers. */
        if (length > 131070UL) length = 131070UL;
        for (channel = 0; channel < 2; ++channel) {
            memcpy(&requests[channel], paula_allocation, sizeof(struct IOAudio));
            requests[channel].ioa_Request.io_Message.mn_ReplyPort = reply;
            requests[channel].ioa_Request.io_Unit = (struct Unit *)(uintptr_t)(channel == 0 ? paula_left_mask : paula_right_mask);
            requests[channel].ioa_Request.io_Command = CMD_WRITE;
            requests[channel].ioa_Request.io_Flags = ADIOF_PERVOL;
            requests[channel].ioa_Request.io_Error = 0;
            requests[channel].ioa_Data = (UBYTE *)(channel == 0 ? paula_left : paula_right) + offset;
            requests[channel].ioa_Length = length;
            requests[channel].ioa_Period = paula_period;
            requests[channel].ioa_Volume = 64; /* Paula's full volume */
            requests[channel].ioa_Cycles = 1;
            SendIO(&requests[channel].ioa_Request);
        }
        left_error = WaitIO(&requests[0].ioa_Request);
        right_error = WaitIO(&requests[1].ioa_Request);
        if (paula_debug_writes < 16) {
            SFX_DEBUG("Paula write %u completed: left=%ld right=%ld", paula_debug_writes,
                (long)left_error, (long)right_error);
            ++paula_debug_writes;
        }
        if (left_error != 0 || right_error != 0) { result = -1; break; }
        offset += length;
    }
    DeleteMsgPort(reply);
    return result;
}

static int paula_device_list(void) { return 1; }

const FB_SFX_DRIVER fb_sfxDriverAmigaPaula = {
    "Amiga Paula", FB_SFX_DRIVER_CAP_BACKGROUND | FB_SFX_DRIVER_CAP_BLOCKING,
    paula_init, paula_exit, paula_write, NULL, NULL, paula_device_list, NULL
};

/* end of sfx_driver_paula.c */
