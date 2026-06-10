/* FB runtime initialization and cleanup for cycle counting profiler

   We use a global constructor and destructor for this. Where possible they
   should run first/last respectively, such that it's safe for FB programs to
   use the FB runtime from inside its own global ctors/dtors. */

#include "../fb.h"
#include "../fb_profile.h"

/* note: they must be static, or shared libraries in Linux would reuse the 
		 same function */

#if defined(HOST_SOLARIS)
	/*
	   Solaris runs .init_array before .init.  Newer GCC releases use
	   .init_array for ordinary constructors, so putting runtime startup in
	   .init would leave FB global constructors running before the runtime
	   has created its TLS keys.

	   Solaris does sort the numbered .init_array/.fini_array input
	   sections.  A low numbered init entry runs before ordinary
	   constructors, and the matching fini entry runs after ordinary
	   destructors because the fini array is walked in reverse order.
	*/
	static void fb_hDoInit( void )
	{
		fb_hRtInit( );
		fb_InitProfileCycles();
	}

	static void fb_hDoExit( void )
	{
		fb_EndProfileCycles(0);
		fb_hRtExit( );
	}

	static void (*priorityhDoInit)( void ) __attribute__((section(".init_array.00100"), used)) = fb_hDoInit;
	static void (*priorityhDoExit)( void ) __attribute__((section(".fini_array.00100"), used)) = fb_hDoExit;
#elif defined(HOST_DARWIN) || defined(HOST_ANDROID)
	/* Darwin/MacOSX does not support ordered ctors/dtors across modules. */
	__attribute__((constructor)) static void fb_hDoInit( void )
	{
		fb_hRtInit( );
		fb_InitProfileCycles();
	}

	__attribute__((destructor)) static void fb_hDoExit( void )
	{
		fb_EndProfileCycles(0);
		fb_hRtExit( );
	}
#else
	static void fb_hDoInit( void )
	{
		fb_hRtInit( );
		fb_InitProfileCycles();
	}

	static void fb_hDoExit( void )
	{
		fb_EndProfileCycles(0);
		fb_hRtExit( );
	}

	/* This puts the init/exit global ctor/dtor for the rtlib in the sorted
	   ctors/dtors section. A named section of .?tors.65435 = Priority(100).
	   (65535 - 100 = 65435)

	   This is what __attribute__((constructor(100))) would do; but that would also
	   trigger a gcc warning, because priorities 0..100 are "reserved for the
	   implementation", so we can't use that.

	   GCC on GNU/Linux seems to use .init_array.<0-padded priority> to implement
	    __attribute__((constructor(priority))) now (instead of
	   .ctors.<65535 - priority>). The .ctors.* sections still work here, so
	   keep using them. */
	static void * priorityhDoInit __attribute__((section(".ctors.65435"), used)) = fb_hDoInit;
	static void * priorityhDoExit __attribute__((section(".dtors.65435"), used)) = fb_hDoExit;
#endif
