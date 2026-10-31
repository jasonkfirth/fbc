/*
 * Project: FreeBASIC browser graphics
 * File: gfx_events.c
 * Purpose: Translate SDL events into the shared FreeBASIC event queue.
 * Responsibilities: Pointer ordering and graphics input hooks.
 * This file does not own DOM text editors or application input policy.
 */
#include "../../fb_gfx.h"
#include "fb_gfx_js.h"

/*
Comment out unused variable, see tatic void fb_js_sleep(int msecs), below.

static int sleep_called = 0;
*/

void fb_js_events_check(void)
{
    SDL_PumpEvents();

    SDL_Event event;

    while( SDL_PollEvent(&event) )
    {
        switch( event.type )
        {
            case SDL_MOUSEMOTION:
            {
                EVENT e =
                {
                    .type = EVENT_MOUSE_MOVE,
                    .x = event.motion.x,
                    .y = event.motion.y,
                    .dx = event.motion.xrel,
                    .dy = event.motion.yrel,
                };
                if( __fb_gfx->scanline_size != 1 ) {
                    e.y /= __fb_gfx->scanline_size;
                    e.dy /= __fb_gfx->scanline_size;
                } 

                fb_hPostEvent(&e);
                break;
            }

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            {
                /* SDL can synthesize a touch button without mouse motion.
                 * EVENT's button and position share a union, so consumers
                 * need a separate position before the button transition. */
                EVENT position =
                {
                    .type = EVENT_MOUSE_MOVE,
                    .x = event.button.x,
                    .y = event.button.y,
                };
                if( __fb_gfx->scanline_size != 1 )
                    position.y /= __fb_gfx->scanline_size;
                fb_hPostEvent(&position);
                EVENT e =
                {
                    .type = event.type == SDL_MOUSEBUTTONDOWN ?
                        EVENT_MOUSE_BUTTON_PRESS : EVENT_MOUSE_BUTTON_RELEASE,
                    .button = fb_js_sdl_button_to_fb_button(event.button.button),
                };

                fb_hPostEvent(&e);
                break;
            }
        }
    }
}

/*static void fb_js_sleep(int msecs)
{
    if( !sleep_called )
    {
        sleep_called = 1;
        emscripten_log(EM_LOG_WARN, "Warning: Call to SLEEP() ignored. It should not be used in Javascript");
    }
}*/

void fb_js_events_init(void)
{
	//__fb_ctx.hooks.sleepproc = fb_js_sleep;

	// we can't use SDL's key events because CAPSLOCK isn't handled by emscripten and the printable chars are always lowercase in SDL
	__fb_ctx.hooks.inkeyproc  = NULL;
	__fb_ctx.hooks.getkeyproc = NULL;
	__fb_ctx.hooks.keyhitproc = NULL;
	__fb_ctx.hooks.multikeyproc = NULL;
	__fb_ctx.hooks.posteventproc = fb_hPostEvent;

    // don't let SDL capture the keyboard
    __fb_js_ctx.doNotCaptureKeyboard = emscripten_run_script_int("Module['doNotCaptureKeyboard']? 1: 0;");
    if(!__fb_js_ctx.doNotCaptureKeyboard) {
        emscripten_run_script("Module['doNotCaptureKeyboard']=1;");
	}

    return;
}

void fb_js_events_exit(void)
{
    if(!__fb_js_ctx.doNotCaptureKeyboard)
        emscripten_run_script("Module['doNotCaptureKeyboard']=null;");
}
/* end of gfx_events.c */
