/* Array bound checking functions */

#include "fb.h"

#if defined(__GNUC__) || defined(__clang__)
#define FB_ARRAY_PRINTF_FORMAT( format_index, first_arg ) \
	__attribute__((format(printf, format_index, first_arg)))
#else
#define FB_ARRAY_PRINTF_FORMAT( format_index, first_arg )
#endif

static void hAppendMsg
	(
		char *msg,
		int *pos,
		const char *fmt,
		...
	) FB_ARRAY_PRINTF_FORMAT( 3, 4 );

static void hAppendMsg
	(
		char *msg,
		int *pos,
		const char *fmt,
		...
	)
{
	va_list args;
	int written;
	int remaining;

	if( *pos >= FB_ERRMSG_SIZE )
		return;

	remaining = FB_ERRMSG_SIZE - *pos;

	va_start( args, fmt );
	written = vsnprintf( &msg[*pos], remaining, fmt, args );
	va_end( args );

	if( written < 0 )
	{
		msg[*pos] = '\0';
		return;
	}

	if( written >= remaining )
		*pos = FB_ERRMSG_SIZE - 1;
	else
		*pos += written;
}

static void *hThrowError
	(
		int errnum,
		ssize_t idx,
		ssize_t lbound,
		ssize_t ubound,
		int linenum,
		const char *filename,
		const char *variablename
	)
{
	int pos = 0;
	char msg[FB_ERRMSG_SIZE];

	hAppendMsg( msg, &pos, "\n" );

	if( variablename ) {
		hAppendMsg( msg, &pos, "'%s' ", variablename );
	} else {
		hAppendMsg( msg, &pos, "array " );
	}

	if( errnum == FB_RTERROR_NOTDIMENSIONED ) {
		hAppendMsg( msg, &pos,
			"not dimensioned and array elements are not allocated" );
	} else if( errnum == FB_RTERROR_WRONGDIMENSIONS ) {
		hAppendMsg( msg, &pos,
			"accessed with wrong number of dimensions, %" FB_LL_FMTMOD "d given but expected %" FB_LL_FMTMOD "d",
			(long long int)idx, (long long int)ubound);
	} else {
		hAppendMsg( msg, &pos,
			"accessed with invalid index = %" FB_LL_FMTMOD "d, must be between %" FB_LL_FMTMOD "d and %" FB_LL_FMTMOD "d",
			(long long int)idx, (long long int)lbound, (long long int)ubound);
	}
	msg[FB_ERRMSG_SIZE-1] = '\0';

	/* call user handler if any defined */
	return (void *)fb_ErrorThrowMsg( errnum, linenum, filename, msg, NULL, NULL );
}

FBCALL void *fb_ArrayDimensionChk
	( 
		ssize_t dimensions,
		FBARRAY *array,
		int linenum,
		const char *filename,
		const char *variablename
	)
{
	/* unallocated array */
	if( (array == NULL) || (array->data == NULL) ) {
		return hThrowError( FB_RTERROR_NOTDIMENSIONED,
			0, 0, 0, linenum, filename, variablename );
	}

	/* wrong number of dimensions? */
	if( ((size_t)dimensions != array->dimensions) ) {
		return hThrowError( FB_RTERROR_WRONGDIMENSIONS,
			dimensions, 0, array->dimensions, linenum, filename, variablename );
	}

	return NULL;
}

FBCALL void *fb_ArrayBoundChkEx
	(
		ssize_t idx,
		ssize_t lbound,
		ssize_t ubound,
		int linenum,
		const char *filename,
		const char *variablename
	)
{
	if( (idx < lbound) || (idx > ubound) ) {
		return hThrowError( FB_RTERROR_OUTOFBOUNDS, idx, lbound, ubound, linenum, filename, variablename );
	} else {
		return NULL;
	}
}

FBCALL void *fb_ArraySngBoundChkEx
	(
		size_t idx,
		size_t ubound,
		int linenum,
		const char *filename,
		const char *variablename
	)
{
	/* Assuming lbound is 0, we know ubound must be >= 0, and we can treat
	   index as unsigned too, possibly letting it overflow to a very big
	   value (if it was negative), reducing the bound check to a single
	   unsigned comparison. */

	if( idx > ubound ) {
		return hThrowError( FB_RTERROR_OUTOFBOUNDS, idx, 0, ubound, linenum, filename, variablename );
	} else {
		return NULL;
	}
}

/* legacy, before version 1.20.0
** these entry points are needed otherwise it is impossible to
** compile a debug vrsion of newer fbc source from an older fbc
*/
FBCALL void *fb_ArrayBoundChk
	(
		ssize_t idx,
		ssize_t lbound,
		ssize_t ubound,
		int linenum,
		const char *filename
	)
{
	return fb_ArrayBoundChkEx( idx, lbound, ubound, linenum, filename, NULL );
}

FBCALL void *fb_ArraySngBoundChk
	(
		size_t idx,
		size_t ubound,
		int linenum,
		const char *filename
	)
{
	return fb_ArraySngBoundChkEx( idx, ubound, linenum, filename, NULL );
}
