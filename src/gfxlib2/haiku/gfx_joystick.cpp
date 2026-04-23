/*
    FreeBASIC gfxlib2 Haiku backend
    --------------------------------

    File: gfx_joystick.cpp

    Purpose:

        Provide joystick support for the Haiku graphics backend.

    Responsibilities:

        • detect available joystick devices
        • read joystick axis values
        • read joystick button states
        • normalize axis values into the range [-1,1]

    This file intentionally does NOT contain:

        • window management
        • event processing
        • rendering logic
        • platform initialization

    Notes:

        Haiku provides joystick support through the BJoystick class.

        Each joystick device exposes:

            • multiple axes
            • a bitmask of button states

        The FreeBASIC graphics API expects a polling interface that
        returns button state and up to eight axes.
*/

#ifndef DISABLE_HAIKU

#include "../fb_gfx.h"

#include <Joystick.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Constants                                                                 */
/* ------------------------------------------------------------------------- */

#define MAX_JOYSTICKS 16


/* ------------------------------------------------------------------------- */
/* Joystick state                                                            */
/* ------------------------------------------------------------------------- */

/*
    Each entry stores detection state and an active BJoystick instance.

    detected
        prevents repeated hardware enumeration

    available
        indicates whether the device successfully opened
*/

typedef struct
{
    int detected;
    int available;
    BJoystick js;

} JOYDATA;


/* ------------------------------------------------------------------------- */
/* Internal state                                                            */
/* ------------------------------------------------------------------------- */

static JOYDATA joy[MAX_JOYSTICKS];
static int inited = FALSE;


/* ------------------------------------------------------------------------- */
/* Axis normalization helper                                                 */
/* ------------------------------------------------------------------------- */

/*
    Convert raw axis value to the range [-1.0, 1.0].

    Haiku joystick axis values are signed 16-bit integers.
*/

static float normalize_axis(int value)
{
    return ((float)value) / 32767.0f;
}


/* ------------------------------------------------------------------------- */
/* Get joystick state                                                        */
/* ------------------------------------------------------------------------- */

FBCALL int fb_GfxGetJoystick(
    int id,
    ssize_t *buttons,
    float *a1,float *a2,float *a3,float *a4,
    float *a5,float *a6,float *a7,float *a8
)
{
    JOYDATA *j;

    FB_GRAPHICS_LOCK();

    /*
        Initialize outputs to safe default values.
    */

    if (buttons) *buttons = -1;

    if (a1) *a1 = -1000.0f;
    if (a2) *a2 = -1000.0f;
    if (a3) *a3 = -1000.0f;
    if (a4) *a4 = -1000.0f;
    if (a5) *a5 = -1000.0f;
    if (a6) *a6 = -1000.0f;
    if (a7) *a7 = -1000.0f;
    if (a8) *a8 = -1000.0f;

    /*
        Initialize joystick table on first use.
    */

    if (!inited)
    {
        fb_hMemSet(joy,0,sizeof(joy));
        inited = TRUE;
    }

    /*
        Validate requested joystick id.
    */

    if (id < 0 || id >= MAX_JOYSTICKS)
    {
        FB_GRAPHICS_UNLOCK();
        return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
    }

    j = &joy[id];


    /* --------------------------------------------------------------------- */
    /* Device detection                                                      */
    /* --------------------------------------------------------------------- */

    if (!j->detected)
    {
        j->detected = TRUE;

        BJoystick tmp;

        int count = tmp.CountDevices();

        if (id < count)
        {
            char name[B_OS_NAME_LENGTH];

            if (tmp.GetDeviceName(id,name) == B_OK)
            {
                if (j->js.Open(name) == B_OK)
                    j->available = TRUE;
            }
        }
    }


    /* --------------------------------------------------------------------- */
    /* Device availability check                                             */
    /* --------------------------------------------------------------------- */

    if (!j->available)
    {
        FB_GRAPHICS_UNLOCK();
        return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
    }


    /* --------------------------------------------------------------------- */
    /* Update device state                                                   */
    /* --------------------------------------------------------------------- */

    j->js.Update();


    /* --------------------------------------------------------------------- */
    /* Read axes                                                             */
    /* --------------------------------------------------------------------- */

    int axes = j->js.CountAxes();

    int16 axis_values[8];
    memset(axis_values,0,sizeof(axis_values));

    j->js.GetAxisValues(axis_values,8);

    if (axes > 0 && a1) *a1 = normalize_axis(axis_values[0]);
    if (axes > 1 && a2) *a2 = normalize_axis(axis_values[1]);
    if (axes > 2 && a3) *a3 = normalize_axis(axis_values[2]);
    if (axes > 3 && a4) *a4 = normalize_axis(axis_values[3]);
    if (axes > 4 && a5) *a5 = normalize_axis(axis_values[4]);
    if (axes > 5 && a6) *a6 = normalize_axis(axis_values[5]);
    if (axes > 6 && a7) *a7 = normalize_axis(axis_values[6]);
    if (axes > 7 && a8) *a8 = normalize_axis(axis_values[7]);


    /* --------------------------------------------------------------------- */
    /* Read buttons                                                          */
    /* --------------------------------------------------------------------- */

    if (buttons)
    {
        uint32 mask = j->js.ButtonValues();
        *buttons = (ssize_t)mask;
    }


    FB_GRAPHICS_UNLOCK();

    return fb_ErrorSetNum(FB_RTERROR_OK);
}

#endif

/* end of gfx_joystick.cpp */
