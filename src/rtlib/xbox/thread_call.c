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

#include "../fb.h"
#include <stdint.h>

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

#if defined DISABLE_FFI || (!defined HOST_X86 && !defined HOST_X86_64)

#if defined HOST_XBOX && defined HOST_X86

#define FB_THREADCALL_XBOX_MAX_WORDS 1024

typedef struct _FBTHREADCALL_XBOX_LAYOUT
{
	size_t size;
	size_t align;
} FBTHREADCALL_XBOX_LAYOUT;

typedef struct _FBTHREADCALL_XBOX_STACK
{
	uint32_t *words;
	int word_count;
} FBTHREADCALL_XBOX_STACK;

typedef struct _FBTHREADCALL_XBOX
{
	void *proc;
	int abi;
	int word_count;
	uint32_t *words;
} FBTHREADCALL_XBOX;

static size_t hAlignUp( size_t value, size_t align )
{
	size_t remainder;

	if( align <= 1 )
		return value;

	remainder = value % align;
	if( remainder == 0 )
		return value;

	return value + align - remainder;
}

static int hTypeLayout( va_list *args_list, int arg_type, FBTHREADCALL_XBOX_LAYOUT *layout )
{
	switch( arg_type ) {
	case FB_THREADCALL_INT8:
	case FB_THREADCALL_UINT8:
		layout->size = 1;
		layout->align = 1;
		return TRUE;

	case FB_THREADCALL_INT16:
	case FB_THREADCALL_UINT16:
		layout->size = 2;
		layout->align = 2;
		return TRUE;

	case FB_THREADCALL_INT32:
	case FB_THREADCALL_UINT32:
	case FB_THREADCALL_FLOAT32:
	case FB_THREADCALL_PTR:
		layout->size = 4;
		layout->align = 4;
		return TRUE;

	case FB_THREADCALL_INT64:
	case FB_THREADCALL_UINT64:
	case FB_THREADCALL_FLOAT64:
		layout->size = 8;
		layout->align = 8;
		return TRUE;

	case FB_THREADCALL_STRUCT:
	{
		int i, num_elems;
		size_t offset = 0;
		size_t max_align = 1;

		num_elems = va_arg( *args_list, int );
		if( (num_elems < 0) || (num_elems > FB_THREADCALL_XBOX_MAX_WORDS) )
			return FALSE;

		for( i = 0; i < num_elems; i++ ) {
			int elem_type;
			FBTHREADCALL_XBOX_LAYOUT elem;

			elem_type = va_arg( *args_list, int );
			if( !hTypeLayout( args_list, elem_type, &elem ) )
				return FALSE;

			offset = hAlignUp( offset, elem.align );
			offset += elem.size;
			if( elem.align > max_align )
				max_align = elem.align;
		}

		layout->size = hAlignUp( offset, max_align );
		layout->align = max_align;
		return TRUE;
	}

	default:
		return FALSE;
	}
}

static int hAppendStackBytes( FBTHREADCALL_XBOX_STACK *stack, const void *data, size_t bytes )
{
	const unsigned char *src = (const unsigned char *)data;
	size_t padded_bytes;
	size_t i, words;

	padded_bytes = hAlignUp( bytes, sizeof( uint32_t ) );
	words = padded_bytes / sizeof( uint32_t );

	if( words > (size_t)(FB_THREADCALL_XBOX_MAX_WORDS - stack->word_count) )
		return FALSE;

	for( i = 0; i < words; i++ ) {
		uint32_t word = 0;
		size_t offset = i * sizeof( uint32_t );
		size_t remaining = (bytes > offset) ? (bytes - offset) : 0;
		size_t copy_bytes = remaining;

		if( copy_bytes > sizeof( uint32_t ) )
			copy_bytes = sizeof( uint32_t );

		if( copy_bytes > 0 )
			memcpy( &word, src + offset, copy_bytes );

		stack->words[stack->word_count++] = word;
	}

	return TRUE;
}

static int hAppendStackWord( FBTHREADCALL_XBOX_STACK *stack, uint32_t word )
{
	if( stack->word_count >= FB_THREADCALL_XBOX_MAX_WORDS )
		return FALSE;

	stack->words[stack->word_count++] = word;
	return TRUE;
}

static int hAppendStackArg
	(
		FBTHREADCALL_XBOX_STACK *stack,
		int arg_type,
		const FBTHREADCALL_XBOX_LAYOUT *layout,
		const void *value
	)
{
	if( value == NULL )
		return FALSE;

	switch( arg_type ) {
	case FB_THREADCALL_INT8:
		return hAppendStackWord( stack, (uint32_t)(int32_t)*(const int8_t *)value );
	case FB_THREADCALL_UINT8:
		return hAppendStackWord( stack, (uint32_t)*(const uint8_t *)value );
	case FB_THREADCALL_INT16:
		return hAppendStackWord( stack, (uint32_t)(int32_t)*(const int16_t *)value );
	case FB_THREADCALL_UINT16:
		return hAppendStackWord( stack, (uint32_t)*(const uint16_t *)value );
	case FB_THREADCALL_INT32:
	case FB_THREADCALL_UINT32:
	case FB_THREADCALL_FLOAT32:
		return hAppendStackBytes( stack, value, 4 );
	case FB_THREADCALL_INT64:
	case FB_THREADCALL_UINT64:
	case FB_THREADCALL_FLOAT64:
		return hAppendStackBytes( stack, value, 8 );
	case FB_THREADCALL_PTR:
		return hAppendStackWord( stack, (uint32_t)(uintptr_t)*(void * const *)value );
	case FB_THREADCALL_STRUCT:
		return hAppendStackBytes( stack, value, layout->size );
	default:
		return FALSE;
	}
}

static void hInvokeXboxThreadCall( void *proc, int abi, uint32_t *words, int word_count )
{
	uint32_t stack_bytes = (uint32_t)word_count * sizeof( uint32_t );
	int cleanup_stack = (abi == FB_THREADCALL_CDECL);

	__asm__ __volatile__(
		"movl %[words], %%esi\n\t"
		"movl %[word_count], %%ecx\n\t"
		"testl %%ecx, %%ecx\n\t"
		"jz 2f\n\t"
		"leal -4(%%esi,%%ecx,4), %%esi\n\t"
	"1:\n\t"
		"pushl (%%esi)\n\t"
		"subl $4, %%esi\n\t"
		"decl %%ecx\n\t"
		"jnz 1b\n\t"
	"2:\n\t"
		"call *%[proc]\n\t"
		"cmpl $0, %[cleanup_stack]\n\t"
		"je 3f\n\t"
		"addl %[stack_bytes], %%esp\n\t"
	"3:\n\t"
		:
		: [proc] "m" (proc),
		  [cleanup_stack] "m" (cleanup_stack),
		  [words] "m" (words),
		  [word_count] "m" (word_count),
		  [stack_bytes] "m" (stack_bytes)
		: "eax", "ecx", "edx", "esi", "memory", "cc"
	);
}

static FBCALL void threadproc( void *param )
{
	FBTHREADCALL_XBOX *info = (FBTHREADCALL_XBOX *)param;

	hInvokeXboxThreadCall( info->proc, info->abi, info->words, info->word_count );

	free( info->words );
	free( info );
}

FBTHREAD *fb_ThreadCall( void *proc, int abi, ssize_t stack_size, int num_args, ... )
{
	FBTHREADCALL_XBOX_STACK stack;
	FBTHREADCALL_XBOX *param;
	FBTHREAD *thread;
	va_list args_list;
	int i;

	if( (proc == NULL) || (num_args < 0) || (num_args > FB_THREADCALL_XBOX_MAX_WORDS) )
		return NULL;

	stack.words = (uint32_t *)calloc( FB_THREADCALL_XBOX_MAX_WORDS, sizeof( uint32_t ) );
	if( stack.words == NULL )
		return NULL;
	stack.word_count = 0;

	va_start( args_list, num_args );
	for( i = 0; i < num_args; i++ ) {
		int arg_type;
		void *value;
		FBTHREADCALL_XBOX_LAYOUT layout;

		arg_type = va_arg( args_list, int );
		if( !hTypeLayout( &args_list, arg_type, &layout ) ) {
			va_end( args_list );
			free( stack.words );
			return NULL;
		}

		value = va_arg( args_list, void * );
		if( !hAppendStackArg( &stack, arg_type, &layout, value ) ) {
			va_end( args_list );
			free( stack.words );
			return NULL;
		}
	}
	va_end( args_list );

	param = (FBTHREADCALL_XBOX *)malloc( sizeof( FBTHREADCALL_XBOX ) );
	if( param == NULL ) {
		free( stack.words );
		return NULL;
	}

	param->proc = proc;
	param->abi = abi;
	param->word_count = stack.word_count;
	param->words = stack.words;

	thread = fb_ThreadCreate( threadproc, param, stack_size );
	if( thread == NULL ) {
		free( param->words );
		free( param );
	}

	return thread;
}

#else

FBTHREAD *fb_ThreadCall( void *proc, int abi, ssize_t stack_size, int num_args, ... )
{
	return NULL;
}

#endif

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

static ffi_type *getArgument( va_list *args_list );

static ffi_type *getStruct( va_list *args_list )
{
    int num_elems = va_arg( (*args_list), int );
    int i, j;

    /* prepare type */
    ffi_type *ffi_arg = (ffi_type *)malloc( sizeof( ffi_type ) );
    ffi_arg->size = 0;
    ffi_arg->alignment = 0;
    ffi_arg->type = FFI_TYPE_STRUCT;
    ffi_arg->elements = 
        (ffi_type **)malloc( sizeof( ffi_type * ) * ( num_elems + 1 ) );
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
                    freeStruct( ffi_arg );
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
    int arg_type = va_arg( (*args_list), int );
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
    int i, j;
    
    /* initialize lists and arrays */
    ffi_args = (ffi_type **)malloc( sizeof( ffi_type * ) * num_args );
    values = (void **)malloc( sizeof( void * ) * num_args );
    va_list args_list; 
    va_start(args_list, num_args);
    
    /* scan arguments and values from var_args */
    for( i=0; i<num_args; i++ )
    {
        ffi_args[i] = getArgument( &args_list );
        if( ffi_args[i] == NULL )
        {
            /* error, free all memory allocated up to this point */
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
    param->proc = proc;
    param->abi = abi;
    param->num_args = num_args;
    param->ffi_arg_types = ffi_args;
    param->values = values;
    
    /* actually start thread */
    return fb_ThreadCreate( threadproc, (void *)param, stack_size );
}

static FBCALL void threadproc( void *param )
{
    FBTHREADCALL *info = ( FBTHREADCALL * )param;
    ffi_status status = FFI_OK;
    ffi_abi abi = -1;
    ffi_cif cif;
    int i;

#ifdef HOST_X86_64
    abi = FFI_DEFAULT_ABI;
#else
    /* check calling convention */
    if( info->abi == FB_THREADCALL_CDECL )
        abi = FFI_SYSV;
#if defined( HOST_WIN32 ) || defined( HOST_XBOX )
    else if( info->abi == FB_THREADCALL_STDCALL )
        abi = FFI_STDCALL;
#endif
    else
        status = ~FFI_OK;

    /* prep FFI call interface */
    if( status == FFI_OK )
#endif
        status = ffi_prep_cif( 
            &cif,               // handle
            abi,                // ABI (CDECL or STDCALL on x86, host default on x86_64)
            info->num_args,     // number of arguments
            &ffi_type_void,     // return type
            info->ffi_arg_types // argument types
        );
        
    /* execute */
    if( status == FFI_OK )
        ffi_call( &cif, FFI_FN( info->proc ), NULL, info->values );
    

    /* free memory and exit */
    for( i=0; i<info->num_args; i++ )
    {
        if( info->ffi_arg_types[i]->type == FFI_TYPE_STRUCT )
            freeStruct( info->ffi_arg_types[i] );
    }
    free( info->values );
    free( info->ffi_arg_types );
    free( info );
}

#endif
