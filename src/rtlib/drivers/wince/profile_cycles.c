/*
    Project: FreeBASIC Runtime Library
    -------------------------

    File: drivers/wince/profile_cycles.c

    Purpose:

        Implement cycle-profiler reporting for the Windows CE PE/COFF ABI.

    Responsibilities:

        - collect compiler-emitted profiling records
        - account for nested procedure timing
        - write the textual cycle report

    This file intentionally does NOT contain: - instrumentation insertion
        - desktop performance-counter setup
        - call profiler implementation

    PE/COFF alignment note:

        The ARM WinCE object format limits object alignment to eight bytes.
        Profiler records use that maximum instead of the generic 16-byte ELF
        alignment.
*/

/* WIP */
/* TODO: update src/compiler/runtime/rtl-system.bas:rtlInitProfile */
/* TODO: test exit code, profiler clean-up should still run
**       even when compiled exe does not encounter fb_End() */
/* TODO: organize this code by only having start-up code here
**       and the test in src/rtlib/profile-cycles.c */
/* TODO: share reporting code with src/rtlib/profiling/profile.c */
/* TODO: add a report option for binary output */

#include "../../fb.h"
#include "../../fb_profile.h"

#include <windows.h>

/* choose a suitable ssize_t printf specifier */
#if !defined(fmtssizet)
	#define fmtssizet "%18d"
	#define fmtssizetarg(value) ((int)(value))
#endif

/* profile section data */
#define FB_PROFILE_CYCLE_SECTION "fb_profilecycledata"
extern char fb_profilecycledata_start[];
extern char fb_profilecycledata_stop[];

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
	double start_time;
} FB_PROFILER_CYCLES;

/* ************************************
** Globals
*/

/* make sure there is at least one record in the profile data section */
static FB_PROFILE_RECORD_VERSION
__attribute__ ((aligned (8))) prof_data_version
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

static int hProfilerReadRecordHeader( const unsigned char *data,
	ssize_t index, ssize_t length, FB_PROFILE_RECORD_HEADER *record )
{
	ssize_t remaining;

	if( (data == NULL) || (record == NULL) || (index < 0) ||
	    (length < 0) || (index > length) )
		return FALSE;
	remaining = length - index;
	if( remaining < (ssize_t)sizeof( *record ) )
		return FALSE;
	memcpy( record, data + (size_t)index, sizeof( *record ) );
	if( (record->size < (ssize_t)sizeof( *record )) ||
	    (record->size > remaining) )
		return FALSE;
	if( (record->id == FB_PROFILE_RECORD_DATA_ID) &&
	    (record->size < (ssize_t)sizeof( FB_PROFILE_RECORD_DATA )) )
		return FALSE;
	return TRUE;
}

static int hProfilerRecordDataAligned( const unsigned char *data,
	ssize_t index )
{
	size_t alignment = (size_t)__alignof__( FB_PROFILE_RECORD_DATA );

	return (data != NULL) && (index >= 0) && (alignment != 0) &&
	       ((uintptr_t)(data + (size_t)index) % alignment == 0);
}

static ssize_t hProfilerCountProcs( unsigned char *data, ssize_t length )
{
	ssize_t index = 0;
	ssize_t count = 0;
	FB_PROFILE_RECORD_HEADER record;

	if( (length < 0) || ((data == NULL) && (length != 0)) )
		return -1;
	while( index < length ) {
		if( !hProfilerReadRecordHeader( data, index, length, &record ) )
			return -1;
		if( record.id == FB_PROFILE_RECORD_DATA_ID ) {
			if( !hProfilerRecordDataAligned( data, index ) )
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
	memcpy( &p1, e1, sizeof( p1 ) );
	memcpy( &p2, e2, sizeof( p2 ) );

	ret = strcmp( p1->module_name, p2->module_name );
	if( ret ) {
		return ret;
	}
	return strcmp( p1->proc_name, p2->proc_name );
}

static void hProfilerSortArray( FB_PROFILE_RECORD_DATA **array, ssize_t count )
{
	if( count > 1 )
		qsort( (void *)array, (size_t)count,
		       sizeof( FB_PROFILE_RECORD_DATA * ), name_sorter );
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

	if( (array == NULL) || (count < 0) || (length < 0) ||
	    ((data == NULL) && (length != 0)) )
		return FALSE;

	while( (index < length) && (i < count) ) {
		if( !hProfilerReadRecordHeader( data, index, length, &record ) )
			return FALSE;
		if( record.id == FB_PROFILE_RECORD_DATA_ID ) {
			if( !hProfilerRecordDataAligned( data, index ) )
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
	uintptr_t data_start;
	uintptr_t data_stop;
	size_t array_bytes;

	fb_ProfileGetFileName( filename, PROFILER_MAX_PATH );

	f = fopen( filename, "w" );
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
	data_start = (uintptr_t)fb_profilecycledata_start;
	data_stop = (uintptr_t)fb_profilecycledata_stop;
	if( (data_stop < data_start) ||
	    ((data_stop - data_start) > (uintptr_t)FB_STRSIZEMSK) ) {
		fclose( f );
		return;
	}
	length = (ssize_t)(data_stop - data_start);

	count = hProfilerCountProcs( data, length );
	if( count > 0 ) {
		if( (size_t)count > (SIZE_MAX / sizeof( FB_PROFILE_RECORD_DATA * )) ) {
			fclose( f );
			return;
		}
		array_bytes = (size_t)count * sizeof( FB_PROFILE_RECORD_DATA * );
		array = (FB_PROFILE_RECORD_DATA **)PROFILER_malloc( array_bytes );
		if( array != NULL ) {
			if( hProfilerBuildArray( array, count, data, length ) ) {
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

/* end of drivers/wince/profile_cycles.c */
