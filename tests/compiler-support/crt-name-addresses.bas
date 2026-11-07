'' Project: FreeBASIC CRT identifier regression tests
'' File: crt-name-addresses.bas
'' Purpose: Keep ordinary CRT external names unchanged when their C names are encoded.
'' Responsibilities: Retain address references for every member of the selected family.
'' This file intentionally does NOT call audible or process-state-changing APIs.
#lang "fb"
#include once "crt.bi"

public function fb_crt_name_address cdecl(byval entry_index as long) as any ptr
	select case entry_index
	case 0: return cptr(any ptr, procptr(_access))
	case 1: return cptr(any ptr, procptr(_strdup))
	case 2: return cptr(any ptr, procptr(_stricmp))
	case 3: return cptr(any ptr, procptr(_strnicmp))
	case 4: return cptr(any ptr, procptr(_errno))
	case 5: return cptr(any ptr, procptr(_i64toa))
	case 6: return cptr(any ptr, procptr(_itoa))
	case 7: return cptr(any ptr, procptr(_ultoa))
	case 8: return cptr(any ptr, procptr(_ui64toa))
	case 9: return cptr(any ptr, procptr(_itow))
	case 10: return cptr(any ptr, procptr(_ultow))
	case 11: return cptr(any ptr, procptr(_i64tow))
	case 12: return cptr(any ptr, procptr(_ui64tow))
	case 13: return cptr(any ptr, procptr(_chdir))
	case 14: return cptr(any ptr, procptr(_putenv))
	case 15: return cptr(any ptr, procptr(_rmdir))
	case 16: return cptr(any ptr, procptr(_beep))
#ifdef __FB_64BIT__
	case 17: return cptr(any ptr, procptr(__acrt_iob_func))
#endif
	case else: return 0
	end select
end function

'' end of crt-name-addresses.bas
