/'
	Project: FreeBASIC C backend regression
	File: c-format-results.bas
	Purpose: Check evaluated translation results used as native format strings.
	Responsibilities: Preserve translated text, call order and pointer identity,
	and expose separate native diagnostic controls through validation defines.
'/

#lang "fb"
#include once "crt/stdio.bi"

#ifdef FORMAT_WRONG_DECL
declare function formatGettext cdecl alias "gettext"(byval text as const zstring ptr) as any ptr
#elseif defined(FORMAT_CONST_RESULT)
declare function formatGettext cdecl alias "gettext"(byval text as const zstring ptr) as const zstring ptr
#else
declare function formatGettext cdecl alias "gettext"(byval text as const zstring ptr) as zstring ptr
#endif
declare function formatDgettext cdecl alias "dgettext"(byval domain as const zstring ptr, byval text as const zstring ptr) as zstring ptr
declare function formatDcgettext cdecl alias "dcgettext"(byval domain as const zstring ptr, byval text as const zstring ptr, byval category as long) as zstring ptr
declare sub resetFormatProbe cdecl alias "probe_format_reset"()
declare function formatSteps cdecl alias "probe_format_steps"() as long
declare function formatArgument cdecl alias "probe_format_argument"(byval value as long) as long
declare function formatResult cdecl alias "probe_format_result"() as zstring ptr
declare function observeFormatResult cdecl alias "probe_format_observe"(byval value as zstring ptr) as long

#ifdef FORMAT_LOCAL
function formatGettext cdecl(byval text as const zstring ptr) as zstring ptr
	return formatResult()
end function
#endif

dim as zstring * 64 bufferText
dim as long written
resetFormatProbe()
#ifdef FORMAT_DYNAMIC
	dim as zstring * 32 dynamicFormat = "source %d"
	written = snprintf(@bufferText, sizeof(bufferText), formatGettext(@dynamicFormat), formatArgument(7))
#elseif defined(FORMAT_SAVED)
	var savedFormat = formatGettext("source %d")
	written = snprintf(@bufferText, sizeof(bufferText), savedFormat, formatArgument(7))
#elseif defined(FORMAT_INDIRECT)
	dim as function cdecl(byval as const zstring ptr) as zstring ptr translate = @formatGettext
	written = snprintf(@bufferText, sizeof(bufferText), translate("source %d"), formatArgument(7))
#elseif defined(FORMAT_BAD_ARGUMENT)
	written = snprintf(@bufferText, sizeof(bufferText), formatGettext("source %d"), strptr("wrong"))
#else
	written = snprintf(@bufferText, sizeof(bufferText), formatGettext("source %d"), formatArgument(7))
#endif
assert(written = 12)
assert(bufferText = "translated 7")
assert(formatSteps() = 41)

resetFormatProbe()
written = snprintf(@bufferText, sizeof(bufferText), formatDgettext("domain", "source %d"), formatArgument(-3))
assert(written = 13)
assert(bufferText = "translated -3")
assert(formatSteps() = 42)

resetFormatProbe()
written = snprintf(@bufferText, sizeof(bufferText), formatDcgettext("domain", "source %d", 5), formatArgument(29))
assert(written = 13)
assert(bufferText = "translated 29")
assert(formatSteps() = 43)

resetFormatProbe()
assert(observeFormatResult(formatGettext("source %d")) = -1)
assert(formatSteps() = 1)
assert(formatResult() <> 0)
print "FORMAT_RESULTS_PASS"

/' end of c-format-results.bas '/
