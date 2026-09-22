'' examples/manual/fileio/get-file.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'GET (File I/O)'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgGetfileio
'' --------

' Load a small text file to a string

Function LoadFile(ByRef filename As String) As String

	Const MAX_TEXT_FILE_BYTES = 4 * 1024 * 1024
	Dim h As Integer
	Dim file_size As LongInt
	Dim txt As String

	h = FreeFile

	If Open( filename For Binary Access Read As #h ) <> 0 Then Return ""

	file_size = LOF(h)

	If file_size > 0 AndAlso file_size <= MAX_TEXT_FILE_BYTES Then

		txt = String(file_size, 0)
		If Get( #h, , txt ) <> 0 Then txt = ""

	End If

	Close #h

	Return txt

End Function

Dim ExampleStr As String
ExampleStr = LoadFile("smallfile.txt")
Print ExampleStr
