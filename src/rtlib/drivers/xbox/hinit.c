/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/hinit.c
 * Purpose: libfb initialization for xbox.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "../../fb_private_thread.h"
#include <nxdk/mount.h>
#include <nxdk/path.h>

#define FB_XBOX_WRITABLE_DRIVE  'E'
#define FB_XBOX_WRITABLE_DEVICE "\\Device\\Harddisk0\\Partition1\\"
#define FB_XBOX_DISC_DRIVE      'D'

__attribute__((weak)) void fb_XboxApplyPackageEnv( void )
{
}

static void fb_XboxMountWritableDrive( void )
{
	if( !nxIsDriveMounted( FB_XBOX_WRITABLE_DRIVE ) )
		nxMountDrive( FB_XBOX_WRITABLE_DRIVE, FB_XBOX_WRITABLE_DEVICE );
}

static void fb_XboxMountDiscDrive( void )
{
	CHAR target_path[MAX_PATH];
	char *filename;

	if( nxIsDriveMounted( FB_XBOX_DISC_DRIVE ) )
		return;

	/*
		nxdk normally provides an automount object for D:, but our Xbox
		package links the needed libraries explicitly instead of through
		nxdk's makefile.  Mount the directory containing the running XBE
		here so read-only test assets on the XISO are available through
		the normal D:\ path.
	*/
	nxGetCurrentXbeNtPath( target_path );
	filename = strrchr( target_path, '\\' );
	if( filename == NULL )
		return;

	*(filename + 1) = '\0';
	nxMountDrive( FB_XBOX_DISC_DRIVE, target_path );
}

#ifdef ENABLE_MT
typedef struct FB_XBOX_LOCK {
	CRITICAL_SECTION section;
	DWORD owner;
	unsigned int recursion;
} FB_XBOX_LOCK;

static FB_XBOX_LOCK fb_global_mutex;
static FB_XBOX_LOCK fb_string_mutex;
static FB_XBOX_LOCK fb_graphics_mutex;
static FB_XBOX_LOCK fb_math_mutex;
static FB_XBOX_LOCK fb_profile_mutex;

static void fb_XboxLockInit( FB_XBOX_LOCK *lock )
{
	InitializeCriticalSection( &lock->section );
	lock->owner = 0;
	lock->recursion = 0;
}

static void fb_XboxLockDelete( FB_XBOX_LOCK *lock )
{
	DeleteCriticalSection( &lock->section );
	lock->owner = 0;
	lock->recursion = 0;
}

static void fb_XboxLockEnter( FB_XBOX_LOCK *lock )
{
	DWORD self = GetCurrentThreadId( );

	/*
		FreeBASIC's runtime lock is recursive on the desktop targets.  nxdk's
		critical-section wrapper does not provide that behaviour reliably under
		xemu, and PRINT re-enters FB_LOCK() while opening SCRN:.  Track the owner
		here so the rest of the runtime can keep using the normal lock API.
	*/
	if( (lock->recursion > 0) && (lock->owner == self) ) {
		lock->recursion++;
		return;
	}

	EnterCriticalSection( &lock->section );
	lock->owner = self;
	lock->recursion = 1;
}

static void fb_XboxLockLeave( FB_XBOX_LOCK *lock )
{
	DWORD self = GetCurrentThreadId( );

	if( (lock->recursion == 0) || (lock->owner != self) )
		return;

	if( lock->recursion > 1 ) {
		lock->recursion--;
		return;
	}

	lock->owner = 0;
	lock->recursion = 0;
	LeaveCriticalSection( &lock->section );
}

FBCALL void fb_Lock( void )      { fb_XboxLockEnter( &fb_global_mutex ); }
FBCALL void fb_Unlock( void )    { fb_XboxLockLeave( &fb_global_mutex ); }
FBCALL void fb_StrLock( void )   { fb_XboxLockEnter( &fb_string_mutex ); }
FBCALL void fb_StrUnlock( void ) { fb_XboxLockLeave( &fb_string_mutex ); }
FBCALL void fb_GraphicsLock  ( void ) { fb_XboxLockEnter( &fb_graphics_mutex ); }
FBCALL void fb_GraphicsUnlock( void ) { fb_XboxLockLeave( &fb_graphics_mutex ); }
FBCALL void fb_MathLock  ( void ) { fb_XboxLockEnter( &fb_math_mutex ); }
FBCALL void fb_MathUnlock( void ) { fb_XboxLockLeave( &fb_math_mutex ); }
FBCALL void fb_ProfileLock  ( void ) { fb_XboxLockEnter( &fb_profile_mutex ); }
FBCALL void fb_ProfileUnlock( void ) { fb_XboxLockLeave( &fb_profile_mutex ); }
#endif

void fb_hInit( void )
{
	unsigned short control_word;

	fb_XboxMountWritableDrive();
	fb_XboxMountDiscDrive();
	fb_XboxApplyPackageEnv();

	/* Get FPU control word */
	__asm__ __volatile__( "fstcw %0" : "=m" (control_word) : );
	/* Set 64-bit and round to nearest */
	/* FSTCW writes the 16-bit output; Cppcheck 2.13 cannot model it. */
	/* cppcheck-suppress uninitvar */
	control_word = (unsigned short)((control_word & 0xF0FFu) | 0x300u);
	/* Write back FPU control word */
	__asm__ __volatile__( "fldcw %0" : : "m" (control_word) );


#ifdef ENABLE_MT
	fb_XboxLockInit( &fb_global_mutex );
	fb_XboxLockInit( &fb_string_mutex );
	fb_XboxLockInit( &fb_graphics_mutex );
	fb_XboxLockInit( &fb_math_mutex );
	fb_XboxLockInit( &fb_profile_mutex );
#endif
}

void fb_hEnd( int unused )
{
#ifdef ENABLE_MT
	fb_XboxLockDelete( &fb_global_mutex );
	fb_XboxLockDelete( &fb_string_mutex );
	fb_XboxLockDelete( &fb_graphics_mutex );
	fb_XboxLockDelete( &fb_math_mutex );
	fb_XboxLockDelete( &fb_profile_mutex );
#endif
}

/* end of drivers/xbox/hinit.c */
