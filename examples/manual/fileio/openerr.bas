'' examples/manual/fileio/openerr.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'OPEN ERR'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOpenErr
'' --------

'' Resource ownership:
'' The ERR device handle is closed after this example's one input/output pass.
Dim a As String
Dim file_number As Integer

file_number = FreeFile
Open Err For Input As #file_number
If Err <> 0 Then
  Print "Could not open ERR"
Else
  Print #file_number, "Please write something and press ENTER"
  Line Input #file_number, a
  Print #file_number, "You wrote"; a
  Close #file_number
End If
Sleep
