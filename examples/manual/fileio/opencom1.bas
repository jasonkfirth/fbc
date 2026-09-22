'' examples/manual/fileio/opencom1.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'OPEN COM'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOpenCom
'' --------

Dim file_number As Integer
file_number = FreeFile

Open Com "COM1:9600,N,,2" As #file_number
If Err > 0 Then
  Print "The port could not be opened."
Else
  Close #file_number
End If
