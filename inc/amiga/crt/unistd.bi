'' FreeBASIC classic Amiga newlib CRT declarations
'' File: amiga/crt/unistd.bi
'' Purpose: Declare the file and process interfaces supplied by the m68k runtime.
'' Responsibilities: Use the pinned newlib scalar types and C export names.
'' This file intentionally does NOT contain process creation or Amiga OS APIs.
''
'' The runtime's newlib_io.c supplies descriptor I/O and getpid. Process IDs
'' identify the current Exec task, rather than Unix child-process lifetimes.

#ifndef __crt_amiga_unistd_bi__
#define __crt_amiga_unistd_bi__

#include once "crt/sys/types.bi"

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#endif
#ifndef R_OK
#define R_OK 4
#define W_OK 2
#define X_OK 1
#define F_OK 0
#endif
#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

extern "c"
declare function getpid () as pid_t
#ifndef __crt_close_declared__
#define __crt_close_declared__
declare function close_ alias "close" (byval fd as long) as long
#endif
declare function read_ alias "read" (byval fd as long, byval buffer as any ptr, byval count as size_t) as ssize_t
declare function write_ alias "write" (byval fd as long, byval buffer as const any ptr, byval count as size_t) as ssize_t
declare function lseek (byval fd as long, byval offset as off_t, byval whence as long) as off_t
declare function isatty (byval fd as long) as long
declare function unlink (byval path as const zstring ptr) as long
end extern

#endif

'' end of amiga/crt/unistd.bi
