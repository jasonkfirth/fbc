' TEST_MODE : COMPILE_AND_RUN_OK

''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: bitfield-ms-layout.bas
''
'' Purpose:
''
''     Verify allocation-unit changes for mixed-width bitfields under the
''     default and Microsoft-compatible layout rules.
''
'' This file intentionally does NOT contain:
''
''     - bitfield value conversion tests
''     - anonymous union layout tests
''

type GCC_LAYOUT
	a : 4 as ubyte
	b : 4 as ushort
	c : 4 as ubyte
end type

#pragma push(msbitfields)
type MS_LAYOUT
	a : 4 as ubyte
	b : 4 as ushort
	c : 4 as ubyte
end type
#pragma pop(msbitfields)

if( sizeof( GCC_LAYOUT ) <> 2 ) then
	end 1
end if

if( sizeof( MS_LAYOUT ) <> 6 ) then
	end 2
end if

dim value as MS_LAYOUT
value.a = 3
value.b = 7
value.c = 11

if( value.a <> 3 ) then
	end 3
end if
if( value.b <> 7 ) then
	end 4
end if
if( value.c <> 11 ) then
	end 5
end if

end 0

'' end of bitfield-ms-layout.bas
