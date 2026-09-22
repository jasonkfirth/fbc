'' examples/manual/faq/gfxlib2/bload-workaround.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'BLOAD/BSAVE text mode work-around'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=FaqPgbloadworkaround
'' --------

Sub _bsave( file As String, p As Any Ptr, sz As Integer )

  '' Resource ownership:
  '' This routine owns ff only after OPEN succeeds. The caller owns the byte
  '' buffer and provides its validated length.
  Dim As Integer ff
  If p = 0 OrElse sz <= 0 Then
    Print "Invalid output buffer"
    Exit Sub
  End If

  ff = FreeFile

  If Open(file For Binary As #ff) <> 0 Then
    Print "Could not open " & file
  Else
	If fb_fileput(ff, 0, ByVal p, sz) <> 0 Then Print "Could not write " & file

    Close #ff
  End If

End Sub

Sub _bload( file As String, p As Any Ptr )

  '' The caller owns p and must provide storage large enough for the file.
  '' This wrapper checks the file handle and rejects empty inputs before GET.
  Dim As Integer ff
  Dim As LongInt file_length
  If p = 0 Then
    Print "Invalid input buffer"
    Exit Sub
  End If

  ff = FreeFile

  If Open(file For Binary As #ff) <> 0 Then
    Print "Could not open " & file
  Else
    file_length = LOF(ff)
    If file_length <= 0 Then
      Print "Could not read bytes from " & file
    ElseIf fb_fileget(ff, 0, ByVal p, file_length) <> 0 Then
      Print "Could not read " & file
    End If

    Close #ff
  End If

End Sub
