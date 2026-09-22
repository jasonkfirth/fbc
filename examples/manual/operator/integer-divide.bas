'' examples/manual/operator/integer-divide.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Operator \ (Integer divide)'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOpIntegerDivide
'' --------

Dim n As Double
Print n \ 5
'' This integer-division example deliberately rounds the non-integer divisor first.
'' FB-LINTER: DISABLE-NEXT-LINE FBL405 FBL-NUM-017
n = 7 \ 2.6  '' => 7 \ 3  => 2.33333  => 2
Print n
'' This integer-division example deliberately rounds the non-integer divisor first.
'' FB-LINTER: DISABLE-NEXT-LINE FBL405 FBL-NUM-017
n = 7 \ 2.4  '' => 7 \ 2 => 3.5 => 3
Print n
Sleep
