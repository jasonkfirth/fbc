/*
    Project: FreeBASIC Runtime Library
    ----------------------------------

    File: fb_serial.h

    Purpose:

        Define the internal serial-port ABI shared by OPEN COM, the portable
        fbcom.bi control API, and platform serial backends.

    Responsibilities:

        - describe parsed OPEN COM options
        - define portable modem status, error, capability, and purge flags
        - declare stream and raw-control backend entry points

    This file intentionally does NOT contain:

        - OPEN COM option parsing
        - platform-native handle layouts
        - serial driver implementations
*/

#ifndef __FB_SERIAL_H__
#define __FB_SERIAL_H__

typedef enum _FB_SERIAL_PARITY {
    FB_SERIAL_PARITY_NONE,
    FB_SERIAL_PARITY_EVEN,
    FB_SERIAL_PARITY_ODD,
    FB_SERIAL_PARITY_SPACE,
    FB_SERIAL_PARITY_MARK
} FB_SERIAL_PARITY;

typedef enum _FB_SERIAL_STOP_BITS {
    FB_SERIAL_STOP_BITS_1,
    FB_SERIAL_STOP_BITS_1_5,
	FB_SERIAL_STOP_BITS_2
} FB_SERIAL_STOP_BITS;

typedef struct _FB_SERIAL_OPTIONS {
	unsigned           uiSpeed;
    unsigned           uiDataBits;
    FB_SERIAL_PARITY   Parity;
    FB_SERIAL_STOP_BITS StopBits;
    unsigned           DurationCTS;        /* CS[msec] */
    unsigned           DurationDSR;        /* DS[msec] */
    unsigned           DurationCD;         /* CD[msec] */
    unsigned           OpenTimeout;        /* OP[msec] */
    int                SuppressRTS;        /* RS */
    int                AddLF;              /* LF, or ASC, or BIN */
    int                CheckParity;        /* PE */
    int                KeepDTREnabled;     /* DT */
    int                DiscardOnError;     /* FE */
    int                IgnoreAllErrors;    /* ME */
    unsigned           IRQNumber;          /* IR2..IR15 */
    unsigned           TransmitBuffer;     /* TBn - a value 0 means: default value */
    unsigned           ReceiveBuffer;      /* RBn - a value 0 means: default value */
} FB_SERIAL_OPTIONS;

/* ------------------------------------------------------------------------- */
/* Portable serial control ABI                                               */
/* ------------------------------------------------------------------------- */

#define FB_COM_LINE_CTS 0x00000001u
#define FB_COM_LINE_DSR 0x00000002u
#define FB_COM_LINE_DCD 0x00000004u
#define FB_COM_LINE_RI  0x00000008u
#define FB_COM_LINE_RTS 0x00000010u
#define FB_COM_LINE_DTR 0x00000020u

#define FB_COM_CAP_INPUT_LINES  0x00000001u
#define FB_COM_CAP_OUTPUT_LINES 0x00000002u
#define FB_COM_CAP_BREAK        0x00000004u
#define FB_COM_CAP_PURGE_RX     0x00000008u
#define FB_COM_CAP_PURGE_TX     0x00000010u
#define FB_COM_CAP_RX_QUEUE     0x00000020u
#define FB_COM_CAP_TX_QUEUE     0x00000040u
#define FB_COM_CAP_ERRORS       0x00000080u

#define FB_COM_ERROR_BREAK       0x00000001u
#define FB_COM_ERROR_FRAMING     0x00000002u
#define FB_COM_ERROR_PARITY      0x00000004u
#define FB_COM_ERROR_OVERRUN     0x00000008u
#define FB_COM_ERROR_RX_OVERFLOW 0x00000010u

#define FB_COM_PURGE_RX 0x00000001u
#define FB_COM_PURGE_TX 0x00000002u

typedef struct _FB_COM_STATUS {
	unsigned int capabilities;
	unsigned int lines;
	unsigned int errors;
	unsigned int rx_queued;
	unsigned int tx_queued;
} FB_COM_STATUS;

STATIC_ASSERT( sizeof( unsigned int ) == 4 );
STATIC_ASSERT( sizeof( FB_COM_STATUS ) == 20 );

/* ------------------------------------------------------------------------- */
/* Platform serial backend                                                   */
/* ------------------------------------------------------------------------- */

       int          fb_DevSerialSetWidth( const char *pszDevice, int width, int default_width );
       int          fb_SerialOpen       ( FB_FILE *handle, int iPort, FB_SERIAL_OPTIONS *options, const char *pszDevice, void **ppvHandle );
       int          fb_SerialGetRemaining( FB_FILE *handle, void *pvHandle, fb_off_t *pLength );
       int          fb_SerialWrite      ( FB_FILE *handle, void *pvHandle, const void *data, size_t length );
       int          fb_SerialWriteWstr  ( FB_FILE *handle, void *pvHandle, const FB_WCHAR *data, size_t length );
       int          fb_SerialRead       ( FB_FILE *handle, void *pvHandle, void *data, size_t *pLength );
       int          fb_SerialReadWstr   ( FB_FILE *handle, void *pvHandle, FB_WCHAR *data, size_t *pLength );
       int          fb_SerialClose      ( FB_FILE *handle, void *pvHandle );
       int          fb_SerialGetStatus  ( FB_FILE *handle, void *pvHandle, FB_COM_STATUS *status );
       int          fb_SerialSetLines   ( FB_FILE *handle, void *pvHandle, unsigned int mask, unsigned int values );
       int          fb_SerialSetBreak   ( FB_FILE *handle, void *pvHandle, int enabled );
       int          fb_SerialPurge      ( FB_FILE *handle, void *pvHandle, unsigned int queues );

/* ------------------------------------------------------------------------- */
/* Public fbcom.bi runtime entry points                                      */
/* ------------------------------------------------------------------------- */

FBCALL int fb_ComGetStatus( int file_number, FB_COM_STATUS *status );
FBCALL int fb_ComSetLines( int file_number, unsigned int mask, unsigned int values );
FBCALL int fb_ComSetBreak( int file_number, int enabled );
FBCALL int fb_ComPurge( int file_number, unsigned int queues );

#endif

/* end of fb_serial.h */
