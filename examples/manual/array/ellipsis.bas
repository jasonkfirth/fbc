'' examples/manual/array/ellipsis.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic '... (Ellipsis)'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgDots
'' --------

Dim As Integer myarray(0 To ...) = {0, 1, 2, 3}
'' The ellipsis initializer fixes this array's non-empty bounds at declaration.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-004
Print LBound(myarray), UBound(myarray)   '' 0, 3
