/* libfb initialization for xbox */

#include "../fb.h"
#include "../fb_private_thread.h"
#include <nxdk/mount.h>
#include <nxdk/path.h>

#define FB_XBOX_WRITABLE_DRIVE  'E'
#define FB_XBOX_WRITABLE_DEVICE "\\Device\\Harddisk0\\Partition1\\"
#define FB_XBOX_DISC_DRIVE      'D'

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

static FB_XBOX_LOCK __fb_global_mutex;
static FB_XBOX_LOCK __fb_string_mutex;
static FB_XBOX_LOCK __fb_graphics_mutex;
static FB_XBOX_LOCK __fb_math_mutex;
static FB_XBOX_LOCK __fb_profile_mutex;

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

FBCALL void fb_Lock( void )      { fb_XboxLockEnter( &__fb_global_mutex ); }
FBCALL void fb_Unlock( void )    { fb_XboxLockLeave( &__fb_global_mutex ); }
FBCALL void fb_StrLock( void )   { fb_XboxLockEnter( &__fb_string_mutex ); }
FBCALL void fb_StrUnlock( void ) { fb_XboxLockLeave( &__fb_string_mutex ); }
FBCALL void fb_GraphicsLock  ( void ) { fb_XboxLockEnter( &__fb_graphics_mutex ); }
FBCALL void fb_GraphicsUnlock( void ) { fb_XboxLockLeave( &__fb_graphics_mutex ); }
FBCALL void fb_MathLock  ( void ) { fb_XboxLockEnter( &__fb_math_mutex ); }
FBCALL void fb_MathUnlock( void ) { fb_XboxLockLeave( &__fb_math_mutex ); }
FBCALL void fb_ProfileLock  ( void ) { fb_XboxLockEnter( &__fb_profile_mutex ); }
FBCALL void fb_ProfileUnlock( void ) { fb_XboxLockLeave( &__fb_profile_mutex ); }
#endif

void fb_hInit( void )
{
	unsigned int control_word;

	fb_XboxMountWritableDrive();
	fb_XboxMountDiscDrive();

	/* Get FPU control word */
	__asm__ __volatile__( "fstcw %0" : "=m" (control_word) : );
	/* Set 64-bit and round to nearest */
	control_word = (control_word & 0xF0FF) | 0x300;
	/* Write back FPU control word */
	__asm__ __volatile__( "fldcw %0" : : "m" (control_word) );


#ifdef ENABLE_MT
	fb_XboxLockInit( &__fb_global_mutex );
	fb_XboxLockInit( &__fb_string_mutex );
	fb_XboxLockInit( &__fb_graphics_mutex );
	fb_XboxLockInit( &__fb_math_mutex );
	fb_XboxLockInit( &__fb_profile_mutex );
#endif
}

void fb_hEnd( int unused )
{
#ifdef ENABLE_MT
	fb_XboxLockDelete( &__fb_global_mutex );
	fb_XboxLockDelete( &__fb_string_mutex );
	fb_XboxLockDelete( &__fb_graphics_mutex );
	fb_XboxLockDelete( &__fb_math_mutex );
	fb_XboxLockDelete( &__fb_profile_mutex );
#endif
}
