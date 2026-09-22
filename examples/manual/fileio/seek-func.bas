'' examples/manual/fileio/seek-func.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'SEEK (Function)'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgSeekreturn
'' --------

Dim f As Integer, position As LongInt

f = FreeFile
If Open("file.ext" For Binary As #f) <> 0 Then
  Print "Could not open file.ext"
Else

  position = Seek(f)

  Close #f
End If
