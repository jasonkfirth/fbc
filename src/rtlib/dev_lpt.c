/*
    Project: FreeBASIC Runtime Library
    ----------------------------------

    File: dev_lpt.c

    Purpose:

        Provide the common OPEN LPT device layer used by the platform printer
        backends.

    Responsibilities:

        - parse and normalize LPT device names
        - manage shared printer device state and LPRINT redirection
        - dispatch writes and closes through the platform printer driver

    This file intentionally does NOT contain:

        - Win32 spooler calls
        - DOS parallel-port I/O
        - Unix print-spooler command handling
*/

#include "fb.h"

static FB_FILE *fb_DevLptFindDeviceByName( int iPort, char * filename, int no_redir )
{
  size_t i;
	/* Test if the printer is already open. */
	for( i=0; i<FB_MAX_FILES; ++i )
	{
		FB_FILE *handle = __fb_ctx.fileTB + i;
		if( handle->type == FB_FILE_TYPE_PRINTER )
		{
			if( no_redir==FALSE || handle->redirection_to==NULL )
			{
				DEV_LPT_INFO *devInfo = (DEV_LPT_INFO*) handle->opaque;
				if( devInfo )
				{
					if( iPort == 0 || iPort == devInfo->iPort )
					{
						if( strcmp(devInfo->pszDevice, filename)==0 ) {
								/* bugcheck */
								DBG_ASSERT( handle!=FB_HANDLE_PRINTER
												&& handle!=FB_HANDLE_PRINTER );
								return handle;
						}
					}
				}
			}
		}
	}
	return NULL;
}

static char *fb_DevLptMakeDeviceName( DEV_LPT_PROTOCOL *lpt_proto )
{
	if( lpt_proto )
	{
		size_t proto_len = strlen( lpt_proto->proto );
		size_t name_len = strlen( lpt_proto->name );
		size_t path_len;
		int written;
		char *p;

		if( name_len > ((size_t)-1) - 2 )
			return NULL;
		if( proto_len > ((size_t)-1) - name_len - 2 )
			return NULL;

		path_len = proto_len + name_len + 2;
		p = calloc( path_len, 1 );
		if( p==NULL )
			return NULL;

		written = snprintf( p, path_len, "%s:%s", lpt_proto->proto, lpt_proto->name );
		if( (written < 0) || ((size_t)written >= path_len) )
		{
			free( p );
			return NULL;
		}

		return p;
	}
	return NULL;
}

static FB_FILE_HOOKS hooks_dev_lpt = {
    NULL,
    fb_DevLptClose,
    NULL,
    NULL,
    NULL,
    NULL,
    fb_DevLptWrite,
    fb_DevLptWriteWstr,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

/*:::::*/
int fb_DevLptOpen( FB_FILE *handle, const char *filename, size_t filename_len )
{
		DEV_LPT_PROTOCOL *lpt_proto = NULL;
    DEV_LPT_INFO *devInfo;
    FB_FILE *redir_handle = NULL;
		FB_FILE *tmp_handle = NULL;
    int res;

    if (!fb_DevLptParseProtocol( &lpt_proto, filename, filename_len , TRUE) )
		{
			if( lpt_proto )
				free( lpt_proto );
			return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
		}
		if( lpt_proto==NULL )
			return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );

    FB_LOCK();

    /* Determine the port number and a normalized device name */
    devInfo = (DEV_LPT_INFO*) calloc(1, sizeof(DEV_LPT_INFO));
		if( devInfo==NULL )
		{
			free( lpt_proto );
			FB_UNLOCK();
			return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
		}

    devInfo->uiRefCount = 1;
		devInfo->iPort = lpt_proto->iPort;
		devInfo->pszDevice = fb_DevLptMakeDeviceName( lpt_proto );
		if( devInfo->pszDevice==NULL )
		{
			free( devInfo );
			free( lpt_proto );
			FB_UNLOCK();
			return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
		}

		devInfo->driver_opaque = NULL;

    /* Test if the printer is already open. */
		tmp_handle = fb_DevLptFindDeviceByName( devInfo->iPort, devInfo->pszDevice, FALSE );
		if( tmp_handle )
		{
			free(devInfo);
      redir_handle = tmp_handle;
			devInfo = (DEV_LPT_INFO*) tmp_handle->opaque;
      ++devInfo->uiRefCount;
		}

    /* Open the printer if not opened already */
    if( devInfo->driver_opaque == NULL ) {
        res = fb_PrinterOpen( devInfo, devInfo->iPort, filename );
    } else {
        res = fb_ErrorSetNum( FB_RTERROR_OK );
        if( FB_HANDLE_USED(redir_handle) ) {
            /* We only allow redirection between OPEN "LPT1:" and LPRINT */
            if( handle==FB_HANDLE_PRINTER ) {
                redir_handle->redirection_to = handle;
                handle->width = redir_handle->width;
                handle->line_length = redir_handle->line_length;
            } else {
                handle->redirection_to = redir_handle;
            }
        } else {
            handle->width = 80;
        }
    }

    if( res == FB_RTERROR_OK ) {
        handle->hooks = &hooks_dev_lpt;
        handle->opaque = devInfo;
				handle->type = FB_FILE_TYPE_PRINTER;
    } else {
        if( devInfo->pszDevice )
            free( devInfo->pszDevice );
        free( devInfo );
    }

		free( lpt_proto );

    FB_UNLOCK();

	return res;
}

int fb_DevPrinterSetWidth( const char *pszDevice, int width, int default_width )
{
		FB_FILE *tmp_handle = NULL;
    int cur = ((default_width==-1) ? 80 : default_width);
    char *pszDev;
		DEV_LPT_PROTOCOL *lpt_proto = NULL;

    if (!fb_DevLptParseProtocol( &lpt_proto, pszDevice, strlen(pszDevice), TRUE) )
		{
			if( lpt_proto )
				free( lpt_proto );
			return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
		}
		if( lpt_proto==NULL )
			return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );

		pszDev = fb_DevLptMakeDeviceName( lpt_proto );
		if( pszDev==NULL )
		{
			free( lpt_proto );
			return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
		}

    /* Test all printers. */
		tmp_handle = fb_DevLptFindDeviceByName( lpt_proto->iPort, pszDev, TRUE );
		if( tmp_handle )
		{
      if( width!=-1 )
          tmp_handle->width = width;
      cur = tmp_handle->width;
		}

		free( lpt_proto );
    free(pszDev);

    return cur;
}

int fb_DevPrinterGetOffset( const char *pszDevice )
{
		FB_FILE *tmp_handle = NULL;
    int cur = 0;
    char *pszDev;
		DEV_LPT_PROTOCOL *lpt_proto = NULL;

    if (!fb_DevLptParseProtocol( &lpt_proto, pszDevice, strlen(pszDevice), TRUE) )
		{
			if( lpt_proto )
				free( lpt_proto );
			return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
		}
		if( lpt_proto==NULL )
			return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );

		pszDev = fb_DevLptMakeDeviceName( lpt_proto );
		if( pszDev==NULL )
		{
			free( lpt_proto );
			return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
		}

    /* Test all printers. */
		tmp_handle = fb_DevLptFindDeviceByName( lpt_proto->iPort, pszDev, TRUE );
		if( tmp_handle )
      cur = tmp_handle->line_length;

		free( lpt_proto );
    free(pszDev);

    return cur;

}

/* end of dev_lpt.c */
