'' Project: FreeBASIC compiler tests
'' File: dim/byval-array-wrong-element-type.bas
'' Purpose: Require matching array element types even at equal byte widths.
'' Runtime copying is covered in dim/byval-array.bas.

' TEST_MODE : COMPILE_ONLY_FAIL

sub edit( byval values() as long )
end sub

dim values(0 to 2) as ulong
edit( values() )

'' end of dim/byval-array-wrong-element-type.bas
