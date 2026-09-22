'' examples/manual/console/lineinput.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'LINE INPUT'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgLineinput
'' --------
''
'' Ownership:
''
'' pz owns a maxlength-byte ZSTRING buffer.  maxlength includes one null
'' terminator byte, so LINE INPUT may retain at most maxlength - 1 characters.

Dim s As String
Line Input "Enter a line"; s
Print "Full line that you entered:"
Print "'"; s; "'"
Print

Const maxlength = 11  '' max 10 characters plus 1 null terminal character
Dim pz As ZString Ptr = CAllocate(maxlength, SizeOf(ZString))
If pz <> 0 Then
	Line Input "Enter a line"; *pz, maxlength
	Print "First " & maxlength - 1 & " characters that you entered:"
	Print "'"; *pz; "'"
	Deallocate(pz)
	pz = 0
Else
	Print "Unable to allocate the input buffer."
End If

'' end of examples/manual/console/lineinput.bas
