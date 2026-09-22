'' examples/manual/error/err2.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'ERR'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgErr
'' --------

'' compile without -e switch

Dim filename As String
Dim file_number As Integer

Do
	Line Input "Input filename: ", filename
	If filename = "" Then End
	file_number = FreeFile
	Open filename For Input As #file_number
	If Err() = 0 Then Exit Do
Loop

Print Using "File '&' opened successfully"; filename
Close #file_number
