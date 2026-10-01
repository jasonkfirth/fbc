/'
    FreeBASIC Runtime Library
    File: string.bi
    Purpose: Declare optional string formatting and byte-string helpers.
    Responsibilities: Public signatures and explicit comparison modes.
    This file does not contain compiler keywords or Unicode collation rules.
'/
#ifndef __STRING_BI__
#define __STRING_BI__

const fbBinaryCompare = 0
const fbTextCompare = 1

'' These overloads select runtime C entry points with explicit aliases.
'' Keep their external names independent of BASIC overload mangling.
extern "c"

#if __FB_LANG__ <> "qb"
extern "rtlib"
#endif

declare function format overload alias "fb_StrFormat" _
          ( byval value as double, _
            byref mask as const string="" ) as string

declare function StrComp overload alias "fb_StrComp" _
          ( byref string1 as const string, _
            byref string2 as const string, _
            byval compare as long = fbBinaryCompare ) as long

'' Indices use the native pointer width, including in the QB dialect.
#if __FB_LANG__ = "qb"
  #ifdef __FB_64BIT__
    #define __FB_STRING_INDEX__ __longint
  #else
    #define __FB_STRING_INDEX__ long
  #endif
#else
  #define __FB_STRING_INDEX__ integer
#endif

declare function Replace overload alias "fb_StrReplace" _
          ( byref expression as const string, _
            byref find_text as const string, _
            byref replacement as const string, _
            byval start as __FB_STRING_INDEX__ = 1, _
            byval count as __FB_STRING_INDEX__ = -1, _
            byval compare as long = fbBinaryCompare ) as string

declare function StrReverse overload alias "fb_StrReverse" _
          ( byref expression as const string ) as string

end extern

#if __FB_LANG__ <> "qb"
#include once "string-unicode.bi"
#endif

#if __FB_LANG__ <> "qb"
end extern
#endif

#undef __FB_STRING_INDEX__

#endif

' end of string.bi
