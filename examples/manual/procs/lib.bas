'' examples/manual/procs/lib.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'LIB'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgLib
'' --------

'' mydll is compiled by FreeBASIC, so this declaration deliberately uses its
'' native Integer result and default FBCALL convention.
'' FB-LINTER: DISABLE-NEXT-LINE FBL320 FBL-ABI-004
Declare Function GetValue Lib "mydll" () As Integer

Print "GetValue = &h"; Hex(GetValue())

' Expected Output :
' GetValue = &h1234
