'' examples/manual/proguide/errors/err.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Error Handling'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgErrorHandling
'' --------

Dim As Integer e, file_number
file_number = FreeFile
Open "xzxwz.zwz" For Input As #file_number
e = Err
Print e
Close #file_number
Sleep
