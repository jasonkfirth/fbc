/'
	Project: FreeBASIC C runtime declaration regression
	File: crt-format-declarations.bas
	Purpose: Expose bounded formatter signatures to an independent C header.
	Responsibilities: Check const format pointers and native va_list signatures.
	This file intentionally does NOT contain runtime startup or formatter execution.
'/

#lang "fb"
#include once "crt/stdio.bi"

type SnprintfProc as function cdecl(byval as zstring ptr, byval as size_t, byval as const zstring ptr, ...) as long

function snprintfAddress cdecl alias "probe_snprintf_address"() as SnprintfProc
	return @snprintf
end function

#ifndef __FB_DOS__
type VsnprintfProc as function cdecl(byval as zstring ptr, byval as size_t, byval as const zstring ptr, byval as va_list) as long

function vsnprintfAddress cdecl alias "probe_vsnprintf_address"() as VsnprintfProc
	return @vsnprintf
end function
#endif

/' end of crt-format-declarations.bas '/
