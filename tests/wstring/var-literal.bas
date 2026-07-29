''
'' FreeBASIC Compiler Test Suite
''
'' File: var-literal.bas
''
'' Verify that VAR can infer a fixed WSTRING from a literal whose size is
'' known while parsing the declaration.  This test does not cover expressions
'' that would require a dynamic WSTRING representation.
''
'' TEST_MODE : COMPILE_AND_RUN_OK

var inferred = wstr( "wide text" )

if( inferred <> wstr( "wide text" ) ) then
	end 1
end if

if( len( inferred ) <> 9 ) then
	end 1
end if

'' end of var-literal.bas
