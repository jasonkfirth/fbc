''
'' FreeBASIC macOS smoke tests
'' ---------------------------
''
'' File: iconv-smoke.bas
''
'' Purpose:
''
''     Verify that libiconv.bi matches the iconv implementation shipped with
''     macOS and that the GNU-compatible names reach Apple's POSIX symbols.
''
'' Responsibilities:
''
''     - check the opaque Darwin iconv allocation layout
''     - open and close a converter through the libiconv-compatible names
''     - convert a Latin-1 string to UTF-8 and validate the output bytes
''
'' This file intentionally does NOT contain:
''
''     - locale-dependent conversions
''     - exhaustive character-set coverage
''

#include once "libiconv.bi"

const SMOKE_OK = 0
const SMOKE_LAYOUT_FAILED = 1
const SMOKE_OPEN_FAILED = 2
const SMOKE_CONVERSION_FAILED = 3
const SMOKE_CLOSE_FAILED = 4

#assert sizeof(iconv_allocation_t) = 64 * sizeof(any ptr)
#assert _LIBICONV_VERSION_ = &h010B

dim input_data(0 to 3) as ubyte = { asc("c"), asc("a"), asc("f"), &he9 }
dim output_data(0 to 7) as ubyte

dim input_buffer as zstring ptr = cptr(zstring ptr, @input_data(0))
dim output_buffer as zstring ptr = cptr(zstring ptr, @output_data(0))
dim input_left as size_t = ubound(input_data) + 1
dim output_left as size_t = ubound(output_data) + 1

dim converter as libiconv_t = libiconv_open("UTF-8", "ISO-8859-1")
if( converter = cptr(libiconv_t, -1) ) then
	end SMOKE_OPEN_FAILED
end if

dim converted as size_t = libiconv( _
	converter, _
	@input_buffer, _
	@input_left, _
	@output_buffer, _
	@output_left _
)

if( converted = cast(size_t, -1) ) then
	libiconv_close(converter)
	end SMOKE_CONVERSION_FAILED
end if

if( input_left <> 0 ) then
	libiconv_close(converter)
	end SMOKE_CONVERSION_FAILED
end if

if( output_left <> 3 ) then
	libiconv_close(converter)
	end SMOKE_CONVERSION_FAILED
end if

if( output_data(0) <> asc("c") or _
    output_data(1) <> asc("a") or _
    output_data(2) <> asc("f") or _
    output_data(3) <> &hc3 or _
    output_data(4) <> &ha9 ) then
	libiconv_close(converter)
	end SMOKE_CONVERSION_FAILED
end if

if( libiconv_close(converter) <> 0 ) then
	end SMOKE_CLOSE_FAILED
end if

end SMOKE_OK

'' end of iconv-smoke.bas
