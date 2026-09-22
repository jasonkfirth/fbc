'' examples/manual/error/resume-next.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'RESUME NEXT'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgResumenext
'' --------

'' Compile with -lang fblite or qb

#lang "fblite"

Dim As Single i, j

'' This legacy handler is the subject of the RESUME NEXT example.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-ERR-004 FBL-ERR-005
On Error Goto ErrHandler

i = 0
j = 5
j = 1 / i ' this line causes a divide-by-zero error; execution jumps to ErrHandler label

Print "ending..."

End ' end the program so that execution does not fall through to the error handler again

'' END above prevents normal execution from entering this teaching handler.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-ERR-006 FBL-CF-004
ErrHandler:

'' The undefined intermediate value is the specific behavior this page shows.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-ERR-007
Resume Next ' execution jumps to 'Print "ending..."' line, but j is now in an undefined state
