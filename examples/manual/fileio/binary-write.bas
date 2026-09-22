'' examples/manual/fileio/binary-write.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'BINARY'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgBinary
'' --------

'' Create a binary data file with one number in it
Dim x As Single = 17.164
Dim f As Integer

f = FreeFile

If Open("MyFile.Dat" For Binary As #f) <> 0 Then
  Print "Could not open MyFile.Dat"
Else
  '' put without a position setting will put from the last known file position
  '' in this case, the very beginning of the file.
  If Put(#f, , x) <> 0 Then Print "Could not write MyFile.Dat"
  Close #f
End If
