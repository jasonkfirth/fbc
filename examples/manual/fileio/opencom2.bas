'' examples/manual/fileio/opencom2.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'OPEN COM'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOpenCom
'' --------

Dim file_number As Integer
file_number = FreeFile

Open Com "COM1:115200" As #file_number
If Err > 0 Then
  Print "The port could not be opened."
Else
  Close #file_number
End If
