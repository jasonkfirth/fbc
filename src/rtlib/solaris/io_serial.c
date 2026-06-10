/*
    FreeBASIC runtime library
    -------------------------

    File: io_serial.c

    Purpose:

        Implement OPEN COM serial-port support for Solaris and illumos.

    Responsibilities:

        * map FreeBASIC COM port names to Solaris/illumos device paths
        * configure POSIX termios serial attributes
        * read, write, query, and close serial file descriptors

    This file intentionally does NOT contain:

        * DOS interrupt handling
        * USB serial device discovery
        * platform lock-device integration
*/

/* serial port access for Solaris and illumos */

#include "../fb.h"
#include <strings.h>
#include "../io_serial_private.h"

#include <sys/filio.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <signal.h>
#include <fcntl.h>

#define ENDSPD		111111
#define BADSPEED	999999
#define SERIAL_TIMEOUT	3
#define SREAD_TIMEOUT	70

static void alrm( int signal_number )
{
	(void)signal_number;
}

static speed_t get_speed( int speed )
{
	static unsigned int sp[][2] =
	{
		{0, B0},
		{50, B50},
		{75, B75},
		{110, B110},
		{134, B134},
		{150, B150},
		{200, B200},
		{300, B300},
		{600, B600},
		{1200, B1200},
		{1800, B1800},
		{2400, B2400},
		{4800, B4800},
		{9600, B9600},
		{19200, B19200},
		{38400, B38400},
#ifdef B57600
		{57600, B57600},
#endif
#ifdef B76800
		{76800, B76800},
#endif
#ifdef B115200
		{115200, B115200},
#endif
#ifdef B153600
		{153600, B153600},
#endif
#ifdef B230400
		{230400, B230400},
#endif
#ifdef B307200
		{307200, B307200},
#endif
#ifdef B460800
		{460800, B460800},
#endif
#ifdef B921600
		{921600, B921600},
#endif
		{ENDSPD, 0},
		{0, 0}
	};

	int n;

	for( n = 0; sp[n][0] != (unsigned int)speed; n++ )
	{
		if( sp[n][0] == ENDSPD )
			return BADSPEED;
	}

	return sp[n][1];
}

static int build_device_name( char *device_name, size_t device_name_size, int port, const char *device )
{
	int written;

	if( port == 0 )
	{
		if( strcasecmp( device, "COM" ) == 0 )
			written = snprintf( device_name, device_name_size, "%s", "/dev/cua/a" );
		else
			written = snprintf( device_name, device_name_size, "%s", device );
	}
	else
	{
		if( (port < 1) || (port > 26) )
			return FALSE;

		written = snprintf( device_name, device_name_size, "/dev/cua/%c", 'a' + port - 1 );
	}

	return (written >= 0) && ((size_t)written < device_name_size);
}

static void make_raw( struct termios *tty )
{
	tty->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	tty->c_oflag &= ~OPOST;
	tty->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tty->c_cflag &= ~(CSIZE | PARENB);
	tty->c_cflag |= CS8;
	tty->c_cc[VMIN] = 1;
	tty->c_cc[VTIME] = 0;
}

int fb_SerialOpen
	(
		FB_FILE *handle,
		int iPort,
		FB_SERIAL_OPTIONS *options,
		const char *pszDevice,
		void **ppvHandle
	)
{
	int res;
	int desired_access;
	int serial_fd;
	char device_name[512];
	struct termios oldtty, newtty;
	speed_t term_speed;
	LINUX_SERIAL_INFO *info;

	if( options->IRQNumber != 0 )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	res = fb_ErrorSetNum( FB_RTERROR_OK );
	desired_access = O_NOCTTY | O_NONBLOCK;
	serial_fd = -1;

	switch( handle->access )
	{
	case FB_FILE_ACCESS_READ:
		desired_access |= O_RDONLY;
		break;
	case FB_FILE_ACCESS_WRITE:
		desired_access |= O_WRONLY;
		break;
	case FB_FILE_ACCESS_READWRITE:
	case FB_FILE_ACCESS_ANY:
	default:
		desired_access |= O_RDWR;
		break;
	}

	if( !build_device_name( device_name, sizeof( device_name ), iPort, pszDevice ) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	term_speed = get_speed( options->uiSpeed );
	if( term_speed == BADSPEED )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	(void)signal( SIGALRM, alrm );
	alarm( SERIAL_TIMEOUT );
	serial_fd = open( device_name, desired_access );
	alarm( 0 );

	if( serial_fd < 0 )
		return fb_ErrorSetNum( FB_RTERROR_FILENOTFOUND );

	if( tcgetattr( serial_fd, &oldtty ) )
		res = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	if( tcflush( serial_fd, TCIOFLUSH ) )
		res = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	if( tcgetattr( serial_fd, &newtty ) )
		res = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	if( res == FB_RTERROR_OK )
	{
		if( options->AddLF )
			newtty.c_lflag |= (ICANON | OPOST | ONLCR);
		else
			make_raw( &newtty );

		if( options->KeepDTREnabled )
			newtty.c_cflag &= ~HUPCL;
		else
			newtty.c_cflag |= HUPCL;

		if( options->DurationDSR || options->DurationCD )
			newtty.c_cflag &= ~CLOCAL;
		else
			newtty.c_cflag |= CLOCAL;

#ifdef CRTSCTS
		if( options->DurationCTS != 0 && !options->SuppressRTS )
			newtty.c_cflag |= CRTSCTS;
		else
			newtty.c_cflag &= ~CRTSCTS;
#endif

		cfsetispeed( &newtty, term_speed );
		cfsetospeed( &newtty, term_speed );

		newtty.c_cflag &= ~CSIZE;
		switch( options->uiDataBits )
		{
		case 5:
			newtty.c_cflag |= CS5;
			break;
		case 6:
			newtty.c_cflag |= CS6;
			break;
		case 7:
			newtty.c_cflag |= CS7;
			break;
		case 8:
		default:
			newtty.c_cflag |= CS8;
			break;
		}

		switch( options->Parity )
		{
		case FB_SERIAL_PARITY_NONE:
		case FB_SERIAL_PARITY_SPACE:
			newtty.c_cflag &= ~PARENB;
			break;
		case FB_SERIAL_PARITY_MARK:
			newtty.c_iflag |= PARMRK;
			/* fall through */
		case FB_SERIAL_PARITY_EVEN:
			newtty.c_iflag |= (INPCK | ISTRIP);
			newtty.c_cflag |= PARENB;
			newtty.c_cflag &= ~PARODD;
			break;
		case FB_SERIAL_PARITY_ODD:
			newtty.c_iflag |= (INPCK | ISTRIP);
			newtty.c_cflag |= (PARENB | PARODD);
			break;
		}

		if( options->IgnoreAllErrors )
			newtty.c_iflag |= IGNPAR;
		else
			newtty.c_iflag &= ~IGNPAR;

		switch( options->StopBits )
		{
		case FB_SERIAL_STOP_BITS_1:
			newtty.c_cflag &= ~CSTOPB;
			break;
		case FB_SERIAL_STOP_BITS_1_5:
		case FB_SERIAL_STOP_BITS_2:
			newtty.c_cflag |= CSTOPB;
			break;
		}

		if( options->SuppressRTS )
		{
			newtty.c_iflag &= ~(IXON | IXOFF | IXANY);
			newtty.c_iflag |= (IXON | IXANY);
		}

		if( tcsetattr( serial_fd, TCSAFLUSH, &newtty ) )
			res = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	if( res != FB_RTERROR_OK )
	{
		tcsetattr( serial_fd, TCSAFLUSH, &oldtty );
		close( serial_fd );
		return res;
	}

	info = (LINUX_SERIAL_INFO *)calloc( 1, sizeof( LINUX_SERIAL_INFO ) );
	if( info == NULL )
	{
		tcsetattr( serial_fd, TCSAFLUSH, &oldtty );
		close( serial_fd );
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
	}

	DBG_ASSERT( ppvHandle != NULL );
	*ppvHandle = info;
	info->sfd = serial_fd;
	info->oldtty = oldtty;
	info->newtty = newtty;
	info->iPort = iPort;
	info->pOptions = options;

	return res;
}

int fb_SerialGetRemaining( FB_FILE *handle, void *pvHandle, fb_off_t *pLength )
{
	int bytes;
	LINUX_SERIAL_INFO *info = (LINUX_SERIAL_INFO *)pvHandle;

	(void)handle;

	if( ioctl( info->sfd, FIONREAD, &bytes ) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	if( pLength )
		*pLength = bytes;

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

int fb_SerialWrite( FB_FILE *handle, void *pvHandle, const void *data, size_t length )
{
	ssize_t written;
	LINUX_SERIAL_INFO *info = (LINUX_SERIAL_INFO *)pvHandle;

	(void)handle;

	(void)signal( SIGALRM, alrm );
	alarm( SERIAL_TIMEOUT );
	written = write( info->sfd, data, length );
	alarm( 0 );

	if( written <= 0 )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	if( length != (size_t)written )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

int fb_SerialRead( FB_FILE *handle, void *pvHandle, void *data, size_t *pLength )
{
	ssize_t count;
	fd_set rfds;
	struct timeval tmout;
	LINUX_SERIAL_INFO *info = (LINUX_SERIAL_INFO *)pvHandle;

	(void)handle;

	FD_ZERO( &rfds );
	FD_SET( info->sfd, &rfds );

	tmout.tv_sec = 0;
	tmout.tv_usec = SREAD_TIMEOUT * 1000L;

	count = 0;
	if( select( info->sfd + 1, &rfds, NULL, NULL, &tmout ) > 0 )
	{
		if( FD_ISSET( info->sfd, &rfds ) )
		{
			count = read( info->sfd, data, *pLength );
			if( count < 0 )
				return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
		}
	}

	*pLength = count;

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

int fb_SerialClose( FB_FILE *handle, void *pvHandle )
{
	LINUX_SERIAL_INFO *info = (LINUX_SERIAL_INFO *)pvHandle;

	(void)handle;

	tcsetattr( info->sfd, TCSAFLUSH, &info->oldtty );
	close( info->sfd );
	free( info );

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of io_serial.c */
