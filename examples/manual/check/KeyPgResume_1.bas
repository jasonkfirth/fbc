'' examples/manual/check/KeyPgResume_1.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'RESUME'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgResume
'' --------

Dim As Integer file_number = FreeFile
If Open( "text" For Input As #file_number ) <> 0 Then
  Print "Unable to open file"
End If
