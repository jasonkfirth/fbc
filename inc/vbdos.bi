'
' Project: FreeBASIC DOS compatibility headers
' ------------------------------------------------
'
' File: vbdos.bi
'
' Purpose:
'
'     Provide the small Visual Basic for DOS interrupt helper interface
'     expected by older QB/VBDOS programs.
'
' Responsibilities:
'
'     - define the RegType and RegTypeX register records used by QB.BI
'       and VBDOS.BI
'     - map INTERRUPT and INTERRUPTX calls onto the FreeBASIC DOS runtime
'
' This file intentionally does NOT contain:
'
'     - general DOS API declarations
'     - keyboard, mouse, or graphics helper routines
'     - a non-DOS emulation layer
'

#ifndef __FB_VBDOS_BI__
#define __FB_VBDOS_BI__

type RegType
	ax    as integer
	bx    as integer
	cx    as integer
	dx    as integer
	bp    as integer
	si    as integer
	di    as integer
	flags as integer
end type

type RegTypeX
	ax    as integer
	bx    as integer
	cx    as integer
	dx    as integer
	bp    as integer
	si    as integer
	di    as integer
	flags as integer
	ds    as integer
	es    as integer
end type

#ifdef __FB_DOS__

declare sub fb_VBDOSInterrupt cdecl alias "fb_VBDOSInterrupt" _
	( _
		byval intnum as integer, _
		byref inreg as RegType, _
		byref outreg as RegType _
	)

declare sub fb_VBDOSInterruptX cdecl alias "fb_VBDOSInterruptX" _
	( _
		byval intnum as integer, _
		byref inreg as RegTypeX, _
		byref outreg as RegTypeX _
	)

#define INTERRUPT(intnum, inreg, outreg) fb_VBDOSInterrupt((intnum), inreg, outreg)
#define INTERRUPTX(intnum, inreg, outreg) fb_VBDOSInterruptX((intnum), inreg, outreg)

#else

#error vbdos.bi is only supported for the DOS target

#endif

#endif

' end of vbdos.bi
