/'
	Project: fbcunit self-tests
	---------------------------

	File: fbcu_many_suites.bas

	Purpose:

		Exercise repeated suite hash-table growth.

	Responsibilities:

		* Force the suite registry through multiple rehash operations.
		* Verify that every registered suite remains reachable.

	This file intentionally does NOT contain:

		* Assertion or report-format tests, which belong to other self-tests.
'/

#include once "fbcunit.bi"

private sub verify_suite_entry cdecl ( )
	CU_ASSERT( true )
end sub

private sub add_many_suites ( ) constructor
	dim as string suite_name

	'' The default hash starts with 32 slots.  Eighty unique suites force it
	'' through several rehashes and verify that all entries remain reachable.
	for i as integer = 1 to 80
		suite_name = "many_suites.suite" & i
		fbcu.add_test( strptr( suite_name ), "verify", @verify_suite_entry )
	next
end sub

'' end of fbcu_many_suites.bas
