' TEST_MODE : COMPILE_AND_RUN_OK

''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: llvm-indexed-address.bas
''
'' Purpose:
''
''     Verify that indexed variables and pointer expressions include their
''     run-time index when the backend prepares a memory address.
''
'' This file intentionally does NOT contain:
''
''     - constant-only array indexes
''     - bounds-checking tests
''

dim shared values(0 to 3) as integer => { 11, 22, 33, 44 }

dim index as integer = 2
if( values(index) <> 33 ) then
	end 1
end if

dim p as integer ptr = @values(0)
if( p[index] <> 33 ) then
	end 2
end if

end 0

'' end of llvm-indexed-address.bas
