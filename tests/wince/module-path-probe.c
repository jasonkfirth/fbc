/*
    Windows CE executable-path API probe
    -------------------------------------

    File: tests/wince/module-path-probe.c

    Purpose:

        Record how a Windows CE image reports the current executable to code
        built by the pinned CeGCC toolchain.

    Responsibilities:

        - compare NULL and explicit-module GetModuleFileNameW calls
        - record the CeGCC argv values derived before main()
        - preserve native error codes in a shared-folder report

    This file intentionally does NOT contain:

        - FreeBASIC runtime initialization
        - current-directory policy
        - emulator launch automation
*/

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <tlhelp32.h>

extern int __argc;
extern char **__argv;

static void print_wide_ascii( FILE *report, const wchar_t *text )
{
	if( text == NULL ) {
		fputs( "(null)", report );
		return;
	}

	while( *text != L'\0' ) {
		wchar_t character = *text++;
		fputc( (character >= 32 && character <= 126) ?
		       (int)character : '?', report );
	}
}

int main( void )
{
	wchar_t explicit_path[512] = L"";
	wchar_t null_path[512] = L"";
	FILE *report;
	HMODULE module;
	DWORD explicit_error;
	DWORD explicit_length;
	DWORD null_error;
	DWORD null_length;
	DWORD module_error;
	BOOL module_found = FALSE;
	MODULEENTRY32 module_entry;
	DWORD process_error;
	BOOL process_found = FALSE;
	HANDLE snapshot;
	PROCESSENTRY32 process_entry;
	int index;

	SetLastError( ERROR_SUCCESS );
	null_length = GetModuleFileNameW( NULL, null_path,
	                                  sizeof( null_path ) /
	                                  sizeof( null_path[0] ) );
	null_error = GetLastError();

	module = GetModuleHandleW( NULL );
	SetLastError( ERROR_SUCCESS );
	explicit_length = GetModuleFileNameW( module, explicit_path,
	                                      sizeof( explicit_path ) /
	                                      sizeof( explicit_path[0] ) );
	explicit_error = GetLastError();

	memset( &process_entry, 0, sizeof( process_entry ) );
	process_entry.dwSize = sizeof( process_entry );
	SetLastError( ERROR_SUCCESS );
	snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if( snapshot != INVALID_HANDLE_VALUE ) {
		if( Process32First( snapshot, &process_entry ) ) {
			do {
				if( process_entry.th32ProcessID == GetCurrentProcessId() ) {
					process_found = TRUE;
					break;
				}
			} while( Process32Next( snapshot, &process_entry ) );
		}
		CloseToolhelp32Snapshot( snapshot );
	}
	process_error = GetLastError();

	memset( &module_entry, 0, sizeof( module_entry ) );
	module_entry.dwSize = sizeof( module_entry );
	SetLastError( ERROR_SUCCESS );
	snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE,
	                                     GetCurrentProcessId() );
	if( snapshot != INVALID_HANDLE_VALUE ) {
		if( Module32First( snapshot, &module_entry ) ) {
			do {
				if( process_found &&
				    module_entry.th32ModuleID ==
				    process_entry.th32ModuleID ) {
					module_found = TRUE;
					break;
				}
			} while( Module32Next( snapshot, &module_entry ) );
		}
		CloseToolhelp32Snapshot( snapshot );
	}
	module_error = GetLastError();

	report = fopen( "\\Storage Card\\module-path-probe.txt", "wb" );
	if( report == NULL )
		return 2;

	fprintf( report, "module-handle=%lu\r\n", (unsigned long)module );
	fprintf( report, "null-length=%lu\r\n", (unsigned long)null_length );
	fprintf( report, "null-error=%lu\r\n", (unsigned long)null_error );
	fputs( "null-path=", report );
	print_wide_ascii( report, null_path );
	fputs( "\r\n", report );
	fprintf( report, "explicit-length=%lu\r\n",
	         (unsigned long)explicit_length );
	fprintf( report, "explicit-error=%lu\r\n",
	         (unsigned long)explicit_error );
	fputs( "explicit-path=", report );
	print_wide_ascii( report, explicit_path );
	fputs( "\r\n", report );
	fprintf( report, "process-found=%d\r\n", process_found );
	fprintf( report, "process-error=%lu\r\n",
	         (unsigned long)process_error );
	fprintf( report, "process-module-id=%lu\r\n",
	         (unsigned long)process_entry.th32ModuleID );
	fputs( "process-path=", report );
	print_wide_ascii( report, process_entry.szExeFile );
	fputs( "\r\n", report );
	fprintf( report, "snapshot-module-found=%d\r\n", module_found );
	fprintf( report, "snapshot-module-error=%lu\r\n",
	         (unsigned long)module_error );
	fputs( "snapshot-module-name=", report );
	print_wide_ascii( report, module_entry.szModule );
	fputs( "\r\n", report );
	fputs( "snapshot-module-path=", report );
	print_wide_ascii( report, module_entry.szExePath );
	fputs( "\r\n", report );
	fprintf( report, "argc=%d\r\n", __argc );
	for( index = 0; index < __argc; ++index )
		fprintf( report, "argv%d=%s\r\n", index, __argv[index] );
	fclose( report );
	return 0;
}

/* end of tests/wince/module-path-probe.c */
