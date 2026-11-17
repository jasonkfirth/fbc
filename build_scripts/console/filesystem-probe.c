/*
 * Project: FreeBASIC console qualification
 * File: filesystem-probe.c
 * Purpose: Compare native Xbox file creation and native directory spelling.
 * Responsibilities: Own one UTF-8 fixture and report the bytes returned by nxdk.
 * This file does not call FreeBASIC's filesystem adapters or scan user media.
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>

int fbqual_native_filesystem( void )
{
	static const char leaf[] = "fbq-native-\xC3\xA9\xE4\xB8\xAD.txt";
	static const char path[] = "E:\\fbq-native-\xC3\xA9\xE4\xB8\xAD.txt";
	WIN32_FIND_DATAA data;
	HANDLE file, search;
	FILE *log;
	unsigned int entries = 0;
	int matched = 0;

	file = CreateFileA( path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
		CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
	if( file == INVALID_HANDLE_VALUE )
		return 91;
	CloseHandle( file );
	log = fopen( "E:\\FBQNATIVE.TXT", "wb" );
	if( log == NULL ) {
		DeleteFileA( path );
		return 92;
	}
	fprintf( log, "expected: %s\n", leaf );
	search = FindFirstFileA( "E:\\*", &data );
	if( search != INVALID_HANDLE_VALUE ) {
		do {
			/* Bound enumeration to the controller's private test volume. */
			if( ++entries > 1024 )
				break;
			if( strncmp(data.cFileName, "fbq-native-", 11) == 0 ) {
				const unsigned char *byte = (const unsigned char *)data.cFileName;
				fprintf( log, "native entry: %s\nbytes:", data.cFileName );
				while( *byte != 0 )
					fprintf( log, " %02x", (unsigned int)*byte++ );
				fputc( '\n', log );
			}
			if( strcmp(data.cFileName, leaf) == 0 )
				matched = 1;
		} while( FindNextFileA(search, &data) );
		FindClose( search );
	}
	fprintf( log, "matched: %d\n", matched );
	if( fclose(log) != 0 ) {
		DeleteFileA( path );
		return 94;
	}
	if( !DeleteFileA(path) )
		return 95;
	return matched ? 0 : 93;
}

/* end of filesystem-probe.c */
