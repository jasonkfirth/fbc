/* read stmt for wstring's */

#include "fb.h"

FBCALL void fb_DataReadWstr( FB_WCHAR *dst, ssize_t dst_size )
{
	FB_WCHAR empty_string[] = _LC("");

	FB_LOCK();

	if( __fb_data_ptr ) {
		if( __fb_data_ptr->len == FB_DATATYPE_OFS ) {
			/* !!!WRITEME!!! */
		} else if( __fb_data_ptr->len & FB_DATATYPE_WSTR ) {
			fb_WstrAssign( dst, dst_size, __fb_data_ptr->wstr );
		} else {
			fb_WstrAssignFromA( dst, dst_size, __fb_data_ptr->zstr, __fb_data_ptr->len );
		}
	} else {
		/* no more DATA, return empty string */
		fb_WstrAssign( dst, dst_size, empty_string );
	}

	fb_DataNext( );

	FB_UNLOCK();
}
