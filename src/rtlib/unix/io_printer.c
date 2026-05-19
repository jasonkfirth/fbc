/* Linux printer driver */

#include "../fb.h"

/* DEV_LPT_INFO->driver_opaque := (FILE *) file_handle */

static char lp_buf[256];

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

			if( name_len > ((size_t)-1) - title_len - 128 )
			{
				free(lpt_proto);
				return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
			}

			filename_len = name_len + title_len + 128;
			filename = alloca( filename_len );
			filename[0] = '\0';

			/* set destination, if not default */
			if( lpt_proto->name && *lpt_proto->name )
			{
				/* does printer exist */
				if( !append_lp_cmd( filename, filename_len,
					"lpstat -v \"%s\" 2>&1 ", lpt_proto->name ) )
				{
					free(lpt_proto);
					return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
				}
				if( exec_lp_cmd( filename, FALSE ) != 0 )
				{
					free(lpt_proto);
					return fb_ErrorSetNum( FB_RTERROR_FILENOTFOUND );
				}

				/* build command for spooler */
				filename[0] = '\0';
				if( !append_lp_cmd( filename, filename_len,
					"lp -d \"%s\" ", lpt_proto->name ) )
				{
					free(lpt_proto);
					return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
				}
			}
			else
			{
				/* is there a default printer */
				if( !append_lp_cmd( filename, filename_len, "lpstat -d 2>&1" ) )
				{
					free(lpt_proto);
					return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
				}
				if( exec_lp_cmd( filename, TRUE ) != 0 )
				{
					free(lpt_proto);
					return fb_ErrorSetNum( FB_RTERROR_FILENOTFOUND );
				}
				/* build command for spooler */
				filename[0] = '\0';
				if( !append_lp_cmd( filename, filename_len, "lp " ) )
				{
					free(lpt_proto);
					return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
				}
			}

			/* set title, if not default */
			if( *lpt_proto->title )
			{
				if( !append_lp_cmd( filename, filename_len,
					"-t \"%s\"", lpt_proto->title ) )
				{
					free(lpt_proto);
					return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
				}
			}
			else
			{
				if( !append_lp_cmd( filename, filename_len, "-t \"FreeBASIC document\"" ) )
				{
					free(lpt_proto);
					return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
				}
			}

			/* do not print job id */
			if( !append_lp_cmd( filename, filename_len, " -s -" ) )
			{
				free(lpt_proto);
				return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
			}

			{
				char *ptr = filename;
				while ((ptr = strpbrk(ptr, "`&;|>^$\\")) != NULL)
					*ptr = '_';
			}

			/* do not print error messages */
			if( !append_lp_cmd( filename, filename_len, " &> /dev/null" ) )
			{
				free(lpt_proto);
				return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
			}
		}

		fp = popen( filename, "w" );
		if(fp == NULL )
		{
			devInfo->driver_opaque = NULL;
			result = fb_ErrorSetNum( FB_RTERROR_FILENOTFOUND );
		}
		else
		{
			devInfo->driver_opaque = fp;
			result = fb_ErrorSetNum( FB_RTERROR_OK );
		}

	} else {
		/* use direct port io */
		filename = alloca( 7 + 11 + 1 );
		if( snprintf(filename, 7 + 11 + 1, "/dev/lp%d", (devInfo->iPort-1)) >= 7 + 11 + 1 )
		{
			free(lpt_proto);
			return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
		}
		fp = fopen(filename, "wb");

		if( fp==NULL ) {
			devInfo->driver_opaque = NULL;
			result = fb_ErrorSetNum( FB_RTERROR_FILENOTFOUND );
		} else {
			devInfo->driver_opaque = fp;
			result = fb_ErrorSetNum( FB_RTERROR_OK );
		}

	}

	if( lpt_proto!=NULL )
		free(lpt_proto);

	return result;
}

int fb_PrinterWrite( DEV_LPT_INFO *devInfo, const void *data, size_t length )
{
	FILE *fp = (FILE*)  devInfo->driver_opaque;
	if( fwrite( data, length, 1, fp ) != 1 ) {
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

int fb_PrinterWriteWstr( DEV_LPT_INFO *devInfo, const FB_WCHAR *buffer, size_t chars )
{
	FILE *fp = (FILE *) devInfo->driver_opaque;

	/* !!!FIXME!!! is this ok? */
	ssize_t bytes;
	char *temp = alloca( chars * 4 + 1 );

	fb_WCharToUTF( FB_FILE_ENCOD_UTF8, buffer, chars, temp, &bytes );
	/* add null-term */
	temp[bytes] = '\0';

	if( fwrite( temp, bytes, 1, fp ) != 1 )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	return fb_ErrorSetNum( FB_RTERROR_OK );
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
