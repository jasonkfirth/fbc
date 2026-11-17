/*
 * Project: FreeBASIC console qualification
 * File: native-thread-main.c
 * Purpose: Enter the thread probe through the SDK's ordinary C startup.
 * Responsibilities: Return the probe result to the SDK CRT and exit observer.
 * This file does not initialize or link the FreeBASIC runtime.
 */
#include <nxdk/mount.h>
extern int fbqual_native_threads( void );

int main( int argc, char **argv )
{
	(void)argc;
	(void)argv;
	/* nxdk automounts the title as D:. The test controller also supplies a
	   private writable partition; mount it through the native SDK API. */
	if( !nxIsDriveMounted('E') && !nxMountDrive('E', "\\Device\\Harddisk0\\Partition1\\") )
		return 87;
	return fbqual_native_threads();
}

/* end of native-thread-main.c */
