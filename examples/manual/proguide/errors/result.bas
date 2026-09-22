'' examples/manual/proguide/errors/result.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Error Handling'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgErrorHandling
'' --------

Dim file_number As Integer, open_result As Integer
file_number = FreeFile
open_result = Open("xzxwz.zwz" For Input As #file_number)
Print open_result
If open_result = 0 Then Close #file_number
Sleep
