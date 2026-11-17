/*
 * Project: FreeBASIC console qualification
 * File: thread-probe.c
 * Purpose: Check native Xbox thread fan-out, sleep and shared stdio behavior.
 * Responsibilities: Own bounded thread handles and retain native API results.
 * This file does not call FreeBASIC's thread or sleep adapters.
 */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>

#define THREAD_COUNT 100
#define ROUND_COUNT 4

/* Set before starting workers, then retained until every worker has joined. */
static FILE *worker_output;
static LONG write_output;
static volatile LONG completed;

static DWORD WINAPI worker( void *argument )
{
	unsigned int index = (unsigned int)(uintptr_t)argument;
	Sleep( 1 + (index * 17) % 99 );
	if( write_output )
		fprintf( worker_output, "ending:%u\n", index );
	InterlockedIncrement( &completed );
	return 0;
}

int fbqual_native_threads( void )
{
	HANDLE threads[THREAD_COUNT];
	FILE *log;
	unsigned int mode, round, created, index;
	DWORD started;
	int result = 0;

	log = fopen( "E:\\FBQNTHR.TXT", "wb" );
	worker_output = fopen( "E:\\FBQNWORK.TXT", "wb" );
	if( log == NULL || worker_output == NULL ) {
		if( log != NULL ) fclose( log );
		if( worker_output != NULL ) fclose( worker_output );
		return 81;
	}
	setvbuf( worker_output, NULL, _IONBF, 0 );
	for( mode = 0; mode < 2 && result == 0; ++mode ) {
		write_output = mode;
		for( round = 0; round < ROUND_COUNT && result == 0; ++round ) {
			completed = 0;
			started = GetTickCount();
			fprintf( log, "start mode=%u round=%u tick=%lu\n", mode, round, started );
			fflush( log );
			for( created = 0; created < THREAD_COUNT; ++created ) {
				threads[created] = CreateThread( NULL, 128 * 1024, worker,
					(void *)(uintptr_t)created, 0, NULL );
				if( threads[created] == NULL ) {
					result = 82;
					break;
				}
				if( (created + 1) % 25 == 0 ) {
					fprintf( log, "created=%u elapsed_ms=%lu\n", created + 1, GetTickCount() - started );
					fflush( log );
				}
			}
			for( index = 0; index < created; ++index ) {
				DWORD status = WaitForSingleObject( threads[index], 5000 );
				if( status != WAIT_OBJECT_0 ) {
					fprintf( log, "wait index=%u status=%lu\n", index, status );
					result = 83;
					for( ; index < created; ++index )
						CloseHandle( threads[index] );
					break;
				}
				CloseHandle( threads[index] );
			}
			fprintf( log, "done created=%u completed=%ld result=%d elapsed_ms=%lu\n", created, completed, result, GetTickCount() - started );
			fflush( log );
			if( completed != THREAD_COUNT && result == 0 )
				result = 84;
		}
	}
	/* On a native wait failure, a worker may still own the output stream.
	   The controller will retain diagnostics and terminate the guest title. */
	if( result == 0 && fclose(worker_output) != 0 )
		result = 85;
	if( fclose(log) != 0 && result == 0 )
		result = 86;
	return result;
}

/* end of thread-probe.c */
