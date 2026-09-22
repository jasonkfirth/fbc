'' examples/manual/math/random2.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'RANDOM'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgRandom
'' --------

'' Resource policy:
'' Each write or read pass owns its FreeFile handle after Open succeeds and
'' closes it before the next pass begins.

'' Layout: bytes 0-19 hold the fixed player name; bytes 20-23 hold the score.
'' File format policy: this 24-byte tutorial record is read by the same FreeBASIC target that wrote it.
Type ScoreEntry Field = 1
	' FB-LINTER: DISABLE-NEXT-LINE FBL-STR-009
	As String * 20 Name
	As Single score
End Type

Dim As ScoreEntry entry
Dim As Integer score_file

'' Generate a fake boring highscore file
score_file = FreeFile

If Open("scores.dat" For Random Access Write As #score_file Len = SizeOf(entry)) <> 0 Then
	Print "Could not write scores.dat"
Else
	For i As Integer = 1 To 10
		entry.name = "Player " & i
		entry.score = i
		Put #score_file, i, entry
	Next
	Close #score_file
End If

'' Read out and display the entries
score_file = FreeFile

If Open("scores.dat" For Random Access Read As #score_file Len = SizeOf(entry)) <> 0 Then
	Print "Could not read scores.dat"
Else
	For i As Integer = 1 To 10
		Get #score_file, i, entry
		Print i & ":", entry.name, Str(entry.score), entry.score
	Next
	Close #score_file
End If
