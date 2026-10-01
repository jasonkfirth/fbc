'' FreeBASIC CRT declarations for classic Amiga newlib
'' ---------------------------------------------------
''
'' File: amiga/crt/sys/types.bi
'' Purpose: Describe primitive types from the pinned m68k C runtime.
'' Responsibilities: Preserve the SDK's 16-bit IDs and 32-bit native offsets.
'' This file intentionally does NOT contain filesystem structures or OS APIs.

#ifndef __crt_sys_types_bi__
#define __crt_sys_types_bi__

#include once "crt/stddef.bi"
#include once "crt/long.bi"

type dev_t as short
type ino_t as ushort
type mode_t as culong
type nlink_t as ushort
type uid_t as ushort
type gid_t as ushort
type off_t as clong
type pid_t as long

#endif

'' end of amiga/crt/sys/types.bi
