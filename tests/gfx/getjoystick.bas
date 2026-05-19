' TEST_MODE : COMPILE_AND_RUN_OK

dim buttons as integer
dim a1 as single
dim a2 as single
dim a3 as single
dim a4 as single
dim a5 as single
dim a6 as single
dim a7 as single
dim a8 as single
dim result as long

result = getjoystick(0, buttons, a1)
if( result = 0 ) then
	ASSERT( a1 >= -1.0 andalso a1 <= 1.0 )
else
	ASSERT( buttons = -1 )
	ASSERT( a1 = -1000.0 )
end if

result = getjoystick(0, buttons, a1, a2, a3, a4, a5, a6, a7, a8)
if( result = 0 ) then
	ASSERT( a1 >= -1.0 andalso a1 <= 1.0 )
	ASSERT( a2 >= -1.0 andalso a2 <= 1.0 )
else
	ASSERT( buttons = -1 )
	ASSERT( a1 = -1000.0 )
	ASSERT( a2 = -1000.0 )
	ASSERT( a3 = -1000.0 )
	ASSERT( a4 = -1000.0 )
	ASSERT( a5 = -1000.0 )
	ASSERT( a6 = -1000.0 )
	ASSERT( a7 = -1000.0 )
	ASSERT( a8 = -1000.0 )
end if
