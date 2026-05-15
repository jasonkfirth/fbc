/* get the executable path */

#include "../fb.h"
#include <unistd.h>

char *fb_hGetExePath( char *dst, ssize_t maxlen )
{
	const char *p = strrchr( __fb_ctx.argv[0], '/' );
	if( p ) {
		ssize_t len = p - __fb_ctx.argv[0];
		if( len > maxlen ) {
			len = maxlen;
		}
		else if( len == 0 ) {
		/* keep the "/" rather than returning "" */
			len = 1;
		}

		memcpy( dst, __fb_ctx.argv[0], len );
		dst[len] = '\0';
	} else {
		const char *path = getenv( "PATH" );
		size_t argv_len = strlen( __fb_ctx.argv[0] );

		while( path && *path ) {
			char candidate[MAX_PATH+1];
			const char *end = strchr( path, ':' );
			size_t path_len = end ? (size_t)(end - path) : strlen( path );

			if( path_len == 0 ) {
				path = end ? end + 1 : NULL;
				continue;
			}

			if( path_len + 1 + argv_len < sizeof( candidate ) ) {
				memcpy( candidate, path, path_len );
				candidate[path_len] = '/';
				memcpy( candidate + path_len + 1, __fb_ctx.argv[0], argv_len + 1 );

				if( access( candidate, X_OK ) == 0 ) {
					p = strrchr( candidate, '/' );
					if( p ) {
						ssize_t len = p - candidate;
						if( len > maxlen ) {
							len = maxlen;
						}
						memcpy( dst, candidate, len );
						dst[len] = '\0';
						return dst;
					}
				}
			}

			path = end ? end + 1 : NULL;
		}

		*dst = '\0';
		return NULL;
	}
	return dst;
}
