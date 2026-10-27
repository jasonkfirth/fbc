'' Project: FreeBASIC compiler tests
'' File: c-type-name-aliases.bas
'' Purpose: Keep distinct type aliases distinct when spelling C identifiers.
'' Responsibilities: Exercise dollar/underscore aliases and independent layouts.
'' This file intentionally does NOT change exported procedure or runtime names.
'' TEST_MODE : COMPILE_AND_RUN_OK
#lang "fb"

'' Replacing $ with _ would give these equal-length aliases the same C tag.
type DollarName alias "fb$tag"
	value as long
end type

type UnderscoreName alias "fb_tag"
	value as double
end type

private function read_dollar( byref item as DollarName ) as long
	return item.value
end function

private function read_underscore( byref item as UnderscoreName ) as double
	return item.value
end function

dim as DollarName dollar_item = type( 23 )
dim as UnderscoreName underscore_item = type( 1.5 )
if( read_dollar( dollar_item ) <> 23 ) then end 1
if( read_underscore( underscore_item ) <> 1.5 ) then end 2
if( sizeof( DollarName ) <> 4 ) then end 3
if( sizeof( UnderscoreName ) <> 8 ) then end 4
print "c-type-name-aliases-ok"
end 0

'' end of c-type-name-aliases.bas
