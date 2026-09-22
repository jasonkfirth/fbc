'' examples/manual/proguide/arrays/array5.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Arrays'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgArrays
'' --------

#macro PRINT_ARRAY_SIZING (array)
	'' Dynamic unsized arrays report zero dimensions through UBound(array, 0).
	'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-004
	If UBound( array , 0 ) = 0 Then
		Print "'" & #array & "' un-sized"
	Else
		'' The sizing report deliberately queries the known first dimension.
		'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-004
		Print "'" & #array & "' sized with " & UBound( array , 0 ) & " dimension";
		'' The plural form uses the same documented dimension count.
		'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-004
		If UBound( array , 0 ) > 1 Then
			Print "s";
		End If
		Print
		'' Array dimension numbers are one-based in this metadata API.
		'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-006
		For I As Integer = 1 To UBound( array , 0 )
			Print "   dimension nb: " & I
			'' The loop bound establishes a valid dimension index for LBound.
			'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-004
			Print "      lower bound: " & LBound( array , I )
			'' The loop bound establishes a valid dimension index for UBound.
			'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-004
			Print "      upper bound: " & UBound( array , I )
		Next I
	End If
#endmacro

Dim As Integer array1( )
PRINT_ARRAY_SIZING( array1 )
Print

Dim As Single array2( Any )
PRINT_ARRAY_SIZING( array2 )
Print

Dim As String array3( 4, 5 To 9 )
PRINT_ARRAY_SIZING( array3 )
Print

Type UDT
	'' This member deliberately begins unsized in all three dimensions.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL-OPT-003
	Dim As Double array4( Any, Any, Any )
End Type

Dim As UDT u
PRINT_ARRAY_SIZING( u.array4 )
Print

ReDim u.array4( -7 To -3, -2 To 5, 6 To 9 )
PRINT_ARRAY_SIZING( u.array4 )
Print

Erase u.array4
'' The macro intentionally reports the just-erased array as un-sized.
'' FB-LINTER: DISABLE-NEXT-LINE FBL522
PRINT_ARRAY_SIZING( u.array4 )
Print

Sleep
