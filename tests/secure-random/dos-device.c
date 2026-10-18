/*
 * FreeBASIC DOS platform qualification: dos-device.c
 * Exercise the real XLRNG$ DOS character-device protocol and its bounds.
 * The test owns its DJGPP transfer buffer and checks DOS return/carry results.
 * This file does not supply entropy or write through an emulator debugger.
 */

#include <dos.h>
#include <dpmi.h>
#include <fcntl.h>
#include <go32.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/movedata.h>

#define DEVICE_LIMIT 4096
#define GUARD_SIZE 16
#define BUFFER_SIZE (DEVICE_LIMIT + GUARD_SIZE * 2)

static unsigned int checks;

static void require( int condition, const char *description )
{
	++checks;
	if( !condition ) {
		printf( "DOS_SECURE_RANDOM_DEVICE_FAIL=%s\n", description );
		exit( 1 );
	}
}

static void prepare( __dpmi_regs *registers, int handle, unsigned short function,
                     unsigned short count, unsigned long address )
{
	memset( registers, 0, sizeof( *registers ) );
	registers->x.ax = function;
	registers->x.bx = (unsigned short)handle;
	registers->x.cx = count;
	registers->x.ds = (unsigned short)(address >> 4);
	registers->x.dx = (unsigned short)(address & 15);
}

static int invoke( __dpmi_regs *registers )
{
	require( __dpmi_int( 0x21, registers ) == 0, "DPMI interrupt dispatch" );
	return (registers->x.flags & 1) == 0;
}

int main( void )
{
	static const unsigned char expected[16] = {
		'X', 'L', 'R', 'N', 'G', '0', '0', '1', 0, 16, 1, 0, 0, 0, 0, 0
	};
	unsigned char before[BUFFER_SIZE], after[BUFFER_SIZE];
	unsigned char sample[32], second[32];
	unsigned short count;
	__dpmi_regs registers;
	int handle;
	short information;
	size_t i;

	require( __tb_size >= BUFFER_SIZE && __tb <= 0x100000UL - BUFFER_SIZE,
	         "owned conventional transfer buffer" );
	handle = _open( "XLRNG$", O_RDWR | O_BINARY );
	require( handle >= 0 && handle <= 65535, "character-device open" );
	information = _get_dev_info( handle );
	require( information != -1 && ((unsigned short)information & 0x4080U) == 0x4080U,
	         "DOS character-device and IOCTL bits" );
	memset( before, 0xa5, sizeof( before ) );

	/* Malformed control requests must fail before touching the caller's
	 * transfer buffer. These are ordinary DOS IOCTL calls, not debugger writes.
	 */
	for( count = 0; count <= 17; ++count ) {
		if( count != 0 && count != 15 && count != 17 )
			continue;
		dosmemput( before, sizeof( before ), __tb );
		prepare( &registers, handle, 0x4402, count, __tb + GUARD_SIZE );
		require( !invoke( &registers ), "invalid control length refused" );
		dosmemget( __tb, sizeof( after ), after );
		require( memcmp( before, after, sizeof( before ) ) == 0, "invalid control leaves span untouched" );
	}
	dosmemput( before, sizeof( before ), __tb );
	prepare( &registers, handle, 0x4402, sizeof( expected ), __tb + GUARD_SIZE );
	require( invoke( &registers ) && registers.x.ax == sizeof( expected ), "complete control reply" );
	dosmemget( __tb, sizeof( after ), after );
	require( memcmp( after + GUARD_SIZE, expected, sizeof( expected ) ) == 0, "versioned capabilities" );
	for( i = 0; i < sizeof( after ); ++i ) {
		if( i < GUARD_SIZE || i >= GUARD_SIZE + sizeof( expected ) )
			require( after[i] == before[i], "control reply guard" );
	}

	/* Use a raw DOS read for the over-limit case, so CRT chunking cannot
	 * turn the negative request into several valid device reads.
	 */
	dosmemput( before, sizeof( before ), __tb );
	prepare( &registers, handle, 0x3f00, DEVICE_LIMIT + 1, __tb + GUARD_SIZE );
	require( !invoke( &registers ), "over-limit device read refused" );
	dosmemget( __tb, sizeof( after ), after );
	require( memcmp( before, after, sizeof( before ) ) == 0, "refused read leaves caller span untouched" );

	memcpy( after, before, sizeof( after ) );
	require( _read( handle, after + GUARD_SIZE, DEVICE_LIMIT ) == DEVICE_LIMIT, "complete maximum CRT read" );
	for( i = 0; i < GUARD_SIZE; ++i ) {
		require( after[i] == 0xa5, "read prefix guard" );
		require( after[GUARD_SIZE + DEVICE_LIMIT + i] == 0xa5, "read suffix guard" );
	}
	require( _read( handle, sample, sizeof( sample ) ) == sizeof( sample ), "first live sample" );
	require( _read( handle, second, sizeof( second ) ) == sizeof( second ), "second live sample" );
	/* This only detects a trivial repeated output, not cryptographic strength. */
	require( memcmp( sample, second, sizeof( sample ) ) != 0, "distinct sample sanity check" );

	prepare( &registers, handle, 0x4000, 1, __tb + GUARD_SIZE );
	require( !invoke( &registers ), "device write refused" );
	prepare( &registers, handle, 0x4403, 16, __tb + GUARD_SIZE );
	require( !invoke( &registers ), "control write refused" );
	prepare( &registers, handle, 0x4200, 0, 0 );
	require( !invoke( &registers ), "device seek refused" );
	require( _close( handle ) == 0, "device close" );
	printf( "DOS_SECURE_RANDOM_DEVICE=PASS checks=%u\n", checks );
	return 0;
}

/* end of dos-device.c */
