'' examples/manual/fileio/lof.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'LOF'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgLof
'' --------

Dim f As Integer
f = FreeFile
If Open("file.ext" For Binary As #f) <> 0 Then
  Print "Could not open file.ext"
Else
  Print LOF(f)
  Close #f
End If
