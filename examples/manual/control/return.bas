'' examples/manual/control/return.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'RETURN (from Gosub)'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgReturnGosub
'' --------

'' GOSUB & RETURN example, compile with "-lang qb" or use "$lang" as below

'$lang: "qb"

Print "Let's Gosub!"
GoSub MyGosub
Print "Back from Gosub!"
Sleep
End

'' The preceding END prevents normal fall-through into this QB GOSUB target.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-CF-006
MyGosub:
Print "In Gosub!"
Return
