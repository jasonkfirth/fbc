'' examples/manual/fileio/opencons.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'OPEN CONS'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOpenCons
'' --------

'' Resource ownership:
'' The input and output console handles are independent and are closed only
'' after both ends opened successfully.
Dim a As String
Dim input_number As Integer, output_number As Integer

input_number = FreeFile
Open Cons For Input As #input_number
If Err <> 0 Then
  Print "Could not open console input"
Else
  output_number = FreeFile

  '' CONS is a device endpoint, not a persistent output path.
  '' FB-LINTER: DISABLE-NEXT-LINE FBL103 FBL-IO-005
  Open Cons For Output As #output_number
  If Err <> 0 Then
    Print "Could not open console output"
  Else
    Print #output_number, "Please write something and press ENTER"
    Line Input #input_number, a
    Print #output_number, "You wrote : "; a
    Close #output_number
  End If

  Close #input_number
End If
Sleep
