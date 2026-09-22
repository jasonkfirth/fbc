'' examples/manual/error/onerror.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'ON ERROR'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOnerror
'' --------

'' Compile with QB (-lang qb) dialect

'$lang: "qb"

'' This legacy handler is the subject of the ON ERROR example.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-ERR-004 FBL-ERR-005
On Error Goto errorhandler
Error 24 '' simulate an error
Print "this message will not be seen"

'' The preceding Error statement transfers control here, not normal flow.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-ERR-006 FBL-CF-004
errorhandler:
n = Err
Print "Error #"; n; "!"
End
