/*
 * Project: FreeBASIC runtime library
 * File: fb_array.h
 * Purpose: Provide fb array runtime support.
 * Responsibilities: Shared runtime types, ABI declarations and internal contracts.
 * This file intentionally does NOT contain: Platform implementations or compiler parsing.
 */
typedef struct FBARRAYDIM_ {
	size_t elements;
	ssize_t lbound;
	ssize_t ubound;
} FBARRAYDIM;

/* Keep the reserved mask out of the enum because it does not fit in int. */
typedef uint32_t FBARRAY_FLAGS;
#define FBARRAY_FLAGS_DIMENSIONS ((FBARRAY_FLAGS)0x0000000fu)
#define FBARRAY_FLAGS_FIXED_DIM  ((FBARRAY_FLAGS)0x00000010u)
#define FBARRAY_FLAGS_FIXED_LEN  ((FBARRAY_FLAGS)0x00000020u)
#define FBARRAY_FLAGS_RESERVED   ((FBARRAY_FLAGS)0xffffffc0u)

/* DATA stores the native address bits for the virtual zero-index origin.
   It can lie outside the allocation, so form and adjust it through
   uintptr_t, not C pointer arithmetic. PTR owns the physical storage.
   DATA remains the first word for generated code and existing modules. */
typedef struct FBARRAY_ {
	void           *data;        /* encoded ptr + diff, must be at ofs 0! */
	void           *ptr;
	size_t          size;
	size_t          element_len;
	size_t          dimensions;
	size_t          flags;       /* FBARRAY_FLAGS */
	FBARRAYDIM      dimTB[1];    /* dimtb[dimensions] */
} FBARRAY;

/* Access views preserve packed BASIC descriptor addresses without
   lowering the alignment of public or by-value storage. */
typedef FBARRAY FBARRAY_ACCESS __attribute__((aligned(1)));
typedef FBARRAYDIM FBARRAYDIM_ACCESS __attribute__((aligned(1)));

typedef void (*FB_DEFCTOR)( void *this_ );
typedef void (*FB_DTORMULT) ( FBARRAY_ACCESS *array, FB_DEFCTOR dtor, size_t base_idx );

FBCALL void *fb_ArrayBoundChkEx
	(
		ssize_t idx,
		ssize_t lbound,
		ssize_t ubound,
		int linenum,
		const char *filename,
		const char *variablename
	);

FBCALL void *fb_ArraySngBoundChkEx
	(
		size_t idx,
		size_t ubound,
		int linenum,
		const char *filename,
		const char *variablename
	);

FBCALL void *fb_ArrayDimensionChk
	(
		ssize_t dimensions,
		FBARRAY_ACCESS *array,
		int linenum,
		const char *filename,
		const char *variablename
	);

/* Legacy entry points kept for debug runtimes built by older compilers. */
FBCALL void *fb_ArrayBoundChk
	(
		ssize_t idx,
		ssize_t lbound,
		ssize_t ubound,
		int linenum,
		const char *filename
	);

FBCALL void *fb_ArraySngBoundChk
	(
		size_t idx,
		size_t ubound,
		int linenum,
		const char *filename
	);

FBCALL size_t     fb_ArrayLen          ( FBARRAY_ACCESS *array );
FBCALL size_t     fb_ArraySize         ( FBARRAY_ACCESS *array );
       void       fb_hArrayCtorObj     ( FBARRAY_ACCESS *array, FB_DEFCTOR ctor );
       void       fb_hArrayDtorObj     ( FBARRAY_ACCESS *array, FB_DEFCTOR dtor, size_t keep_idx );
       void       fb_hArrayDtorStr     ( FBARRAY_ACCESS *array, FB_DEFCTOR dtor, size_t keep_idx );
FBCALL void       fb_ArrayDestructObj  ( FBARRAY_ACCESS *array, FB_DEFCTOR dtor );
FBCALL void       fb_ArrayDestructStr  ( FBARRAY_ACCESS *array );
FBCALL int        fb_ArrayClear        ( FBARRAY_ACCESS *array );
FBCALL int        fb_ArrayClearObj     ( FBARRAY_ACCESS *array, FB_DEFCTOR ctor, FB_DEFCTOR dtor );
FBCALL int        fb_ArrayErase        ( FBARRAY_ACCESS *array );
FBCALL int        fb_ArrayEraseObj     ( FBARRAY_ACCESS *array, FB_DEFCTOR ctor, FB_DEFCTOR dtor );
FBCALL int        fb_ArrayFill         ( FBARRAY_ACCESS *array, int fillchar );
FBCALL FBARRAY_ACCESS   *fb_ArrayGetDesc      ( FBARRAY_ACCESS *array );
FBCALL void       fb_ArrayStrErase     ( FBARRAY_ACCESS *array );
       int        fb_ArrayRedimEx      ( FBARRAY_ACCESS *array, size_t element_len, int doclear, int isvarlen, size_t dimensions, ... );
       int        fb_ArrayRedimObj     ( FBARRAY_ACCESS *array, size_t element_len, FB_DEFCTOR ctor, FB_DEFCTOR dtor, size_t dimensions, ... );
       int        fb_ArrayRedimPresvEx ( FBARRAY_ACCESS *array, size_t element_len, int doclear, int isvarlen, size_t dimensions, ... );
       int        fb_ArrayRedimPresvObj( FBARRAY_ACCESS *array, size_t element_len, FB_DEFCTOR ctor, FB_DEFCTOR dtor, size_t dimensions, ... );
FBCALL int        fb_ArrayRedimTo      ( FBARRAY_ACCESS *dest, const FBARRAY_ACCESS *source, int isvarlen, FB_DEFCTOR ctor, FB_DEFCTOR dtor );
FBCALL void       fb_ArrayResetDesc    ( FBARRAY_ACCESS *array );
FBCALL ssize_t    fb_ArrayLBound       ( FBARRAY_ACCESS *array, ssize_t dimension );
FBCALL ssize_t    fb_ArrayUBound       ( FBARRAY_ACCESS *array, ssize_t dimension );
       size_t     fb_hArrayCalcElements( size_t dimensions, const ssize_t *lboundTB, const ssize_t *uboundTB );
       ssize_t    fb_hArrayCalcDiff    ( size_t dimensions, const ssize_t *lboundTB, const ssize_t *uboundTB );

int fb_hArrayAlloc
	(
		FBARRAY_ACCESS *array,
		size_t element_len,
		int doclear,
		FB_DEFCTOR ctor,
		size_t dimensions,
		va_list ap
	);

int fb_hArrayRealloc
	(
		FBARRAY_ACCESS *array,
		size_t element_len,
		int doclear,
		FB_DEFCTOR ctor,
		FB_DTORMULT dtor_mult,
		FB_DEFCTOR dtor,
		size_t dimensions,
		va_list ap
	);

/* end of fb_array.h */
