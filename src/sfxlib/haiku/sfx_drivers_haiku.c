#include <stddef.h>
#include "../fb_sfx_driver.h"

extern const FB_SFX_DRIVER fb_sfxDriverHaiku;

const FB_SFX_DRIVER* __fb_sfx_drivers_list[] = {
    &fb_sfxDriverHaiku,
    NULL
};
