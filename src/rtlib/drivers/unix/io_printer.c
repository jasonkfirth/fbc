/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/io_printer.c
 * Purpose: Linux printer driver.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

/* DEV_LPT_INFO->driver_opaque := (FILE *) file_handle */

#if defined(__GNUC__) || defined(__clang__)
#define FB_LP_PRINTF_FORMAT( format_index, first_arg ) \
	__attribute__((format(printf, format_index, first_arg)))
#else
#define FB_LP_PRINTF_FORMAT( format_index, first_arg )
#endif

static char lp_buf[256];

static int append_lp_cmd( char *dst, size_t dst_len, const char *fmt, ... )
	FB_LP_PRINTF_FORMAT( 3, 4 );

static int append_lp_cmd( char *dst, size_t dst_len, const char *fmt, ... )
{
	va_list args;
	size_t len;
	int written;

	len = strlen( dst );
	if( len >= dst_len )
		return FALSE;

	va_start( args, fmt );
	written = vsnprintf( dst + len, dst_len - len, fmt, args );
	va_end( args );

	return (written >= 0) && ((size_t)written < (dst_len - len));
}

static int exec_lp_cmd( const char *cmd, int test_default )
{
	int have_default = TRUE; // Assume a default printer
	int result = -1;

	FILE *fp = popen( cmd, "r" );
	if( fp ) {
		while( !feof( fp ) ) {
			if( !fgets( lp_buf, 256, fp ) ) {
				if( test_default && have_default && (strlen( lp_buf ) > 2) )
					if( (lp_buf[0] == 'n' || lp_buf[0] == 'N') &&
					    (lp_buf[1] == 'o' || lp_buf[1] == 'O') )
						have_default = FALSE;
			}
		}

		result = pclose( fp ) >> 8;

		if( test_default && !have_default )
			result = -1;
	}

	return result;
}

int fb_PrinterOpen( DEV_LPT_INFO *devInfo, int iPort, const char *pszDeviceRaw )
{
	int result;
	char *filename = NULL;
	/* /dev/lp plus a signed 32-bit decimal port number and its terminator. */
	char port_filename[7 + 11 + 1];
	FILE *fp;

	DEV_LPT_PROTOCOL *lpt_proto = NULL;
	if ( !fb_DevLptParseProtocol( &lpt_proto, pszDeviceRaw, strlen(pszDeviceRaw), TRUE ) )
	{
		if( lpt_proto!=NULL )
			free(lpt_proto);
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}
	if( lpt_proto==NULL )
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );

	devInfo->iPort = iPort;

	if( devInfo->iPort==0 ) {
		/* Use spooler */

		/* create a buffer for our commands */
		{
			size_t name_len = strlen( lpt_proto->name );
			size_t title_len = strlen( lpt_proto->title );
			size_t filename_len;

			if( title_len > SIZE_MAX - 128 || name_len > SIZE_MAX - title_len - 128 )
			{
				result = FB_RTERROR_OUTOFMEM;
				goto done;
			}

			filename_len = name_len + title_len + 128;
			filename = malloc( filename_len );
			if( filename == NULL ) {
				result = FB_RTERROR_OUTOFMEM;
				goto done;
			}
			filename[0] = '\0';

			/* set destination, if not default */
			if( lpt_proto->name && *lpt_proto->name )
			{
				/* does printer exist */
				if( !append_lp_cmd( filename, filename_len,
					"lpstat -v \"%s\" 2>&1 ", lpt_proto->name ) )
				{
					result = FB_RTERROR_OUTOFMEM;
					goto done;
				}
				if( exec_lp_cmd( filename, FALSE ) != 0 )
				{
					result = FB_RTERROR_FILENOTFOUND;
					goto done;
				}

				/* build command for spooler */
				filename[0] = '\0';
				if( !append_lp_cmd( filename, filename_len,
					"lp -d \"%s\" ", lpt_proto->name ) )
				{
					result = FB_RTERROR_OUTOFMEM;
					goto done;
				}
			}
			else
			{
				/* is there a default printer */
				if( !append_lp_cmd( filename, filename_len, "lpstat -d 2>&1" ) )
				{
					result = FB_RTERROR_OUTOFMEM;
					goto done;
				}
				if( exec_lp_cmd( filename, TRUE ) != 0 )
				{
					result = FB_RTERROR_FILENOTFOUND;
					goto done;
				}
				/* build command for spooler */
				filename[0] = '\0';
				if( !append_lp_cmd( filename, filename_len, "lp " ) )
				{
					result = FB_RTERROR_OUTOFMEM;
					goto done;
				}
			}

			/* set title, if not default */
			if( *lpt_proto->title )
			{
				if( !append_lp_cmd( filename, filename_len,
					"-t \"%s\"", lpt_proto->title ) )
				{
					result = FB_RTERROR_OUTOFMEM;
					goto done;
				}
			}
			else
			{
				if( !append_lp_cmd( filename, filename_len, "-t \"FreeBASIC document\"" ) )
				{
					result = FB_RTERROR_OUTOFMEM;
					goto done;
				}
			}

			/* do not print job id */
			if( !append_lp_cmd( filename, filename_len, " -s -" ) )
			{
				result = FB_RTERROR_OUTOFMEM;
				goto done;
			}

			{
				char *ptr = filename;
				while ((ptr = strpbrk(ptr, "`&;|>^$\\")) != NULL)
					*ptr = '_';
			}

			/* do not print error messages */
			if( !append_lp_cmd( filename, filename_len, " &> /dev/null" ) )
			{
				result = FB_RTERROR_OUTOFMEM;
				goto done;
			}
		}

		fp = popen( filename, "w" );
		if(fp == NULL )
		{
			devInfo->driver_opaque = NULL;
			result = FB_RTERROR_FILENOTFOUND;
		}
		else
		{
			devInfo->driver_opaque = fp;
			result = FB_RTERROR_OK;
		}

	} else {
		/* use direct port io */
		if( devInfo->iPort < 1 ||
		    snprintf(port_filename, sizeof(port_filename), "/dev/lp%d", (devInfo->iPort-1)) >= (int)sizeof(port_filename) )
		{
			result = FB_RTERROR_ILLEGALFUNCTIONCALL;
			goto done;
		}
		fp = fopen(port_filename, "wb");

		if( fp==NULL ) {
			devInfo->driver_opaque = NULL;
			result = FB_RTERROR_FILENOTFOUND;
		} else {
			devInfo->driver_opaque = fp;
			result = FB_RTERROR_OK;
		}

	}

done:
	free(filename);
	free(lpt_proto);
	return fb_ErrorSetNum( result );
}

int fb_PrinterWrite( DEV_LPT_INFO *devInfo, const void *data, size_t length )
{
	FILE *fp = (FILE*)  devInfo->driver_opaque;
	if( length == 0 )
		return fb_ErrorSetNum( FB_RTERROR_OK );
	if( fwrite( data, length, 1, fp ) != 1 ) {
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

int fb_PrinterWriteWstr( DEV_LPT_INFO *devInfo, const FB_WCHAR *buffer, size_t chars )
{
	FILE *fp = (FILE *) devInfo->driver_opaque;
	char local_buffer[256], *temp = local_buffer;
	size_t capacity;
	ssize_t bytes = 0;
	int result = FB_RTERROR_OK;

	if( chars == 0 )
		return fb_ErrorSetNum( FB_RTERROR_OK );
	if( buffer == NULL )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	/* The converter's count and result are signed, with at most four UTF-8
	   bytes per input unit and one byte of temporary terminator space. */
	if( chars > (size_t)(FB_STRSIZEMSK - 1) / 4 )
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
	capacity = chars * 4 + 1;
	if( capacity > sizeof( local_buffer ) ) {
		temp = malloc( capacity );
		if( temp == NULL )
			return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
	}

	/* Write the converter's byte count, including embedded zero bytes. */
	if( fb_WCharToUTF( FB_FILE_ENCOD_UTF8, buffer, (ssize_t)chars, temp, &bytes ) == NULL ||
	    bytes < 0 || (size_t)bytes >= capacity )
		result = FB_RTERROR_ILLEGALFUNCTIONCALL;
	else if( bytes != 0 && fwrite( temp, 1, (size_t)bytes, fp ) != (size_t)bytes )
		result = FB_RTERROR_FILEIO;

	if( temp != local_buffer )
		free( temp );
	return fb_ErrorSetNum( result );
}


int fb_PrinterClose( DEV_LPT_INFO *devInfo )
{
	if( devInfo->iPort == 0 )
	{
		/* close spooler */
		int result = ( pclose( (FILE *) devInfo->driver_opaque ) >> 8 );
		devInfo->driver_opaque = NULL;
		if( result )
			return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}
	else 
	{
		/* close direct port io */
		fclose( (FILE *) devInfo->driver_opaque );
		devInfo->driver_opaque = NULL;
	}

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of drivers/unix/io_printer.c */
