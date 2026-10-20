/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/sys_hsecure_random.c
 * Purpose: Provide sys hsecure random runtime support.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
/*
 * FreeBASIC DOS runtime: sys_hsecure_random.c
 * Read checked host entropy from the versioned XLRNG$ DOS device.
 * The DOSBox-X platform extension owns host CSPRNG access. The runtime checks
 * a real character device and its control protocol before reading any bytes.
 * This file does not generate randomness, accept a seed file or use RND.
 */

#include "../../fb.h"
#include "../../fb_secure_random.h"
#include <dos.h>
#include <dpmi.h>
#include <errno.h>
#include <fcntl.h>
#include <go32.h>
#include <io.h>
#include <sys/movedata.h>

int fb_hSecureRandom( void *destination, size_t byte_count )
{
	/* Control reply, 16 bytes:
	 * 0..7  : ASCII XLRNG001, protocol version one
	 * 8..9  : little-endian maximum read, 4096
	 * 10..11: little-endian flags, bit zero = host system CSPRNG
	 * 12..15: reserved, zero
	 */
	static const unsigned char expected[16] = {
		'X', 'L', 'R', 'N', 'G', '0', '0', '1', 0, 16, 1, 0, 0, 0, 0, 0
	};
	/* Copy a small immutable block when clearing the real-mode workspace.
	 * This avoids retaining secret bytes there or allocating another 4 KiB
	 * buffer on every caller's DOS thread stack.
	 */
	static const unsigned char zeroes[256] = { 0 };
	unsigned char capabilities[16];
	unsigned char *cursor = destination;
	size_t transfer_limit;
	size_t clear_bytes = 0;
	__dpmi_regs registers;
	int handle;
	short information;
	int status = FB_SECURE_RANDOM_IO_ERROR;

	/* DJGPP's real-mode transfer area is process-wide. Keep all CRT and
	 * IOCTL use under the existing DOS scheduler/runtime exclusion lock.
	 */
	FB_LOCK();
	handle = _open( "XLRNG$", O_RDONLY | O_BINARY );
	if( handle < 0 ) {
		status = (errno == ENOENT || errno == ENODEV) ?
		         FB_SECURE_RANDOM_UNAVAILABLE : FB_SECURE_RANDOM_IO_ERROR;
		goto finished;
	}
	information = _get_dev_info( handle );
	/* DOS device-information bits: bit seven is a character device and
	 * bit fourteen advertises control-channel IOCTL. Reject ordinary files.
	 */
	if( information == -1 || ((unsigned short)information & 0x4080U) != 0x4080U ) {
		status = FB_SECURE_RANDOM_UNAVAILABLE;
		goto finished;
	}
	if( handle > 65535 || __tb_size < sizeof( capabilities ) ||
	    __tb > 0x100000UL - sizeof( capabilities ) )
		goto finished;
	transfer_limit = __tb_size;
	if( transfer_limit > 4096 )
		transfer_limit = 4096;
	if( transfer_limit > 0x100000UL - __tb )
		transfer_limit = 0x100000UL - __tb;
	clear_bytes = sizeof( capabilities );
	dosmemput( zeroes, clear_bytes, __tb );
	memset( &registers, 0, sizeof( registers ) );
	registers.x.ax = 0x4402; /* DOS IOCTL: read device control channel. */
	registers.x.bx = (unsigned short)handle;
	registers.x.cx = sizeof( capabilities );
	registers.x.ds = (unsigned short)(__tb >> 4);
	registers.x.dx = (unsigned short)(__tb & 15);
	if( __dpmi_int( 0x21, &registers ) < 0 ||
	    (registers.x.flags & 1) != 0 || registers.x.ax != sizeof( capabilities ) )
		goto finished;
	dosmemget( __tb, sizeof( capabilities ), capabilities );
	if( memcmp( capabilities, expected, sizeof( expected ) ) != 0 ) {
		status = FB_SECURE_RANDOM_UNAVAILABLE;
		goto finished;
	}
	while( byte_count != 0 ) {
		size_t chunk = byte_count > transfer_limit ? transfer_limit : byte_count;
		if( clear_bytes < chunk )
			clear_bytes = chunk;
		/* Read directly into the checked real-mode workspace. This makes
		 * the location of every temporary secret explicit; a CRT read could
		 * retain an additional copy in an internal staging buffer.
		 */
		memset( &registers, 0, sizeof( registers ) );
		registers.x.ax = 0x3f00; /* DOS read from handle, into DS:DX. */
		registers.x.bx = (unsigned short)handle;
		registers.x.cx = (unsigned short)chunk;
		registers.x.ds = (unsigned short)(__tb >> 4);
		registers.x.dx = (unsigned short)(__tb & 15);
		/* This device protocol promises complete reads or an explicit error.
		 * The public function clears even a partially changed span on error.
		 */
		if( __dpmi_int( 0x21, &registers ) < 0 ||
		    (registers.x.flags & 1) != 0 || registers.x.ax != chunk )
			goto finished;
		dosmemget( __tb, chunk, cursor );
		cursor += chunk;
		byte_count -= chunk;
	}
	status = FB_SECURE_RANDOM_OK;

finished:
	if( handle >= 0 && _close( handle ) != 0 )
		status = FB_SECURE_RANDOM_IO_ERROR;
	/* Clear the largest span used, including when a shorter final read or
	 * a failed transfer would otherwise leave bytes from an earlier block.
	 * dosmemput is an observable write to DOS memory, outside C heap storage.
	 */
	while( clear_bytes != 0 ) {
		size_t chunk = clear_bytes > sizeof( zeroes ) ? sizeof( zeroes ) : clear_bytes;
		dosmemput( zeroes, chunk, __tb + clear_bytes - chunk );
		clear_bytes -= chunk;
	}
	FB_UNLOCK();
	return status;
}

/* end of drivers/dos/sys_hsecure_random.c */
