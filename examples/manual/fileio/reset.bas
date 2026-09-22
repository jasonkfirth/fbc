'' examples/manual/fileio/reset.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'RESET'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgReset
'' --------

'' Resource policy:
'' RESET closes the checked manual fixture after its output record is written.
Dim file_number As Integer
file_number = FreeFile

'' This named file is a deliberate RESET fixture that remains inspectable.
'' FB-LINTER: DISABLE-NEXT-LINE FBL103 FBL-IO-005
If Open("test.txt" For Output As #file_number) <> 0 Then
  Print "Could not open test.txt"
Else
  Print #file_number, "testing 123"
  Reset
End If
