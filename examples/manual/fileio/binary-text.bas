'' examples/manual/fileio/binary-text.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'BINARY'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgBinary
'' --------

'' Read entire contents of a file to a string
Const MAX_TEXT_FILE_BYTES = 4 * 1024 * 1024
Dim h As Integer
Dim txt As String

h = FreeFile

If Open("myfile.txt" For Binary Access Read As #h) = 0 Then
  Dim As LongInt file_size = LOF(h)

  If file_size > 0 AndAlso file_size <= MAX_TEXT_FILE_BYTES Then
	'' our string has as many characters as the checked file size in bytes
	txt = String(file_size, 0)
	'' size of txt is known.  entire string filled with file data
	If Get(#h, , txt) <> 0 Then txt = ""
  End If

  Close #h
End If

Print txt
