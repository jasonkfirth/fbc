'' examples/manual/module/import.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'IMPORT'
'' Purpose: Read a C DLL's exported 32-bit integer.
'' Responsibilities: DLL import linkage and matching the C data width.
'' This example contains no DLL implementation or runtime loading code.
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgImport
'' --------

/'  import.bas :
	Compile with
	  fbc import.bas
'/
#inclib "mydll"

'' C int remains 32 bits on both Win32 and Win64.
Extern Import MyDll_Data Alias "MyDll_Data" As Long

Print "&h" + Hex( MyDll_Data )

' Output:
' &h1234

'' end of import.bas
