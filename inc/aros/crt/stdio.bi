''
'' FreeBASIC CRT declarations for AROS POSIXC
'' -------------------------------------------
''
'' File: crt/stdio.bi
''
'' Purpose:
''
''     Define the AROS POSIXC standard I/O interface.
''
'' Responsibilities:
''
''     - reproduce AROS stream types, limits, and standard-stream accessors
''     - declare the standard narrow and wide I/O entry points
''     - keep the private AROS stream structure opaque
''
'' This file intentionally does NOT contain:
''
''     - declarations copied from a BSD fallback
''     - FreeBASIC file-number implementation details
''     - AROS Intuition, CyberGraphX, or AHI declarations
''
#ifndef __crt_stdio_bi__
#define __crt_stdio_bi__

#include once "crt/long.bi"
#include once "crt/stddef.bi"
#include once "crt/stdarg.bi"

#define EOF_ (-1)

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

'' AROS exposes struct __sFILE through stdcio.library and POSIXC.  Its fields
'' are private; only the pointer identity is part of this declaration surface.
type FILE as __sFILE

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2
#define BUFSIZ 1024
#define FOPEN_MAX 16
#define FILENAME_MAX 256
#define L_tmpnam FILENAME_MAX
#define TMP_MAX 10240
#define P_tmpdir "T:"

type fpos_t as ulong

extern "c"
declare function __posixc_getstdin alias "__posixc_getstdin" () as FILE ptr
declare function __posixc_getstdout alias "__posixc_getstdout" () as FILE ptr
declare function __posixc_getstderr alias "__posixc_getstderr" () as FILE ptr
end extern

#define stdin __posixc_getstdin()
#define stdout __posixc_getstdout()
#define stderr __posixc_getstderr()

'' ---------------------------------------------------------------------------
'' Narrow-character I/O
'' ---------------------------------------------------------------------------

extern "c"
declare function remove (byval filename as const zstring ptr) as long
declare function rename (byval oldname as const zstring ptr, byval newname as const zstring ptr) as long
declare function fopen (byval filename as const zstring ptr, byval mode as const zstring ptr) as FILE ptr
declare function freopen (byval filename as const zstring ptr, byval mode as const zstring ptr, byval stream as FILE ptr) as FILE ptr
declare function fclose (byval stream as FILE ptr) as long
declare function fflush (byval stream as FILE ptr) as long
declare function tmpfile () as FILE ptr
declare function tmpnam (byval buffer as zstring ptr) as zstring ptr
declare function tempnam (byval directory as const zstring ptr, byval prefix as const zstring ptr) as zstring ptr
declare sub setbuf (byval stream as FILE ptr, byval buffer as zstring ptr)
declare function setvbuf (byval stream as FILE ptr, byval buffer as zstring ptr, byval mode as long, byval bytes as size_t) as long

declare function fprintf (byval stream as FILE ptr, byval format as const zstring ptr, ...) as long
declare function fscanf (byval stream as FILE ptr, byval format as const zstring ptr, ...) as long
declare function printf (byval format as const zstring ptr, ...) as long
declare function scanf (byval format as const zstring ptr, ...) as long
declare function snprintf (byval buffer as zstring ptr, byval bytes as size_t, byval format as const zstring ptr, ...) as long
declare function sprintf (byval buffer as zstring ptr, byval format as const zstring ptr, ...) as long
declare function sscanf (byval buffer as const zstring ptr, byval format as const zstring ptr, ...) as long
declare function vfprintf (byval stream as FILE ptr, byval format as const zstring ptr, byval arguments as va_list) as long
declare function vfscanf (byval stream as FILE ptr, byval format as const zstring ptr, byval arguments as va_list) as long
declare function vprintf (byval format as const zstring ptr, byval arguments as va_list) as long
declare function vscanf (byval format as const zstring ptr, byval arguments as va_list) as long
declare function vsnprintf (byval buffer as zstring ptr, byval bytes as size_t, byval format as const zstring ptr, byval arguments as va_list) as long
declare function vsprintf (byval buffer as zstring ptr, byval format as const zstring ptr, byval arguments as va_list) as long
declare function vsscanf (byval buffer as const zstring ptr, byval format as const zstring ptr, byval arguments as va_list) as long

declare function fgetc (byval stream as FILE ptr) as long
declare function fgets (byval buffer as zstring ptr, byval bytes as long, byval stream as FILE ptr) as zstring ptr
declare function fputc (byval character as long, byval stream as FILE ptr) as long
declare function fputs (byval text as const zstring ptr, byval stream as FILE ptr) as long
declare function getc (byval stream as FILE ptr) as long
declare function getchar () as long
declare function gets (byval buffer as zstring ptr) as zstring ptr
declare function putc (byval character as long, byval stream as FILE ptr) as long
declare function putchar (byval character as long) as long
declare function puts (byval text as const zstring ptr) as long
declare function ungetc (byval character as long, byval stream as FILE ptr) as long
declare function fread (byval buffer as any ptr, byval bytes as size_t, byval count as size_t, byval stream as FILE ptr) as size_t
declare function fwrite (byval buffer as const any ptr, byval bytes as size_t, byval count as size_t, byval stream as FILE ptr) as size_t

declare function fgetpos (byval stream as FILE ptr, byval position as fpos_t ptr) as long
declare function fsetpos (byval stream as FILE ptr, byval position as const fpos_t ptr) as long
declare function fseek (byval stream as FILE ptr, byval offset as clong, byval origin as long) as long
declare function ftell (byval stream as FILE ptr) as clong
declare sub rewind (byval stream as FILE ptr)
declare sub clearerr (byval stream as FILE ptr)
declare function feof (byval stream as FILE ptr) as long
declare function ferror (byval stream as FILE ptr) as long
declare sub perror (byval text as const zstring ptr)

declare function popen (byval command as const zstring ptr, byval mode as const zstring ptr) as FILE ptr
declare function pclose (byval stream as FILE ptr) as long
declare function getw (byval stream as FILE ptr) as long
declare function putw (byval value as long, byval stream as FILE ptr) as long
end extern

'' ---------------------------------------------------------------------------
'' Wide-character I/O
'' ---------------------------------------------------------------------------

extern "c"
declare function fwprintf (byval stream as FILE ptr, byval format as const wchar_t ptr, ...) as long
declare function fwscanf (byval stream as FILE ptr, byval format as const wchar_t ptr, ...) as long
declare function swprintf (byval buffer as wchar_t ptr, byval bytes as size_t, byval format as const wchar_t ptr, ...) as long
declare function swscanf (byval buffer as const wchar_t ptr, byval format as const wchar_t ptr, ...) as long
declare function wprintf (byval format as const wchar_t ptr, ...) as long
declare function wscanf (byval format as const wchar_t ptr, ...) as long
declare function vfwprintf (byval stream as FILE ptr, byval format as const wchar_t ptr, byval arguments as va_list) as long
declare function vfwscanf (byval stream as FILE ptr, byval format as const wchar_t ptr, byval arguments as va_list) as long
declare function vswprintf (byval buffer as wchar_t ptr, byval bytes as size_t, byval format as const wchar_t ptr, byval arguments as va_list) as long
declare function vswscanf (byval buffer as const wchar_t ptr, byval format as const wchar_t ptr, byval arguments as va_list) as long
declare function vwprintf (byval format as const wchar_t ptr, byval arguments as va_list) as long
declare function vwscanf (byval format as const wchar_t ptr, byval arguments as va_list) as long

declare function fgetwc (byval stream as FILE ptr) as wint_t
declare function fgetws (byval buffer as wchar_t ptr, byval characters as long, byval stream as FILE ptr) as wchar_t ptr
declare function fputwc (byval character as wint_t, byval stream as FILE ptr) as wint_t
declare function fputws (byval text as const wchar_t ptr, byval stream as FILE ptr) as long
declare function fwide (byval stream as FILE ptr, byval mode as long) as long
declare function getwc (byval stream as FILE ptr) as wint_t
declare function getwchar () as wint_t
declare function putwc (byval character as wchar_t, byval stream as FILE ptr) as wint_t
declare function putwchar (byval character as wchar_t) as wint_t
declare function ungetwc (byval character as wint_t, byval stream as FILE ptr) as wint_t
end extern

#endif

'' end of crt/stdio.bi
