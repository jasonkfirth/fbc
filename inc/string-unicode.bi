/'
    FreeBASIC Runtime Library
    File: string-unicode.bi
    Purpose: Declare wide and UTF-8 overloads of the optional string helpers.
    Responsibilities: Preserve mixed-argument resolution and result types.
    This file intentionally does NOT contain algorithms or byte declarations.
'/
#ifndef __STRING_UNICODE_BI__
#define __STRING_UNICODE_BI__

'' The aliases name runtime C functions, including overloaded declarations.
extern "c"
'' Select the target's runtime calling convention as well as its C aliases.
extern "rtlib"

'' Included by string.bi after the byte overloads and comparison constants.
'' Every argument combination is explicit; return type follows the source.
'' Wide and UTF-8 text helpers count Unicode scalars and preserve surrogate pairs.
'' FB_NO_USTRING retains compatibility with libraries owning that type name.
''
'' -------------------------------------------------------------------------
'' Wide string declarations
'' -------------------------------------------------------------------------

declare function Replace overload alias "fb_TextReplace_ssw" _
    ( byref expression as const string, byref find_text as const string, _
      byref replacement as const wstring, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as string

declare function Replace overload alias "fb_TextReplace_sws" _
    ( byref expression as const string, byref find_text as const wstring, _
      byref replacement as const string, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as string

declare function Replace overload alias "fb_TextReplace_sww" _
    ( byref expression as const string, byref find_text as const wstring, _
      byref replacement as const wstring, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as string

declare function Replace overload alias "fb_TextReplace_wss" _
    ( byref expression as const wstring, byref find_text as const string, _
      byref replacement as const string, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as wstring

declare function Replace overload alias "fb_TextReplace_wsw" _
    ( byref expression as const wstring, byref find_text as const string, _
      byref replacement as const wstring, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as wstring

declare function Replace overload alias "fb_TextReplace_wws" _
    ( byref expression as const wstring, byref find_text as const wstring, _
      byref replacement as const string, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as wstring

declare function Replace overload alias "fb_TextReplace_www" _
    ( byref expression as const wstring, byref find_text as const wstring, _
      byref replacement as const wstring, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as wstring

declare function StrComp overload alias "fb_TextComp_sw" _
    ( byref string1 as const string, byref string2 as const wstring, _
      byval compare as long = fbBinaryCompare ) as long

declare function StrComp overload alias "fb_TextComp_ws" _
    ( byref string1 as const wstring, byref string2 as const string, _
      byval compare as long = fbBinaryCompare ) as long

declare function StrComp overload alias "fb_TextComp_ww" _
    ( byref string1 as const wstring, byref string2 as const wstring, _
      byval compare as long = fbBinaryCompare ) as long

declare function StrReverse overload alias "fb_TextReverse_w" _
    ( byref expression as const wstring ) as wstring

declare function Format overload alias "fb_TextFormat_w" _
    ( byval value as double, byref mask as const wstring ) as wstring

#if defined(__FB_HAS_USTRING__) and not defined(FB_NO_USTRING)
'' -------------------------------------------------------------------------
'' UTF-8 and mixed string declarations
'' -------------------------------------------------------------------------
declare function Replace overload alias "fb_TextReplace_ssu" _
    ( byref expression as const string, byref find_text as const string, _
      byref replacement as const ustring, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as string

declare function Replace overload alias "fb_TextReplace_swu" _
    ( byref expression as const string, byref find_text as const wstring, _
      byref replacement as const ustring, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as string

declare function Replace overload alias "fb_TextReplace_sus" _
    ( byref expression as const string, byref find_text as const ustring, _
      byref replacement as const string, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as string

declare function Replace overload alias "fb_TextReplace_suw" _
    ( byref expression as const string, byref find_text as const ustring, _
      byref replacement as const wstring, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as string

declare function Replace overload alias "fb_TextReplace_suu" _
    ( byref expression as const string, byref find_text as const ustring, _
      byref replacement as const ustring, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as string

declare function Replace overload alias "fb_TextReplace_wsu" _
    ( byref expression as const wstring, byref find_text as const string, _
      byref replacement as const ustring, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as wstring

declare function Replace overload alias "fb_TextReplace_wwu" _
    ( byref expression as const wstring, byref find_text as const wstring, _
      byref replacement as const ustring, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as wstring

declare function Replace overload alias "fb_TextReplace_wus" _
    ( byref expression as const wstring, byref find_text as const ustring, _
      byref replacement as const string, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as wstring

declare function Replace overload alias "fb_TextReplace_wuw" _
    ( byref expression as const wstring, byref find_text as const ustring, _
      byref replacement as const wstring, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as wstring

declare function Replace overload alias "fb_TextReplace_wuu" _
    ( byref expression as const wstring, byref find_text as const ustring, _
      byref replacement as const ustring, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as wstring

declare function Replace overload alias "fb_TextReplace_uss" _
    ( byref expression as const ustring, byref find_text as const string, _
      byref replacement as const string, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as ustring

declare function Replace overload alias "fb_TextReplace_usw" _
    ( byref expression as const ustring, byref find_text as const string, _
      byref replacement as const wstring, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as ustring

declare function Replace overload alias "fb_TextReplace_usu" _
    ( byref expression as const ustring, byref find_text as const string, _
      byref replacement as const ustring, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as ustring

declare function Replace overload alias "fb_TextReplace_uws" _
    ( byref expression as const ustring, byref find_text as const wstring, _
      byref replacement as const string, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as ustring

declare function Replace overload alias "fb_TextReplace_uww" _
    ( byref expression as const ustring, byref find_text as const wstring, _
      byref replacement as const wstring, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as ustring

declare function Replace overload alias "fb_TextReplace_uwu" _
    ( byref expression as const ustring, byref find_text as const wstring, _
      byref replacement as const ustring, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as ustring

declare function Replace overload alias "fb_TextReplace_uus" _
    ( byref expression as const ustring, byref find_text as const ustring, _
      byref replacement as const string, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as ustring

declare function Replace overload alias "fb_TextReplace_uuw" _
    ( byref expression as const ustring, byref find_text as const ustring, _
      byref replacement as const wstring, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as ustring

declare function Replace overload alias "fb_TextReplace_uuu" _
    ( byref expression as const ustring, byref find_text as const ustring, _
      byref replacement as const ustring, byval start as integer = 1, _
      byval count as integer = -1, byval compare as long = fbBinaryCompare ) as ustring

declare function StrComp overload alias "fb_TextComp_su" _
    ( byref string1 as const string, byref string2 as const ustring, _
      byval compare as long = fbBinaryCompare ) as long

declare function StrComp overload alias "fb_TextComp_wu" _
    ( byref string1 as const wstring, byref string2 as const ustring, _
      byval compare as long = fbBinaryCompare ) as long

declare function StrComp overload alias "fb_TextComp_us" _
    ( byref string1 as const ustring, byref string2 as const string, _
      byval compare as long = fbBinaryCompare ) as long

declare function StrComp overload alias "fb_TextComp_uw" _
    ( byref string1 as const ustring, byref string2 as const wstring, _
      byval compare as long = fbBinaryCompare ) as long

declare function StrComp overload alias "fb_TextComp_uu" _
    ( byref string1 as const ustring, byref string2 as const ustring, _
      byval compare as long = fbBinaryCompare ) as long

declare function StrReverse overload alias "fb_UStrReverse" _
    ( byref expression as const ustring ) as ustring

declare function Format overload alias "fb_UStrFormat" _
    ( byval value as double, byref mask as const ustring ) as ustring

#endif

end extern
end extern

#endif

' end of string-unicode.bi
