/'
    FreeBASIC Windows header regression test
    ----------------------------------------

    File: win-mcn-getdaystate.bas

    Purpose:

        Verify the Vista-and-later MCN_GETDAYSTATE notification value.

    Responsibilities:

        - include the public common-controls binding on Windows targets
        - reject the obsolete pre-Vista-derived constant

    This file intentionally does NOT contain:

        - a live month-calendar window
        - message-loop or notification-dispatch behavior
'/

' TEST_MODE : COMPILE_ONLY_OK

#ifdef __FB_WIN32__
	#include once "windows.bi"
	#include once "win/commctrl.bi"

	#if MCN_GETDAYSTATE <> culng( -747 )
		#error MCN_GETDAYSTATE must match the Vista-and-later Windows SDK value
	#endif
#endif

/' end of win-mcn-getdaystate.bas '/
