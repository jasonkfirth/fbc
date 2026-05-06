/* dir() */

#include "../fb.h"

typedef struct _FB_DIRCTX {
	int in_use;
} FB_DIRCTX;

void fb_DIRCTX_Destructor( void *data )
{
	(void)data;
}

FBCALL FBSTRING *fb_Dir( FBSTRING *filespec, int attrib, int *out_attrib )
{
	(void)attrib;

	if( out_attrib != NULL )
		*out_attrib = 0;

	fb_hStrDelTemp( filespec );

	return &__fb_ctx.null_desc;
}
