'' Project: FreeBASIC C-emitter tests
'' File: const-copy-byte-view.bas
'' Purpose: Retain constness while copying a packed record's plain member.
'' Responsibilities: Check copied values, independent storage and managed strings.
'' This file intentionally does NOT write through the const source parameter.

'' TEST_MODE : COMPILE_AND_RUN_OK

type Payload
	wide as longint
	values(0 to 2) as long
end type

type Owner field = 1
	marker as ubyte
	data as Payload
	text as string
end type

sub check_copy( byref original as const Owner )
	dim copied as Owner = original
	if( copied.marker <> 93 or copied.data.wide <> 117 ) then end 1
	if( copied.data.values(0) <> 11 or copied.data.values(2) <> 33 ) then end 2
	if( copied.text <> "original" ) then end 3
	copied.data.wide = -33
	copied.data.values(1) = 70
	copied.text = "changed"
	if( original.data.wide <> 117 or original.data.values(1) <> 22 ) then end 4
	if( original.marker <> 93 or original.text <> "original" ) then end 5
end sub

dim source as Owner
source.marker = 93
source.data.wide = 117
source.data.values(0) = 11
source.data.values(1) = 22
source.data.values(2) = 33
source.text = "original"
check_copy( source )
end 0

'' end of const-copy-byte-view.bas
