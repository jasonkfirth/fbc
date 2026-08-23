/'
    Project: FreeBASIC runtime test suite
    -------------------------------------

    File: file/com-control.bas

    Purpose:

        Verify the hardware-independent fbcom.bi ABI and validation paths.

    Responsibilities:

        - check the fixed status record layout
        - reject invalid, screen, and ordinary file handles
        - reject malformed line and queue masks before reaching a backend
        - verify that failed status queries clear caller-owned output

    This file intentionally does NOT contain:

        - assumptions about installed serial hardware
        - platform-specific device names
        - modem-line changes on a real port

    Dialect and profile assumptions:

        The suite runs in the default FB dialect and links against the runtime
        under test. No COM port is opened, so the tests are deterministic on
        build hosts, virtual machines, and cross-target emulators.

    Targets:

        Every target that runs the normal fbcunit file-test shard.

    Module API boundary:

        This test module registers only the fbc_tests.file_.com_control suite.
        It exports no production runtime interface.
'/

#include once "fbcunit.bi"
#include once "fbcom.bi"
#include once "fberror.bi"

using FB

SUITE( fbc_tests.file_.com_control )

	TEST( public_layout )
		CU_ASSERT_EQUAL( sizeof( ComStatus ), 5 * sizeof( ulong ) )
		CU_ASSERT_EQUAL( COM_LINE_RTS or COM_LINE_DTR, &h30ul )
		CU_ASSERT_EQUAL( COM_PURGE_RX or COM_PURGE_TX, &h03ul )
	END_TEST

	TEST( status_failure_is_deterministic )
		dim as ComStatus status = ( _
			&hfffffffful, &hfffffffful, &hfffffffful, _
			&hfffffffful, &hfffffffful )

		CU_ASSERT_EQUAL( ComGetStatus( 0, status ), _
			FB_RTERROR_ILLEGALFUNCTIONCALL )
		CU_ASSERT_EQUAL( status.capabilities, 0 )
		CU_ASSERT_EQUAL( status.lines, 0 )
		CU_ASSERT_EQUAL( status.errors, 0 )
		CU_ASSERT_EQUAL( status.rx_queued, 0 )
		CU_ASSERT_EQUAL( status.tx_queued, 0 )
	END_TEST

	TEST( ordinary_file_is_not_a_com_port )
		dim as ComStatus status
		dim as integer file_number = freefile

		if( open( __FILE__ for input as #file_number ) <> 0 ) then
			CU_FAIL( "could not open ordinary-file fixture" )
		else
			CU_ASSERT_EQUAL( ComGetStatus( file_number, status ), _
				FB_RTERROR_ILLEGALFUNCTIONCALL )
			CU_ASSERT_EQUAL( ComSetLines( file_number, COM_LINE_RTS, _
				COM_LINE_RTS ), FB_RTERROR_ILLEGALFUNCTIONCALL )
			CU_ASSERT_EQUAL( ComSetBreak( file_number, true ), _
				FB_RTERROR_ILLEGALFUNCTIONCALL )
			CU_ASSERT_EQUAL( ComPurge( file_number, COM_PURGE_RX ), _
				FB_RTERROR_ILLEGALFUNCTIONCALL )

			close #file_number
		end if
	END_TEST

	TEST( invalid_masks )
		CU_ASSERT_EQUAL( ComSetLines( 1, 0, 0 ), _
			FB_RTERROR_ILLEGALFUNCTIONCALL )
		CU_ASSERT_EQUAL( ComSetLines( 1, COM_LINE_CTS, COM_LINE_CTS ), _
			FB_RTERROR_ILLEGALFUNCTIONCALL )
		CU_ASSERT_EQUAL( ComSetLines( 1, COM_LINE_RTS, COM_LINE_CTS ), _
			FB_RTERROR_ILLEGALFUNCTIONCALL )
		CU_ASSERT_EQUAL( ComPurge( 1, 0 ), _
			FB_RTERROR_ILLEGALFUNCTIONCALL )
		CU_ASSERT_EQUAL( ComPurge( 1, &h80000000ul ), _
			FB_RTERROR_ILLEGALFUNCTIONCALL )
	END_TEST

END_SUITE

/' end of com-control.bas '/
