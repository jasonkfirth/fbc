/*
    Project: FreeBASIC Runtime Library
    ----------------------------------

    File: win32/io_serial.c

    Purpose:

        Implement OPEN COM serial-port access for native Windows targets.

    Responsibilities:

        - translate FreeBASIC OPEN COM options into Win32 DCB settings
        - open COM1 through COM9 and extended COM device names
        - read, write, query, and close the underlying Win32 HANDLE
        - keep the implementation usable by both 32-bit and 64-bit MinGW

    This file intentionally does NOT contain:

        - OPEN COM option parsing
        - DOS IRQ based serial handling
        - Unix termios serial handling
*/

#include "../fb.h"
#include "../io_serial_private.h"

#define GET_MSEC_TIME() ((DWORD) (fb_Timer() * 1000.0))

/* ------------------------------------------------------------------------- */
/* Windows serial helpers                                                    */
/* ------------------------------------------------------------------------- */

/*
    Win32 COM port names

    Windows accepts the short names COM1 through COM9 directly.  Ports above
    COM9 must be opened through the Win32 device namespace, for example
    "\\\\.\\COM10".  The FreeBASIC parser keeps the trailing ':' used by
    OPEN COM, so this helper removes it after building the Win32 path.
*/
static char *fb_hSerialMakeDeviceName( int iPort, const char *pszDevice )
{
    const char *prefix = "";
    size_t prefix_len = 0;
    size_t device_len;
    char *pszDev, *p;

    if( iPort==0 )
        pszDevice = "COM1:";
    else if( iPort > 9 ) {
        prefix = "\\\\.\\";
        prefix_len = strlen( prefix );
    }

    device_len = strlen( pszDevice );

    if( device_len > ((size_t)-1) - prefix_len - 1 )
        return NULL;

    pszDev = calloc( prefix_len + device_len + 1, 1 );
    if( pszDev==NULL )
        return NULL;

    strcpy( pszDev, prefix );
    strcat( pszDev, pszDevice );

    p = strchr( pszDev, ':' );
    if( p )
        *p = '\0';

    return pszDev;
}

static int fb_hSerialValidateOptions( FB_SERIAL_OPTIONS *options )
{
    DBG_ASSERT( options!=NULL );

    if( options->uiSpeed==0 )
        return FALSE;

    if( options->uiDataBits < 5 || options->uiDataBits > 8 )
        return FALSE;

    if( options->StopBits==FB_SERIAL_STOP_BITS_1_5 &&
        options->uiDataBits!=5 )
        return FALSE;

    return TRUE;
}

static int fb_hSerialWaitSignal( HANDLE hDevice, DWORD dwMask, DWORD dwResult, DWORD dwTimeout )
{
    DWORD dwStartTime = GET_MSEC_TIME();
    DWORD dwModemStatus = 0;

    if( !GetCommModemStatus( hDevice, &dwModemStatus ) )
        return FALSE;

    while ( ((GET_MSEC_TIME() - dwStartTime) <= dwTimeout)
            && ((dwModemStatus & dwMask)!=dwResult) )
    {
        if( !GetCommModemStatus( hDevice, &dwModemStatus ) )
            return FALSE;
    }
    return ((dwModemStatus & dwMask)==dwResult);
}

static
int fb_hSerialCheckLines( HANDLE hDevice, FB_SERIAL_OPTIONS *pOptions )
{
    DBG_ASSERT( pOptions!=NULL );
    if( pOptions->DurationCD!=0 ) {
        if( !fb_hSerialWaitSignal( hDevice,
                                   MS_RLSD_ON, MS_RLSD_ON,
                                   pOptions->DurationCD ) )
            return FALSE;
    }

    if( pOptions->DurationDSR!=0 ) {
        if( !fb_hSerialWaitSignal( hDevice,
                                   MS_DSR_ON, MS_DSR_ON,
                                   pOptions->DurationDSR ) )
            return FALSE;
    }
    return TRUE;
}

int fb_SerialOpen( FB_FILE *handle,
                   int iPort, FB_SERIAL_OPTIONS *options,
                   const char *pszDevice, void **ppvHandle )
{
    DWORD dwDefaultTxBufferSize = 16384;
    DWORD dwDefaultRxBufferSize = 16384;
    DWORD dwDesiredAccess = 0;
    char *pszDev;
    HANDLE hDevice;
    int res;

    /* The IRQ stuff is not supported on Windows ... */
    if( options->IRQNumber!=0 )
        return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

    if( !fb_hSerialValidateOptions( options ) )
        return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

    res = fb_ErrorSetNum( FB_RTERROR_OK );

    switch( handle->access ) {
    case FB_FILE_ACCESS_READ:
        dwDesiredAccess = GENERIC_READ;
        break;
    case FB_FILE_ACCESS_WRITE:
        dwDesiredAccess = GENERIC_WRITE;
        break;
    case FB_FILE_ACCESS_READWRITE:
    case FB_FILE_ACCESS_ANY:
        dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
        break;
    }

    pszDev = fb_hSerialMakeDeviceName( iPort, pszDevice );
    if( pszDev==NULL )
        return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );

    if( iPort==0 )
        iPort = 1;

#if 0
    /* Configure COM properties explicitly from the requested serial options. */
    COMMCONFIG cc;
    if( !GetDefaultCommConfig( pszDev, &cc, &dwSizeCC ) ) {
    }
#endif

    /* Open device */
    hDevice =
        CreateFileA( pszDev,
                     dwDesiredAccess,
                     0 /* dwShareMode: must be zero (exclusive access) for COM port according to MSDN */,
                     NULL,
                     OPEN_EXISTING,
                     0,
                     NULL );

    free( pszDev );

    if( hDevice==INVALID_HANDLE_VALUE )
        return fb_ErrorSetNum( FB_RTERROR_FILENOTFOUND );

    /* Set rx/tx buffer sizes */
    if( res==FB_RTERROR_OK ) {
        COMMPROP prop;
        memset( &prop, 0, sizeof( COMMPROP ) );
        prop.wPacketLength = sizeof( COMMPROP );

        if( !GetCommProperties( hDevice, &prop ) ) {
            res = fb_ErrorSetNum( FB_RTERROR_NOPRIVILEGES );
        } else {
            if( prop.dwCurrentTxQueue ) {
                dwDefaultTxBufferSize = prop.dwCurrentTxQueue;
            } else if( prop.dwMaxTxQueue ) {
                dwDefaultTxBufferSize = prop.dwMaxTxQueue;
            }

					  if( prop.dwCurrentRxQueue ) {
                dwDefaultRxBufferSize = prop.dwCurrentRxQueue;
            } else if( prop.dwMaxRxQueue ) {
                dwDefaultRxBufferSize = prop.dwMaxRxQueue;
            }

						if( options->TransmitBuffer )
							dwDefaultTxBufferSize = options->TransmitBuffer;

						if( options->ReceiveBuffer )
							dwDefaultRxBufferSize = options->ReceiveBuffer;


            if( !SetupComm( hDevice,
                            dwDefaultRxBufferSize,
                            dwDefaultTxBufferSize ) )
            {
                res = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
            }
        }
    }

    /* set timeouts */
    if( res==FB_RTERROR_OK ) {
        COMMTIMEOUTS timeouts;
        if( !GetCommTimeouts( hDevice, &timeouts ) ) {
            res = fb_ErrorSetNum( FB_RTERROR_NOPRIVILEGES );
        } else {
            if( options->DurationCTS!=0 ) {
                timeouts.ReadIntervalTimeout = options->DurationCTS;
                timeouts.ReadTotalTimeoutMultiplier =
                    timeouts.ReadTotalTimeoutConstant = 0;
            }
            if( !SetCommTimeouts( hDevice, &timeouts ) ) {
                res = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
            }
        }
    }

    /* setup generic COM port configuration */
    if( res==FB_RTERROR_OK ) {
        DCB dcb;
        memset( &dcb, 0, sizeof( DCB ) );
        dcb.DCBlength = sizeof( DCB );
        if( !GetCommState( hDevice, &dcb ) ) {
            res = fb_ErrorSetNum( FB_RTERROR_NOPRIVILEGES );
        } else {
            dcb.BaudRate = options->uiSpeed;
            dcb.fBinary = !options->AddLF; /* Windows serial ports are byte streams. */
            dcb.fParity = options->CheckParity;
            dcb.fOutxCtsFlow = options->DurationCTS!=0;
            dcb.fDtrControl = ( (options->KeepDTREnabled) ? DTR_CONTROL_ENABLE : DTR_CONTROL_DISABLE );

            /* Not sure about this one ... */
            dcb.fDsrSensitivity = options->DurationDSR!=0;
            dcb.fOutxDsrFlow = FALSE;

            /* No XON/XOFF */
            dcb.fOutX = FALSE;
            dcb.fInX = FALSE;
            dcb.fNull = FALSE;

            /* Not sure about this one ... */
            dcb.fRtsControl = ( ( options->SuppressRTS ) ? RTS_CONTROL_DISABLE : RTS_CONTROL_HANDSHAKE );

            dcb.fAbortOnError = FALSE;
            dcb.ByteSize = (BYTE) options->uiDataBits;

            switch ( options->Parity ) {
            case FB_SERIAL_PARITY_NONE:
                dcb.Parity = NOPARITY;
                break;
            case FB_SERIAL_PARITY_EVEN:
                dcb.Parity = EVENPARITY;
                break;
            case FB_SERIAL_PARITY_ODD:
                dcb.Parity = ODDPARITY;
                break;
            case FB_SERIAL_PARITY_SPACE:
                dcb.Parity = SPACEPARITY;
                break;
            case FB_SERIAL_PARITY_MARK:
                dcb.Parity = MARKPARITY;
                break;
            }

            switch ( options->StopBits ) {
            case FB_SERIAL_STOP_BITS_1:
                dcb.StopBits = ONESTOPBIT;
                break;
            case FB_SERIAL_STOP_BITS_1_5:
                dcb.StopBits = ONE5STOPBITS;
                break;
            case FB_SERIAL_STOP_BITS_2:
                dcb.StopBits = TWOSTOPBITS;
                break;
            }

            if( !SetCommState( hDevice, &dcb ) ) {
                res = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
            } else {
                EscapeCommFunction( hDevice, SETDTR );
						}
        }
    }

    if( !fb_hSerialCheckLines( hDevice, options ) ) {
        res = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
    }

    if( res!=FB_RTERROR_OK ) {
        CloseHandle( hDevice );
    } else {
        W32_SERIAL_INFO *pInfo = calloc( 1, sizeof(W32_SERIAL_INFO) );
        if( pInfo==NULL ) {
            CloseHandle( hDevice );
            return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
        }

        DBG_ASSERT( ppvHandle!=NULL );
        *ppvHandle = pInfo;
        pInfo->hDevice = hDevice;
        pInfo->iPort = iPort;
        pInfo->pOptions = options;
    }

    return res;
}

int fb_SerialGetRemaining( FB_FILE *handle,
                           void *pvHandle, fb_off_t *pLength )
{
    W32_SERIAL_INFO *pInfo = (W32_SERIAL_INFO*) pvHandle;
    DWORD dwErrors;
    COMSTAT Status;

    (void)handle;

    if( !ClearCommError( pInfo->hDevice, &dwErrors, &Status ) )
        return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
    if( pLength )
        *pLength = (long) Status.cbInQue;
    return fb_ErrorSetNum( FB_RTERROR_OK );
}

int fb_SerialWrite( FB_FILE *handle,
                    void *pvHandle, const void *data, size_t length )
{
    W32_SERIAL_INFO *pInfo = (W32_SERIAL_INFO*) pvHandle;
    DWORD dwWriteCount;

    (void)handle;

    if( !fb_hSerialCheckLines( pInfo->hDevice, pInfo->pOptions ) ) {
        return fb_ErrorSetNum( FB_RTERROR_FILEIO );
    }

    if( !WriteFile( pInfo->hDevice,
                   data,
                   length,
                   &dwWriteCount,
                   NULL ) )
        return fb_ErrorSetNum( FB_RTERROR_FILEIO );

    if( length != (size_t) dwWriteCount )
        return fb_ErrorSetNum( FB_RTERROR_FILEIO );

    return fb_ErrorSetNum( FB_RTERROR_OK );
}

int fb_SerialRead( FB_FILE *handle,
                   void *pvHandle, void *data, size_t *pLength )
{
    W32_SERIAL_INFO *pInfo = (W32_SERIAL_INFO*) pvHandle;
    DWORD dwReadCount;

    (void)handle;

    DBG_ASSERT( pLength!=NULL );

    if( !fb_hSerialCheckLines( pInfo->hDevice, pInfo->pOptions ) ) {
        return fb_ErrorSetNum( FB_RTERROR_FILEIO );
    }

    if( !ReadFile( pInfo->hDevice,
                   data,
                   *pLength,
                   &dwReadCount,
                   NULL ) )
        return fb_ErrorSetNum( FB_RTERROR_FILEIO );

    *pLength = (size_t) dwReadCount;

    return fb_ErrorSetNum( FB_RTERROR_OK );
}

int fb_SerialClose( FB_FILE *handle, void *pvHandle )
{
    W32_SERIAL_INFO *pInfo = (W32_SERIAL_INFO*) pvHandle;

    (void)handle;

    CloseHandle( pInfo->hDevice );
    free(pInfo);
    return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of win32/io_serial.c */
