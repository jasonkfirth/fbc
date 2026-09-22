'' examples/manual/fileio/openscrn.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'OPEN SCRN'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOpenScrn
'' --------

'' Resource ownership:
'' The SCRN device handle is closed after the example's input/output pass.
Dim a As String
Dim file_number As Integer

file_number = FreeFile
Open Scrn For Input As #file_number
If Err <> 0 Then
  Print "Could not open SCRN"
Else
  Print #file_number, "Please write something and press ENTER"
  Line Input #file_number, a
  Print #file_number, "You wrote"; a
  Close #file_number
End If
Sleep
