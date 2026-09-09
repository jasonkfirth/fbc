/'
    FreeBASIC Runtime Library
    File: vbcompat.bi
    Purpose: Collect optional Visual Basic compatibility declarations.
    Responsibilities: Include runtime helpers and expose VB constant aliases.
    This file does not implement a VB dialect, Variant, or an object model.
'/
#ifndef __VBCOMPAT_BI__
#define __VBCOMPAT_BI__

''
const vbFalse = 0
const vbTrue = not vbFalse

'' DATE/TIME
#include once "datetime.bi"

#ifndef vbUseSystem
const vbUseSystem       = fbUseSystem
#endif

const vbFirstJan1       = fbFirstJan1
const vbFirstFourDays   = fbFirstFourDays
const vbFirstFullWeek	= fbFirstFullWeek

const vbSunday          = fbSunday
const vbMonday          = fbMonday
const vbTuesday         = fbTuesday
const vbWednesday       = fbWednesday
const vbThursday        = fbThursday
const vbFriday          = fbFriday
const vbSaturday        = fbSaturday

'' STRING
#include once "string.bi"

const vbBinaryCompare = fbBinaryCompare
const vbTextCompare = fbTextCompare

'' DIR
#include once "dir.bi"

const vbReadOnly		= fbReadOnly
const vbHidden			= fbHidden
const vbSystem			= fbSystem
const vbDirectory		= fbDirectory
const vbArchive			= fbArchive
'' Keep the historical FreeBASIC alias. Use fbFileAttrNormal with SetAttr.
const vbNormal			= fbNormal

'' CHAR
#if __FB_LANG__ = "qb"
	#define __FB_VBCHR__ chr$
	#define vbNullChar chr$( 0 )
#else
	#define __FB_VBCHR__ chr
	#define vbNullChar chr( 0 )
#endif
const vbBack			= __FB_VBCHR__( 08 )
const vbCr				= __FB_VBCHR__( 13 )
const vbCrLf			= __FB_VBCHR__( 13, 10 )
const vbLf				= __FB_VBCHR__( 10 )
#if defined(__FB_DOS__) or defined(__FB_WIN32__)
const vbNewLine			= vbCrLf
#else
const vbNewLine			= vbLf
#endif

const vbNullString		= ""
const vbFormFeed		= __FB_VBCHR__( 12 )
const vbTab				= __FB_VBCHR__( 09 )
const vbVerticalTab		= __FB_VBCHR__( 11 )
#undef __FB_VBCHR__

'' FILE
#include once "file.bi"

#endif

' end of vbcompat.bi
