#ifndef FB_XBOX_H
#define FB_XBOX_H

#include <errno.h>
#include <hal/xbox.h>
#include <hal/fileio.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#define FBCALL __stdcall

/* newline for console/file I/O */
#define FB_NEWLINE "\r\n"
#define FB_NEWLINE_WSTR _LC("\r\n")

/* newline for printer I/O */
#define FB_BINARY_NEWLINE "\r\n"
#define FB_BINARY_NEWLINE_WSTR _LC("\r\n")
#define FB_LL_FMTMOD "ll"
#define FB_CONSOLE_MAXPAGES 1
#define FB_DYLIB HANDLE
/*
 * DATA records are emitted by the compiler as ordinary C structs for Xbox.
 * Leaving this descriptor unpacked keeps the runtime view aligned with that
 * generated layout.
 */
#define FB_DATADESC_PACKED

typedef long fb_off_t;
typedef int ssize_t;
int _stricmp(const char *s1, const char *s2);
int _strnicmp(const char *s1, const char *s2, size_t n);
#define strcasecmp  _stricmp
#define strncasecmp _strnicmp
#define alloca(size) __builtin_alloca(size)
#define fseeko(stream, offset, whence) fseek(stream, offset, whence)
#define ftello(stream)                 ftell(stream)
#define mbstowcs __xbox_mbstowcs
#define wcstombs __xbox_wcstombs
#ifndef NSIG
#define NSIG 16
#endif

static __inline__ size_t __xbox_mbstowcs(wchar_t *dst, const char *src, size_t count)
{
	size_t i;

	if( src == NULL )
		return (size_t)-1;

	if( dst == NULL )
		return strlen( src );

	for( i = 0; i < count; i++ ) {
		unsigned char c = src[i];
		if( c == '\0' ) {
			dst[i] = L'\0';
			return i;
		}
		dst[i] = (c <= 127) ? c : L'?';
	}

	return i;
}

static __inline__ size_t __xbox_wcstombs(char *dst, const wchar_t *src, size_t count)
{
	size_t i;

	if( src == NULL )
		return (size_t)-1;

	if( dst == NULL ) {
		for( i = 0; src[i] != L'\0'; i++ )
			;
		return i;
	}

	for( i = 0; i < count; i++ ) {
		wchar_t c = src[i];
		if( c == L'\0' ) {
			dst[i] = '\0';
			return i;
		}
		dst[i] = (c <= 127) ? (char)c : '?';
	}

	return i;
}

static __inline__ int __xbox_fopen_s(FILE **file, const char *path, const char *mode)
{
	if( file == NULL )
		return EINVAL;

	*file = fopen(path, mode);
	return (*file != NULL) ? 0 : errno;
}

static __inline__ int __xbox_wfopen_s(FILE **file, const wchar_t *path, const wchar_t *mode)
{
	char path_buffer[MAX_PATH];
	char mode_buffer[16];

	if( file == NULL || path == NULL || mode == NULL )
		return EINVAL;

	if( __xbox_wcstombs(path_buffer, path, sizeof(path_buffer)) >= sizeof(path_buffer) )
		return EINVAL;

	if( __xbox_wcstombs(mode_buffer, mode, sizeof(mode_buffer)) >= sizeof(mode_buffer) )
		return EINVAL;

	return __xbox_fopen_s(file, path_buffer, mode_buffer);
}

#ifndef fopen_s
#define fopen_s __xbox_fopen_s
#endif

#ifndef _wfopen_s
#define _wfopen_s __xbox_wfopen_s
#endif

/* OpenXDK compatibility structures and constants used by the Xbox rtlib. */
#define Executive 0
#define KernelMode 0
#define UserMode 1

/* These functions are not present in the OpenXDK's headers */
int swprintf(wchar_t *wcs, size_t maxlen, const wchar_t *format, ...);
double wcstod(const wchar_t*, wchar_t**);
unsigned long wcstoul(const wchar_t *, wchar_t **, int);
unsigned long long  wcstoull(const wchar_t * __restrict__, wchar_t ** __restrict__, int);

#endif
