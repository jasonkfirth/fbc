' TEST_MODE : COMPILE_AND_RUN_OK

''
'' FreeBASIC command sweep tests
'' -----------------------------
''
'' File: command-sweep.bas
''
'' Purpose:
''
''     Compile and run one program that touches QB-only language and
''     runtime compatibility features.
''
'' Responsibilities:
''
''     - exercise default typing and type suffixes
''     - exercise OPTION DYNAMIC, COMMON, typeless REDIM, DATA/READ/RESTORE,
''       GOSUB/RETURN, CALL, MK*/CV* conversion helpers, and STR$
''     - keep the run bounded and non-interactive
''
'' This file intentionally does NOT contain:
''
''     - gfxlib coverage
''     - direct hardware port I/O
''     - platform-specific device names
''

#define CHECK(e) if (e) = 0 then fb_Assert(__FILE__, __LINE__, __FUNCTION__, #e)

option dynamic
defint a-z
defsng s-s
defdbl d-d
defstr q-q

dim shared helper_result as integer
declare sub helper( byval value as integer )

common shared_array()
redim shared_array(1 to 3)
shared_array(1) = 10
shared_array(2) = 20
shared_array(3) = 30

CHECK( lbound( shared_array ) = 1 )
CHECK( ubound( shared_array ) = 3 )
CHECK( shared_array(1) + shared_array(2) + shared_array(3) = 60 )

redim values%(0 to 2)
values%(0) = 4
values%(1) = 5
values%(2) = 6

CHECK( values%(0) + values%(1) + values%(2) = 15 )

a = 11
s = 1.5
d = 2.25
q = "qb"

CHECK( a% = 11 )
CHECK( s! = 1.5 )
CHECK( d# = 2.25 )
CHECK( q$ = "qb" )

data 3, 4, 5
read a, b, c
CHECK( a + b + c = 12 )
restore
read a
CHECK( a = 3 )

gosub local_subroutine
CHECK( a = 99 )

call helper( 7 )
CHECK( helper_result = 14 )

dim packed_integer as string
dim packed_single as string
dim packed_double as string
dim packed_long as string

packed_integer = mki$( 1234 )
packed_single = mks$( 12.5 )
packed_double = mkd$( 25.25 )
packed_long = mkl$( 123456 )

CHECK( cvi( packed_integer ) = 1234 )
CHECK( cvs( packed_single ) = 12.5 )
CHECK( cvd( packed_double ) = 25.25 )
CHECK( cvl( packed_long ) = 123456 )

CHECK( str$( 12 ) = " 12" )
CHECK( str$( -12 ) = "-12" )

end 0

local_subroutine:
	a = 99
return

sub helper( byval value as integer )
	helper_result = value * 2
end sub

'' end of command-sweep.bas
