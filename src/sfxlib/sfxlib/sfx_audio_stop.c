/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_audio_stop.c

    Purpose:

        Implement the AUDIO STOP command.

        This command terminates playback of the currently active
        audio stream started by AUDIO PLAY or AUDIO LOOP.

    Responsibilities:

        • stop the active audio stream
        • close any open file associated with playback
        • reset streaming state safely

    This file intentionally does NOT contain:

        • audio decoding logic
        • mixer algorithms
        • platform driver interaction

    Architectural overview:

        AUDIO STOP
              │
        streaming subsystem
              │
        mixer input removal
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#include <stdio.h>
#include <stdlib.h>


/* ------------------------------------------------------------------------- */
/* External stream state                                                     */
/* ------------------------------------------------------------------------- */

/*
    The stream playback state is maintained by sfx_audio_play.c.
*/

extern FILE *g_audio_file;
extern float *g_audio_data;
extern int   g_audio_frames;
extern int   g_audio_channels;
extern int   g_audio_samplerate;
extern int   g_audio_position;
extern int   g_audio_playing;
extern int   g_audio_paused;
extern int   g_audio_loop;


/* ------------------------------------------------------------------------- */
/* AUDIO STOP                                                                */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxAudioStop()

    Stop playback of the currently active audio stream.
*/

void fb_sfxAudioStop(void)
{
    if (!fb_sfxEnsureInitialized())
        return;

    fb_sfxRuntimeLock();
    if (!g_audio_playing)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    if (g_audio_file)
    {
        fclose(g_audio_file);
        g_audio_file = NULL;
    }

    if (g_audio_data)
    {
        free(g_audio_data);
        g_audio_data = NULL;
    }

    g_audio_frames = 0;
    g_audio_channels = 0;
    g_audio_samplerate = 0;
    g_audio_position = 0;
    g_audio_playing = 0;
    g_audio_paused = 0;
    g_audio_loop = 0;
    fb_sfxRuntimeUnlock();
}


/* ------------------------------------------------------------------------- */
/* Internal helper                                                           */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxAudioStopInternal()

    Internal stop helper used by other streaming commands.
*/

void fb_sfxAudioStopInternal(void)
{
    fb_sfxAudioStop();
}


/* end of sfx_audio_stop.c */
