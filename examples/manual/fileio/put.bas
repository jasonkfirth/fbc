'' examples/manual/fileio/put.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'PUT (File I/O)'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgPutfileio
'' --------

' Create variables for the file number, and the number to put
Dim As Integer f
Dim As Long value

' Find the first free file number
f = FreeFile()

' Open the file "file.ext" for binary usage, using the file number "f"
If Open("file.ext" For Binary As #f) <> 0 Then
  Print "Could not open file.ext"
Else

  value= 10

  ' Write the bytes of the integer 'value' into the file, using file number "f"
  ' starting at the beginning of the file (position 1)
  ' file.ext is a same-target raw Integer record for this manual transfer.
  ' FB-LINTER: DISABLE-NEXT-LINE FBL-DOC-BIN-003
  If Put(#f, 1, value) <> 0 Then Print "Could not write file.ext"

  ' Close the file
  Close #f
End If
