/*
    FreeBASIC Windows CE MIPS toolchain probe
    ------------------------------------------

    File: tests/wince/mips_toolchain_smoke.c

    Purpose:

        Verify that the open LLVM and CeGCC components can produce a native
        MIPS Windows CE executable before the FreeBASIC libraries are built.

    Responsibilities:

        - exercise the R4000 PE/COFF import-library ABI
        - enter through the Windows CE process startup contract
        - write deterministic completion evidence to the shared storage card

    This file intentionally does NOT contain:

        - FreeBASIC runtime initialization
        - C runtime or compiler-builtins dependencies
        - graphics, sound, or test-harness behavior
*/

#include <windows.h>

void WinMainCRTStartup( HINSTANCE instance, HINSTANCE previous_instance,
                        LPWSTR command_line, int show_command )
{
	static const char message[] =
		"FreeBASIC Windows CE MIPS toolchain smoke passed\r\n";
	DWORD bytes_written = 0;
	HANDLE report;

	(void)instance;
	(void)previous_instance;
	(void)command_line;
	(void)show_command;

	report = CreateFileW( L"\\Storage Card\\fb-wince-mips-toolchain.txt",
	                      GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
	                      FILE_ATTRIBUTE_NORMAL, NULL );
	if( report == INVALID_HANDLE_VALUE )
		ExitProcess( 2 );

	if( !WriteFile( report, message, sizeof( message ) - 1,
	                &bytes_written, NULL ) ||
	    bytes_written != sizeof( message ) - 1 ) {
		CloseHandle( report );
		ExitProcess( 3 );
	}

	CloseHandle( report );
	ExitProcess( 0 );
}

/* end of tests/wince/mips_toolchain_smoke.c */
