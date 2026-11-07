'' Project: FreeBASIC variant helper library
'' File: variant-string.bas
'' Purpose: Convert the library's narrow text input into an owned Unicode BSTR.
'' Responsibilities: Preserve CP_ACP interpretation and transfer complete values.
'' This file intentionally does NOT perform VARIANT operations or retain input.

#include once "variant.bi"
#include once "intern.bi"

function hVarAllocBstrFromZstring( byval rhs as zstring ptr ) as BSTR

	if( rhs = NULL ) then return NULL

	'' With a null-terminated input, Windows counts the terminator. BSTR's
	'' stored length excludes it; SysAllocStringLen reserves it separately.
	var wlen = MultiByteToWideChar(CP_ACP, 0, rhs, -1, NULL, 0)
	if( wlen <= 0 ) then return NULL

	var text = SysAllocStringLen(NULL, wlen - 1)
	if( text = NULL ) then return NULL

	if( MultiByteToWideChar(CP_ACP, 0, rhs, -1, text, wlen) = 0 ) then
		SysFreeString(text)
		return NULL
	end if

	return text

end function

'' end of variant-string.bas
