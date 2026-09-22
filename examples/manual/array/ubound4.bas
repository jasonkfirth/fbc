'' examples/manual/array/ubound4.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'UBOUND'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgUbound
'' --------

'' determining whether an array is empty
Dim array() As Integer

'' This output intentionally shows the empty-array bound convention below.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-004
Print "lbound: "; LBound( array ), "ubound: "; UBound( array )  ''  0 and -1

If LBound( array ) > UBound( array ) Then
	Print "array is empty"
Else
	Print "array is not empty"
End If
