/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_balance.c

    Purpose:

        Implement the BASIC BALANCE command.

        Balance adjusts the relative output level between the
        left and right stereo channels for the entire audio
        output system.

    Responsibilities:

        • manage global stereo balance state
        • enforce valid balance limits
        • provide helper functions for retrieving balance state

    This file intentionally does NOT contain:

        • mixer algorithms
        • oscillator generation
        • driver interaction
        • command parsing

    Architectural overview:

        BALANCE command
             │
             ▼
        global balance state
             │
             ▼
        output scaling stage
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Balance limits                                                            */
/* ------------------------------------------------------------------------- */

#define FB_SFX_BALANCE_LEFT   -1.0f
#define FB_SFX_BALANCE_CENTER  0.0f
#define FB_SFX_BALANCE_RIGHT   1.0f


/* ------------------------------------------------------------------------- */
/* Set balance                                                               */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxBalance()

    Set the global stereo balance.

    Parameters:

        position   stereo bias (-1.0 = left, 0 = center, 1.0 = right)
*/

void fb_sfxBalance(float position)
{
    if (!fb_sfxEnsureInitialized())
        return;

    if (position < FB_SFX_BALANCE_LEFT)
        position = FB_SFX_BALANCE_LEFT;

    if (position > FB_SFX_BALANCE_RIGHT)
        position = FB_SFX_BALANCE_RIGHT;

    fb_sfxRuntimeLock();
    __fb_sfx->balance = position;
    fb_sfxRuntimeUnlock();
    SFX_DEBUG(
        "sfx_balance: balance set to %f",
        position
    );
}


/* ------------------------------------------------------------------------- */
/* Get balance                                                               */
/* ------------------------------------------------------------------------- */

float fb_sfxBalanceGet(void)
{
    float balance;

    if (!__fb_sfx)
        return FB_SFX_BALANCE_CENTER;

    fb_sfxRuntimeLock();
    balance = __fb_sfx->balance;
    fb_sfxRuntimeUnlock();

    return balance;
}


/* ------------------------------------------------------------------------- */
/* Reset balance                                                             */
/* ------------------------------------------------------------------------- */

void fb_sfxBalanceReset(void)
{
    if (!fb_sfxEnsureInitialized())
        return;

    fb_sfxRuntimeLock();
    __fb_sfx->balance = FB_SFX_BALANCE_CENTER;
    fb_sfxRuntimeUnlock();
    SFX_DEBUG("sfx_balance: balance reset to center");
}


/* end of sfx_balance.c */
