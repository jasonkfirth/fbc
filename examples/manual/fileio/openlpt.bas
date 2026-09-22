'' examples/manual/fileio/openlpt.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'OPEN LPT'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOpenLpt
'' --------

'This simple program will print a PostScript file to a PostScript compatible printer.
'' Resource ownership:
'' The source file remains open while its checked printer handle receives the
'' text stream. The printer closes before the source file.
Dim As Integer FFI, PPO
Dim As String temp

FFI = FreeFile()
If Open("sample.ps" For Input Access Read As #FFI) <> 0 Then
  Print "Could not open sample.ps"
Else
  PPO = FreeFile()

  '' LPT1 is an explicit printer endpoint, not a persistent output path.
  '' FB-LINTER: DISABLE-NEXT-LINE FBL103 FBL-IO-005
  Open Lpt "LPT1:" For Output As #PPO
  If Err <> 0 Then
    Print "Could not open LPT1:"
  Else
    While EOF(FFI) = 0
      Line Input #FFI, temp
      Print #PPO, temp
    Wend
    Close #PPO
  End If

  Close #FFI
End If

Print "Printing Completed!"
