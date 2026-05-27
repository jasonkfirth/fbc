/* asmjs fbgfx driver */

#include "../fb_gfx.h"
#include "fb_gfx_js.h"
#include "../fb_gfx_gl.h"

JS_GFXDRIVER_CTX __fb_js_ctx =
{
    .inited = FALSE,
    .changingScreen = FALSE,
    .updated = FALSE,
    .blitting = FALSE,
    .canvas = NULL,
    .doNotCaptureKeyboard = 0,
};

static void driver_exit(void);

static int driver_has_dirty_lines(void)
{
	int y;

	if( __fb_gfx == NULL || __fb_gfx->dirty == NULL )
		return FALSE;

	for( y = 0; y < __fb_gfx->h; y++ )
	{
		if( __fb_gfx->dirty[y] )
			return TRUE;
	}

	return FALSE;
}

static int js_has_browser_window(void)
{
	return EM_ASM_INT({
		return (typeof window !== 'undefined') &&
		       (typeof document !== 'undefined');
	});
}

static void driver_blit()
{
	int copied = FALSE;

	if( !driver_has_dirty_lines() )
		return;

    if(SDL_LockSurface(__fb_js_ctx.canvas) == 0)
    {
        __fb_js_ctx.blit(__fb_js_ctx.canvas->pixels, __fb_js_ctx.canvas->pitch);

        SDL_UnlockSurface(__fb_js_ctx.canvas);
		copied = TRUE;
    }

	SDL_Flip(__fb_js_ctx.canvas);
	if( copied )
		fb_hMemSet(__fb_gfx->dirty, FALSE, __fb_gfx->h);

}

static void driver_update(void *unused)
{
	if( !__fb_js_ctx.inited || __fb_gfx == NULL || __fb_gfx->framebuffer == NULL )
		return;

    int ini_time = SDL_GetTicks();

	if( !__fb_js_ctx.changingScreen && !__fb_js_ctx.blitting )
    {
        __fb_js_ctx.blitting = TRUE;
        driver_blit();
        __fb_js_ctx.blitting = FALSE;

        __fb_js_ctx.updated = TRUE;

        fb_js_events_check( );
	}

	int delay = (1000/GFX_JS_FPS) - (SDL_GetTicks() - ini_time);

	emscripten_async_call(driver_update, NULL, MAX( delay, 1 ) );
}


static int driver_init(char *title, int w, int h, int depth_arg, int refresh_rate, int flags)
{
	if( w == 0 || h == 0 || depth_arg == 0 )
		return 0;

	if (flags & DRIVER_OPENGL)
		return -1;

	if( !js_has_browser_window() )
		return -1;

	__fb_js_ctx.changingScreen = TRUE;

	if( !__fb_js_ctx.inited )
    {
        fb_js_events_init();
        SDL_Init(SDL_INIT_VIDEO);
    }

	if( __fb_js_ctx.canvas != NULL )
		SDL_FreeSurface(__fb_js_ctx.canvas);

	__fb_js_ctx.canvas = SDL_SetVideoMode(w, h, 32, SDL_HWSURFACE);

	__fb_js_ctx.blit = fb_hGetBlitter(__fb_js_ctx.canvas->format->BitsPerPixel, TRUE);

	__fb_js_ctx.changingScreen = FALSE;
	__fb_js_ctx.blitting = FALSE;

	int was_inited = __fb_js_ctx.inited;
	__fb_js_ctx.inited = TRUE;

	if( !was_inited )
	{
		__fb_js_ctx.updated = 0;
		emscripten_async_call(driver_update, NULL, 1000/GFX_JS_FPS);
	}

	return 0;
}

static int WGL_init(char *title, int w, int h, int depth_arg, int refresh_rate, int flags)
{
	if( w == 0 || h == 0 || depth_arg == 0 )
		return 0;

	if (!(flags & DRIVER_OPENGL))
		return -1;

	if( !js_has_browser_window() )
		return -1;

	__fb_js_ctx.changingScreen = TRUE;

	if( !__fb_js_ctx.inited )
    {
        fb_js_events_init();
        SDL_Init(SDL_INIT_VIDEO);
    }

	if( __fb_js_ctx.canvas != NULL )
		SDL_FreeSurface(__fb_js_ctx.canvas);

	fb_hGL_NormalizeParameters(flags);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, __fb_gl_params.color_red_bits); 
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, __fb_gl_params.color_green_bits);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, __fb_gl_params.color_blue_bits);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, __fb_gl_params.color_alpha_bits);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, __fb_gl_params.depth_bits);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); 
	__fb_js_ctx.canvas = SDL_SetVideoMode(w, h, 32, SDL_DOUBLEBUF | SDL_DOUBLEBUF | SDL_OPENGL);

	__fb_js_ctx.changingScreen = FALSE;
	__fb_js_ctx.blitting = FALSE;

	int was_inited = __fb_js_ctx.inited;
	__fb_js_ctx.inited = TRUE;

	if( !was_inited )
	{
		__fb_js_ctx.updated = 0;
		//emscripten_async_call(driver_update, NULL, 1000/GFX_JS_FPS);
	}

	return 0;
}


static void driver_exit(void)
{
	if( __fb_js_ctx.inited )
	{
		__fb_js_ctx.inited = FALSE;

		if( !__fb_js_ctx.updated )
			driver_blit();

		if( __fb_js_ctx.canvas != NULL )
		{
			SDL_FreeSurface(__fb_js_ctx.canvas);
			__fb_js_ctx.canvas = NULL;
		}

		fb_js_events_exit();

		SDL_Quit();
	}
}

static void WGL_exit(void)
{
	if( __fb_js_ctx.inited )
	{
		__fb_js_ctx.inited = FALSE;

		if( __fb_js_ctx.canvas != NULL )
		{
			SDL_FreeSurface(__fb_js_ctx.canvas);
			__fb_js_ctx.canvas = NULL;
		}

		fb_js_events_exit();

		SDL_Quit();
	}
}

static void driver_lock(void)
{
	__fb_js_ctx.blitting = TRUE;
}

static void driver_unlock(void)
{
	if( __fb_js_ctx.inited && __fb_js_ctx.canvas != NULL && __fb_gfx != NULL && __fb_gfx->framebuffer != NULL )
	{
		driver_blit();
		__fb_js_ctx.updated = TRUE;
	}

	__fb_js_ctx.blitting = FALSE;
}

static void driver_wait_vsync(void)
{
	/* The browser owns real presentation timing; use the driver's frame cadence. */
	fb_Delay(1000/GFX_JS_FPS);
}

static void WGL_Flip(void)
{
	SDL_GL_SwapBuffers();
}

int fb_js_sdl_buttons_to_fb_buttons(int sdl_buttons)
{
	int fb_buttons = 0;

	if( sdl_buttons & SDL_BUTTON_LMASK)
        fb_buttons |= BUTTON_LEFT;
	if( sdl_buttons & SDL_BUTTON_MMASK)
        fb_buttons |= BUTTON_MIDDLE;
	if( sdl_buttons & SDL_BUTTON_RMASK)
        fb_buttons |= BUTTON_RIGHT;
	if( sdl_buttons & SDL_BUTTON_X1MASK)
        fb_buttons |= BUTTON_X1;
	if( sdl_buttons & SDL_BUTTON_X2MASK)
        fb_buttons |= BUTTON_X2;

    return fb_buttons;
}

int fb_js_sdl_button_to_fb_button(int sdl_button)
{
	switch( sdl_button )
	{
		case SDL_BUTTON_LEFT:   return BUTTON_LEFT;
		case SDL_BUTTON_MIDDLE: return BUTTON_MIDDLE;
		case SDL_BUTTON_RIGHT:  return BUTTON_RIGHT;
		case SDL_BUTTON_X1:     return BUTTON_X1;
		case SDL_BUTTON_X2:     return BUTTON_X2;
	}

	return 0;
}

static int driver_get_mouse(int *x, int *y, int *z, int *buttons, int *clip)
{
	int touch_x;
	int touch_y;
	int touch_buttons;
	SDL_PumpEvents();

	uint32_t state = SDL_GetMouseState(x, y);
	if (buttons) *buttons = fb_js_sdl_buttons_to_fb_buttons(state);

	if (fb_hJsGetTouchMouse(&touch_x, &touch_y, &touch_buttons)) {
		if (x) *x = touch_x;
		if (y) *y = touch_y;
		if (buttons) *buttons = touch_buttons;
	}

	if (z) *z = 0;
	if (clip) *clip = 0;

	return 0;
}

static void driver_set_mouse(int x, int y, int cursor, int clip)
{
	if( x >= 0 && y >= 0 )
		SDL_WarpMouse(x, y);

	if( cursor >= 0 )
		SDL_ShowCursor(cursor ? SDL_ENABLE : SDL_DISABLE);

	/* SDL 1.2 under Emscripten does not expose cursor clipping. */
	(void)clip;
}

static int modes[] = {
    SCREENLIST(640, 480),
    SCREENLIST(512, 512),
    SCREENLIST(320, 240),
    SCREENLIST(320, 200),
    SCREENLIST(320, 100),
    SCREENLIST(256, 256),
    SCREENLIST(160, 120),
    SCREENLIST(80, 80)
};

static int *driver_fetch_modes(int depth, int *size)
{
	*size = sizeof(modes) / sizeof(int);
	return memcpy((void*)malloc(sizeof(modes)), modes, sizeof(modes));
}

static void driver_poll_events(void)
{
	fb_js_events_check( );
}

static void driver_set_window_title(char *title)
{
    SDL_WM_SetCaption(title, NULL);
}

/* GFXDRIVER */
static const GFXDRIVER fb_gfxDriverJS =
{
	"asmjs",                 /* char *name; */
	driver_init,             /* int (*init)(char *title, int w, int h, int depth, int refresh_rate, int flags); */
	driver_exit,             /* void (*exit)(void); */
	driver_lock,             /* void (*lock)(void); */
	driver_unlock,           /* void (*unlock)(void); */
	NULL,                    /* void (*set_palette)(int index, int r, int g, int b); */
	driver_wait_vsync,       /* void (*wait_vsync)(void); */
	driver_get_mouse,        /* int (*get_mouse)(int *x, int *y, int *z, int *buttons, int *clip); */
	fb_hJsGetTouchCount,     /* int (*get_touch_count)(void); */
	fb_hJsGetTouch,          /* int (*get_touch)(int index, int *x, int *y, int *id); */
	driver_set_mouse,        /* void (*set_mouse)(int x, int y, int cursor, int clip); */
	driver_set_window_title, /* void (*set_window_title)(char *title); */
	NULL,                    /* int (*set_window_pos)(int x, int y); */
	driver_fetch_modes,      /* int *(*fetch_modes)(int depth, int *size); */
	NULL,                    /* void (*flip)(void); */
	driver_poll_events,      /* void (*poll_events)(void); */
	NULL                     /* void (*update)(void); */
};

/* GFXDRIVER */
static const GFXDRIVER fb_gfxWebGL =
{
	"WebGL",                 /* char *name; */
	WGL_init,                /* int (*init)(char *title, int w, int h, int depth, int refresh_rate, int flags); */
	WGL_exit,                /* void (*exit)(void); */
	driver_lock,             /* void (*lock)(void); */
	driver_unlock,           /* void (*unlock)(void); */
	NULL,                    /* void (*set_palette)(int index, int r, int g, int b); */
	driver_wait_vsync,       /* void (*wait_vsync)(void); */
	driver_get_mouse,        /* int (*get_mouse)(int *x, int *y, int *z, int *buttons, int *clip); */
	fb_hJsGetTouchCount,     /* int (*get_touch_count)(void); */
	fb_hJsGetTouch,          /* int (*get_touch)(int index, int *x, int *y, int *id); */
	driver_set_mouse,        /* void (*set_mouse)(int x, int y, int cursor, int clip); */
	driver_set_window_title, /* void (*set_window_title)(char *title); */
	NULL,                    /* int (*set_window_pos)(int x, int y); */
	driver_fetch_modes,      /* int *(*fetch_modes)(int depth, int *size); */
	WGL_Flip,                /* void (*flip)(void); */
	driver_poll_events,      /* void (*poll_events)(void); */
	NULL                     /* void (*update)(void); */
};

const GFXDRIVER *__fb_gfx_drivers_list[] = {
	&fb_gfxDriverJS,
	&fb_gfxWebGL,
	NULL
};

void fb_hScreenInfo(ssize_t *width, ssize_t *height, ssize_t *depth, ssize_t *refresh)
{
	*width = 512;
	*height = 512;
	*depth = 32;
	*refresh = GFX_JS_FPS;
}

FBCALL int fb_GfxGetJoystick(int id, ssize_t *buttons, float *a1, float *a2, float *a3, float *a4, float *a5, float *a6, float *a7, float *a8)
{
	ssize_t xpad_buttons;
	ssize_t dpad;
	float lx, ly, rx, ry, lt, rt;
	int status;

	if (buttons) *buttons = -1;
	if (a1) *a1 = -1000.0f;
	if (a2) *a2 = -1000.0f;
	if (a3) *a3 = -1000.0f;
	if (a4) *a4 = -1000.0f;
	if (a5) *a5 = -1000.0f;
	if (a6) *a6 = -1000.0f;
	if (a7) *a7 = -1000.0f;
	if (a8) *a8 = -1000.0f;

	if ((id < 0) || (id >= 4))
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);

	status = fb_GfxGetXPad(id, &xpad_buttons, &lx, &ly, &rx, &ry, &lt, &rt, &dpad);
	if (status != XPAD_STATUS_CONNECTED)
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);

	if (buttons) *buttons = xpad_buttons;
	if (a1) *a1 = lx;
	if (a2) *a2 = ly;
	if (a3) *a3 = lt;
	if (a4) *a4 = rx;
	if (a5) *a5 = ry;
	if (a6) *a6 = rt;
	if (a7) *a7 = (dpad & XPAD_DPAD_RIGHT) ? 1.0f : ((dpad & XPAD_DPAD_LEFT) ? -1.0f : 0.0f);
	if (a8) *a8 = (dpad & XPAD_DPAD_DOWN) ? 1.0f : ((dpad & XPAD_DPAD_UP) ? -1.0f : 0.0f);

	return fb_ErrorSetNum(FB_RTERROR_OK);
}
