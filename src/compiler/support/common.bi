'' Project: FreeBASIC compiler - compiler support services
'' -----------------------------------------
''
'' File: support/common.bi
''
'' Purpose:
''
''     Define shared internal constants and checked allocation interfaces.
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

#ifndef __COMMON_BI__
#define __COMMON_BI__

#ifdef BOOLEAN
'' fbc sources do not use the boolean datatype and therefore
'' probably shouldn't use the intrinsic false/true constants,
'' so undef the built-in false/true constants and #define
'' the values expected (jeffm - july 2015)
#undef FALSE
#undef TRUE
#endif
#define FALSE 0
#define TRUE (-1)
#define NULL 0
#include once "support/allocate.bi"

#endif

'' end of support/common.bi
