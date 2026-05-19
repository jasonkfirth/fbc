/* Dynamic library loading functions */

#include "../fb.h"
#include "fb_private_console.h"
#include <dlfcn.h>

FBCALL void *fb_DylibLoad( FBSTRING *library )
{
	typedef struct {
		const char *prefix;
		const char *suffix;
	} LIBNAME_FORMAT;

	void *res = NULL;
	int i;
	char libname[MAX_PATH];
	// Sometimes you will see .so files on Darwin too
	static const LIBNAME_FORMAT libnameformat[] = {
							  { "", "" },
							  { "lib", "" },
#ifdef HOST_DARWIN
							  { "lib", ".dylib" },
#endif
							  { "lib", ".so" },
							  { "./", "" },
							  { "./lib", "" },
#ifdef HOST_DARWIN
							  { "./lib", ".dylib" },
#endif
							  { "./lib", ".so" },
							  { NULL, NULL } };

	// Just in case the shared lib is an FB one, temporarily reset the
	// terminal, to let the 2nd rtlib capture the original terminal state.
	// That way both rtlibs can restore the terminal properly on exit.
	// Note: The shared lib rtlib exits *after* the program rtlib, in case
	// the user forgot to dylibfree().
	FB_LOCK( );
	fb_hExitConsole();
	FB_UNLOCK( );

	if( (library) && (library->data) ) {
		for( i = 0; libnameformat[i].prefix; i++ ) {
			int written = snprintf( libname,
									sizeof( libname ),
									"%s%s%s",
									libnameformat[i].prefix,
									library->data,
									libnameformat[i].suffix );
			if( (written < 0) || ((size_t)written >= sizeof( libname )) )
				continue;
			fb_hConvertPath( libname );
			res = dlopen( libname, RTLD_LAZY );
			if( res )
				break;
		}
	}

	/* del if temp */
	fb_hStrDelTemp( library );

	FB_LOCK( );
	fb_hInitConsole();
	FB_UNLOCK( );

	return res;
}

FBCALL void *fb_DylibSymbol( void *library, FBSTRING *symbol )
{
	void *proc = NULL;

	if( library == NULL )
		library = dlopen( NULL, RTLD_LAZY );

	if( (symbol) && (symbol->data) )
		proc = dlsym( library, symbol->data );

	/* del if temp */
	fb_hStrDelTemp( symbol );

	return proc;
}

FBCALL void *fb_DylibSymbolByOrd( void *library, short int symbol )
{
	/* Not applicable to Linux */
	return NULL;
}

FBCALL void fb_DylibFree( void *library )
{
	// See above; if it's an FB lib it will restore the terminal state
	// on shutdown
	FB_LOCK( );
	fb_hExitConsole();
	FB_UNLOCK( );

	dlclose( library );

	FB_LOCK( );
	fb_hInitConsole();
	FB_UNLOCK( );
}
