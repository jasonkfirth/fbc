/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    JavaScript/WebAudio output driver for Emscripten.
*/

#include "../fb_sfx.h"
#include "../fb_sfx_driver.h"
#include "../fb_sfx_internal.h"

#include <stddef.h>
#include <emscripten.h>

static int g_webaudio_worker_running = 0;
static int g_webaudio_worker_buffer_frames = FB_SFX_DEFAULT_BUFFER;

static int webaudio_worker_frames(void)
{
    int frames;

    frames = (g_webaudio_worker_buffer_frames > 0)
        ? (g_webaudio_worker_buffer_frames / 4)
        : (FB_SFX_DEFAULT_BUFFER / 4);

    if (frames < 256)
        frames = 256;
    else if (frames > 2048)
        frames = 2048;

    return frames;
}

static void webaudio_worker_tick(void *unused)
{
    (void)unused;

    if (!g_webaudio_worker_running)
        return;

    /*
        WebAudio pulls from a JavaScript queue, but the common sfxlib mixer is
        still C code.  Native backends keep that mixer moving from an audio
        worker thread.  JavaScript is single-threaded here, so use an
        Emscripten async callback to provide the same background pump for
        ordinary BASIC programs that only call SOUND.
    */

    if (!fb_sfxForegroundFeedActive())
        fb_sfxUpdate(webaudio_worker_frames());

    if (g_webaudio_worker_running)
        emscripten_async_call(webaudio_worker_tick, NULL, 5);
}

static void webaudio_worker_start(int buffer_frames)
{
    if (buffer_frames <= 0)
        buffer_frames = FB_SFX_DEFAULT_BUFFER;

    g_webaudio_worker_buffer_frames = buffer_frames;

    if (g_webaudio_worker_running)
        return;

    g_webaudio_worker_running = 1;
    emscripten_async_call(webaudio_worker_tick, NULL, 1);
}

static void webaudio_worker_stop(void)
{
    g_webaudio_worker_running = 0;
}

EM_JS(int, fb_sfx_js_webaudio_init, (int rate, int channels, int buffer_frames), {
    if (typeof window === 'undefined')
        return -1;

    var AudioContext = window.AudioContext || window.webkitAudioContext;
    if (!AudioContext)
        return -1;

    channels = Math.max(1, Math.min(channels | 0, 2));

    var size = buffer_frames | 0;
    if (size < 256)
        size = 256;
    if (size > 16384)
        size = 16384;
    size = 1 << Math.round(Math.log(size) / Math.log(2));

    try {
        var state = Module.__fbSfxWebAudio;
        if (state && state.context)
            return 0;

        var context = new AudioContext({ sampleRate: rate | 0 });
        var processor = context.createScriptProcessor(size, 0, channels);

        state = {
            context: context,
            processor: processor,
            channels: channels,
            streamEpoch: 0,
            queue: [],
            offset: 0
        };

        processor.onaudioprocess = function(event) {
            var outputs = [];
            for (var ch = 0; ch < channels; ++ch)
                outputs[ch] = event.outputBuffer.getChannelData(ch);

            var frames = event.outputBuffer.length;
            for (var i = 0; i < frames; ++i) {
                while (state.queue.length && state.offset >= state.queue[0].frames) {
                    state.queue.shift();
                    state.offset = 0;
                }

                if (!state.queue.length) {
                    for (var silentCh = 0; silentCh < channels; ++silentCh)
                        outputs[silentCh][i] = 0.0;
                    continue;
                }

                var block = state.queue[0];
                var base = state.offset * channels;
                for (var outCh = 0; outCh < channels; ++outCh)
                    outputs[outCh][i] = block.samples[base + outCh] || 0.0;
                state.offset++;
            }
        };

        processor.connect(context.destination);
        Module.__fbSfxWebAudio = state;

        if (context.state === 'suspended' && context.resume)
            context.resume().catch(function() {});

        return 0;
    } catch (e) {
        return -1;
    }
});

EM_JS(void, fb_sfx_js_webaudio_exit, (void), {
    var state = Module.__fbSfxWebAudio;
    if (!state)
        return;

    try {
        if (state.processor)
            state.processor.disconnect();
        if (state.context)
            state.context.close();
    } catch (e) {
    }

    Module.__fbSfxWebAudio = null;
});

EM_JS(int, fb_sfx_js_webaudio_write,
      (const float *samples, int frames, int channels, unsigned long stream_epoch), {
    var state = Module.__fbSfxWebAudio;
    if (!state || !state.context || !state.processor)
        return -1;

    channels = state.channels | 0;
    frames = frames | 0;
    stream_epoch = stream_epoch >>> 0;
    if (frames <= 0)
        return 0;

    /*
        RawOpen and RawClose establish hard stream boundaries. C-side ring
        buffers are cleared at those boundaries, but blocks already handed to
        WebAudio live in JavaScript and must be discarded separately.
    */
    if ((state.streamEpoch >>> 0) !== stream_epoch) {
        state.queue.length = 0;
        state.offset = 0;
        state.streamEpoch = stream_epoch;
    }

    var count = frames * channels;
    var start = samples >> 2;
    var block = new Float32Array(count);
    block.set(HEAPF32.subarray(start, start + count));

    state.queue.push({
        samples: block,
        frames: frames
    });

    /*
        ScriptProcessor consumes one callback block at a time. Keeping the
        active block plus two future blocks tolerates ordinary browser jitter
        while bounding command-to-sound latency. The old 64-block limit could
        retain roughly three seconds at 44.1 kHz.

        Preserve queue[0] because state.offset belongs to it. When a producer
        gets too far ahead, discard the oldest not-yet-started block instead.
        Real-time game audio should stay current rather than play obsolete
        engine or interface state seconds later.
    */
    while (state.queue.length > 3) {
        state.queue.splice(1, 1);
    }

    if (state.context.state === 'suspended' && state.context.resume)
        state.context.resume().catch(function() {});

    return frames;
});

static int webaudio_driver_init(int rate, int channels, int buffer_frames, int flags)
{
    int result;

    (void)flags;

    result = fb_sfx_js_webaudio_init(rate, channels, buffer_frames);
    if (result == 0)
        webaudio_worker_start(buffer_frames);

    return result;
}

static void webaudio_driver_exit(void)
{
    webaudio_worker_stop();
    fb_sfx_js_webaudio_exit();
}

static int webaudio_driver_write(const float *samples, int frames)
{
    return fb_sfx_js_webaudio_write(samples,
                                    frames,
                                    FB_SFX_INTERNAL_CHANNELS,
                                    fb_sfxOutputStreamEpoch());
}

const FB_SFX_DRIVER fb_sfxDriverWebAudio =
{
    "webaudio",
    0,
    webaudio_driver_init,
    webaudio_driver_exit,
    webaudio_driver_write,
    NULL,
    NULL,
    NULL,
    NULL
};

/* end of sfx_driver_webaudio.c */
