'' examples/manual/array/clear.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'CLEAR'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgClear
'' --------
''
'' Ownership:
''
'' p owns the 20-byte raw buffer used by the two CLEAR memory operations.  The
'' buffer is checked before dereferencing and released after its contents are
'' displayed.

'create an array with 100 elements
Dim array(0 To 99) As Integer

'clear the contents of the array to 0, starting with the first element
Clear array(0), , 100 * SizeOf(Integer)


'allocate 20 bytes of memory
Dim As Byte Ptr p
p = Allocate(20)

If p <> 0 Then
	'set each of the first ten bytes to 0
	Clear *p, 0, 10

	'set each of the next ten bytes to 42
	Clear p[10], 42, 10

	'check the values of the allocated bytes
	For i As Integer = 0 To 19
		Print i, p[i]
	Next

	'deallocate memory
	Deallocate p
	p = 0
Else
	Print "Unable to allocate the raw buffer."
End If

'' end of examples/manual/array/clear.bas
