/* get the executable's name */

#include "../fb.h"
#include <sys/stat.h>

char *fb_hGetExeName( char *dst, ssize_t maxlen )
{
	char *p;
	char linkname[1024];
	struct stat finfo;

	if( maxlen <= 0 )
		return NULL;

	memset( dst, 0, (size_t)maxlen );
	if( maxlen == 1 )
		return NULL;

	sprintf(linkname, "/proc/%d/exe", getpid());
	if ((stat(linkname, &finfo) == 0) && (readlink(linkname, dst, maxlen - 1) > -1)) {
		/* Linux-like proc fs is available */
		p = strrchr(dst, '/');
		if (p != NULL)
			++p;
		else
			p = dst;
	} else {
		p = NULL;
	}

	return p;
}
