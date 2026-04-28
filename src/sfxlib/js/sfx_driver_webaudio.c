/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    JavaScript/WebAudio output driver for Emscripten.
*/

#include "../fb_sfx.h"
#include "../fb_sfx_driver.h"

#include <stddef.h>
#include <emscripten.h>

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

EM_JS(int, fb_sfx_js_webaudio_write, (const float *samples, int frames, int channels), {
    var state = Module.__fbSfxWebAudio;
    if (!state || !state.context || !state.processor)
        return -1;

    channels = state.channels | 0;
    frames = frames | 0;
    if (frames <= 0)
        return 0;

    var count = frames * channels;
    var start = samples >> 2;
    var block = new Float32Array(count);
    block.set(HEAPF32.subarray(start, start + count));

    state.queue.push({
        samples: block,
        frames: frames
    });

    while (state.queue.length > 64) {
        state.queue.shift();
        state.offset = 0;
    }

    if (state.context.state === 'suspended' && state.context.resume)
        state.context.resume().catch(function() {});

    return frames;
});

static int webaudio_driver_init(int rate, int channels, int buffer_frames, int flags)
{
    (void)flags;
    return fb_sfx_js_webaudio_init(rate, channels, buffer_frames);
}

static void webaudio_driver_exit(void)
{
    fb_sfx_js_webaudio_exit();
}

static int webaudio_driver_write(const float *samples, int frames)
{
    return fb_sfx_js_webaudio_write(samples, frames, FB_SFX_INTERNAL_CHANNELS);
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
