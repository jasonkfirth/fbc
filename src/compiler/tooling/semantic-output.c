/*
 * Project: FreeBASIC compiler - semantic output
 * File: tooling/semantic-output.c
 * Purpose: Publish sidecars without overwriting inputs or hiding I/O errors.
 * Responsibilities: Own staging, file identities, checked writes, and replacement.
 * This file intentionally does NOT contain: BASIC parsing or model serialization.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#include <direct.h>
#define OUTPUT_WINDOWS 1
#else
#include <unistd.h>
#endif

/* ------------------------------------------------------------------------- */
/* Filesystem identities and writer ownership                                 */
/* ------------------------------------------------------------------------- */

typedef struct OUTPUT_IDENTITY {
	int valid;
#ifdef OUTPUT_WINDOWS
	DWORD volume, high, low;
#else
	dev_t device;
	ino_t inode;
#endif
} OUTPUT_IDENTITY;

typedef struct OUTPUT_PROTECTED {
	char *path;
	OUTPUT_IDENTITY identity;
	struct OUTPUT_PROTECTED *next;
} OUTPUT_PROTECTED;

typedef struct SEMANTIC_OUTPUT {
	FILE *stream;
	char *destination, *directory, *staging;
	OUTPUT_PROTECTED *protected;
	int failed;
} SEMANTIC_OUTPUT;

static int path_separator( char ch )
{
#if defined(OUTPUT_WINDOWS) || defined(__DJGPP__)
	return ch == '/' || ch == '\\';
#else
	return ch == '/';
#endif
}

static char *path_join( const char *directory, const char *name )
{
	size_t length = strlen( directory ), suffix = strlen( name );
	char *result;
	int separator = length && !path_separator( directory[length - 1] );
#if defined(OUTPUT_WINDOWS) || defined(__DJGPP__)
	if( length && directory[length - 1] == ':' ) separator = 0;
#endif
	if( length > SIZE_MAX - suffix - 2 ) return NULL;
	result = malloc( length + suffix + 2 );
	if( result == NULL ) return NULL;
	memcpy( result, directory, length );
	if( separator ) result[length++] = '/';
	memcpy( result + length, name, suffix + 1 );
	return result;
}

static char *canonical_path( const char *path )
{
#if defined(OUTPUT_WINDOWS) || defined(__DJGPP__)
	return _fullpath( NULL, path, 0 );
#else
	char *result = realpath( path, NULL );
	char *parent, *resolved;
	const char *name;
	size_t length;
	if( result != NULL ) return result;
	/* Output files and future emissions may not exist yet. Resolve their
	 * parent directory, including directory symlinks, before adding the name. */
	name = strrchr( path, '/' );
	if( name == NULL ) {
		resolved = realpath( ".", NULL );
		name = path;
	} else {
		length = (size_t)(name - path);
		if( length == 0 ) length = 1;
		parent = malloc( length + 1 );
		if( parent == NULL ) return NULL;
		memcpy( parent, path, length );
		parent[length] = '\0';
		resolved = realpath( parent, NULL );
		free( parent );
		++name;
	}
	if( resolved == NULL ) return NULL;
	result = path_join( resolved, name );
	free( resolved );
	return result;
#endif
}

static int same_path( const char *left, const char *right )
{
#if defined(OUTPUT_WINDOWS) || defined(__DJGPP__)
	return strcasecmp( left, right ) == 0;
#else
	return strcmp( left, right ) == 0;
#endif
}

static int file_identity( const char *path, OUTPUT_IDENTITY *identity )
{
	memset( identity, 0, sizeof( *identity ) );
#ifdef OUTPUT_WINDOWS
	BY_HANDLE_FILE_INFORMATION info;
	HANDLE file = CreateFileA( path, 0,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
		OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
	if( file == INVALID_HANDLE_VALUE ) {
		DWORD error = GetLastError();
		return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
	}
	int ok = GetFileInformationByHandle( file, &info ) != 0;
	CloseHandle( file );
	if( !ok ) return 0;
	identity->volume = info.dwVolumeSerialNumber;
	identity->high = info.nFileIndexHigh;
	identity->low = info.nFileIndexLow;
	identity->valid = 1;
#else
	struct stat info;
	if( stat( path, &info ) != 0 ) return errno == ENOENT || errno == ENOTDIR;
	identity->device = info.st_dev;
	identity->inode = info.st_ino;
	/* Some legacy filesystems provide no inode identity. Canonical paths
	 * still protect their names; zero must not identify every file as equal. */
	identity->valid = info.st_ino != 0;
#endif
	return 1;
}

static int same_identity( const OUTPUT_IDENTITY *left, const OUTPUT_IDENTITY *right )
{
	if( !left->valid || !right->valid ) return 0;
#ifdef OUTPUT_WINDOWS
	return left->volume == right->volume && left->high == right->high && left->low == right->low;
#else
	return left->device == right->device && left->inode == right->inode;
#endif
}

static int ordinary_destination( const char *path )
{
#ifdef OUTPUT_WINDOWS
	DWORD attributes = GetFileAttributesA( path );
	if( attributes == INVALID_FILE_ATTRIBUTES ) {
		DWORD error = GetLastError();
		return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
	}
	return !(attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DEVICE));
#else
	struct stat info;
#ifdef __DJGPP__
	int status = stat( path, &info );
#else
	int status = lstat( path, &info );
#endif
	if( status != 0 ) return errno == ENOENT;
	/* Never replace devices, directories, FIFOs, or an existing symlink.
	 * In particular, /dev/full must be rejected without opening it. */
	return S_ISREG( info.st_mode );
#endif
}

/* ------------------------------------------------------------------------- */
/* Checked staging and final publication                                      */
/* ------------------------------------------------------------------------- */

static void release_output( SEMANTIC_OUTPUT *output )
{
	OUTPUT_PROTECTED *file, *next;
	if( output == NULL ) return;
	if( output->stream != NULL ) fclose( output->stream );
	if( output->staging != NULL ) remove( output->staging );
	if( output->directory != NULL ) rmdir( output->directory );
	for( file = output->protected; file != NULL; file = next ) {
		next = file->next;
		free( file->path );
		free( file );
	}
	free( output->destination );
	free( output->directory );
	free( output->staging );
	free( output );
}

void *fbSemanticOutputOpen( const char *filename )
{
	SEMANTIC_OUTPUT *output;
	char *parent, *candidate;
	char tag[64];
	size_t length;
	unsigned long nonce = (unsigned long)time( NULL ) ^ (unsigned long)clock();
	unsigned int attempt;
	if( filename == NULL || filename[0] == '\0' || !ordinary_destination( filename ) ) return NULL;
	output = calloc( 1, sizeof( *output ) );
	if( output == NULL ) return NULL;
	output->destination = canonical_path( filename );
	if( output->destination == NULL ) { release_output( output ); return NULL; }
	length = strlen( output->destination );
	while( length && !path_separator( output->destination[length - 1] ) ) --length;
	parent = malloc( length + 1 );
	if( parent == NULL ) { release_output( output ); return NULL; }
	memcpy( parent, output->destination, length );
	parent[length] = '\0';
	/* mkdir provides exclusive ownership without relying on a temporary
	 * filename check followed by a truncating open. Keeping staging beside
	 * the destination also keeps the final rename on the same filesystem. */
	for( attempt = 0; attempt < 256; ++attempt ) {
#ifdef __DJGPP__
		snprintf( tag, sizeof( tag ), "fs%04lx%02x", nonce & 0xffffUL, attempt );
#else
		snprintf( tag, sizeof( tag ), ".fb-semantic-%08lx-%02x", nonce & 0xffffffffUL, attempt );
#endif
		candidate = path_join( parent, tag );
		if( candidate == NULL ) break;
#ifdef OUTPUT_WINDOWS
		int created = _mkdir( candidate ) == 0;
#else
		int created = mkdir( candidate, 0700 ) == 0;
#endif
		if( created ) { output->directory = candidate; break; }
		free( candidate );
		if( errno != EEXIST ) break;
	}
	free( parent );
	if( output->directory == NULL ) { release_output( output ); return NULL; }
	output->staging = path_join( output->directory, "model.tmp" );
	if( output->staging == NULL ) { release_output( output ); return NULL; }
	output->stream = fopen( output->staging, "wb" );
	if( output->stream == NULL ) { release_output( output ); return NULL; }
	return output;
}

int fbSemanticOutputProtect( void *handle, const char *filename )
{
	SEMANTIC_OUTPUT *output = handle;
	OUTPUT_PROTECTED *file;
	OUTPUT_IDENTITY destination;
	char *path;
	if( output == NULL || output->failed ) return 0;
	if( filename == NULL || filename[0] == '\0' ) return 1;
	path = canonical_path( filename );
	if( path == NULL ) { output->failed = 1; return 0; }
	for( file = output->protected; file != NULL; file = file->next ) {
		if( same_path( path, file->path ) ) { free( path ); return 1; }
	}
	file = calloc( 1, sizeof( *file ) );
	if( file == NULL ) { free( path ); output->failed = 1; return 0; }
	file->path = path;
	file->next = output->protected;
	output->protected = file;
	if( !file_identity( path, &file->identity ) || !file_identity( output->destination, &destination ) ||
		same_path( path, output->destination ) || same_identity( &file->identity, &destination ) ) {
		output->failed = 1;
		return 0;
	}
	return 1;
}

int fbSemanticOutputWrite( void *handle, const void *data, size_t bytes )
{
	SEMANTIC_OUTPUT *output = handle;
	if( output == NULL || output->failed ) return 0;
	if( bytes && (data == NULL || fwrite( data, 1, bytes, output->stream ) != bytes) ) output->failed = 1;
	if( ferror( output->stream ) ) output->failed = 1;
	return !output->failed;
}

int fbSemanticOutputFinish( void *handle, int publish )
{
	SEMANTIC_OUTPUT *output = handle;
	OUTPUT_PROTECTED *file;
	OUTPUT_IDENTITY destination, current;
	int ok;
	if( output == NULL ) return 1;
	if( output->stream != NULL ) {
		if( fflush( output->stream ) != 0 ) output->failed = 1;
		if( fclose( output->stream ) != 0 ) output->failed = 1;
		output->stream = NULL;
	}
	if( publish && !output->failed ) {
		if( !ordinary_destination( output->destination ) || !file_identity( output->destination, &destination ) ) output->failed = 1;
		for( file = output->protected; !output->failed && file != NULL; file = file->next ) {
			if( !file_identity( file->path, &current ) || same_path( file->path, output->destination ) ||
				same_identity( &file->identity, &destination ) || same_identity( &current, &destination ) ) output->failed = 1;
		}
		if( !output->failed ) {
#ifdef OUTPUT_WINDOWS
			/* C rename does not replace an existing Windows destination.
			 * MoveFileEx performs that replacement without deleting it first. */
			if( !MoveFileExA( output->staging, output->destination, MOVEFILE_REPLACE_EXISTING ) ) output->failed = 1;
#else
			if( rename( output->staging, output->destination ) != 0 ) output->failed = 1;
#endif
		}
	}
	ok = !output->failed;
	release_output( output );
	return ok;
}

/* end of tooling/semantic-output.c */
