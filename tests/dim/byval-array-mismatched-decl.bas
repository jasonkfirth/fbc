'' Project: FreeBASIC compiler tests
'' File: dim/byval-array-mismatched-decl.bas
'' Purpose: Require declarations and definitions to agree on array copying.
'' Successful array calls are covered in dim/byval-array.bas.

' TEST_MODE : COMPILE_ONLY_FAIL

declare sub edit( byval values() as integer )

sub edit( values() as integer )
end sub

'' end of dim/byval-array-mismatched-decl.bas
