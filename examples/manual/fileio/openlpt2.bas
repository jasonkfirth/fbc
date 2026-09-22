'' examples/manual/fileio/openlpt2.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'OPEN LPT'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOpenLpt
'' --------

'' Sends contents of text file test.txt to Windows printer named "ReceiptPrinter"
'' Resource ownership:
'' The source file stays open while the checked printer device receives each
'' line. The printer handle closes before the source handle.
Dim RptInput As String
Dim PrintFileNum As Integer, RptFileFileNum As Integer

RptFileFileNum = FreeFile
Open "test.txt" For Input As #RptFileFileNum
If Err <> 0 Then
  Print "Could not open test.txt"
Else
  PrintFileNum = FreeFile
  '' The LPT form must use statement syntax; Err immediately tests this open.
  '' FB-LINTER: DISABLE-NEXT-LINE FBL-IO-001
  Open Lpt "LPT:ReceiptPrinter,TITLE=ReceiptWinTitle,EMU=TTY" As _
    #PrintFilenum

  '' The compiler-supported device OPEN statement provides its result via Err.
  '' FB-LINTER: DISABLE-NEXT-LINE FBL613
  If Err <> 0 Then
    Print "Could not open ReceiptPrinter"
  Else
    While EOF(RptFileFileNum) = 0
      Line Input #RptFileFileNum, RptInput
      Print #PrintFileNum, RptInput
    Wend

    Close #PrintFileNum  ' Interestingly, does not require CHR(12).  But if pagination is desired, CHR(12) is the way.
  End If

  Close #RptFileFileNum
End If

Print "Press any key to end program..."
GetKey

End
