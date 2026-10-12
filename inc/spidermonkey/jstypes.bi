''
''
'' jstypes -- header translated with help of SWIG FB wrapper
'' File: jstypes.bi
'' Purpose: Define SpiderMonkey 1.6 C scalar types and basic value macros.
'' Responsibilities: Fixed C integer widths and native word-sized values.
'' This file contains no engine implementation or CPU configuration.
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __jstypes_bi__
#define __jstypes_bi__

#include once "crt/stddef.bi"
#include once "spidermonkey/jscpucfg.bi"

type JSUint8 as ubyte
type JSInt8 as byte
type JSUint16 as ushort
type JSInt16 as short
type JSUint32 as ulong
type JSInt32 as long
type JSInt64 as longint
type JSUint64 as ulongint
type JSIntn as long
type JSUintn as ulong
type JSFloat64 as double
type JSSize as size_t
type JSPtrdiff as ptrdiff_t
type JSUptrdiff as uinteger
type JSBool as JSIntn
type JSPackedBool as JSUint8

' JSBool uses C's 0/1 representation, including when encoded in a jsval.
#define JS_TRUE cast(JSIntn, 1)
#define JS_FALSE cast(JSIntn, 0)
#define JS_BIT(n) (cast(JSUint32, 1) shl (n))
#define JS_BITMASK(n) (JS_BIT(n) - cast(JSUint32, 1))
type JSWord as integer
type JSUword as uinteger

#include once "spidermonkey/jsotypes.bi"

#endif
'' end of jstypes.bi
