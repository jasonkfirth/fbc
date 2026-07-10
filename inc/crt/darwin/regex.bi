''
'' FreeBASIC Darwin CRT bindings
'' -----------------------------
''
'' File: crt/darwin/regex.bi
''
'' Purpose:
''
''     Expose the POSIX regular-expression API provided by macOS libSystem.
''
'' Responsibilities:
''
''     - define Darwin regex flags and error codes
''     - mirror the public regex_t and regmatch_t ABI layouts
''     - declare the standard compile, execute, error, and cleanup functions
''
'' This file intentionally does NOT contain:
''
''     - the incompatible glibc regex implementation details
''     - Darwin's non-POSIX wide-character regex extensions
''

#ifndef __crt_darwin_regex_bi__
#define __crt_darwin_regex_bi__

#include once "crt/stddef.bi"
#include once "crt/sys/types.bi"

extern "C"

const _REGEX_H = 1

type regoff_t as off_t

type regex_t
	re_magic as long
	re_nsub as size_t
	re_endp as const zstring ptr
	re_g as any ptr
end type

type regmatch_t
	rm_so as regoff_t
	rm_eo as regoff_t
end type

const REG_BASIC = 0
const REG_EXTENDED = &o0001
const REG_ICASE = &o0002
const REG_NOSUB = &o0004
const REG_NEWLINE = &o0010
const REG_NOSPEC = &o0020
const REG_LITERAL = REG_NOSPEC
const REG_PEND = &o0040
const REG_MINIMAL = &o0100
const REG_UNGREEDY = REG_MINIMAL
const REG_DUMP = &o0200
const REG_ENHANCED = &o0400

const REG_ENOSYS = -1

type reg_errcode_t as long
enum
	REG_NOERROR = 0
	REG_NOMATCH = 1
	REG_BADPAT = 2
	REG_ECOLLATE = 3
	REG_ECTYPE = 4
	REG_EESCAPE = 5
	REG_ESUBREG = 6
	REG_EBRACK = 7
	REG_EPAREN = 8
	REG_EBRACE = 9
	REG_BADBR = 10
	REG_ERANGE = 11
	REG_ESPACE = 12
	REG_BADRPT = 13
	REG_EMPTY = 14
	REG_ASSERT = 15
	REG_INVARG = 16
	REG_ILLSEQ = 17
	REG_BADMAX = 18
end enum

const REG_NOTBOL = &o00001
const REG_NOTEOL = &o00002
const REG_STARTEND = &o00004
const REG_TRACE = &o00400
const REG_LARGE = &o01000
const REG_BACKR = &o02000
const REG_BACKTRACKING_MATCHER = REG_BACKR

declare function regcomp(byval preg as regex_t ptr, byval pattern as const zstring ptr, byval cflags as long) as long
declare function regexec(byval preg as const regex_t ptr, byval string_ as const zstring ptr, byval nmatch as size_t, byval pmatch as regmatch_t ptr, byval eflags as long) as long
declare function regerror(byval errcode as long, byval preg as const regex_t ptr, byval errbuf as zstring ptr, byval errbuf_size as size_t) as size_t
declare sub regfree(byval preg as regex_t ptr)

end extern

#endif

'' end of crt/darwin/regex.bi
