/'
	Project: FreeBASIC C backend regression
	File: c-plural-format-results.bas
	Purpose: Exercise evaluated plural translations used as native formats.
	Responsibilities: Check translated output, argument evaluation and storage identity.
	Native diagnostic controls belong to the accompanying Python driver.
'/

#lang "fb"
#include once "crt/stdio.bi"

#if defined(__FB_64BIT__) and defined(__FB_UNIX__)
type plural_count_t as uinteger alias "long"
#else
type plural_count_t as ulong alias "long"
#endif

declare function pluralGettext cdecl alias "ngettext"(byval one as const zstring ptr, byval many as const zstring ptr, byval count as plural_count_t) as zstring ptr
declare function pluralDgettext cdecl alias "dngettext"(byval domain as const zstring ptr, byval one as const zstring ptr, byval many as const zstring ptr, byval count as plural_count_t) as zstring ptr
declare function pluralDcgettext cdecl alias "dcngettext"(byval domain as const zstring ptr, byval one as const zstring ptr, byval many as const zstring ptr, byval count as plural_count_t, byval category as long) as zstring ptr
declare sub resetPluralProbe cdecl alias "probe_plural_reset"(byval count as plural_count_t)
declare function pluralCount cdecl alias "probe_plural_count"(byval count as plural_count_t) as plural_count_t
declare function pluralArgument cdecl alias "probe_plural_argument"() as long
declare function checkPluralText cdecl alias "probe_plural_text"(byval text as const zstring ptr, byval written as long) as long
declare function checkPluralIdentity cdecl alias "probe_plural_identity"(byval text as zstring ptr) as long

function pluralFormatCase cdecl alias "plural_format_case"(byval kind as long, byval count as plural_count_t) as long

	dim as zstring * 32 text
	dim as long written
	resetPluralProbe(count)
	select case kind
	case 0
		written = snprintf(@text, sizeof(text), pluralGettext("source %d", "sources %d", pluralCount(count)), pluralArgument())
	case 1
		written = snprintf(@text, sizeof(text), pluralDgettext("domain", "source %d", "sources %d", pluralCount(count)), pluralArgument())
	case 2
		written = snprintf(@text, sizeof(text), pluralDcgettext("domain", "source %d", "sources %d", pluralCount(count), 5), pluralArgument())
	case else
		return 0
	end select
	return checkPluralText(@text, written)
end function

function pluralIdentityCase cdecl alias "plural_identity_case"(byval kind as long, byval count as plural_count_t) as long

	resetPluralProbe(count)
	select case kind
	case 0
		return checkPluralIdentity(pluralGettext("source %d", "sources %d", pluralCount(count)))
	case 1
		return checkPluralIdentity(pluralDgettext("domain", "source %d", "sources %d", pluralCount(count)))
	case 2
		return checkPluralIdentity(pluralDcgettext("domain", "source %d", "sources %d", pluralCount(count), 5))
	end select
	return 0
end function

#ifndef PLURAL_FORMAT_EXPORTS
for kind as long = 0 to 2
	for i as long = 0 to 3
		var count = cast(plural_count_t, i)
		if( i = 3 ) then count = not cast(plural_count_t, 0)
		assert(pluralFormatCase(kind, count) <> 0)
		assert(pluralIdentityCase(kind, count) <> 0)
	next
next
print "PLURAL_FORMAT_RESULTS_PASS"
#endif

/' end of c-plural-format-results.bas '/
