'' examples/manual/math/random1.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'RANDOM'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgRandom
'' --------

'' This example generates a test file and then lets you view random records
'' that are read live from the file.

'' Layout: byte 0 stores the string length; bytes 1-10 store the fixed string.
'' File format policy: this 11-byte tutorial record is read by the same FreeBASIC target that wrote it.
Type Entry
	slen As Byte
	' FB-LINTER: DISABLE-NEXT-LINE FBL-STR-009
	sdata As String * 10
End Type

Dim u As Entry
Dim s As String
Dim record_file As Integer

Data ".,-?!'@:", "abc",      "def"
Data "ghi",      "jkl",      "mno"
Data "pqrs",     "tuv",      "wxyz"

record_file = FreeFile

If Open("testfile" For Random As #record_file Len = SizeOf(Entry)) <> 0 Then
	Print "Could not open testfile"
Else
	'' Write out 9 records with predefined data
	For i As Integer = 1 To 9
		Read s
		u.slen = Len(s)
		u.sdata = s
		Put #record_file, i, u
	Next

	'' Let the user view records by specifying their index number
	Do
		Dim i As Integer
		Input "Record number: ", i
		If i < 1 Or i > 9 Then Exit Do

		Get #record_file, i, u
		Print i & ": " & Left( u.sdata, u.slen )
		Print
	Loop

	Close #record_file
End If
