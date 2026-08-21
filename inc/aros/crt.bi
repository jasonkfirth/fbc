''
'' FreeBASIC C runtime declarations for AROS POSIXC
'' ------------------------------------------------
''
'' File: crt.bi
''
'' Purpose:
''
''     Provide the complete top-level C runtime declaration set for AROS.
''
'' Responsibilities:
''
''     - route standard C declarations through the AROS replacement tree
''     - preserve the public include order used by the shared CRT umbrella
''
'' This file intentionally does NOT contain:
''
''     - declarations for other operating systems
''     - individual POSIXC structures or function prototypes
''
#ifndef __CRT_BI__
#define __CRT_BI__

#include once "crt/string.bi"
#include once "crt/math.bi"
#include once "crt/time.bi"
#include once "crt/wchar.bi"
#include once "crt/ctype.bi"
#include once "crt/stdlib.bi"
#include once "crt/stdio.bi"
#include once "crt/fcntl.bi"
#include once "crt/errno.bi"

#endif

'' end of crt.bi
