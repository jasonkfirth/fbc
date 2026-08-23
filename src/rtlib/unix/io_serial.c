/*
    Project: FreeBASIC Runtime Library
    ----------------------------------

    File: unix/io_serial.c

    Purpose:

        Implement OPEN COM stream I/O for POSIX-style termios devices shared
        by Android, Darwin, the BSDs, and Haiku.

    Responsibilities:

        - map COM numbers or explicit device paths to terminal devices
        - translate OPEN COM framing and flow-control options into termios
        - provide bounded non-blocking reads and writes
        - restore the previous terminal configuration on close

    This file intentionally does NOT contain:

        - modem-line control exposed by fbcom.bi
        - Linux lockdev integration
        - AROS serial.device or RISC OS DeviceFS handling

    Platform behavior:

        Device naming is not standardized across Unix systems. Explicit names
        such as /dev/ttyUSB0: or /dev/cu.usbserial: are always passed through.
        COM1-style names use the conventional built-in callout-device pattern
        for each target, but applications should use an explicit path for USB
        adapters and systems whose device names are assigned dynamically.
*/

#include "../fb.h"
#include "../io_serial_private.h"

#include <errno.h>
#include <fcntl.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#define FB_SERIAL_DEVICE_NAME_MAX 512
#define FB_SERIAL_READ_TIMEOUT_USEC 70000
#define FB_SERIAL_WRITE_TIMEOUT_SEC 3

/* ------------------------------------------------------------------------- */
/* Baud-rate and device-name translation                                     */
/* ------------------------------------------------------------------------- */

typedef struct FB_SERIAL_SPEED_ENTRY {
	unsigned int baud;
	speed_t native_speed;
} FB_SERIAL_SPEED_ENTRY;

static int fb_hSerialGetSpeed( unsigned int baud, speed_t *native_speed )
{
	static const FB_SERIAL_SPEED_ENTRY speeds[] = {
		{ 0, B0 },
#ifdef B50
		{ 50, B50 },
#endif
#ifdef B75
		{ 75, B75 },
#endif
#ifdef B110
		{ 110, B110 },
#endif
#ifdef B134
		{ 134, B134 },
#endif
#ifdef B150
		{ 150, B150 },
#endif
#ifdef B200
		{ 200, B200 },
#endif
		{ 300, B300 },
		{ 600, B600 },
		{ 1200, B1200 },
#ifdef B1800
		{ 1800, B1800 },
#endif
		{ 2400, B2400 },
		{ 4800, B4800 },
		{ 9600, B9600 },
		{ 19200, B19200 },
		{ 38400, B38400 },
#ifdef B57600
		{ 57600, B57600 },
#endif
#ifdef B115200
		{ 115200, B115200 },
#endif
#ifdef B230400
		{ 230400, B230400 },
#endif
#ifdef B460800
		{ 460800, B460800 },
#endif
#ifdef B500000
		{ 500000, B500000 },
#endif
#ifdef B576000
		{ 576000, B576000 },
#endif
#ifdef B921600
		{ 921600, B921600 },
#endif
#ifdef B1000000
		{ 1000000, B1000000 },
#endif
#ifdef B1152000
		{ 1152000, B1152000 },
#endif
#ifdef B1500000
		{ 1500000, B1500000 },
#endif
#ifdef B2000000
		{ 2000000, B2000000 },
#endif
#ifdef B2500000
		{ 2500000, B2500000 },
#endif
#ifdef B3000000
		{ 3000000, B3000000 },
#endif
#ifdef B3500000
		{ 3500000, B3500000 },
#endif
#ifdef B4000000
		{ 4000000, B4000000 },
#endif
	};
	size_t i;

	if( native_speed == NULL )
		return FALSE;

	for( i = 0; i < (sizeof( speeds ) / sizeof( speeds[0] )); i++ ) {
		if( speeds[i].baud == baud ) {
			*native_speed = speeds[i].native_speed;
			return TRUE;
		}
	}

	return FALSE;
}

static int fb_hSerialDefaultDevice( int port, char *device, size_t size )
{
	int written;
	int index = port - 1;

#if defined HOST_FREEBSD
	written = snprintf( device, size, "/dev/cuau%d", index );
#elif defined HOST_NETBSD
	written = snprintf( device, size, "/dev/dty%02d", index );
#elif defined HOST_OPENBSD
	written = snprintf( device, size, "/dev/cua%02d", index );
#elif defined HOST_DRAGONFLY
	written = snprintf( device, size, "/dev/cuaa%d", index );
#elif defined HOST_DARWIN
	written = snprintf( device, size, "/dev/cu.serial%d", port );
#elif defined HOST_HAIKU
	written = snprintf( device, size, "/dev/ports/serial%d", port );
#else
	written = snprintf( device, size, "/dev/ttyS%d", index );
#endif

	return (written >= 0) && ((size_t)written < size);
}

static int fb_hSerialDeviceName( int port, const char *requested,
	char *device, size_t size )
{
	int written;

	if( (requested == NULL) || (device == NULL) || (size == 0) )
		return FALSE;

	if( port > 0 )
		return fb_hSerialDefaultDevice( port, device, size );

	if( strcasecmp( requested, "COM" ) == 0 ) {
#if defined HOST_FREEBSD
		requested = "/dev/cuau0";
#elif defined HOST_NETBSD
		requested = "/dev/dty00";
#elif defined HOST_OPENBSD
		requested = "/dev/cua00";
#elif defined HOST_DRAGONFLY
		requested = "/dev/cuaa0";
#elif defined HOST_DARWIN
		requested = "/dev/cu.modem";
#elif defined HOST_HAIKU
		requested = "/dev/ports/serial1";
#else
		requested = "/dev/modem";
#endif
	}

	written = snprintf( device, size, "%s", requested );
	return (written >= 0) && ((size_t)written < size);
}

/* ------------------------------------------------------------------------- */
/* Termios configuration                                                     */
/* ------------------------------------------------------------------------- */

static int fb_hSerialConfigure( int fd, const FB_SERIAL_OPTIONS *options,
	struct termios *configured )
{
	speed_t speed;
	struct termios tty;

	if( (options == NULL) || (configured == NULL) ||
	    !fb_hSerialGetSpeed( options->uiSpeed, &speed ) )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	if( (options->uiDataBits < 5) || (options->uiDataBits > 8) )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	if( tcgetattr( fd, &tty ) != 0 )
		return FB_RTERROR_FILEIO;

	cfmakeraw( &tty );
	tty.c_cflag |= CREAD;
	tty.c_cc[VMIN] = 0;
	tty.c_cc[VTIME] = 0;

	if( options->AddLF ) {
		tty.c_lflag |= ICANON;
		tty.c_oflag |= OPOST | ONLCR;
	}

	if( options->KeepDTREnabled )
		tty.c_cflag &= ~HUPCL;
	else
		tty.c_cflag |= HUPCL;

	if( options->DurationDSR || options->DurationCD )
		tty.c_cflag &= ~CLOCAL;
	else
		tty.c_cflag |= CLOCAL;

#ifdef CRTSCTS
	if( (options->DurationCTS != 0) && !options->SuppressRTS )
		tty.c_cflag |= CRTSCTS;
	else
		tty.c_cflag &= ~CRTSCTS;
#else
	if( (options->DurationCTS != 0) && !options->SuppressRTS )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;
#endif

	tty.c_cflag &= ~CSIZE;
	switch( options->uiDataBits ) {
	case 5:
		tty.c_cflag |= CS5;
		break;
	case 6:
		tty.c_cflag |= CS6;
		break;
	case 7:
		tty.c_cflag |= CS7;
		break;
	case 8:
		tty.c_cflag |= CS8;
		break;
	}

	tty.c_cflag &= ~(PARENB | PARODD);
#ifdef CMSPAR
	tty.c_cflag &= ~CMSPAR;
#endif
	tty.c_iflag &= ~(INPCK | ISTRIP | IGNPAR | PARMRK);

	switch( options->Parity ) {
	case FB_SERIAL_PARITY_NONE:
		break;
	case FB_SERIAL_PARITY_EVEN:
		tty.c_cflag |= PARENB;
		tty.c_iflag |= INPCK;
		break;
	case FB_SERIAL_PARITY_ODD:
		tty.c_cflag |= PARENB | PARODD;
		tty.c_iflag |= INPCK;
		break;
	case FB_SERIAL_PARITY_SPACE:
#ifdef CMSPAR
		tty.c_cflag |= PARENB | CMSPAR;
		break;
#else
		return FB_RTERROR_ILLEGALFUNCTIONCALL;
#endif
	case FB_SERIAL_PARITY_MARK:
#ifdef CMSPAR
		tty.c_cflag |= PARENB | PARODD | CMSPAR;
		break;
#else
		return FB_RTERROR_ILLEGALFUNCTIONCALL;
#endif
	default:
		return FB_RTERROR_ILLEGALFUNCTIONCALL;
	}

	if( options->IgnoreAllErrors )
		tty.c_iflag |= IGNPAR;

	if( options->StopBits == FB_SERIAL_STOP_BITS_1 )
		tty.c_cflag &= ~CSTOPB;
	else
		tty.c_cflag |= CSTOPB;

	if( options->SuppressRTS ) {
		tty.c_iflag &= ~(IXON | IXOFF | IXANY);
		tty.c_iflag |= IXON | IXANY;
	}

	if( (cfsetispeed( &tty, speed ) != 0) ||
	    (cfsetospeed( &tty, speed ) != 0) )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	if( tcsetattr( fd, TCSAFLUSH, &tty ) != 0 )
		return FB_RTERROR_FILEIO;

	*configured = tty;
	return FB_RTERROR_OK;
}

/* ------------------------------------------------------------------------- */
/* OPEN COM stream backend                                                   */
/* ------------------------------------------------------------------------- */

int fb_SerialOpen( FB_FILE *handle, int port, FB_SERIAL_OPTIONS *options,
	const char *requested_device, void **serial_handle )
{
	char device[FB_SERIAL_DEVICE_NAME_MAX];
	LINUX_SERIAL_INFO *info;
	struct termios old_tty;
	struct termios new_tty;
	int access_flags;
	int fd;
	int result;

	if( (handle == NULL) || (options == NULL) || (serial_handle == NULL) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	*serial_handle = NULL;

	if( options->IRQNumber != 0 )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	if( !fb_hSerialDeviceName( port, requested_device, device,
	    sizeof( device ) ) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	switch( handle->access ) {
	case FB_FILE_ACCESS_READ:
		access_flags = O_RDONLY;
		break;
	case FB_FILE_ACCESS_WRITE:
		access_flags = O_WRONLY;
		break;
	case FB_FILE_ACCESS_READWRITE:
	case FB_FILE_ACCESS_ANY:
		access_flags = O_RDWR;
		break;
	default:
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	fd = open( device, access_flags | O_NOCTTY | O_NONBLOCK );
	if( fd < 0 )
		return fb_ErrorSetNum( FB_RTERROR_FILENOTFOUND );

	if( tcgetattr( fd, &old_tty ) != 0 ) {
		close( fd );
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	if( tcflush( fd, TCIOFLUSH ) != 0 ) {
		close( fd );
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	result = fb_hSerialConfigure( fd, options, &new_tty );
	if( result != FB_RTERROR_OK ) {
		tcsetattr( fd, TCSAFLUSH, &old_tty );
		close( fd );
		return fb_ErrorSetNum( result );
	}

	info = (LINUX_SERIAL_INFO *)calloc( 1, sizeof( *info ) );
	if( info == NULL ) {
		tcsetattr( fd, TCSAFLUSH, &old_tty );
		close( fd );
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
	}

	info->sfd = fd;
	info->oldtty = old_tty;
	info->newtty = new_tty;
	info->iPort = port;
	info->pOptions = options;
	*serial_handle = info;

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

int fb_SerialGetRemaining( FB_FILE *handle, void *serial_handle,
	fb_off_t *length )
{
	LINUX_SERIAL_INFO *info = (LINUX_SERIAL_INFO *)serial_handle;
	int queued = 0;

	(void)handle;

	if( (info == NULL) || (length == NULL) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

#ifdef FIONREAD
	if( ioctl( info->sfd, FIONREAD, &queued ) != 0 ) {
		fd_set read_set;
		struct timeval timeout;
		int selected;

		/* Haiku currently documents FIONREAD for sockets only. Readiness still
		   gives EOF/LOC the portable fact it needs: at least one byte can be
		   consumed without blocking. */
		FD_ZERO( &read_set );
		FD_SET( info->sfd, &read_set );
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		selected = select( info->sfd + 1, &read_set, NULL, NULL, &timeout );
		if( selected < 0 )
			return fb_ErrorSetNum( FB_RTERROR_FILEIO );
		queued = (selected > 0) ? 1 : 0;
	}
#else
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
#endif

	*length = (queued > 0) ? (fb_off_t)queued : 0;
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

int fb_SerialWrite( FB_FILE *handle, void *serial_handle, const void *data,
	size_t length )
{
	LINUX_SERIAL_INFO *info = (LINUX_SERIAL_INFO *)serial_handle;
	const unsigned char *bytes = (const unsigned char *)data;
	size_t offset = 0;

	(void)handle;

	if( (info == NULL) || ((data == NULL) && (length != 0)) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	while( offset < length ) {
		fd_set write_set;
		struct timeval timeout;
		ssize_t written;
		int selected;

		do {
			/* select may alter both the descriptor set and timeout, so rebuild
			   them before retrying an interrupted wait. */
			FD_ZERO( &write_set );
			FD_SET( info->sfd, &write_set );
			timeout.tv_sec = FB_SERIAL_WRITE_TIMEOUT_SEC;
			timeout.tv_usec = 0;
			selected = select( info->sfd + 1, NULL, &write_set, NULL,
				&timeout );
		} while( (selected < 0) && (errno == EINTR) );

		if( selected <= 0 )
			return fb_ErrorSetNum( FB_RTERROR_FILEIO );

		written = write( info->sfd, bytes + offset, length - offset );
		if( written > 0 ) {
			offset += (size_t)written;
			continue;
		}

		if( (written < 0) && ((errno == EINTR) || (errno == EAGAIN)) )
			continue;

		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

int fb_SerialRead( FB_FILE *handle, void *serial_handle, void *data,
	size_t *length )
{
	LINUX_SERIAL_INFO *info = (LINUX_SERIAL_INFO *)serial_handle;
	fd_set read_set;
	struct timeval timeout;
	ssize_t received;
	int selected;

	(void)handle;

	if( (info == NULL) || (length == NULL) ||
	    ((data == NULL) && (*length != 0)) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	if( *length == 0 )
		return fb_ErrorSetNum( FB_RTERROR_OK );

	do {
		FD_ZERO( &read_set );
		FD_SET( info->sfd, &read_set );
		timeout.tv_sec = 0;
		timeout.tv_usec = FB_SERIAL_READ_TIMEOUT_USEC;
		selected = select( info->sfd + 1, &read_set, NULL, NULL, &timeout );
	} while( (selected < 0) && (errno == EINTR) );

	if( selected == 0 ) {
		*length = 0;
		return fb_ErrorSetNum( FB_RTERROR_OK );
	}

	if( selected < 0 )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	do {
		received = read( info->sfd, data, *length );
	} while( (received < 0) && (errno == EINTR) );

	if( (received < 0) && (errno == EAGAIN) )
		received = 0;
	else if( received < 0 )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	*length = (size_t)received;
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

int fb_SerialClose( FB_FILE *handle, void *serial_handle )
{
	LINUX_SERIAL_INFO *info = (LINUX_SERIAL_INFO *)serial_handle;
	int result = FB_RTERROR_OK;

	(void)handle;

	if( info == NULL )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	if( tcsetattr( info->sfd, TCSAFLUSH, &info->oldtty ) != 0 )
		result = FB_RTERROR_FILEIO;

	if( close( info->sfd ) != 0 )
		result = FB_RTERROR_FILEIO;

	free( info );
	return fb_ErrorSetNum( result );
}

/* end of unix/io_serial.c */
