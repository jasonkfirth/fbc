/'
	Project: FreeBASIC C backend regression
	File: c-format-locals.bas
	Purpose: Retain native format checking through private pointer stores.
	Responsibilities: Check direct and replacement literals, pointer identity
	and the diagnostic boundaries for aliases, calls and control-flow joins.
	The paired C fixture owns native value comparisons and observation state.
'/
#lang "fb"
#include once "crt/stdio.bi"

declare function localFormatObserve cdecl alias "probe_local_format_observe"(byval actual as const zstring ptr, byval expected as const zstring ptr) as long
declare sub localFormatBarrier cdecl alias "probe_local_format_barrier"()

declare function localFormatArgument cdecl alias "probe_local_format_argument"(byval value as long) as long

declare function localFormatCheckpoint cdecl alias "setjmp"(byval buffer as any ptr) as long
declare sub localFormatReplace cdecl alias "probe_local_format_replace"(byref value as const zstring ptr)
declare function localFormatInput cdecl alias "probe_local_format_input"() as const zstring ptr
declare function localFormatCases cdecl alias "probe_local_format_cases"() as long
declare function localForeignLegacy cdecl alias "fb_FormatValue"(byval a as const zstring ptr, byval b as const zstring ptr) as const zstring ptr
declare function localForeignReserved cdecl alias "fb_c_format_value"(byval a as const zstring ptr, byval b as const zstring ptr) as const zstring ptr

#if defined(__FB_64BIT__) and defined(__FB_UNIX__)
type local_count_t as uinteger alias "long"
#else
type local_count_t as ulong alias "long"
#endif
declare function localGettext cdecl alias "gettext"(byval text as const zstring ptr) as zstring ptr
declare function localNgettext cdecl alias "ngettext"(byval one as const zstring ptr, byval many as const zstring ptr, byval count as local_count_t) as zstring ptr
declare function localTranslationObserve cdecl alias "probe_local_translation_observe"(byval text as const zstring ptr) as long
declare sub localTranslationMutate cdecl alias "probe_local_translation_mutate"()

function localFormatDirect cdecl alias "probe_local_format_direct"(byval destination as zstring ptr, byval capacity as size_t, byval value as long) as long
#ifdef FORMAT_LOCAL_STATIC
	static as const zstring ptr formatText = strptr(!"%d")
#else
	dim as const zstring ptr formatText = strptr(!"%d")
#endif
#ifdef FORMAT_LOCAL_DYNAMIC
	formatText = localFormatInput()
#elseif defined(FORMAT_LOCAL_BRANCH)
	if( value <> 0 ) then
		formatText = strptr(!"%d")
	else
		formatText = strptr(!"%d")
	end if
#elseif defined(FORMAT_LOCAL_ESCAPE)
	localFormatReplace(formatText)
#elseif defined(FORMAT_LOCAL_AFTER_CALL) or defined(FORMAT_LOCAL_BAD_AFTER_CALL)
	localFormatBarrier()
#elseif defined(FORMAT_LOCAL_NONLOCAL)
	localFormatCheckpoint(0)
#elseif defined(FORMAT_LOCAL_ALIAS_WRITE)
	dim as const zstring ptr ptr aliasSlot = @formatText
	*aliasSlot = localFormatInput()
#elseif defined(FORMAT_LOCAL_COPY)
	dim as const zstring ptr otherFormat = strptr(!"%d")
	formatText = otherFormat
#endif
#if defined(FORMAT_LOCAL_BAD_ARGUMENT) or defined(FORMAT_LOCAL_BAD_AFTER_CALL)
	return snprintf(destination, capacity, formatText, strptr("wrong"))
#else
	return snprintf(destination, capacity, formatText, value)
#endif
end function

function localFormatAcrossCall cdecl alias "probe_local_format_across_call"(byval destination as zstring ptr, byval capacity as size_t, byval value as long) as long
	dim as const zstring ptr formatText = strptr(!"across %d")
	localFormatBarrier()
	return snprintf(destination, capacity, formatText, localFormatArgument(value))
end function

function localFormatReassign cdecl alias "probe_local_format_reassign"(byval destination as zstring ptr, byval capacity as size_t, byval value as long) as long
	dim as const zstring ptr formatText = strptr(!"unused %s")
	formatText = strptr(!"value %d")
	return snprintf(destination, capacity, formatText, value)
end function

function localFormatSprintf cdecl alias "probe_local_format_sprintf"(byval destination as zstring ptr, byval value as long) as long
	dim as const zstring ptr formatText = strptr(!"sprintf %d")
	return sprintf(destination, formatText, value)
end function

function localFormatIdentity cdecl alias "probe_local_format_identity"(byval destination as zstring ptr, byval capacity as size_t, byval value as long) as long
	dim as const zstring ptr formatText = strptr(!"identity %d")
	var written = snprintf(destination, capacity, formatText, value)
	if( localFormatObserve(formatText, strptr(!"identity %d")) = 0 ) then return -1
	return written
end function

function localFormatForeign cdecl alias "probe_local_format_foreign"(byval destination as zstring ptr, byval capacity as size_t, byval value as long, byval selector as long) as const zstring ptr
	dim as const zstring ptr formatText = strptr(!"%d")
	snprintf(destination, capacity, formatText, value)
	if( selector = 0 ) then return localForeignLegacy(strptr(!"source"), strptr(!"format"))
	return localForeignReserved(strptr(!"source"), strptr(!"format"))
end function

function localFormatTranslated cdecl alias "probe_local_format_translated"(byval destination as zstring ptr, byval capacity as size_t, byval value as long) as long
	dim as zstring ptr translatedText
	dim as const zstring ptr formatText
#ifdef FORMAT_TRANSLATED_DYNAMIC
	dim as zstring * 32 sourceText = "source %d"
	translatedText = localGettext(@sourceText)
#else
	translatedText = localGettext("source %d")
#endif
	formatText = translatedText
#ifdef FORMAT_TRANSLATED_AFTER_CALL
	localFormatBarrier()
#elseif defined(FORMAT_TRANSLATED_MUTATE)
	'' The observer already owns an alias to the translation buffer.
	localTranslationMutate()
#elseif defined(FORMAT_TRANSLATED_ESCAPE)
	localFormatReplace(formatText)
#elseif defined(FORMAT_TRANSLATED_BRANCH)
	if( value <> 0 ) then formatText = translatedText
#elseif defined(FORMAT_TRANSLATED_ALIAS_WRITE)
	dim as const zstring ptr ptr aliasSlot = @formatText
	*aliasSlot = localFormatInput()
#elseif defined(FORMAT_TRANSLATED_INTEGER)
	dim as uinteger addressBits = cuint(translatedText)
	formatText = cptr(const zstring ptr, addressBits)
#endif
#ifdef FORMAT_TRANSLATED_BAD_ARGUMENT
	return snprintf(destination, capacity, formatText, strptr("wrong"))
#elseif defined(FORMAT_TRANSLATED_ARGUMENT_CALL)
	return snprintf(destination, capacity, formatText, localFormatArgument(value))
#else
	var written = snprintf(destination, capacity, formatText, value)
	if( localTranslationObserve(translatedText) = 0 ) then return -1
	return written
#endif
end function

function localFormatTranslatedPlural cdecl alias "probe_local_format_translated_plural"(byval destination as zstring ptr, byval capacity as size_t, byval value as long, byval count as local_count_t) as long
	dim as zstring ptr translatedText
	dim as const zstring ptr formatText
#ifdef FORMAT_TRANSLATED_BAD_ONE
	translatedText = localNgettext("one %s", "many %d", count)
#elseif defined(FORMAT_TRANSLATED_BAD_MANY)
	translatedText = localNgettext("one %d", "many %s", count)
#else
	translatedText = localNgettext("one %d", "many %d", count)
#endif
	formatText = translatedText
	var written = snprintf(destination, capacity, formatText, value)
	if( localTranslationObserve(translatedText) = 0 ) then return -1
	return written
end function

#ifndef FORMAT_LOCAL_EXPORTS
assert(localFormatCases() = -1)
print "FORMAT_LOCALS_PASS"
#endif

/' end of c-format-locals.bas '/
