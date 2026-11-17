'' Project: FreeBASIC Wii CRT bindings
'' File: crt/wii/unistd.bi
'' Purpose: Expose the Newlib process identifier used by portable file clients.
'' Responsibilities: Preserve the SDK's signed 32-bit pid_t return ABI.
'' This file intentionally does NOT declare POSIX process creation or signals.
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.

#ifndef __crt_wii_unistd_bi__
#define __crt_wii_unistd_bi__

extern "c"
declare function getpid () as long
end extern

#endif

'' end of crt/wii/unistd.bi
