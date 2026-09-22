/'
    FreeBASIC compiler regression test
    ----------------------------------

    File: gas64-array-index.bas

    Purpose:

        Exercise gas64 address arithmetic for a local two-dimensional array.
        The first index is materialized before multiplication when the array
        element is emitted, so the virtual-register mapping must survive that
        move.

    Responsibilities:

        - cover the materialized multiply path in the gas64 emitter
        - keep the source valid on other targets by compiling the body only
          where gas64 is available

    This file intentionally does NOT contain:

        - assumptions about a particular graphics or runtime library
        - a run-time result check, because the test is compile-only
'/

' TEST_MODE : COMPILE_ONLY_OK

#if defined( __FB_X86__ ) and defined( __FB_64BIT__ ) and not defined( __FB_DARWIN__ )
	#cmdline "-gen gas64"

sub write_array( )
	 dim as integer values( 0 to 11, 0 to 30 )
	 dim as integer row, column

	 for row = 0 to 11
		 for column = 0 to 30
			 values( row, column ) = row * 31 + column
		 next column
	 next row
end sub

write_array( )
#endif

/' end of gas64-array-index.bas '/
