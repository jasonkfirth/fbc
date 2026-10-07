'' FreeBASIC Windows CE process identity compatibility
'' File: wince/crt/unistd.bi
'' Purpose: Expose the native process identifier through the portable spelling.
'' Responsibilities: Bind GetCurrentProcessId with the Windows calling convention.
'' This file intentionally does NOT contain desktop CRT or POSIX process APIs.

#ifndef __crt_wince_unistd_bi__
#define __crt_wince_unistd_bi__

'' Coredll has no getpid export. Its native API returns an unsigned 32-bit
'' process identifier; mapping the name here also preserves its calling ABI.
extern "Windows"
declare function getpid alias "GetCurrentProcessId" () as ulong
end extern

#endif

'' end of wince/crt/unistd.bi
