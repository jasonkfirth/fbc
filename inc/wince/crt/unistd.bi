'' FreeBASIC Windows CE process identity compatibility
'' File: wince/crt/unistd.bi
'' Purpose: Expose the native process identifier through the portable spelling.
'' Responsibilities: Bind the runtime bridge to the SDK's process-ID accessor.
'' This file intentionally does NOT contain desktop CRT or POSIX process APIs.

#ifndef __crt_wince_unistd_bi__
#define __crt_wince_unistd_bi__

'' CeGCC implements GetCurrentProcessId inline in kfuncs.h, without a Coredll
'' export. libfb/libfbmt provide this C ABI bridge so BASIC code uses the SDK's
'' accessor without embedding architecture-specific kernel-data addresses.
extern "c"
declare function getpid alias "fb_hWinCEGetProcessId" () as ulong
end extern

#endif

'' end of wince/crt/unistd.bi
