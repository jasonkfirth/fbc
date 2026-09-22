'' examples/manual/operator/integer-divide-assign.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Operator \= (Integer divide and Assign)'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOpCombineIntegerDivide
'' --------

Dim n As Double
n = 6
'' The floating right operand intentionally demonstrates that \= truncates its input.
'' FB-LINTER: DISABLE-NEXT-LINE FBL405 FBL-NUM-017
n \= 2.2
Print n
Sleep
