''
''
'' types -- header translated with help of SWIG FB wrapper
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __crt_sys_types_bi__
#define __crt_sys_types_bi__

#include once "crt/stddef.bi"

#if defined(__FB_WIN32__) or defined(__FB_XBOX__)
#include once "crt/sys/win32/types.bi"
#elseif defined(__FB_DOS__)
#include once "crt/sys/dos/types.bi"
#elseif defined(__FB_LINUX__) or defined(__FB_ANDROID__)
#include once "crt/sys/linux/types.bi"
#elseif defined(__FB_CYGWIN__)
#include once "crt/sys/cygwin/types.bi"
#elseif defined(__FB_FREEBSD__)
#include once "crt/sys/freebsd/types.bi"
#elseif defined(__FB_DRAGONFLY__)
#include once "crt/sys/dragonfly/types.bi"
#elseif defined(__FB_OPENBSD__)
#include once "crt/sys/openbsd/types.bi"
#elseif defined(__FB_NETBSD__)
#include once "crt/sys/netbsd/types.bi"
#elseif defined(__FB_HAIKU__)
#include once "crt/sys/haiku/types.bi"
#elseif defined(__FB_DARWIN__)
'' May not be correct
#include once "crt/sys/linux/types.bi"
#else
#error Platform unsupported
#endif

#endif
