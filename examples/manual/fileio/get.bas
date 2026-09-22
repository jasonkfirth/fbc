'' examples/manual/fileio/get.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'GET (File I/O)'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgGetfileio
'' --------

'' Resource policy:
'' The caller owns the binary file handle. get_mem owns its temporary buffer
'' only after Allocate succeeds and clears it after Deallocate.

Sub get_long(ByVal f As Integer)

	Dim buffer As Long ' Long variable

	' Read a Long (4 bytes) from the file into buffer, using file number "f".
	Get #f, , buffer

	' print out result
	Print buffer
	Print

End Sub

Sub get_array(ByVal f As Integer)

	Dim an_array(0 To 10-1) As Long ' array of Longs

	' Read 10 Longs (10 * 4 = 40 bytes) from the file into an_array, using file number "f".
	Get #f, , an_array()

	' print out result
	For i As Integer = 0 To 10-1
		Print an_array(i)
	Next
	Print

End Sub

Sub get_mem(ByVal f As Integer)

	Dim pmem As Long Ptr

	' allocate memory for 5 Longs
	pmem = Allocate(5 * SizeOf(Long))
	If pmem = 0 Then
		Print "Could not allocate memory for 5 Longs"
		Print
		Exit Sub
	End If

	' Read 5 Longs (5 * 4 = 20 bytes) from the file into allocated memory
	Get #f, , *pmem, 5 ' Note pmem must be dereferenced (*pmem, or pmem[0])

	' print out result using [] Pointer Indexing
	For i As Integer = 0 To 5-1
		Print pmem[i]
	Next
	Print

	' free pointer memory to prevent memory leak
	Deallocate pmem
	pmem = 0

End Sub

' Find the first free file file number.
Dim f As Integer
f = FreeFile

' Open the file "file.ext" for binary usage, using the file number "f".
If Open("file.ext" For Binary As #f) <> 0 Then
	Print "Could not open file.ext"
Else

  get_long(f)

  get_array(f)

  get_mem(f)

	' Close the file.
	Close #f
End If
