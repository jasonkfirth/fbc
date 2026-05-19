/*
    FreeBASIC gfxlib2 FreeBSD joystick backend
    ------------------------------------------

    File: gfx_joystick.c

    Purpose:

        Provide GETJOYSTICK support on FreeBSD systems that expose the
        Linux joydev-compatible /dev/input/jsN interface.

    Responsibilities:

        - select the shared Unix joydev implementation for FreeBSD builds

    This file intentionally does NOT contain:

        - HID descriptor parsing
        - Xbox semantic controller mapping
        - force feedback or rumble support
*/

#include "../fb_gfx.h"
#include "../unix/gfx_joydev_common.inc"

/* end of gfx_joystick.c */
