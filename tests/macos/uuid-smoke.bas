''
'' FreeBASIC macOS smoke tests
'' ---------------------------
''
'' File: uuid-smoke.bas
''
'' Purpose:
''
''     Verify that uuid.bi uses the UUID routines exported by macOS libSystem
''     without requiring a nonexistent standalone libuuid.
''
'' Responsibilities:
''
''     - generate a UUID
''     - format and parse its canonical string representation
''     - verify comparison and null-UUID operations
''
'' This file intentionally does NOT contain:
''
''     - Linux-only UUID timestamp or variant extensions
''     - randomness quality tests
''

#include once "uuid.bi"

const SMOKE_OK = 0
const SMOKE_GENERATE_FAILED = 1
const SMOKE_FORMAT_FAILED = 2
const SMOKE_PARSE_FAILED = 3
const SMOKE_CLEAR_FAILED = 4

dim generated(0 to 15) as ubyte
dim parsed(0 to 15) as ubyte
dim formatted as zstring * 37

uuid_generate(@generated(0))
if( uuid_is_null(@generated(0)) <> 0 ) then
	end SMOKE_GENERATE_FAILED
end if

uuid_unparse_lower(@generated(0), @formatted)
if( len(formatted) <> 36 ) then
	end SMOKE_FORMAT_FAILED
end if

if( uuid_parse(@formatted, @parsed(0)) <> 0 ) then
	end SMOKE_PARSE_FAILED
end if

if( uuid_compare(@generated(0), @parsed(0)) <> 0 ) then
	end SMOKE_PARSE_FAILED
end if

uuid_clear(@parsed(0))
if( uuid_is_null(@parsed(0)) = 0 ) then
	end SMOKE_CLEAR_FAILED
end if

end SMOKE_OK

'' end of uuid-smoke.bas
