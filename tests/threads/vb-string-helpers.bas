/'
    FreeBASIC Runtime Tests
    File: threads/vb-string-helpers.bas
    Purpose: Exercise string helper temporaries in concurrent callers.
    Responsibilities: Independent string ownership and per-thread error status.
    This file does not permit unsynchronized writes to shared string data.
'/

#include "fbcunit.bi"
#include once "string.bi"

SUITE( fbc_tests.threads.vb_string_helpers )

	private sub worker( byval parameter as any ptr )
		dim failures as long ptr = parameter
		for i as integer = 1 to 2000
			dim text as string = Replace( StrReverse( "CbA" ), "abc", "done", 1, -1, fbTextCompare )
			if text <> "done" then *failures += 1
			if StrComp( text, "DONE", fbTextCompare ) <> 0 then *failures += 1
			text = Replace( "bad", "a", "b", 0 )
			if Err <> 1 then *failures += 1
			text = StrReverse( "ok" )
			if Err <> 0 or text <> "ko" then *failures += 1
		next
	end sub

	TEST( concurrent_temporaries )
		'' Each worker writes a separate result, inspected only after ThreadWait.
		'' fbcunit itself is called only by the joining thread.
		dim handles(0 to 3) as any ptr
		dim failures(0 to 3) as long
		for i as integer = 0 to 3
			handles(i) = ThreadCreate( @worker, @failures(i) )
			CU_ASSERT( handles(i) <> 0 )
		next
		for i as integer = 0 to 3
			if handles(i) <> 0 then ThreadWait( handles(i) )
			CU_ASSERT_EQUAL( failures(i), 0 )
		next
	END_TEST

END_SUITE

' end of threads/vb-string-helpers.bas
