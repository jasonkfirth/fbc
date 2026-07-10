''
'' FreeBASIC macOS smoke tests
'' ---------------------------
''
'' File: regex-smoke.bas
''
'' Purpose:
''
''     Verify the Darwin CRT regular-expression layout and the POSIX regex
''     implementation exported by macOS libSystem.
''
'' Responsibilities:
''
''     - check SDK-derived regex_t and regmatch_t layouts
''     - compile and execute an expression with capture groups
''     - verify the no-match return code and release compiled state
''
'' This file intentionally does NOT contain:
''
''     - locale-dependent matching
''     - Darwin-only wide-character regex extensions
''

#include once "crt/regex.bi"

const SMOKE_OK = 0
const SMOKE_COMPILE_FAILED = 1
const SMOKE_MATCH_FAILED = 2
const SMOKE_NO_MATCH_FAILED = 3

#assert sizeof(regex_t) = 32
#assert offsetof(regex_t, re_magic) = 0
#assert offsetof(regex_t, re_nsub) = 8
#assert offsetof(regex_t, re_endp) = 16
#assert offsetof(regex_t, re_g) = 24
#assert sizeof(regmatch_t) = 16

dim expression as regex_t
dim matches(0 to 2) as regmatch_t

dim result as long = regcomp( _
	@expression, _
	"^([[:alpha:]]+)-([[:digit:]]+)$", _
	REG_EXTENDED _
)

if( result <> REG_NOERROR ) then
	end SMOKE_COMPILE_FAILED
end if

result = regexec(@expression, "freebasic-120", 3, @matches(0), 0)
if( result <> REG_NOERROR ) then
	regfree(@expression)
	end SMOKE_MATCH_FAILED
end if

if( matches(0).rm_so <> 0 or matches(0).rm_eo <> 13 or _
    matches(1).rm_so <> 0 or matches(1).rm_eo <> 9 or _
    matches(2).rm_so <> 10 or matches(2).rm_eo <> 13 ) then
	regfree(@expression)
	end SMOKE_MATCH_FAILED
end if

result = regexec(@expression, "freebasic", 0, 0, 0)
if( result <> REG_NOMATCH ) then
	regfree(@expression)
	end SMOKE_NO_MATCH_FAILED
end if

regfree(@expression)
end SMOKE_OK

'' end of regex-smoke.bas
