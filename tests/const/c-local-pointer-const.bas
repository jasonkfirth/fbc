/'
	Project: FreeBASIC compiler tests
	---------------------------------

	File: c-local-pointer-const.bas

	Purpose:

		Verify that C backend temporary pointers preserve pointee const.

	Responsibilities:

		* Exercise the temporary pointer generated for WITH.
		* Exercise const pointer function results and call temporaries.

	This file intentionally does NOT contain:

		* Runtime assertions.  GCC's qualifier diagnostics are the oracle.
'/

' TEST_MODE : COMPILE_ONLY_OK

#cmdline "-gen gcc"
#cmdline "-Wc -Wno-unknown-warning-option"
#cmdline "-Wc -Werror=discarded-qualifiers"
#cmdline "-restart"

type record_type
	value as integer
end type

sub inspect_record( byref item as const record_type )
	with item
		print .value
	end with
end sub

function pass_text( byval text as const zstring ptr ) as const zstring ptr
	return text
end function

dim item as record_type = ( 1 )
inspect_record( item )
print *pass_text( @"text" )

' end of c-local-pointer-const.bas
