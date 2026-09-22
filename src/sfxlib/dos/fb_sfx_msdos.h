/* FreeBASIC Sound Library: dos/fb_sfx_msdos.h
 * Private DOS driver configuration and interrupt-consumer interfaces.
 * Hardware handling and mixing are implemented in their respective modules.
 */

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

#if defined(ENABLE_MT) && defined(FB_DOS_PDMLWP)
int fb_sfxMsdosIrqInit(int port, int irq);
void fb_sfxMsdosIrqExit(void);
void fb_sfxMsdosIrqBegin(void);
int fb_sfxMsdosIrqDone(void);
unsigned int fb_sfxMsdosIrqCount(void);
int fb_sfxMsdosPcSpeakerIrqInit(void);
void fb_sfxMsdosPcSpeakerIrqExit(void);
int fb_sfxMsdosPcSpeakerIrqActive(void);
unsigned int fb_sfxMsdosPcSpeakerSamples(void);
unsigned int fb_sfxMsdosPcSpeakerUnderruns(void);
int fb_sfxMsdosPcSpeakerIrqWrite(const float *samples, int frames, int channels, int drain);
#endif

#endif

/* end of fb_sfx_msdos.h */
