/* write [#] functions */

#include "fb.h"

static void hAppendWriteSuffix( char *buffer, size_t buffer_len, const char *suffix )
{
	size_t len = strlen( buffer );
	size_t suffix_len = strlen( suffix );

	if( len >= buffer_len )
		return;

	if( suffix_len >= buffer_len - len )
		suffix_len = buffer_len - len - 1;

	memcpy( buffer + len, suffix, suffix_len );
	buffer[len + suffix_len] = '\0';
}

/*:::::*/
FBCALL void fb_WriteSingle ( int fnum, float val, int mask )
{
	char buffer[8+1+8+1+2];

	fb_hFloat2Str( (double)val, buffer, 7, 0 );

	if( mask & FB_PRINT_BIN_NEWLINE )
		hAppendWriteSuffix( buffer, sizeof( buffer ), FB_BINARY_NEWLINE );
	else if( mask & FB_PRINT_NEWLINE )
		hAppendWriteSuffix( buffer, sizeof( buffer ), FB_NEWLINE );
	else
		hAppendWriteSuffix( buffer, sizeof( buffer ), "," );

	fb_hFilePrintBufferEx( FB_FILE_TO_HANDLE( fnum ), buffer, strlen( buffer ) );

}

/*:::::*/
FBCALL void fb_WriteDouble ( int fnum, double val, int mask )
{
	char buffer[16+1+8+1];

	fb_hFloat2Str( val, buffer, 16, 0 );

	if( mask & FB_PRINT_BIN_NEWLINE )
		hAppendWriteSuffix( buffer, sizeof( buffer ), FB_BINARY_NEWLINE );
	else if( mask & FB_PRINT_NEWLINE )
		hAppendWriteSuffix( buffer, sizeof( buffer ), FB_NEWLINE );
	else
		hAppendWriteSuffix( buffer, sizeof( buffer ), "," );

	fb_hFilePrintBufferEx( FB_FILE_TO_HANDLE( fnum ), buffer, strlen( buffer ) );
}
