/*
    FreeBASIC Haiku backend
    --------------------------------

    File: io_xfocus.c

    Purpose:

        Provide console focus detection for the Haiku platform.

    Responsibilities:

        • report whether the application currently has input focus
        • satisfy the focus query interface expected by the runtime

    This file intentionally does NOT contain:

        • window management
        • graphics driver logic
        • input event processing
        • rendering code

    Design notes:

        Some platforms allow the runtime to determine whether the
        console or application window currently has keyboard focus.

        The Haiku platform does not expose a simple console-focused
        API equivalent to those available on other systems.

        In practice, a running Haiku GUI application normally receives
        keyboard input only when it is active. Because of this, the
        safest behaviour for the runtime is to report that focus exists.

        This implementation therefore returns "focused" by default.

        If future Haiku APIs expose a reliable focus query mechanism,
        this implementation can be expanded accordingly.
*/

#include "haiku_debug.h"

#include <stdio.h>
#include <stdlib.h>


/* ------------------------------------------------------------------------- */
/* Console focus query                                                       */
/* ------------------------------------------------------------------------- */

/*
    fb_hConsoleHasFocus()

    This function is used by the runtime to determine whether keyboard
    input should be accepted by the application.

    Return value:

        1  -> application has focus
        0  -> application does not have focus
*/

int fb_hConsoleHasFocus(void)
{
    HAIKU_DEBUG("Console focus queried");

    /*
        Current behaviour:

        Always return focused.

        This matches the behaviour of many GUI backends where input
        routing is handled by the window system itself.
    */

    return 1;
}


/* end of io_xfocus.c */
