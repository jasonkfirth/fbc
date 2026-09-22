'' examples/manual/fileio/freefile-bad.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'FREEFILE'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgFreefile
'' --------

'' This intentionally demonstrates why FREEFILE does not reserve its result.
'' FR owns the first open handle. FS is expected to receive the same number,
'' so the second checked OPEN reports the collision and never owns a handle.
Dim As Integer fr, fs
' The WRONG way:
fr = FreeFile
'' The repeated request is the teaching point of this deliberately bad example.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-IO-017
fs = FreeFile '' fs has taken the same file number as fr

If Open("file1" For Input As #fr) <> 0 Then
  Print "Could not open file1"
Else
  If Open("file2" For Input As #fs) <> 0 Then
    Print "file2 cannot use the same handle as file1"
  Else
    Print "Unexpectedly opened file2"
    Close #fs
  End If

  Close #fr
End If
