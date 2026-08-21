''
'' FreeBASIC Windows CE numeric-format diagnostic
'' ------------------------------------------------
''
'' File: tests/wince/number_format_smoke.bas
''
'' Purpose:
''
''     Record the exact Windows CE runtime STR() output for representative
''     single- and double-precision values.
''
'' Responsibilities:
''
''     - format runtime floating-point variables
''     - record their lengths and byte values without console output
''     - return a nonzero status if the diagnostic file cannot be created
''
'' This file intentionally does NOT contain:
''
''     - expected-value assertions
''     - compiler constant-folding coverage
''     - production debug output
''

dim as single single_value = 2.5f
dim as double double_value = 2.5
dim as string single_text = str( single_value )
dim as string double_text = str( double_value )
dim as integer output_file = freefile( )
dim as zstring * 64 mingw_single_text
dim as zstring * 64 mingw_double_text

extern "c"
	declare function mingw_snprintf cdecl alias "__mingw_snprintf" _
		(byval buffer as zstring ptr, _
		 byval buffer_size as uinteger, _
		 byval format_text as const zstring ptr, _
		 ...) as long
end extern

mingw_snprintf( @mingw_single_text, sizeof( mingw_single_text ), _
	"%.7g", single_value )
mingw_snprintf( @mingw_double_text, sizeof( mingw_double_text ), _
	"%.16g", double_value )

if open( "\Storage Card\number-format-smoke.txt" _
         for output as #output_file )<>0 then
	end 1
end if

print #output_file, "single="""; single_text; """ length="; len( single_text )
for byte_index as integer = 0 to len( single_text ) - 1
	print #output_file, "single-byte["; byte_index; "]="; asc( single_text, byte_index + 1 )
next

print #output_file, "double="""; double_text; """ length="; len( double_text )
for byte_index as integer = 0 to len( double_text ) - 1
	print #output_file, "double-byte["; byte_index; "]="; asc( double_text, byte_index + 1 )
next

print #output_file, "mingw-single="""; mingw_single_text; """"
print #output_file, "mingw-double="""; mingw_double_text; """"

close #output_file
end 0

'' end of tests/wince/number_format_smoke.bas
