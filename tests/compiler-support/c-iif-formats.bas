/'
	Project: FreeBASIC C backend regression
	File: c-iif-formats.bas
	Purpose: Preserve native format checks across conditional pointer results.
	Responsibilities: Exercise literal and translated choices with one evaluation
	of the condition and selected translation, including native plural counts.
	The paired C fixture owns reference output and observation counters.
'/
#lang "fb"
#include once "crt/stdio.bi"

#if defined(__FB_64BIT__) and defined(__FB_UNIX__)
type branch_count_t as uinteger alias "long"
#else
type branch_count_t as ulong alias "long"
#endif

declare function branchSelect cdecl alias "probe_branch_select"(byval choice as long) as long
declare function branchGettext cdecl alias "dcgettext"(byval domain as const zstring ptr, byval text as const zstring ptr, byval category as long) as zstring ptr
declare function branchPlural cdecl alias "ngettext"(byval one as const zstring ptr, byval many as const zstring ptr, byval count as branch_count_t) as zstring ptr
declare function branchInput cdecl alias "probe_branch_input"() as const zstring ptr
declare function branchCases cdecl alias "probe_branch_cases"() as long
declare sub branchMutate cdecl alias "probe_branch_mutate"(byval text as zstring ptr)

#ifdef IIF_FORMAT_BAD_LEFT
#define branch_left strptr(!"left %s")
#else
#define branch_left strptr(!"left %d")
#endif
#ifdef IIF_FORMAT_BAD_RIGHT
#define branch_right strptr(!"right %s")
#else
#define branch_right strptr(!"right %d")
#endif

function branchLiteral cdecl alias "probe_branch_literal"(byval destination as zstring ptr, byval capacity as size_t, byval value as long, byval choice as long) as long
#ifdef IIF_FORMAT_MUTABLE_LITERAL
	branchMutate(branch_left)
#endif
#ifdef IIF_FORMAT_DYNAMIC
	return snprintf(destination, capacity, iif(branchSelect(choice), branchInput(), branch_right), value)
#else
	return snprintf(destination, capacity, iif(branchSelect(choice), branch_left, branch_right), value)
#endif
end function

function branchUnused cdecl alias "probe_branch_unused"(byval destination as zstring ptr, byval capacity as size_t, byval value as long, byval choice as long) as long
	return snprintf(destination, capacity, iif(branchSelect(choice), strptr(!"value %d"), strptr(!"done")), value)
end function

function branchTranslated cdecl alias "probe_branch_translated"(byval destination as zstring ptr, byval capacity as size_t, byval value as long, byval choice as long) as long
	return snprintf(destination, capacity, iif(branchSelect(choice), branchGettext(0, branch_left, 5), branchGettext(0, branch_right, 5)), value)
end function

#ifdef IIF_FORMAT_BAD_PLURAL_LEFT
#define plural_left strptr(!"left many %s")
#else
#define plural_left strptr(!"left many %d")
#endif
#ifdef IIF_FORMAT_BAD_PLURAL_RIGHT
#define plural_right strptr(!"right many %s")
#else
#define plural_right strptr(!"right many %d")
#endif

function branchPluralCase cdecl alias "probe_branch_plural"(byval destination as zstring ptr, byval capacity as size_t, byval value as long, byval choice as long, byval count as branch_count_t) as long
	return snprintf(destination, capacity, iif(branchSelect(choice), branchPlural(branch_left, plural_left, count), branchPlural(branch_right, plural_right, count)), value)
end function

#ifndef IIF_FORMAT_EXPORTS
assert(branchCases() = -1)
print "IIF_FORMATS_PASS"
#endif

/' end of c-iif-formats.bas '/
