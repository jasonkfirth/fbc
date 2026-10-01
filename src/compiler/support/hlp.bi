'' Project: FreeBASIC compiler - compiler support services
'' -----------------------------------------
''
'' File: support/hlp.bi
''
'' Purpose:
''
''     Declare shared pathname, numeric formatting, and identifier utilities.
''
'' Responsibilities:
''
''     - provide the shared utility or contract described below
''     - keep utility behavior independent of parser state
''
'' This file intentionally does NOT contain:
''
''     - grammar rules or backend instruction selection
''

#ifndef __HELP_BI__
#define __HELP_BI__

#ifndef INVALID
const INVALID = -1
#endif

''
'' helper module protos
''

declare function hMakeProfileLabelName _
	( _
	) as zstring ptr

declare function hFBrelop2IRrelop _
	( _
		byval tk as integer _
	) as integer

declare function hFileExists _
	( _
		byval filename as zstring ptr _
	) as integer

declare sub hClearName _
	( _
		byval src as zstring ptr _
	)

declare sub hUcase _
	( _
		byval src as const zstring ptr, _
		byval dst as zstring ptr _
	)

declare function hStripUnderscore _
	( _
		byval symbol as zstring ptr _
	) as string

declare function hStripExt( byref path as string ) as string

declare function hStripPath _
	( _
		byval filename as zstring ptr _
	) as string

declare function hStripFilename _
	( _
		byval filename as zstring ptr _
	) as string

declare function hGetFileExt _
	( _
		byval fname as zstring ptr _
	) as string

declare sub hReplaceSlash( byval s as zstring ptr, byval char as integer )

declare function pathNormalizeHost( byref path as string ) as string
declare function pathStripDiv( byref path as string ) as string
declare function pathIsAbsolute( byval path as zstring ptr ) as integer

declare function hFloatToHex _
	( _
		byval value as double, _
		byval dtype as integer _
	) as string

declare function hFloatToHex_C99 _
	( _
		byval value as double _
	) as string

declare function hCheckFileFormat _
	( _
		byval f as integer _
	) as integer

declare function hCurDir( ) as string
declare function pathStripCurdir( byref path as string ) as string

declare function hHexUInt _
	( _
		byval value as uinteger _
	) as zstring ptr

declare function hIsValidSymbolName( byval sym as zstring ptr ) as integer

declare function strUnquote(byref s as string) as string

#include once "support/strings/hlp-str.bi"

#define hIsCharLower(_c) (((_c) >= asc("a")) andalso ((_c) <= asc("z")))
#define hIsCharUpper(_c) (((_c) <= asc("Z")) andalso ((_c) >= asc("A")))
#define hIsChar(_c) (hIsCharLower((_c)) orelse hIsCharUpper((_c)))
#define hIsCharNumeric(_c) (((_c) <= asc("9")) andalso ((_c) >= asc("0")))

#include once "support/numeric/fp-policy.bi"

#endif ''__HELP_BI__

'' end of support/hlp.bi
