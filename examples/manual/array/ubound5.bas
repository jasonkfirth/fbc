'' examples/manual/array/ubound5.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'UBOUND'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgUbound
'' --------

Sub printArrayDimensions( array() As Integer )
	'' The dimension-count query is defined as zero for an undimensioned array.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-004
	Dim As Integer dimensions = UBound(array, 0)
	Print "dimensions: " & dimensions

	If dimensions = 0 Then Exit Sub

	'' For each dimension...
	For d As Integer = 1 To dimensions
		'' dimensions is non-zero here, so this indexes a known dimension.
		'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-004
		Print "dimension " & d & ": " & LBound( array, d ) & " to " & UBound( array, d )
	Next
End Sub

Dim array() As Integer
printArrayDimensions( array() )

Print "---"

ReDim array(10 To 11, 20 To 22)
printArrayDimensions( array() )
