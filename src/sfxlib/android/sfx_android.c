#include "../fb_sfx_driver.h"
#include "fb_sfx_android.h"

#include <stddef.h>

extern const FB_SFX_DRIVER __fb_sfxDriverNull;

const FB_SFX_DRIVER *__fb_sfx_drivers_list[] =
{
	&fb_sfxDriverAAudio,
	&fb_sfxDriverOpenSLES,
	&__fb_sfxDriverNull,
	NULL
};
