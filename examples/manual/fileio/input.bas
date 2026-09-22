'' examples/manual/fileio/input.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'INPUT #'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgInputPp
'' --------

'' Resource policy:
'' Each successful Open owns file_number until Close. The input pass runs only
'' after the output pass created the formatted tutorial data.

Dim a As Integer
Dim b As String
Dim c As Single
Dim file_number As Integer

file_number = FreeFile

If Open("myfile.txt" For Output As #file_number) <> 0 Then
	Print "Could not write myfile.txt"
Else
	'' This example intentionally demonstrates BASIC's formatted record syntax.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL517
	Write #file_number, 1, "Hello, World", 34.5
	Close #file_number

	file_number = FreeFile
	If Open("myfile.txt" For Input As #file_number) <> 0 Then
		Print "Could not read myfile.txt"
	Else
		'' This example intentionally demonstrates BASIC's formatted record syntax.
		'' FB-LINTER: DISABLE-NEXT-LINE FBL517
		Input #file_number, a, b, c
		Close #file_number
	End If
End If

Print a, b, c
