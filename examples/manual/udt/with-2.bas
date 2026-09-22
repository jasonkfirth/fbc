'' examples/manual/udt/with-2.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'WITH'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgWith
'' --------
''
'' Ownership:
''
'' the_rectangle owns the five zero-initialized rect_type values allocated for
'' the dereferenced WITH demonstration and releases them after the loop.

Type rect_type
	x As Single
	y As Single
End Type

Dim the_rectangle As rect_type Ptr
Dim As Integer loopvar, temp, t

the_rectangle = CAllocate( 5 * Len( rect_type ) )

If the_rectangle <> 0 Then
	For loopvar = 0 To 4

		With the_rectangle[loopvar]  '' dereferenced pointer

			temp = .x
			.x = 234 * t + 48 + .y
			.y = 321 * t + 2

		End With

	Next

	Deallocate(the_rectangle)
	the_rectangle = 0
Else
	Print "Unable to allocate the rectangle array."
End If

'' end of examples/manual/udt/with-2.bas
