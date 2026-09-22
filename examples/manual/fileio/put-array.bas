'' examples/manual/fileio/put-array.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'PUT (File I/O)'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgPutfileio
'' --------

' Create an integer array
Dim buffer(1 To 10) As Integer
For i As Integer = 1 To 10
	buffer(i) = i
Next

' Find the first free file file number
Dim f As Integer
f = FreeFile()

' Open the file "file.ext" for binary usage, using the file number "f"
If Open("file.ext" For Binary As #f) <> 0 Then
  Print "Could not open file.ext"
Else
  ' Write the array into the file, using file number "f"
  ' starting at the beginning of the file (position 1).
  ' file.ext contains this same-target Integer array record.
  ' FB-LINTER: DISABLE-NEXT-LINE FBL-DOC-BIN-003
  If Put(#f, 1, buffer()) <> 0 Then Print "Could not write file.ext"

  ' Close the file
  Close #f
End If
