#include "../fb_sfx_internal.h"

int fb_sfxDecodeFile(const char *filename,
                     float **samples,
                     int *frames,
                     int *channels,
                     int *sample_rate)
{
	(void)filename;

	if (samples)
		*samples = NULL;
	if (frames)
		*frames = 0;
	if (channels)
		*channels = 0;
	if (sample_rate)
		*sample_rate = 0;

	return -1;
}
