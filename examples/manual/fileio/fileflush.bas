'' examples/manual/fileio/fileflush.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'FILEFLUSH'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgFileflush
'' --------

'' Resource policy:
'' f1 owns the output stream until the final Close. f2 is acquired only after
'' the output stream opened and is closed before f1.

#include "file.bi"

Dim As Integer f1, f2
Dim As String s

Print "File length", "File string"

f1 = FreeFile

If Open("fileflushtest.txt" For Output As #f1) <> 0 Then
	Print "Could not write fileflushtest.txt"
Else
	Print #f1, "successful file flush"

	f2 = FreeFile
	If Open("fileflushtest.txt" For Input As #f2) <> 0 Then
		Print "Could not read fileflushtest.txt"
	Else
		Line Input #f2, s
		Print FileLen("fileflushtest.txt"), "'" & s & "'"  '' the string is not yet physically written to the file

		FileFlush(f1)
		Line Input #f2, s
		Print FileLen("fileflushtest.txt"), "'" & s & "'"  '' the string is now physically written to the file

		Close #f2
	End If

	Close #f1
End If

Sleep
