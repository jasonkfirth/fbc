'' examples/manual/memory/callocate.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'CALLOCATE'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgCallocate
'' --------

' The pointer owns this allocation until it is passed to Deallocate.
Const elementCount As Integer = 10
Const valueStep As Integer = 10

' Allocate and initialize space for the integer elements.
Dim p As Integer Ptr = CAllocate(elementCount, SizeOf(Integer))

If p = 0 Then
	Print "Unable to allocate the integer buffer."
	End 1
End If

' Fill the memory with integer values.
For index As Integer = 0 To elementCount - 1
	p[index] = (index + 1) * valueStep
Next

' Display the integer values.
For index As Integer = 0 To elementCount - 1
	Print p[index] ;
Next

' Free the memory.
Deallocate(p)
p = 0
