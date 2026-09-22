'' examples/manual/fileio/openlpt1.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'OPEN LPT'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOpenLpt
'' --------

' Send some text to the Windows printer on LPT1:, using driver text imaging.
'' Resource policy:
'' The LPT handle is closed after the one printer write completes.
Dim file_number As Integer
file_number = FreeFile

'' LPT1 is an explicit printer endpoint, not a persistent output path.
'' FB-LINTER: DISABLE-NEXT-LINE FBL103 FBL-IO-005
Open Lpt "LPT1:EMU=TTY" For Output As #file_number
If Err <> 0 Then
  Print "Could not open LPT1:EMU=TTY"
Else
  Print #file_number, "Testing!"
  Close #file_number
End If
