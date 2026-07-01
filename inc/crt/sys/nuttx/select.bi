''
''
'' sys\select -- NuttX CRT declarations
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __crt_sys_nuttx_select_bi__
#define __crt_sys_nuttx_select_bi__

#include once "crt/stdint.bi"
#include once "crt/string.bi"

#define FD_SETSIZE 256
#define __SELECT_NUINT32 ((FD_SETSIZE + 31) \ 32)
#define _FD_NDX(fd) ((fd) shr 5)
#define _FD_BIT(fd) ((fd) and &h1f)

#macro __FD_ZERO(set)
	memset(set, 0, sizeof(fd_set))
#endmacro

#define __FD_SET(fd, set) (set)->arr(_FD_NDX(fd)) or= (cast(uint32_t, 1) shl _FD_BIT(fd))
#define __FD_CLR(fd, set) (set)->arr(_FD_NDX(fd)) and= not (cast(uint32_t, 1) shl _FD_BIT(fd))
#define __FD_ISSET(fd, set) (((set)->arr(_FD_NDX(fd)) and (cast(uint32_t, 1) shl _FD_BIT(fd))) <> 0)

#endif
