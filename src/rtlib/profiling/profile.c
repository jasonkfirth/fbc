/*
 * Project: FreeBASIC runtime library
 * File: profiling/profile.c
 * Purpose: Provide profile runtime support.
 * Responsibilities: Profiling state, measurements and report generation.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
/*
** profile.c -- profiling core functions
**
** chng: apr/2024 add API to set output file name and report options [jeffm]
**       apr/2024 dynamic string table [jeffm]
**       apr/2024 add profiler lock (replacing fb lock) [jeffm]
*/

/* TODO: test the start-up and exit code more */

#include "fb.h"
#include "fb_profile.h"

#include <limits.h>
#include <string.h>

#ifdef HOST_WIN32
	#include <windows.h>
#endif
#include <time.h>

/* ************************************
** Globals - common to all profilers
*/

static FB_PROFILER_GLOBAL *fb_profiler = NULL;

/* ************************************
** Helpers
*/

unsigned int fb_ProfileHashName( const char *p )
{
	unsigned int hash = 0;
	while( *p ) {
		hash += (unsigned int)*p;
		hash += hash << 10;
		hash ^= hash >> 6;
		++p;
	}
	hash += hash << 3;
	hash ^= hash >> 11;
	hash += hash << 15;
	return hash;
}

/* ************************************
** String Storage
*/

STRING_INFO *STRING_TABLE_alloc( STRING_TABLE *strings, int length )
{
	STRING_INFO *info;
	void *record_storage;
	size_t padded_length;
	size_t record_size;
	int size;

	if( !strings || !strings->tb || (length <= 0) ||
		((size_t)length > (size_t)STRING_INFO_TB_SIZE) ) {
		return NULL;
	}
	if( (strings->tb->bytes_used < 0) ||
		(strings->tb->bytes_used > STRING_INFO_TB_SIZE) ) {
		return NULL;
	}

	/* minumum size per string (assuming 4 bytes per int)
	** - space for STRING_INFO
	** - pad with additional NUL bytes to make size a multiple of 4
	*/
	padded_length = (size_t)length + (sizeof(int)-1);
	padded_length &= ~((size_t)sizeof(int)-1);
	record_size = sizeof(STRING_INFO) + padded_length;

	if( record_size > (size_t)STRING_INFO_TB_SIZE ) {
		return NULL;
	}
	size = (int)record_size;

	if( size > (STRING_INFO_TB_SIZE - strings->tb->bytes_used) ) {
		STRING_INFO_TB *tb;

		if( strings->tb->string_tb_id == INT_MAX ) {
			return NULL;
		}
		tb = (STRING_INFO_TB*)PROFILER_calloc( 1, sizeof(STRING_INFO_TB) );
		if( !tb ) {
			return NULL;
		}

		tb->bytes_used = 0;
		tb->string_tb_id = strings->tb->string_tb_id + 1;
		tb->next = strings->tb;
		strings->tb = tb;
	}

	record_storage = &strings->tb->data.bytes[strings->tb->bytes_used];
	info = record_storage;
	info->size = size;
	strings->tb->bytes_used += size;

	return info;
}

void STRING_TABLE_constructor( STRING_TABLE *strings )
{
	if( strings ) {
		STRING_INFO_TB *tb = (STRING_INFO_TB*)PROFILER_calloc( 1, sizeof(STRING_INFO_TB) );
		if( tb ) {
			tb->bytes_used = 0;
			tb->string_tb_id = 1;
			tb->next = NULL;
		}
		strings->tb = tb;
	}
}

void STRING_TABLE_destructor( STRING_TABLE *strings )
{
	if( strings ) {
		STRING_INFO_TB *tb = strings->tb;
		while( tb ) {
			STRING_INFO_TB *tb_next = tb->next;
			PROFILER_free( tb );
			tb = tb_next;
		}
	}
}

STRING_INFO *STRING_TABLE_add( STRING_TABLE *strings, const char *src, unsigned int hashkey )
{
	STRING_INFO *info;
	size_t source_length = 0;
	size_t max_source_length = (size_t)STRING_INFO_TB_SIZE - sizeof(STRING_INFO);
	int length;

	if( !src ) {
		return NULL;
	}
	while( (source_length < max_source_length) && (src[source_length] != '\0') ) {
		source_length += 1;
	}
	if( source_length == max_source_length ) {
		return NULL;
	}
	length = (int)(source_length + 1);

	info = STRING_TABLE_alloc( strings, length );
	if( info ) {
		info->length = length;
		info->hashkey = hashkey;

		/* copy the string */
		memcpy( info + 1, src, (size_t)length );
	}
	return info;
}

int STRING_TABLE_max_len( STRING_TABLE *strings )
{
	STRING_INFO_TB *tb;
	STRING_INFO info;
	const int header_size = (int)sizeof(STRING_INFO);
	int max_len = 0, index;
	if( strings ) {
		tb = strings->tb;
		while ( tb ) {
			if( (tb->bytes_used < 0) || (tb->bytes_used > STRING_INFO_TB_SIZE) ) {
				tb = tb->next;
				continue;
			}
			index = 0;
			while( index < tb->bytes_used )
			{
				int bytes_remaining = tb->bytes_used - index;
				if( bytes_remaining < header_size ) {
					break;
				}
				memcpy( &info, &tb->data.bytes[index], sizeof(info) );
				if( (info.size < header_size) || (info.size > bytes_remaining) ||
					(info.length < 1) || ((size_t)info.length > (size_t)info.size - sizeof(info)) ) {
					break;
				}
				if( info.length > max_len ) {
					max_len = info.length;
				}
				index += info.size;
			}
			tb = tb->next;
		}
	}
	return max_len;
}

STRING_INFO *STRING_HASH_TB_find( STRING_HASH_TB *tb, const char *src, unsigned int hashkey )
{
	STRING_INFO *info;
	int hash_index = (int)(hashkey % (unsigned int)STRING_HASH_TB_SIZE);

	if( !src ) {
		return NULL;
	}

	while( tb ) {
		info = tb->items[hash_index];
		if( info ) {
			if( (info->hashkey == hashkey) && (strcmp( (char *)(info + 1), src ) == 0) ) {
				return info;
			}
		} else {
			break;
		}
		tb = tb->next;
	}
	return NULL;
}

STRING_INFO *STRING_HASH_TABLE_find( STRING_HASH_TABLE *hash, const char *src, unsigned int hashkey )
{
	if( !hash ) {
		return NULL;
	}
	return STRING_HASH_TB_find( hash->tb, src, hashkey );
}

void STRING_HASH_TABLE_constructor( STRING_HASH_TABLE *hash, STRING_TABLE *strings )
{
	if( hash ) {
		hash->tb = NULL;
		hash->strings = strings;
	}
}

void STRING_HASH_TABLE_destructor( STRING_HASH_TABLE *hash )
{
	if( hash ) {
		STRING_HASH_TB *tb = hash->tb;
		while ( tb ) {
			STRING_HASH_TB *tb_next = tb->next;
			PROFILER_free( tb );
			tb = tb_next;
		}
		hash->tb = NULL;
	}
}

STRING_INFO *STRING_HASH_TABLE_add( STRING_HASH_TABLE *hash, const char *src, unsigned int hashkey )
{
	STRING_HASH_TB *tb;
	STRING_INFO *info;
	int hash_index;

	if( !hash || !src ) {
		return NULL;
	}
	tb = hash->tb;
	hash_index = (int)(hashkey % (unsigned int)STRING_HASH_TB_SIZE);

	if( !tb ) {
		tb = (STRING_HASH_TB*)PROFILER_calloc( 1, sizeof( STRING_HASH_TB ) );
		hash->tb = tb;
	}

	while( tb )
	{
		info = tb->items[hash_index];
		if( info ) {
			if( (info->hashkey == hashkey) && (strcmp( (char *)(info + 1), src ) == 0) ) {
				return info;
			}
		} else {
			info = STRING_TABLE_add( hash->strings, src, hashkey );
			tb->items[hash_index] = info;
			return info;
		}
		if( !tb->next ) {
			tb->next = (STRING_HASH_TB*)PROFILER_calloc( 1, sizeof( STRING_HASH_TB ) );
		}
		tb = tb->next;
	}
	return NULL;
}

STRING_INFO *STRING_HASH_TABLE_add_info( STRING_HASH_TABLE *hash, STRING_INFO *new_info )
{
	STRING_HASH_TB *tb;
	STRING_INFO *info;
	int hash_index;

	if( !hash || !new_info ) {
		return NULL;
	}
	tb = hash->tb;
	hash_index = (int)(new_info->hashkey % (unsigned int)STRING_HASH_TB_SIZE);

	if( !tb ) {
		tb = (STRING_HASH_TB*)PROFILER_calloc( 1, sizeof( STRING_HASH_TB ) );
		hash->tb = tb;
	}

	while( tb )
	{
		info = tb->items[hash_index];
		if( info ) {
			if( info == new_info ) {
				return info;
			}
		} else {
			tb->items[hash_index] = new_info;
			return new_info;
		}
		if( !tb->next ) {
			tb->next = (STRING_HASH_TB*)PROFILER_calloc( 1, sizeof( STRING_HASH_TB ) );
		}
		tb = tb->next;
	}
	return NULL;
}

void STRING_HASH_constructor( STRING_HASH *hash, STRING_HASH_TABLE *strings_hash )
{
	if( hash ) {
		STRING_HASH_TABLE_constructor( &hash->hash, strings_hash->strings );
		hash->strings_hash = strings_hash;
	}
}

void STRING_HASH_destructor( STRING_HASH *hash )
{
	if( hash ) {
		STRING_HASH_TABLE_destructor( &hash->hash );
		hash->strings_hash = NULL;
	}
}

void STRING_HASH_add( STRING_HASH *hash, const char *src, unsigned int hashkey )
{
	STRING_INFO *info = STRING_HASH_TABLE_add( hash->strings_hash, src, hashkey );
	if( info ) {
		STRING_HASH_TABLE_add_info( &hash->hash, info );
	}
}

STRING_INFO *STRING_HASH_find( STRING_HASH *hash, const char *src, unsigned int hashkey )
{
	return STRING_HASH_TB_find( hash->hash.tb, src, hashkey );
}

char *PROFILER_add_ignore( FB_PROFILER_GLOBAL *prof, const char *src, unsigned int hashkey )
{
	STRING_INFO *info = STRING_HASH_TABLE_add( &prof->ignores_hash, src, hashkey );
	return (char*)(info+1);
}

/* ************************************
** Profiler Metrics (internal stats)
*/

static int hProfilerMetricsAddSize( int total, size_t size )
{
	if( (total < 0) || (size > (size_t)INT_MAX) ||
		((size_t)total > (size_t)INT_MAX - size) ) {
		return INT_MAX;
	}
	return total + (int)size;
}

void fb_hPROFILER_METRICS_Clear( FB_PROFILER_METRICS *metrics )
{
	if( metrics ) {
		metrics->count_threads = 0;

		metrics->string_bytes_allocated = 0;
		metrics->string_bytes_used = 0;
		metrics->string_bytes_free = 0;
		metrics->string_count_blocks = 0;
		metrics->string_count_strings = 0;

		metrics->hash_bytes_allocated = 0;
		metrics->hash_count_blocks = 0;
		metrics->hash_count_items = 0;
		metrics->hash_count_slots = 0;

		metrics->procs_bytes_allocated = 0;
		metrics->procs_count_blocks = 0;
		metrics->procs_count_items = 0;
		metrics->procs_count_slots = 0;
	}
}

void fb_hPROFILER_METRICS_Strings( FB_PROFILER_METRICS *metrics, STRING_TABLE *strings )
{
	STRING_INFO_TB *tb;
	STRING_INFO info;
	const int header_size = (int)sizeof(STRING_INFO);
	int index;

	if( metrics && strings ) {
		tb = strings->tb;
		while ( tb ) {
			if( (tb->bytes_used < 0) || (tb->bytes_used > STRING_INFO_TB_SIZE) ) {
				tb = tb->next;
				continue;
			}
			metrics->string_bytes_allocated = hProfilerMetricsAddSize( metrics->string_bytes_allocated, sizeof(STRING_INFO_TB) );
			metrics->string_bytes_used = hProfilerMetricsAddSize( metrics->string_bytes_used, (size_t)tb->bytes_used );
			metrics->string_bytes_free = hProfilerMetricsAddSize( metrics->string_bytes_free, sizeof(STRING_INFO_TB) - (size_t)tb->bytes_used );
			metrics->string_count_blocks += 1;

			index = 0;
			while( index < tb->bytes_used )
			{
				int bytes_remaining = tb->bytes_used - index;
				if( bytes_remaining < header_size ) {
					break;
				}
				memcpy( &info, &tb->data.bytes[index], sizeof(info) );
				if( (info.size < header_size) || (info.size > bytes_remaining) ||
					(info.length < 1) || ((size_t)info.length > (size_t)info.size - sizeof(info)) ) {
					break;
				}
				if( info.length > metrics->string_max_len ) {
					metrics->string_max_len = info.length;
				}
				metrics->string_count_strings += 1;
				index += info.size;
			}
			tb = tb->next;
		}
	}
}

void fb_hPROFILER_METRICS_HashTable( FB_PROFILER_METRICS *metrics, STRING_HASH_TABLE *hash )
{
	STRING_HASH_TB *tb;
	int i;
	if( metrics && hash ) {
		tb = hash->tb;
		while( tb ) {
			metrics->hash_bytes_allocated = hProfilerMetricsAddSize( metrics->hash_bytes_allocated, sizeof(STRING_HASH_TB) );
			metrics->hash_count_blocks += 1;

			for( i=0; i < STRING_HASH_TB_SIZE; i++ )
			{
				if( tb->items[i] ) {
					metrics->hash_count_items += 1;
				}
				metrics->hash_count_slots += 1;

			}
			tb = tb->next;
		}
	}
}

void fb_hPROFILER_METRICS_Global( FB_PROFILER_METRICS *metrics, FB_PROFILER_GLOBAL *global )
{
	fb_hPROFILER_METRICS_Strings( metrics, &global->strings );
	fb_hPROFILER_METRICS_HashTable( metrics, &global->strings_hash );
	fb_hPROFILER_METRICS_HashTable( metrics, &global->ignores_hash );
}

/* ************************************
** Profiling
*/

FB_PROFILER_GLOBAL *PROFILER_GLOBAL_create( void ) {
	if( fb_profiler ) {
		return fb_profiler;
	}

	fb_profiler = PROFILER_calloc( 1, sizeof( FB_PROFILER_GLOBAL ) );
	if( fb_profiler ) {
		time_t rawtime = { 0 };
		struct tm *ptm = { 0 };

		STRING_TABLE_constructor( &fb_profiler->strings );
		STRING_HASH_TABLE_constructor( &fb_profiler->strings_hash, &fb_profiler->strings );
		STRING_HASH_TABLE_constructor( &fb_profiler->ignores_hash, &fb_profiler->strings );
		fb_profiler->filename[0] = 0;
		fb_profiler->launch_time[0] = 0;

		time( &rawtime );
		ptm = localtime( &rawtime );
		if( ptm != NULL ) {
			snprintf( fb_profiler->launch_time, sizeof(fb_profiler->launch_time),
			         "%02d-%02d-%04d, %02d:%02d:%02d",
			         (ptm->tm_mon + 1) % 100, ptm->tm_mday % 100,
			         (1900 + ptm->tm_year) % 10000, ptm->tm_hour % 100,
			         ptm->tm_min % 100, ptm->tm_sec % 100 );
		}
		fb_profiler->launch_time[sizeof(fb_profiler->launch_time)-1] = '\0';
	}
	return fb_profiler;
}

void PROFILER_GLOBAL_destroy( void ) {
	if( fb_profiler ) {
		STRING_HASH_TABLE_destructor( &fb_profiler->strings_hash );
		STRING_HASH_TABLE_destructor( &fb_profiler->ignores_hash );
		STRING_TABLE_destructor( &fb_profiler->strings );
		PROFILER_free( fb_profiler );
		fb_profiler = NULL;
	}

}

/* ************************************
** Profiling Options
*/

static int hProfileCopyFilename( char *dst, const char *src, int length )
{
	size_t len;
	size_t capacity;

	if( !fb_profiler || !dst || !src || (length <= 0) ) {
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	capacity = (size_t)length;
	len = strlen(src);

	if( (len < 1) || (len >= (size_t)PROFILER_MAX_PATH - 1) || (len >= capacity) ) {
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	strncpy( dst, src, capacity );
	dst[capacity - 1] = '\0';

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/*:::::*/
FBCALL int fb_ProfileSetFileName( const char *filename )
{
	int ret;

	FB_PROFILE_LOCK();

	if( fb_profiler && filename ) {
		ret = hProfileCopyFilename( fb_profiler->filename, filename, PROFILER_MAX_PATH );
	} else {
		ret = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	FB_PROFILE_UNLOCK();

	return ret;
}

/*:::::*/
FBCALL int fb_ProfileGetFileName( char *filename, int length )
{
	int ret;

	FB_PROFILE_LOCK();

	if( fb_profiler && filename && (length > 0) ) {
		char buffer[PROFILER_MAX_PATH];
			const char *fname;
			if( fb_profiler->filename[0] ) {
				fname = fb_profiler->filename;
			} else {
				fname = fb_hGetExeName( buffer, PROFILER_MAX_PATH-1 );
				if( fname ) {
					size_t exe_len = strlen( buffer );
					/* Include the extension's terminator in the capacity check. */
					if( exe_len <= sizeof( buffer ) - sizeof( DEFAULT_PROFILE_EXT ) ) {
						memcpy( buffer + exe_len, DEFAULT_PROFILE_EXT, sizeof( DEFAULT_PROFILE_EXT ) );
						fname = buffer;
					} else {
						fname = DEFAULT_PROFILE_FILE;
					}
				} else {
					fname = DEFAULT_PROFILE_FILE;
				}
			}
		ret = hProfileCopyFilename( filename, fname, length );
	} else {
		ret = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	FB_PROFILE_UNLOCK();

	return ret;
}

/*:::::*/
FBCALL int fb_ProfileGetOptions( void )
{
	int options = 0;

	FB_PROFILE_LOCK();

	if( fb_profiler ) {
		options = fb_profiler->options;
	}

	FB_PROFILE_UNLOCK();

	return options;
}

/*:::::*/
FBCALL int fb_ProfileSetOptions( int options )
{
	int previous_options = 0;

	FB_PROFILE_LOCK();

	if( fb_profiler ) {
		previous_options = fb_profiler->options;
		fb_profiler->options = options;
	}

	FB_PROFILE_UNLOCK();

	return previous_options;
}

/*:::::*/
FBCALL void fb_ProfileIgnore( const char * procname )
{
	FB_PROFILE_LOCK();

	if( fb_profiler && procname ) {
		unsigned int hashkey;
		hashkey = fb_ProfileHashName( procname );
		PROFILER_add_ignore( fb_profiler, procname, hashkey );
	}

	FB_PROFILE_UNLOCK();
}

/*:::::*/
FBCALL FB_PROFILER_GLOBAL *fb_ProfileGetGlobalProfiler( void )
{
	return fb_profiler;
}

/* end of profiling/profile.c */
