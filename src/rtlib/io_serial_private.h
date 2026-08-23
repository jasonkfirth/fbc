/*
    Project: FreeBASIC Runtime Library
    ----------------------------------

    File: io_serial_private.h

    Purpose:

        Describe the platform-owned handles stored behind an OPEN COM file.

    Responsibilities:

        - define Windows, POSIX terminal, and DOS serial handle records
        - keep platform-native types out of the generic COM device layer

    This file intentionally does NOT contain:

        - public fbcom.bi types
        - device-opening or modem-control logic
        - ownership of parsed OPEN COM options
*/

#ifndef __FB_IO_SERIAL_PRIVATE_H__
#define __FB_IO_SERIAL_PRIVATE_H__

#if defined HOST_WIN32
	#include <windows.h>
	typedef struct _W32_SERIAL_INFO {
		HANDLE hDevice;
		int iPort;
		FB_SERIAL_OPTIONS *pOptions;
		unsigned int output_lines;
	} W32_SERIAL_INFO;
#elif defined HOST_LINUX || defined HOST_ANDROID || defined HOST_SOLARIS || \
      defined HOST_FREEBSD || defined HOST_NETBSD || defined HOST_OPENBSD || \
      defined HOST_DRAGONFLY || defined HOST_DARWIN || defined HOST_HAIKU
	/* Uncomment HAS_LOCKDEV to active lock file funcionality, not forget
	 * compile whith -llockdev
	 */
	/* #define HAS_LOCKDEV 1 */
	typedef struct _LINUX_SERIAL_INFO {
		int sfd;
		struct termios oldtty, newtty;
		#ifdef HAS_LOCKDEV
			pid_t pplckid;
		#endif
		int iPort;
		FB_SERIAL_OPTIONS *pOptions;
	} LINUX_SERIAL_INFO;
#elif defined HOST_DOS
	typedef struct {
		int com_num;
		FB_SERIAL_OPTIONS *pOptions;
	} DOS_SERIAL_INFO;
#elif defined HOST_AROS
	struct MsgPort;
	struct IOExtSer;
	typedef struct _AROS_SERIAL_INFO {
		struct MsgPort *reply_port;
		struct IOExtSer *request;
		FB_SERIAL_OPTIONS *pOptions;
		int unit;
	} AROS_SERIAL_INFO;
#elif defined HOST_RISCOS
	typedef struct _RISCOS_SERIAL_INFO {
		FB_SERIAL_OPTIONS *pOptions;
		int old_input_stream;
		int old_state;
		int old_format;
		int old_rx_baud;
		int old_tx_baud;
		unsigned char read_ahead;
		int has_read_ahead;
	} RISCOS_SERIAL_INFO;
#endif

#endif

/* end of io_serial_private.h */
