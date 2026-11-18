/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/profile_cycles.c
 * Purpose: Provide profile cycles runtime support.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
/*
    DOS cycle-profiler implementation

    This complete target file is selected by basename precedence.  Keep DOS
    behavior here instead of adding HOST_DOS branches to the shared source.
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

#include "../../fb.h"
#include "../../fb_profile.h"
#include <stdint.h>

/* choose a suitable ssize_t printf specifier */
#if !defined(fmtssizet)
	#define fmtssizet "%18zd"
	#define fmtssizetarg(value) ((ssize_t)(value))
#endif

/* profile section data */
#define FB_PROFILE_CYCLE_SECTION "fb_profilecycledata"

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

extern FB_PROFILE_RECORD_HEADER fb_profilecycledata_start[];
extern FB_PROFILE_RECORD_HEADER fb_profilecycledata_stop[];

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

static int hProfilerNextRecord(FB_PROFILE_RECORD_HEADER *data, size_t length,
	                           size_t index, FB_PROFILE_RECORD_HEADER **record,
	                           size_t *record_units)
{
	FB_PROFILE_RECORD_HEADER *current;
	size_t byte_size, units;

	if (!data || !record || !record_units || (index >= length))
		return 0;
	current = data + index;
	if (current->size < (ssize_t)sizeof(*current))
		return 0;
	byte_size = (size_t)current->size;
	if ((byte_size % sizeof(*current)) != 0)
		return 0;
	units = byte_size / sizeof(*current);
	if ((units > length - index) ||
	    ((current->id == FB_PROFILE_RECORD_DATA_ID) &&
	     (byte_size < sizeof(FB_PROFILE_RECORD_DATA))))
		return 0;
	*record = current;
	*record_units = units;
	return 1;
}

static size_t hProfilerCountProcs(FB_PROFILE_RECORD_HEADER *data,
	                               size_t length)
{
	size_t index = 0, count = 0, record_units;
	FB_PROFILE_RECORD_HEADER *record;

	while (index < length) {
		if (!hProfilerNextRecord(data, length, index, &record, &record_units))
			return 0;
		if (record->id == FB_PROFILE_RECORD_DATA_ID)
			count++;
		index += record_units;
	}
	return count;
}

static int name_sorter( const void *e1, const void *e2 )
{
	FB_PROFILE_RECORD_DATA *p1, *p2;
	int ret;
	memcpy(&p1, e1, sizeof(p1));
	memcpy(&p2, e2, sizeof(p2));

	ret = strcmp( p1->module_name, p2->module_name );
	if( ret ) {
		return ret;
	}
	return strcmp( p1->proc_name, p2->proc_name );
}

static void hProfilerSortArray( FB_PROFILE_RECORD_DATA **array, size_t count )
{
	qsort(array, count, sizeof(*array), name_sorter);
}

static size_t hProfilerBuildArray(FB_PROFILE_RECORD_DATA **array, size_t count,
	                               FB_PROFILE_RECORD_HEADER *data,
	                               size_t length)
{
	size_t index = 0, i = 0, record_units;
	FB_PROFILE_RECORD_HEADER *record;

	if (!array)
		return 0;

	while ((index < length) && (i < count)) {
		if (!hProfilerNextRecord(data, length, index, &record, &record_units))
			return 0;
		if (record->id == FB_PROFILE_RECORD_DATA_ID) {
			array[i] = (FB_PROFILE_RECORD_DATA *)record;
			i++;
		}
		index += record_units;
	}
	return i;
}

static void hProfilerReport (
	FB_PROFILER_CYCLES *prof, FILE *f,
	FB_PROFILE_RECORD_DATA **array, size_t count
)
{
	FB_PROFILE_RECORD_DATA *rec;
	char * last_module = NULL;
	size_t index;

	if( (prof->global->options & PROFILE_OPTION_HIDE_HEADER) == 0 )
	{
		fprintf( f, "module  function             total             inside         call count\n\n" );
	}

	for( index = 0; index < count; index++ )
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
	FB_PROFILE_RECORD_HEADER *data;
	uintptr_t data_start, data_stop;
	size_t length, count;
	FB_PROFILE_RECORD_DATA **array;

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

	data_start = (uintptr_t)fb_profilecycledata_start;
	data_stop = (uintptr_t)fb_profilecycledata_stop;
	if ((data_stop < data_start) ||
	    (((data_stop - data_start) % sizeof(*data)) != 0)) {
		fclose(f);
		return;
	}
	data = fb_profilecycledata_start;
	length = (size_t)((data_stop - data_start) / sizeof(*data));

	count = hProfilerCountProcs( data, length );
	if( count ) {
		if (count > (size_t)-1 / sizeof(*array)) {
			fclose(f);
			return;
		}
		array = PROFILER_malloc(sizeof(*array) * count);
		if( array ) {
			if (hProfilerBuildArray(array, count, data, length) == count) {
				hProfilerSortArray(array, count);
				hProfilerReport(prof, f, array, count);
			}
			PROFILER_free(array);
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

/* end of drivers/dos/profile_cycles.c */
