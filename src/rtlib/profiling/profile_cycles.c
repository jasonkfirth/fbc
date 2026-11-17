/*
    Project: FreeBASIC Runtime Library
    File: profiling/profile_cycles.c
    Purpose: Collect cycle measurements and generate profiler reports.
    Responsibilities: Manage cycle profiler state and write measurement data.
    This file intentionally does NOT contain: compiler instrumentation.
*/

/* profile_cycles.c -- cycle counting profiler */

/* WIP */
/* TODO: update src/compiler/runtime/rtl-system.bas:rtlInitProfile */
/* TODO: test exit code, profiler clean-up should still run
**       even when compiled exe does not encounter fb_End() */
/* TODO: organize this code by only having start-up code here
**       and the test in src/rtlib/profile-cycles.c */
/* TODO: share reporting code with src/rtlib/profiling/profile.c */
/* TODO: add a report option for binary output */

#include "fb.h"
#include <stdint.h>
#include <string.h>
#include "fb_profile.h"

#ifdef HOST_WIN32
	#include <windows.h>
#endif

/* choose a suitable ssize_t printf specifier */
#if !defined(fmtssizet)
	#if defined(HOST_CYGWIN)
		#define fmtssizet "%18zd"
		#define fmtssizetarg(value) ((ssize_t)(value))
	#elif defined(HOST_WIN32)
		#if defined(_UCRT) || __USE_MINGW_ANSI_STDIO
			#define fmtssizet "%18zd"
			#define fmtssizetarg(value) ((ssize_t)(value))
		#else
			#define fmtssizet "%18Id"
			#define fmtssizetarg(value) ((ptrdiff_t)(value))
		#endif
	#else
		#define fmtssizet "%18zd"
		#define fmtssizetarg(value) ((ssize_t)(value))
	#endif
#endif

#if defined(HOST_DARWIN)
/*
	Darwin's Mach-O assembler requires section names in "segment,section"
	form, and section names are limited to 16 bytes.  It also exposes section
	bounds through section$start/section$end symbols instead of the ELF
	__start_/__stop_ symbols used by GNU ld.
*/
#define FB_PROFILE_CYCLE_SECTION "__DATA,fb_profcycdata"
extern char fb_profilecycledata_start[] __asm("section$start$__DATA$fb_profcycdata");
extern char fb_profilecycledata_stop[] __asm("section$end$__DATA$fb_profcycdata");
#else
/* profile section data */
#define FB_PROFILE_CYCLE_SECTION "fb_profilecycledata"
extern char fb_profilecycledata_start[] FB_C_SYMBOL("__start_fb_profilecycledata");
extern char fb_profilecycledata_stop[] FB_C_SYMBOL("__stop_fb_profilecycledata");
#endif

/* profiler record ids - these indicate what the record is */
enum FB_PROFILE_REDORD_ID
{
	FB_PROFILE_RECORD_VERSION_ID = 1,
	FB_PROFILE_RECORD_DATA_ID    = 2
};

/* the current profiler version number */
#define FB_PROFILE_VERSION 100

typedef struct FB_PROFILE_RECORD_HEADER
{
	ssize_t size;
	ssize_t id;
} FB_PROFILE_RECORD_HEADER;

typedef struct FB_PROFILE_RECORD_VERSION
{
	ssize_t size;
	ssize_t id;
	ssize_t version;
	void * reserved1;
} FB_PROFILE_RECORD_VERSION;

typedef struct FB_PROFILE_RECORD_DATA
{
	ssize_t size;
	ssize_t id;
	char * module_name;
	char * proc_name;
	ssize_t init0;
	ssize_t grand_total;
	ssize_t reinit;
	ssize_t internal_total;
	ssize_t call_count;
	ssize_t reserved;
} FB_PROFILE_RECORD_DATA;

#if defined(_MSC_VER)
#define FB_PROFILE_ALIGNOF(type) __alignof(type)
#else
#define FB_PROFILE_ALIGNOF(type) __alignof__(type)
#endif

typedef struct FB_PROFILE_FILE_DATA
{
	ssize_t init0;
	ssize_t grand_total;
	ssize_t reinit;
	ssize_t internal_total;
	ssize_t call_count;
} FB_PROFILE_FILE_DATA;

/* cycles profiler global context */
/* use FB_PROFILE_LOCK()/FB_PROFILE_UNLOCK when accessing */
typedef struct FB_PROFILER_CYCLES_
{
	FB_PROFILER_GLOBAL *global;
#if defined(HOST_AROS) && defined(__arm__)
	/* Keep the floating timestamp naturally aligned on the ARM target ABI. */
	uint32_t start_time_alignment_padding;
#endif
	double start_time;
} FB_PROFILER_CYCLES;

/* ************************************
** Globals
*/

/* make sure there is at least one record in the profile data section */
static FB_PROFILE_RECORD_VERSION
__attribute__ ((aligned (16))) prof_data_version
__attribute__((section(FB_PROFILE_CYCLE_SECTION), used)) =
	{
		sizeof( FB_PROFILE_RECORD_VERSION ),
		FB_PROFILE_RECORD_VERSION_ID,
		FB_PROFILE_VERSION, 0
	};
static FB_PROFILER_CYCLES *fb_profiler = NULL;

/* ************************************
** Profiling
*/

static FB_PROFILER_CYCLES *PROFILER_CYCLES_create( void )
{
	if( fb_profiler == NULL ) {
		FB_PROFILER_CYCLES *prof;
		prof = PROFILER_calloc( 1, sizeof( FB_PROFILER_CYCLES ) );
		if( prof ) {
			prof->global = PROFILER_GLOBAL_create( );
			if( prof->global ) {
				prof->global->profiler_ctx = (void * )prof;
				fb_profiler = prof;
			} else {
				PROFILER_free( prof );
			}
		}
	}
	return fb_profiler;
}

static void PROFILER_CYCLES_destroy( void )
{
	if( fb_profiler ) {
		PROFILER_GLOBAL_destroy( );
		PROFILER_free( fb_profiler );
		fb_profiler = NULL;
	}
}

/* ************************************
** Report Generation
*/

#if 0
static hProfilerReportBinary( void )
{
	f1 = fopen( "profilename.prf", "w" );
	f2 = fopen( "profilecycles.prf", "w" );

	while( index < length )
	{
		FB_PROFILE_RECORD_HEADER *rec = (FB_PROFILE_RECORD_HEADER *)&data[index];
		switch ( rec->id )
		{
			case FB_PROFILE_RECORD_VERSION_ID:
			{
				// FB_PROFILE_RECORD_VERSION *ver = (FB_PROFILE_RECORD_VERSION *)rec;
				break;
			}
			case FB_PROFILE_RECORD_DATA_ID:
			{
				FB_PROFILE_RECORD_DATA *dat = (FB_PROFILE_RECORD_DATA *)rec;
				FB_PROFILE_FILE_DATA fil;

				fil.init0          = dat->init0;
				fil.grand_total    = dat->grand_total;
				fil.reinit         = dat->reinit;
				fil.internal_total = dat->internal_total;
				fil.call_count     = dat->call_count;

				fprintf( f1, "%s\n", dat->proc_name );
				fprintf( f1, "%s\n", dat->module_name );
				fwrite( &fil, sizeof(FB_PROFILE_FILE_DATA), 1, f2 );
				break;
			}
			default:
				break;
		}
		index += rec->size;
	}

	fclose( f2 );
	fclose( f1 );
}
#endif

static int hProfilerReadRecordHeader(const unsigned char *data, ssize_t index,
	ssize_t length, FB_PROFILE_RECORD_HEADER *record)
{
	ssize_t remaining;

	if ((data == NULL) || (record == NULL) || (index < 0) ||
	    (length < 0) || (index > length))
		return FALSE;
	remaining = length - index;
	if (remaining < (ssize_t)sizeof(*record))
		return FALSE;
	memcpy(record, data + (size_t)index, sizeof(*record));
	if ((record->size < (ssize_t)sizeof(*record)) ||
	    (record->size > remaining))
		return FALSE;
	return TRUE;
}

static int hProfilerRecordDataAligned(const unsigned char *data, ssize_t index)
{
	size_t alignment = (size_t)FB_PROFILE_ALIGNOF(FB_PROFILE_RECORD_DATA);

	return (data != NULL) && (index >= 0) && (alignment != 0) &&
		((uintptr_t)(data + (size_t)index) % alignment == 0);
}

static ssize_t hProfilerCountProcs( unsigned char *data, ssize_t length )
{
	ssize_t index = 0, count = 0;
	FB_PROFILE_RECORD_HEADER record;

	if ((length < 0) || ((data == NULL) && (length != 0)))
		return -1;
	while (index < length) {
		if (!hProfilerReadRecordHeader(data, index, length, &record))
			return -1;
		if (record.id == FB_PROFILE_RECORD_DATA_ID) {
			if (!hProfilerRecordDataAligned(data, index))
				return -1;
			count += 1;
		}
		index += record.size;
	}
	return count;
}

static int name_sorter( const void *e1, const void *e2 )
{
	FB_PROFILE_RECORD_DATA *p1;
	FB_PROFILE_RECORD_DATA *p2;
	int ret;
	memcpy( (void *)&p1, e1, sizeof(FB_PROFILE_RECORD_DATA *) );
	memcpy( (void *)&p2, e2, sizeof(FB_PROFILE_RECORD_DATA *) );

	ret = strcmp( p1->module_name, p2->module_name );
	if( ret ) {
		return ret;
	}
	return strcmp( p1->proc_name, p2->proc_name );
}

static void hProfilerSortArray( FB_PROFILE_RECORD_DATA **array, ssize_t count )
{
	if( count > 1 )
		qsort( (void *)array, (size_t)count, sizeof(FB_PROFILE_RECORD_DATA *), name_sorter );
}

static int hProfilerBuildArray (
	FB_PROFILE_RECORD_DATA **array, ssize_t count,
	unsigned char *data, ssize_t length
)
{
	ssize_t index = 0;
	ssize_t i = 0;
	FB_PROFILE_RECORD_HEADER record;
	void *record_storage;

	if ((array == NULL) || (count < 0) || (length < 0) ||
	    ((data == NULL) && (length != 0)))
		return FALSE;

	while( (index < length) && (i < count) ) {
		if (!hProfilerReadRecordHeader(data, index, length, &record))
			return FALSE;
		if (record.id == FB_PROFILE_RECORD_DATA_ID) {
			if (!hProfilerRecordDataAligned(data, index))
				return FALSE;
			record_storage = data + (size_t)index;
			array[i] = record_storage;
			i += 1;
		}
		index += record.size;
	}
	return i == count;
}

static void hProfilerReport (
	FB_PROFILER_CYCLES *prof, FILE *f,
	FB_PROFILE_RECORD_DATA **array, ssize_t count
)
{
	FB_PROFILE_RECORD_DATA *rec;
	char * last_module = NULL;
	ssize_t index;

	if( (prof->global->options & PROFILE_OPTION_HIDE_HEADER) == 0 )
	{
		fprintf( f, "module  function             total             inside         call count\n\n" );
	}

	for( index = 0; index < count; index ++ )
	{
		rec = array[index];
		if( last_module != rec->module_name ) {
			if( last_module != NULL ) {
				fprintf( f, "\n" );
			}
			last_module = rec->module_name;
			fprintf( f, "%s\n\n", rec->module_name );
		}

		fprintf( f, "        %s\n", rec->proc_name );
		fprintf( f, "                " fmtssizet " " fmtssizet " " fmtssizet "\n",
			fmtssizetarg(rec->grand_total),
			fmtssizetarg(rec->internal_total),
			fmtssizetarg(rec->call_count)
		);
	}
}

static void hProfilerWriteReport( FB_PROFILER_CYCLES *prof )
{
	char filename[PROFILER_MAX_PATH];
	FILE *f;
	unsigned char *data;
	ssize_t length, count;
	FB_PROFILE_RECORD_DATA **array;

	fb_ProfileGetFileName( filename, PROFILER_MAX_PATH );

	f = fb_hOpenFile( filename, "w" );
	if( f == NULL )
		return;

	if( (prof->global->options & PROFILE_OPTION_HIDE_HEADER) == 0 )
	{
		fprintf( f, "Cycle Count Profiling results:\n" );
		fprintf( f, "------------------------------\n\n" );

		fb_hGetExeName( filename, PROFILER_MAX_PATH-1 );
		fprintf( f, "Executable name: %s\n", filename );
		fprintf( f, "Launched on: %s\n", prof->global->launch_time );
		fprintf( f, "Total program execution time: %5.4g seconds\n", fb_Timer() - prof->start_time );
	}

	data = (unsigned char *)&fb_profilecycledata_start[0];
	length = (ssize_t)&fb_profilecycledata_stop[0] - (ssize_t)&fb_profilecycledata_start[0];

	count = hProfilerCountProcs( data, length );
	if( (count > 0) &&
	    ((size_t)count <= SIZE_MAX / sizeof(FB_PROFILE_RECORD_DATA *)) ) {
		array = (FB_PROFILE_RECORD_DATA **)PROFILER_malloc(
			sizeof(FB_PROFILE_RECORD_DATA *) * (size_t)count );
		if( array ) {
			if (hProfilerBuildArray(array, count, data, length)) {
				hProfilerSortArray( array, count );
				hProfilerReport( prof, f, array, count );
			}
			PROFILER_free( (void *)array );
		}
	}

	fclose( f );
}

/* ************************************
** Cylcles Profing Public API
*/

/*:::::*/
FBCALL void fb_InitProfileCycles( void )
{
	if( fb_profiler ) {
		return;
	}
	fb_profiler = PROFILER_CYCLES_create( );
	if( fb_profiler ) {
		fb_profiler->start_time = fb_Timer();
	}
}

/*:::::*/
FBCALL int fb_EndProfileCycles( int errorlevel )
{
	if( !fb_profiler ) {
		return errorlevel;
	}

	hProfilerWriteReport( fb_profiler );
	PROFILER_CYCLES_destroy( );
	return errorlevel;
}

/*:::::*/
FBCALL FB_PROFILER_CYCLES *fb_ProfileGetCyclesProfiler( void )
{
	return fb_profiler;
}

/* end of profiling/profile_cycles.c */
