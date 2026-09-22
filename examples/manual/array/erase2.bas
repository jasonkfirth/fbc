'' examples/manual/array/erase2.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'ERASE'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgErase
'' --------

Dim MyArray1(1 To 10) As Integer
ReDim MyArray2(1 To 10) As Integer

Print "MyArray1", LBound( MyArray1 ), UBound( MyArray1 ) ' prints: MyArray1       1             10
'' The dynamic array is allocated above, so these bounds describe its initial range.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-004
Print "MyArray2", LBound( MyArray2 ), UBound( MyArray2 ) ' prints: MyArray2       1             10

'' This line deliberately contrasts ERASE on fixed and dynamic arrays.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-007
Erase MyArray1, MyArray2

Print "MyArray1", LBound( MyArray1 ), UBound( MyArray1 ) ' prints: MyArray1       1             10
'' The empty dynamic-array descriptor intentionally reports the 0 to -1 range.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-004
Print "MyArray2", LBound( MyArray2 ), UBound( MyArray2 ) ' prints: MyArray2       0            -1

'' end of erase2.bas
