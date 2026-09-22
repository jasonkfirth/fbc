'' examples/manual/fileio/binary-read.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'BINARY'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgBinary
'' --------

'' Now read the number from the file
Dim x As Single = 0
Dim f As Integer

f = FreeFile

If Open("MyFile.Dat" For Binary As #f) <> 0 Then
  Print "Could not open MyFile.Dat"
Else
  If Get(#f, , x) <> 0 Then Print "Could not read MyFile.Dat"
  Close #f
End If

Print x
