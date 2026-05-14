/**
 * ThreadCall: Launches any procedure as new thread, based on libffi.
 *
 * For example:
 *
 * FB code:
 *    declare sub MySub(x as integer, y as integer)
 *    thread = threadcall MySub(2, 3)
 *    threadwait thread
 *
 * Turned into this by fbc:
 *    a = 2
 *    b = 3
 *    thread = fb_ThreadCall(@MySub, STDCALL, 2, INT, @a, INT, @b)
 *    fb_ThreadWait(thread)
 *
 * fb_ThreadCall() packs the call and parameter data it's given into an array
 * of pointers and then launches a thread. The new thread reconstructs the call
 * using LibFFI and then calls the user's procedure.
 */

#include "fb.h"

#if defined DISABLE_FFI

FBTHREAD *fb_ThreadCall( void *proc, int abi, ssize_t stack_size, int num_args, ... )
{
	return NULL;
}

#else

#include <ffi.h>

#define FB_THREADCALL_MAX_ELEMS 1024

typedef struct _FBTHREADCALL
{
	void         *proc;
	int           abi;
	int           num_args;
	ffi_type    **ffi_arg_types;
	void        **values;
} FBTHREADCALL;

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
	FB_THREADCALL_PTR
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
    
    free( arg->elements );
    free( arg );
}

static void freeThreadCall( FBTHREADCALL *info )
{
    int i;

    if( info == NULL )
        return;

    for( i=0; i<info->num_args; i++ )
    {
        if( info->ffi_arg_types[i]->type == FFI_TYPE_STRUCT )
            freeStruct( info->ffi_arg_types[i] );
    }

    free( info->values );
    free( info->ffi_arg_types );
    free( info );
}

static ffi_type *getArgument( va_list *args_list );

static ffi_type *getStruct( va_list *args_list )
{
    ssize_t elem_count = va_arg( (*args_list), ssize_t );
    int num_elems;
    int i, j;

    if( (elem_count < 0) || (elem_count > FB_THREADCALL_MAX_ELEMS) )
        return NULL;
    num_elems = (int)elem_count;

    /* prepare type */
    ffi_type *ffi_arg = (ffi_type *)malloc( sizeof( ffi_type ) );
    if( ffi_arg == NULL )
        return NULL;

    ffi_arg->size = 0;
    ffi_arg->alignment = 0;
    ffi_arg->type = FFI_TYPE_STRUCT;
    ffi_arg->elements = 
        (ffi_type **)malloc( sizeof( ffi_type * ) * ( num_elems + 1 ) );
    if( ffi_arg->elements == NULL )
    {
        free( ffi_arg );
        return NULL;
    }
    ffi_arg->elements[num_elems] = NULL;
    
    /* scan elements */
    for( i=0; i<num_elems; i++ )
    {
        ffi_arg->elements[i] = getArgument( args_list );
        if( ffi_arg->elements[i] == NULL )
        {
            /* error, free memory and return NULL */
            for( j=0; j<i; j++ )
            {
                if( ffi_arg->elements[j]->type == FFI_TYPE_STRUCT )
                    freeStruct( ffi_arg->elements[j] );
            }
            free( ffi_arg->elements );
            free( ffi_arg );
            return NULL;
        }
    }
    
    return ffi_arg;
}

static ffi_type *getArgument( va_list *args_list )
{
    ssize_t arg_type = va_arg( (*args_list), ssize_t );

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

static FBCALL void threadproc( void *param );

FBTHREAD *fb_ThreadCall( void *proc, int abi, ssize_t stack_size, int num_args, ... )
{
    ffi_type     **ffi_args;
    void         **values;
    FBTHREADCALL  *param;
    FBTHREAD     *thread;
    int i, j;

    if( (proc == NULL) || (num_args < 0) || (num_args > FB_THREADCALL_MAX_ELEMS) )
        return NULL;
    
    /* initialize lists and arrays */
    ffi_args = NULL;
    values = NULL;
    if( num_args > 0 )
    {
        ffi_args = (ffi_type **)malloc( sizeof( ffi_type * ) * num_args );
        values = (void **)malloc( sizeof( void * ) * num_args );
        if( (ffi_args == NULL) || (values == NULL) )
        {
            free( values );
            free( ffi_args );
            return NULL;
        }
    }

    va_list args_list; 
    va_start(args_list, num_args);
    
    /* scan arguments and values from var_args */
    for( i=0; i<num_args; i++ )
    {
        ffi_args[i] = getArgument( &args_list );
        if( ffi_args[i] == NULL )
        {
            /* error, free all memory allocated up to this point */
            va_end( args_list );
            for( j=0; j<i; j++ )
            {
                if( ffi_args[j]->type == FFI_TYPE_STRUCT )
                    freeStruct( ffi_args[j] );
            }
            free(values);
            free(ffi_args);
            return NULL;
        }
        values[i] = va_arg( args_list, void * );
    }
    va_end( args_list );
    
    /* pack into thread parameter */
    param = malloc( sizeof( FBTHREADCALL ) );
    if( param == NULL )
    {
        for( j=0; j<num_args; j++ )
        {
            if( ffi_args[j]->type == FFI_TYPE_STRUCT )
                freeStruct( ffi_args[j] );
        }
        free(values);
        free(ffi_args);
        return NULL;
    }
    param->proc = proc;
    param->abi = abi;
    param->num_args = num_args;
    param->ffi_arg_types = ffi_args;
    param->values = values;
    
    /* actually start thread */
    thread = fb_ThreadCreate( threadproc, (void *)param, stack_size );
    if( thread == NULL )
        freeThreadCall( param );

    return thread;
}

static FBCALL void threadproc( void *param )
{
    FBTHREADCALL *info = ( FBTHREADCALL * )param;
    ffi_status status = FFI_OK;
    ffi_abi abi = -1;
    ffi_cif cif;

#if defined HOST_X86 && !defined HOST_64BIT
    /* check calling convention */
    if( info->abi == FB_THREADCALL_CDECL )
        abi = FFI_SYSV;
#if defined( HOST_WIN32 )
    else if( info->abi == FB_THREADCALL_STDCALL )
        abi = FFI_STDCALL;
#endif
    else
        status = ~FFI_OK;

    /* prep FFI call interface */
    if( status == FFI_OK )
#else
    /*
     * libffi exposes the native platform ABI as FFI_DEFAULT_ABI on non-x86
     * targets. 32-bit x86 remains special because CDECL and STDCALL are
     * separate ABIs there.
     */
    if( (info->abi == FB_THREADCALL_CDECL) ||
        (info->abi == FB_THREADCALL_STDCALL) )
        abi = FFI_DEFAULT_ABI;
    else
        status = ~FFI_OK;

    if( status == FFI_OK )
#endif
        status = ffi_prep_cif( 
            &cif,               // handle
            abi,                // ABI (CDECL or STDCALL on x86, host default otherwise)
            info->num_args,     // number of arguments
            &ffi_type_void,     // return type
            info->ffi_arg_types // argument types
        );
        
    /* execute */
    if( status == FFI_OK )
        ffi_call( &cif, FFI_FN( info->proc ), NULL, info->values );
    

    /* free memory and exit */
    freeThreadCall( info );
}

#endif
