/*
 * Project: FreeBASIC runtime library
 * File: threads/thread_call.c
 * Purpose: Provide thread call runtime support.
 * Responsibilities: Thread identities, invocation helpers and TLS contexts.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
/**
 * ThreadCall: Launches any procedure as new thread, based on libffi.
 *
 * For example:
 *
 * FB code:
 *    declare sub MySub cdecl(byval x as long, byval y as long)
 *    thread = threadcall MySub(2, 3)
 *    threadwait thread
 *
 * Turned into this by fbc:
 *    a = 2
 *    b = 3
 *    thread = fb_ThreadCall(@MySub, CDECL, 0, 2, INT32, @a, INT32, @b)
 *    fb_ThreadWait(thread)
 *
 * The compiler passes an address for each lowered ABI argument. The
 * launching thread prepares the call and copies those values before
 * returning. A plain pointer is copied as a pointer; its pointee is
 * borrowed. The STRING tag also copies the descriptor and its text.
 */

#include "fb.h"

#if defined DISABLE_FFI

FBTHREAD *fb_ThreadCall( void *proc, int abi, ssize_t stack_size, int num_args, ... )
{
	(void)proc;
	(void)abi;
	(void)stack_size;
	(void)num_args;
	return NULL;
}

#else

/*
 * Some Darwin SDK libffi headers test FFI_GO_CLOSURES without defining it
 * first.  Define it to the value the preprocessor would otherwise use so
 * strict -Wundef builds can include the system header cleanly.
 */
#if defined(__APPLE__) && !defined(FFI_GO_CLOSURES)
#define FFI_GO_CLOSURES 0
#endif

#include <ffi.h>

#define FB_THREADCALL_MAX_ELEMS 1024

typedef struct FB_THREAD_CALL
{
	ffi_cif       cif;
	void         *proc;
	int           num_args;
#if defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 8)
	unsigned char args_alignment_padding[4];
#endif
	ffi_type    **ffi_arg_types;
	void        **values;
	FBSTRING     *strings;
} FBTHREADCALL;

typedef struct {
	const void *value;
	int copy_string;
#if defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 8)
	unsigned char tail_padding[4];
#endif
} FBTHREADCALL_INPUT;

/* mirrored in compiler/rtl.bi */
enum {
	FB_THREADCALL_STDCALL,
	FB_THREADCALL_CDECL,
	FB_THREADCALL_INT8,
	FB_THREADCALL_UINT8,
	FB_THREADCALL_INT16,
	FB_THREADCALL_UINT16,
	FB_THREADCALL_INT32,
	FB_THREADCALL_UINT32,
	FB_THREADCALL_INT64,
	FB_THREADCALL_UINT64,
	FB_THREADCALL_FLOAT32,
	FB_THREADCALL_FLOAT64,
	FB_THREADCALL_STRUCT,
	FB_THREADCALL_PTR,
	FB_THREADCALL_STRING
};

static void freeStruct( ffi_type *arg )
{
    int i = 0;
    ffi_type **elem = arg->elements;
    
    while( *elem != NULL )
    {
        /* cap element count to limit buffer overrun */
        if ( i >= FB_THREADCALL_MAX_ELEMS )
            break;
        
        /* free embedded types */
        if( (*elem)->type == FFI_TYPE_STRUCT )
            freeStruct( *elem );
            
        elem++;
        i++;
    }
    
	    free( (void *)arg->elements );
	    free( arg );
}

static void freeThreadCall( FBTHREADCALL *info )
{
    int i;

    if( info == NULL )
        return;

    for( i=0; i<info->num_args; i++ )
    {
        if( info->ffi_arg_types != NULL && info->ffi_arg_types[i] != NULL &&
            info->ffi_arg_types[i]->type == FFI_TYPE_STRUCT )
            freeStruct( info->ffi_arg_types[i] );
        if( info->values != NULL )
            free( info->values[i] );
        if( info->strings != NULL )
            fb_StrDelete( &info->strings[i] );
    }

	    free( info->strings );
	    free( (void *)info->values );
	    free( (void *)info->ffi_arg_types );
	    free( info );
}

static ffi_type *getArgument( ssize_t arg_type, va_list *args_list );

static ffi_type *getStruct( va_list *args_list )
{
    ssize_t elem_count = va_arg( (*args_list), ssize_t );
    int num_elems;
    int i, j;

    if( (elem_count <= 0) || (elem_count > FB_THREADCALL_MAX_ELEMS) )
        return NULL;
    num_elems = (int)elem_count;

    /* prepare type */
    ffi_type *structure_type = (ffi_type *)malloc( sizeof( ffi_type ) );
    if( structure_type == NULL )
        return NULL;

    structure_type->size = 0;
    structure_type->alignment = 0;
    structure_type->type = FFI_TYPE_STRUCT;
	structure_type->elements =
        (ffi_type **)malloc( sizeof( ffi_type * ) * ( (size_t)num_elems + 1u ) );
    if( structure_type->elements == NULL )
    {
        free( structure_type );
        return NULL;
    }
    structure_type->elements[num_elems] = NULL;
    
    /* scan elements */
    for( i=0; i<num_elems; i++ )
    {
        ssize_t type = va_arg( (*args_list), ssize_t );
        structure_type->elements[i] = getArgument( type, args_list );
        if( structure_type->elements[i] == NULL )
        {
            /* error, free memory and return NULL */
            for( j=0; j<i; j++ )
            {
                if( structure_type->elements[j]->type == FFI_TYPE_STRUCT )
                    freeStruct( structure_type->elements[j] );
            }
	            free( (void *)structure_type->elements );
	            free( structure_type );
            return NULL;
        }
    }
    
    return structure_type;
}

static ffi_type *getArgument( ssize_t arg_type, va_list *args_list )
{
    switch( arg_type )
    {
        case FB_THREADCALL_INT8:    return &ffi_type_sint8;
        case FB_THREADCALL_UINT8:   return &ffi_type_uint8;
        case FB_THREADCALL_INT16:   return &ffi_type_sint16;
        case FB_THREADCALL_UINT16:  return &ffi_type_uint16;
        case FB_THREADCALL_INT32:   return &ffi_type_sint32;
        case FB_THREADCALL_UINT32:  return &ffi_type_uint32;
        case FB_THREADCALL_INT64:   return &ffi_type_sint64;
        case FB_THREADCALL_UINT64:  return &ffi_type_uint64;
        case FB_THREADCALL_FLOAT32: return &ffi_type_float;
        case FB_THREADCALL_FLOAT64: return &ffi_type_double;
        case FB_THREADCALL_STRUCT:  return getStruct( args_list );
        case FB_THREADCALL_PTR:     return &ffi_type_pointer;
        default:
            return NULL;
    }
}

/* Prepare on the launching thread. libffi fills in the sizes of
   aggregate argument types here, before we read any caller storage. */
static int prepareCall( FBTHREADCALL *call, int convention )
{
	ffi_abi abi = FFI_DEFAULT_ABI;
#if defined HOST_X86 && !defined HOST_64BIT
	if( convention == FB_THREADCALL_CDECL )
		abi = FFI_SYSV;
#if defined(HOST_WIN32)
	else if( convention == FB_THREADCALL_STDCALL )
		abi = FFI_STDCALL;
#endif
	else
		return FALSE;
#else
	if( convention != FB_THREADCALL_CDECL && convention != FB_THREADCALL_STDCALL )
		return FALSE;
#endif
	return ffi_prep_cif( &call->cif, abi, (unsigned int)call->num_args,
	                     &ffi_type_void, call->ffi_arg_types ) == FFI_OK;
}

static int captureArgument( FBTHREADCALL *call, int index, const FBTHREADCALL_INPUT *input )
{
	size_t bytes = call->ffi_arg_types[index]->size;
	if( input->value == NULL || bytes == 0 )
		return FALSE;

	call->values[index] = malloc( bytes );
	if( call->values[index] == NULL )
		return FALSE;
	if( input->copy_string ) {
		const FBSTRING_ACCESS *source;
		FBSTRING *copy = &call->strings[index];
		ssize_t length;

		memcpy( &source, input->value, sizeof(source) );
		if( source == NULL || bytes != sizeof(copy) )
			return FALSE;
		length = FB_STRSIZE(source);
		if( length > 0 ) {
			if( source->data == NULL || fb_hStrAlloc( copy, length ) == NULL )
				return FALSE;
			memcpy( copy->data, source->data, (size_t)length );
			copy->data[length] = '\0';
		}
		memcpy( call->values[index], &copy, sizeof(copy) );
	} else {
		memcpy( call->values[index], input->value, bytes );
	}
	return TRUE;
}

static FBCALL void threadproc( void *param )
{
	FBTHREADCALL *call = (FBTHREADCALL *)param;
	ffi_call( &call->cif, FFI_FN(call->proc), NULL, call->values );
	freeThreadCall( call );
}

FBTHREAD *fb_ThreadCall( void *proc, int abi, ssize_t stack_size, int num_args, ... )
{
	FBTHREADCALL *call;
	FBTHREADCALL_INPUT *inputs = NULL;
	FBTHREAD *thread;
	int i, copy_strings = FALSE;

	if( proc == NULL || num_args < 0 || num_args > FB_THREADCALL_MAX_ELEMS )
		return NULL;
	call = (FBTHREADCALL *)calloc( 1, sizeof(*call) );
	if( call == NULL )
		return NULL;
	call->proc = proc;
	call->num_args = num_args;

	if( num_args > 0 ) {
		va_list args_list;
		size_t count = (size_t)num_args;
		inputs = (FBTHREADCALL_INPUT *)malloc( count * sizeof(*inputs) );
		call->ffi_arg_types = (ffi_type **)calloc( count, sizeof(*call->ffi_arg_types) );
		call->values = (void **)calloc( count, sizeof(*call->values) );
		if( inputs == NULL || call->ffi_arg_types == NULL || call->values == NULL )
			goto fail;

		va_start( args_list, num_args );
		for( i = 0; i < num_args; ++i ) {
			ssize_t type = va_arg( args_list, ssize_t );
			inputs[i].copy_string = (type == FB_THREADCALL_STRING);
			if( inputs[i].copy_string ) {
				copy_strings = TRUE;
				call->ffi_arg_types[i] = &ffi_type_pointer;
			} else {
				call->ffi_arg_types[i] = getArgument( type, &args_list );
			}
			if( call->ffi_arg_types[i] == NULL )
				break;
			inputs[i].value = va_arg( args_list, void * );
		}
		va_end( args_list );
		if( i != num_args )
			goto fail;

		if( copy_strings ) {
			call->strings = (FBSTRING *)calloc( count, sizeof(*call->strings) );
			if( call->strings == NULL )
				goto fail;
		}
	}
	if( !prepareCall( call, abi ) )
		goto fail;
	for( i = 0; i < num_args; ++i ) {
		if( !captureArgument( call, i, &inputs[i] ) )
			goto fail;
	}
	free( inputs );

	/* The launching frame owns only its original arguments now. */
	thread = fb_ThreadCreate( threadproc, call, stack_size );
	if( thread == NULL )
		freeThreadCall( call );
	return thread;

fail:
	free( inputs );
	freeThreadCall( call );
	return NULL;
}

#endif

/* end of threads/thread_call.c */
