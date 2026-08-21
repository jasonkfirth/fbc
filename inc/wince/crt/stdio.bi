''
'' FreeBASIC CRT declarations for Windows CE Coredll
'' -------------------------------------------------
''
'' File: crt/stdio.bi
''
'' Purpose:
''
''     Define the Windows CE Coredll standard I/O interface.
''
'' Responsibilities:
''
''     - reproduce Coredll stream types, limits, and standard-stream accessors
''     - declare the standard narrow and wide I/O entry points
''     - keep the private Coredll stream structure opaque
''
'' This file intentionally does NOT contain:
''
''     - desktop MSVCRT stream structures or the _iob array
''     - FreeBASIC file-number implementation details
''     - Windows CE window, graphics, or multimedia declarations
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

'' Coredll deliberately declares FILE as void.  Keeping it opaque here avoids
'' exposing the desktop MSVCRT _iobuf layout, which Windows CE does not use.
type FILE as any

#define _IOFBF &h0000
#define _IOLBF &h0040
#define _IONBF &h0004
#define BUFSIZ 512
#define FOPEN_MAX 20
#define FILENAME_MAX 260
#define L_tmpnam 16
#define TMP_MAX 32767
#define P_tmpdir "\"

type fpos_t as clong

extern "c"
declare function _getstdfilex alias "_getstdfilex" _
    (byval stream_number as long) as FILE ptr
end extern

'' Unlike desktop MSVCRT, Coredll returns its standard streams through an
'' accessor.  The indices are part of the CeGCC stdio contract.
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#define stdin _getstdfilex(STDIN_FILENO)
#define stdout _getstdfilex(STDOUT_FILENO)
#define stderr _getstdfilex(STDERR_FILENO)

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
declare function swprintf (byval buffer as wchar_t ptr, byval format as const wchar_t ptr, ...) as long
declare function swscanf (byval buffer as const wchar_t ptr, byval format as const wchar_t ptr, ...) as long
declare function wprintf (byval format as const wchar_t ptr, ...) as long
declare function wscanf (byval format as const wchar_t ptr, ...) as long
declare function vfwprintf (byval stream as FILE ptr, byval format as const wchar_t ptr, byval arguments as va_list) as long
declare function vfwscanf (byval stream as FILE ptr, byval format as const wchar_t ptr, byval arguments as va_list) as long
declare function vswprintf (byval buffer as wchar_t ptr, byval format as const wchar_t ptr, byval arguments as va_list) as long
declare function vswscanf (byval buffer as const wchar_t ptr, byval format as const wchar_t ptr, byval arguments as va_list) as long
declare function vwprintf (byval format as const wchar_t ptr, byval arguments as va_list) as long
declare function vwscanf (byval format as const wchar_t ptr, byval arguments as va_list) as long

declare function fgetwc (byval stream as FILE ptr) as wint_t
declare function fgetws (byval buffer as wchar_t ptr, byval characters as long, byval stream as FILE ptr) as wchar_t ptr
declare function fputwc (byval character as wint_t, byval stream as FILE ptr) as wint_t
declare function fputws (byval text as const wchar_t ptr, byval stream as FILE ptr) as long
declare function getwc (byval stream as FILE ptr) as wint_t
declare function getwchar () as wint_t
declare function putwc (byval character as wchar_t, byval stream as FILE ptr) as wint_t
declare function putwchar (byval character as wchar_t) as wint_t
declare function ungetwc (byval character as wint_t, byval stream as FILE ptr) as wint_t
end extern

#endif

'' end of crt/stdio.bi
