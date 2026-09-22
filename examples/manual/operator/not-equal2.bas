'' examples/manual/operator/not-equal2.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Operator <> (Not equal)'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOpNotEqual
'' --------

   If (69 <> 420) Then Print "(69 <> 420) is true."
   '' Parentheses deliberately make the equality comparison the NOT operand.
   '' FB-LINTER: DISABLE-NEXT-LINE FBL-BOOL-006
   If Not (69 = 420) Then Print "not (69 = 420) is true."
