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

        GETXPAD is layered on the same Haiku device state.  BJoystick does
        not promise an Xbox-specific semantic layout, so the XPAD mapper uses
        the same common Linux joystick ordering used by many Xbox-compatible
        gamepads:

            axis 0/1  left stick
            axis 2    left trigger
            axis 3/4  right stick
            axis 5    right trigger
            axis 6/7  d-pad

        Devices that expose a different HID layout still remain available
        through GETJOYSTICK with their raw axis and button ordering.
*/

#ifndef DISABLE_HAIKU

#include "../fb_gfx.h"

#include <stdint.h>
#include <Joystick.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Constants                                                                 */
/* ------------------------------------------------------------------------- */

#define MAX_JOYSTICKS 16
#define XPAD_TRIGGER_DIGITAL_THRESHOLD (30.0f / 255.0f)


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

    Haiku's BJoystick API delivers axis samples as signed 16-bit integers.
    The polling helper widens them to 32-bit storage so the joystick and
    XPAD paths can share the same normalization code.
*/

static float normalize_axis(int value)
{
    if (value <= -32768)
        return -1.0f;

    if (value >= 32767)
        return 1.0f;

    return ((float)value) / 32767.0f;
}

static float clamp_unit(float value)
{
    if (value < 0.0f)
        return 0.0f;

    if (value > 1.0f)
        return 1.0f;

    return value;
}


/* ------------------------------------------------------------------------- */
/* Shared polling helpers                                                    */
/* ------------------------------------------------------------------------- */

static void clear_joystick_outputs(
    ssize_t *buttons,
    float *a1,float *a2,float *a3,float *a4,
    float *a5,float *a6,float *a7,float *a8
)
{
    if (buttons) *buttons = -1;

    if (a1) *a1 = -1000.0f;
    if (a2) *a2 = -1000.0f;
    if (a3) *a3 = -1000.0f;
    if (a4) *a4 = -1000.0f;
    if (a5) *a5 = -1000.0f;
    if (a6) *a6 = -1000.0f;
    if (a7) *a7 = -1000.0f;
    if (a8) *a8 = -1000.0f;
}

static void ensure_joystick_table(void)
{
    if (inited)
        return;

    fb_hMemSet(joy,0,sizeof(joy));
    inited = TRUE;
}

static int get_joystick(int id, JOYDATA **out)
{
    JOYDATA *j;

    if (out)
        *out = NULL;

    ensure_joystick_table();

    if (id < 0 || id >= MAX_JOYSTICKS)
        return FALSE;

    j = &joy[id];

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

    if (!j->available)
        return FALSE;

    if (out)
        *out = j;

    return TRUE;
}

static void read_axes(BJoystick *js, int32_t *axis_values, int max_axes)
{
    int axis_count;
    int16_t axis_values_16[8];

    memset(axis_values,0,(size_t)max_axes * sizeof(axis_values[0]));

    if (max_axes <= 0)
        return;

    axis_count = max_axes;
    if (axis_count > 8)
        axis_count = 8;

    memset(axis_values_16,0,(size_t)axis_count * sizeof(axis_values_16[0]));
    js->GetAxisValues(axis_values_16,axis_count);

    for (int i = 0; i < axis_count; i++)
    {
        axis_values[i] = (int32_t)axis_values_16[i];
    }
}

static float axis_value(const int32_t *axis_values, int axes, int axis)
{
    if (axis < 0 || axis >= axes)
        return 0.0f;

    return normalize_axis(axis_values[axis]);
}

static float trigger_value(const int32_t *axis_values, int axes, int axis)
{
    return clamp_unit((axis_value(axis_values,axes,axis) + 1.0f) * 0.5f);
}


/* ------------------------------------------------------------------------- */
/* Get joystick state                                                        */
/* ------------------------------------------------------------------------- */

extern "C" FBCALL int fb_GfxGetJoystick(
    int id,
    ssize_t *buttons,
    float *a1,float *a2,float *a3,float *a4,
    float *a5,float *a6,float *a7,float *a8
)
{
    JOYDATA *j;

    FB_GRAPHICS_LOCK();

    clear_joystick_outputs(buttons,a1,a2,a3,a4,a5,a6,a7,a8);

    if (!get_joystick(id,&j))
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

    int32_t axis_values[8];
    read_axes(&j->js,axis_values,8);

    if (axes > 0 && a1) *a1 = axis_value(axis_values,axes,0);
    if (axes > 1 && a2) *a2 = axis_value(axis_values,axes,1);
    if (axes > 2 && a3) *a3 = axis_value(axis_values,axes,2);
    if (axes > 3 && a4) *a4 = axis_value(axis_values,axes,3);
    if (axes > 4 && a5) *a5 = axis_value(axis_values,axes,4);
    if (axes > 5 && a6) *a6 = axis_value(axis_values,axes,5);
    if (axes > 6 && a7) *a7 = axis_value(axis_values,axes,6);
    if (axes > 7 && a8) *a8 = axis_value(axis_values,axes,7);


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


/* ------------------------------------------------------------------------- */
/* GETXPAD bridge                                                            */
/* ------------------------------------------------------------------------- */

static ssize_t xpad_buttons(uint32 mask, float left_trigger, float right_trigger)
{
    ssize_t buttons = 0;

    if (mask & (1u << 0))
        buttons |= XPAD_BUTTON_A;
    if (mask & (1u << 1))
        buttons |= XPAD_BUTTON_B;
    if (mask & (1u << 2))
        buttons |= XPAD_BUTTON_X;
    if (mask & (1u << 3))
        buttons |= XPAD_BUTTON_Y;
    if (mask & (1u << 4))
        buttons |= XPAD_BUTTON_L1;
    if (mask & (1u << 5))
        buttons |= XPAD_BUTTON_R1;
    if (mask & (1u << 6))
        buttons |= XPAD_BUTTON_SELECT;
    if (mask & (1u << 7))
        buttons |= XPAD_BUTTON_START;
    if (mask & (1u << 8))
        buttons |= XPAD_BUTTON_GUIDE;
    if (mask & (1u << 9))
        buttons |= XPAD_BUTTON_L3;
    if (mask & (1u << 10))
        buttons |= XPAD_BUTTON_R3;

    if (left_trigger > XPAD_TRIGGER_DIGITAL_THRESHOLD)
        buttons |= XPAD_BUTTON_L2;
    if (right_trigger > XPAD_TRIGGER_DIGITAL_THRESHOLD)
        buttons |= XPAD_BUTTON_R2;

    return buttons;
}

static ssize_t xpad_dpad(const int32_t *axis_values, int axes)
{
    ssize_t dpad = 0;
    float x;
    float y;

    x = axis_value(axis_values,axes,6);
    y = axis_value(axis_values,axes,7);

    if (x < -0.5f)
        dpad |= XPAD_DPAD_LEFT;
    else if (x > 0.5f)
        dpad |= XPAD_DPAD_RIGHT;

    if (y < -0.5f)
        dpad |= XPAD_DPAD_UP;
    else if (y > 0.5f)
        dpad |= XPAD_DPAD_DOWN;

    return dpad;
}

extern "C" int fb_hGfxHaikuGetXPad(
    int id,
    ssize_t *buttons,
    float *lstick_x,float *lstick_y,
    float *rstick_x,float *rstick_y,
    float *ltrigger,float *rtrigger,
    ssize_t *dpad
)
{
    JOYDATA *j;
    int axes;
    int32_t axis_values[8];
    float left_trigger;
    float right_trigger;
    uint32 mask;

    if (!get_joystick(id,&j))
        return XPAD_STATUS_MISSING;

    j->js.Update();

    axes = j->js.CountAxes();
    read_axes(&j->js,axis_values,8);

    left_trigger = trigger_value(axis_values,axes,2);
    right_trigger = trigger_value(axis_values,axes,5);
    mask = j->js.ButtonValues();

    if (buttons)
        *buttons = xpad_buttons(mask,left_trigger,right_trigger);
    if (lstick_x)
        *lstick_x = axis_value(axis_values,axes,0);
    if (lstick_y)
        *lstick_y = -axis_value(axis_values,axes,1);
    if (rstick_x)
        *rstick_x = axis_value(axis_values,axes,3);
    if (rstick_y)
        *rstick_y = -axis_value(axis_values,axes,4);
    if (ltrigger)
        *ltrigger = left_trigger;
    if (rtrigger)
        *rtrigger = right_trigger;
    if (dpad)
        *dpad = xpad_dpad(axis_values,axes);

    return XPAD_STATUS_CONNECTED;
}

#endif

/* end of gfx_joystick.cpp */
