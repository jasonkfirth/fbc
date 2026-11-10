' Project: FreeBASIC compiler tests
' File: tests/crt/clong-variadic-constant.bas
' Purpose: Preserve CLong and CULong types for constants passed to C varargs.
' Responsibilities: Compile checked %jd and %ju calls through the GCC backend.
' This file intentionally does NOT contain: C backend code or printf support.
' TEST_MODE : COMPILE_AND_RUN_OK

#cmdline "-gen gcc"
#cmdline "-Wc -Werror=format"
#cmdline "-restart"

#lang "fb"
#include once "../../inc/crt/long.bi"
#include once "../../inc/crt/stdio.bi"

#if defined(__FB_64BIT__) and not defined(__FB_WIN32__)

Dim formatted As ZString * 64

If sprintf(@formatted, "%jd", Cast(CLong, -128)) < 0 Then End 1
If formatted <> "-128" Then End 2

If sprintf(@formatted, "%ju", Cast(CULong, 127)) < 0 Then End 3
If formatted <> "127" Then End 4

If sprintf(@formatted, "%ju", Cast(CULong, &Hffffffffffffffff)) < 0 Then End 5
If formatted <> "18446744073709551615" Then End 6

#endif

End 0

' end of clong-variadic-constant.bas
