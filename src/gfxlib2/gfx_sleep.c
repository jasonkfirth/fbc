/* sleep function */

#include "fb_gfx.h"

int fb_hGfxInputBufferChanged( void );

static void poll_events(void)
{
	FB_GRAPHICS_LOCK( );

	if ((__fb_gfx) && (__fb_gfx->driver->poll_events))
		__fb_gfx->driver->poll_events();

	FB_GRAPHICS_UNLOCK( );
}

#ifndef HOST_JS
static void update_display(void)
{
	if ((__fb_gfx) && (__fb_gfx->driver->update))
		__fb_gfx->driver->update();
}
#endif

void fb_GfxSleep ( int msecs )
{
	/* infinite? wait until any key is pressed */
	if( msecs == -1 )
	{
		while( !fb_hGfxInputBufferChanged( ) ) {
			poll_events( );
			fb_Delay( 50 );
		}
		return;
	}

	/* if above n-mili-seconds, check for key input, otherwise,
	   don't screw the precision with slow console checks */
	if( msecs >= 100 )
		while( msecs > 50 )
		{
			poll_events( );

			if( fb_hGfxInputBufferChanged( ) )
				return;

			fb_Delay( 50 );
			msecs -= 50;
		}

	if( msecs == 0 )
	{
		poll_events( );
		/*
			The JavaScript driver uses its update hook as a timer callback that
			schedules the next browser update.  Calling it from every SLEEP 0
			would create a growing number of update timers instead of a single
			presentation request.
		*/
#ifndef HOST_JS
		update_display( );
#endif
	}

	if( msecs >= 0 )
		fb_Delay( msecs );
}
