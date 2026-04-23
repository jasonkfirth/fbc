#ifndef FB_SFX_MSDOS_H
#define FB_SFX_MSDOS_H

#include "../fb_sfx.h"

typedef struct FB_SFX_MSDOS_CONFIG
{
    int base_port;
    int irq;
    int dma8;
    int dma16;
    int mpu_port;
    int card_type;
    int have_blaster;
    int have_base_port;
    int have_irq;
    int have_dma8;
    int have_dma16;
    int have_mpu_port;
    int valid;
} FB_SFX_MSDOS_CONFIG;

int fb_sfxMsdosParseBlaster(FB_SFX_MSDOS_CONFIG *config);

#endif
