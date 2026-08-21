/*
    FreeBASIC gfxlib2 Windows CE XPAD backend
    -----------------------------------------

    File: wince/gfx_xpad.c

    Purpose:

        Provide the GETXPAD API without desktop XInput dependencies.

    Responsibilities:

        - export the shared controller polling entry point
        - clear every output for unavailable controllers
        - distinguish invalid indices from missing devices

    This file intentionally does NOT contain:

        - desktop XInput dynamic loading
        - Windows CE hardware-specific controller drivers
        - keyboard, mouse, or touch input mapping
*/

#include "../fb_gfx.h"

#define WINCE_XPAD_MAX_DEVICES 16

static void xpad_clear_outputs( ssize_t *buttons,
	                            float *lstick_x, float *lstick_y,
	                            float *rstick_x, float *rstick_y,
	                            float *ltrigger, float *rtrigger,
	                            ssize_t *dpad )
{
	if( buttons != NULL )
		*buttons = 0;
	if( lstick_x != NULL )
		*lstick_x = 0.0f;
	if( lstick_y != NULL )
		*lstick_y = 0.0f;
	if( rstick_x != NULL )
		*rstick_x = 0.0f;
	if( rstick_y != NULL )
		*rstick_y = 0.0f;
	if( ltrigger != NULL )
		*ltrigger = 0.0f;
	if( rtrigger != NULL )
		*rtrigger = 0.0f;
	if( dpad != NULL )
		*dpad = 0;
}

FBCALL int fb_GfxGetXPad( int id, ssize_t *buttons,
	                      float *lstick_x, float *lstick_y,
	                      float *rstick_x, float *rstick_y,
	                      float *ltrigger, float *rtrigger,
	                      ssize_t *dpad )
{
	xpad_clear_outputs( buttons, lstick_x, lstick_y,
	                    rstick_x, rstick_y,
	                    ltrigger, rtrigger, dpad );

	if( id < 0 || id >= WINCE_XPAD_MAX_DEVICES )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	fb_ErrorSetNum( FB_RTERROR_OK );
	return XPAD_STATUS_MISSING;
}

/* end of wince/gfx_xpad.c */
