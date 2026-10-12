'' FreeBASIC PCRE examples
'' File: CRegex.bi
'' Purpose: Declare an owning regular-expression wrapper.
'' Responsibilities: Search iteration and checked capture access.
'' This file contains no PCRE engine implementation.
'' Each object has one thread owner and must not be copied.

#ifndef FB_EXAMPLES_REGEX_PCRE_CREGEX_BI
#define FB_EXAMPLES_REGEX_PCRE_CREGEX_BI

#inclib "CRegex"

#include once "pcre.bi"

type CRegex
	enum options
		NONE			= &h0000
		CASELESS		= &h0001
		MULTILINE		= &h0002
		DOTALL			= &h0004
		EXTENDED		= &h0008
		ANCHORED		= &h0010
		DOLLAR_ENDONLY	= &h0020
		EXTRA_			= &h0040
		NOTBOL			= &h0080
		NOTEOL			= &h0100
		UNGREEDY		= &h0200
		NOTEMPTY		= &h0400
		UTF8			= &h0800
		NO_AUTO_CAPTURE	= &h1000
		NO_UTF8_CHECK	= &h2000
		AUTO_CALLOUT	= &h4000
		PARTIAL			= &h8000
	end enum

	declare constructor _
		( _
			byval pattern as zstring ptr, _
			byval opt as options = NONE _
		)

	declare function getMaxMatches() as integer
	declare function getErrorCode() as long

	declare function search _
		( _
			byval subject as zstring ptr, _
			byval lgt as integer = -1, _
			byval opt as options = NONE _
		) as integer

	declare function searchNext _
		( _
			byval opt as options = NONE _
		) as integer

	'' Returned capture pointers belong to this object until its next search.
	declare function getStr _
		( _
			byval i as integer = 1 _
		) as zstring ptr

	declare function getOfs _
		( _
			byval i as integer = 1 _
		) as integer

	declare function getLen _
		( _
			byval i as integer = 1 _
		) as integer

	declare destructor()

private:
	'' Native patterns and captures have one owner. Prevent implicit shallow copies.
	declare constructor(byref other as const CRegex)
	declare operator let(byref other as const CRegex)
	declare sub clearSubstrlist()

	as pcre ptr reg
	as pcre_extra ptr extra
	as long ptr vectb
	as zstring ptr subject
	as string owned_subject
	as long error_code
	as long matched_count
	as boolean has_match
	as long sublen
	as long substrcnt
	as const zstring ptr ptr substrlist
end type

#endif

'' end of CRegex.bi
