/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/sys_exepath_native.c

    Purpose:

        Resolve the current executable's native UTF-16 path on Windows CE,
        including reduced OEM images that reject GetModuleFileNameW.

    Responsibilities:

        - use GetModuleFileNameW as the normal constant-time path
        - obtain the executable filename through Toolhelp when necessary
        - locate that filename below common CE storage roots
        - avoid a static dependency on the optional Toolhelp library

    This file intentionally does NOT contain:

        - narrow-string conversion
        - process-local current-directory mutation
        - desktop process-image APIs
        - executable search-path policy for launching another program
*/

#include "../fb.h"

#include <limits.h>
#include <windows.h>
#include <tlhelp32.h>

#define FB_WINCE_SEARCH_DEPTH 24

typedef HANDLE (WINAPI *FB_WINCE_CREATE_SNAPSHOT)( DWORD, DWORD );
typedef BOOL (WINAPI *FB_WINCE_PROCESS_FIRST)( HANDLE, LPPROCESSENTRY32 );
typedef BOOL (WINAPI *FB_WINCE_PROCESS_NEXT)( HANDLE, LPPROCESSENTRY32 );
typedef BOOL (WINAPI *FB_WINCE_CLOSE_SNAPSHOT)( HANDLE );

static int fb_hWinCEJoinPath( const wchar_t *directory,
	                          const wchar_t *name,
	                          wchar_t *destination,
	                          size_t destination_length )
{
	size_t directory_length;
	size_t name_length;
	int needs_separator;

	if( directory == NULL || name == NULL || destination == NULL ||
	    destination_length == 0 ) {
		return FALSE;
	}

	directory_length = wcslen( directory );
	name_length = wcslen( name );
	needs_separator = (directory_length > 0) &&
	                  (directory[directory_length - 1] != L'\\');
	if( directory_length + (size_t)needs_separator + name_length >=
	    destination_length ) {
		return FALSE;
	}

	memcpy( destination, directory,
	        directory_length * sizeof( wchar_t ) );
	if( needs_separator )
		destination[directory_length++] = L'\\';
	memcpy( destination + directory_length, name,
	        (name_length + 1) * sizeof( wchar_t ) );
	return TRUE;
}

static int fb_hWinCEFindExecutable( const wchar_t *directory,
	                                const wchar_t *filename,
	                                wchar_t *destination,
	                                size_t destination_length,
	                                unsigned depth )
{
	wchar_t candidate[MAX_PATH + 1];
	wchar_t pattern[MAX_PATH + 1];
	WIN32_FIND_DATAW data;
	HANDLE search;
	int found = FALSE;

	if( depth > FB_WINCE_SEARCH_DEPTH )
		return FALSE;
	if( !fb_hWinCEJoinPath( directory, L"*", pattern,
	                       ARRAY_SIZE( pattern ) ) ) {
		return FALSE;
	}

	search = FindFirstFileW( pattern, &data );
	if( search == INVALID_HANDLE_VALUE )
		return FALSE;

	do {
		if( (wcscmp( data.cFileName, L"." ) == 0) ||
		    (wcscmp( data.cFileName, L".." ) == 0) ) {
			continue;
		}
		if( !fb_hWinCEJoinPath( directory, data.cFileName, candidate,
		                       ARRAY_SIZE( candidate ) ) ) {
			continue;
		}

		if( (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ) {
			found = fb_hWinCEFindExecutable( candidate, filename,
			                                   destination,
			                                   destination_length,
			                                   depth + 1 );
		} else if( lstrcmpiW( data.cFileName, filename ) == 0 ) {
			size_t length = wcslen( candidate );

			if( length < destination_length ) {
				memcpy( destination, candidate,
				        (length + 1) * sizeof( wchar_t ) );
				found = TRUE;
			}
		}
	} while( !found && FindNextFileW( search, &data ) );

	FindClose( search );
	return found;
}

static int fb_hWinCEGetProcessFilename( wchar_t *filename,
	                                    size_t filename_length )
{
	FB_WINCE_CLOSE_SNAPSHOT close_snapshot;
	FB_WINCE_CREATE_SNAPSHOT create_snapshot;
	FB_WINCE_PROCESS_FIRST process_first;
	FB_WINCE_PROCESS_NEXT process_next;
	PROCESSENTRY32 entry;
	HMODULE toolhelp;
	HANDLE snapshot;
	int found = FALSE;

	if( filename == NULL || filename_length == 0 )
		return FALSE;

	toolhelp = LoadLibraryW( L"toolhelp.dll" );
	if( toolhelp == NULL )
		return FALSE;

	create_snapshot = (FB_WINCE_CREATE_SNAPSHOT)GetProcAddressW(
		toolhelp, L"CreateToolhelp32Snapshot" );
	process_first = (FB_WINCE_PROCESS_FIRST)GetProcAddressW(
		toolhelp, L"Process32First" );
	process_next = (FB_WINCE_PROCESS_NEXT)GetProcAddressW(
		toolhelp, L"Process32Next" );
	close_snapshot = (FB_WINCE_CLOSE_SNAPSHOT)GetProcAddressW(
		toolhelp, L"CloseToolhelp32Snapshot" );
	if( create_snapshot == NULL || process_first == NULL ||
	    process_next == NULL || close_snapshot == NULL ) {
		FreeLibrary( toolhelp );
		return FALSE;
	}

	snapshot = create_snapshot( TH32CS_SNAPPROCESS, 0 );
	if( snapshot != INVALID_HANDLE_VALUE ) {
		memset( &entry, 0, sizeof( entry ) );
		entry.dwSize = sizeof( entry );
		if( process_first( snapshot, &entry ) ) {
			do {
				if( entry.th32ProcessID == GetCurrentProcessId() ) {
					size_t length = wcslen( entry.szExeFile );

					if( length > 0 && length < filename_length ) {
						memcpy( filename, entry.szExeFile,
						        (length + 1) * sizeof( wchar_t ) );
						found = TRUE;
					}
					break;
				}
			} while( process_next( snapshot, &entry ) );
		}
		close_snapshot( snapshot );
	}

	FreeLibrary( toolhelp );
	return found;
}

int fb_hWinCEGetExecutablePathWC( wchar_t *destination,
	                              size_t destination_length )
{
	static const wchar_t *search_roots[] = {
		L"\\Storage Card",
		L"\\SD Card",
		L"\\Flash Disk",
		L"\\Hard Disk",
		L"\\Program Files",
		L"\\My Documents",
		L"\\Application Data",
		L"\\"
	};
	wchar_t filename[MAX_PATH + 1];
	DWORD length;
	size_t index;

	if( destination == NULL || destination_length == 0 ||
	    destination_length > UINT_MAX ) {
		return FALSE;
	}

	length = GetModuleFileNameW( NULL, destination,
	                             (DWORD)destination_length );
	if( length > 0 && length < destination_length )
		return TRUE;

	destination[0] = L'\0';
	if( !fb_hWinCEGetProcessFilename( filename,
	                                 ARRAY_SIZE( filename ) ) ) {
		return FALSE;
	}

	for( index = 0; index < ARRAY_SIZE( search_roots ); ++index ) {
		if( fb_hWinCEFindExecutable( search_roots[index], filename,
		                              destination, destination_length, 0 ) ) {
			return TRUE;
		}
	}

	return FALSE;
}

/* end of wince/sys_exepath_native.c */
