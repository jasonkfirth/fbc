typedef   signed char       int8;
typedef unsigned char      uint8;
typedef   signed short      int16;
typedef unsigned short     uint16;
typedef   signed int        int32;
typedef unsigned int       uint32;
typedef   signed long long  int64;
typedef unsigned long long uint64;
typedef struct { char *data; int64 len; int64 size; } FBSTRING;
typedef int8 boolean;
void fb_PrintString( const int32, const FBSTRING*, const int32 );
FBSTRING* fb_StrAllocTempDescZEx( const char*, const int64 );
void fb_Init( int32, char**, int32 );
void fb_End( const int32 );
void fb_Sleep( const int32 );
int64 _InterlockedExchangeAdd64( int64*, int64 );
int32 _InterlockedExchangeAdd( int32*, int32 );
uint32 BASSMOD_GetVersion( void );
int32 BASSMOD_Init( int32, uint32, uint32 );
void BASSMOD_Free( void );
int32 BASSMOD_MusicLoad( int32, void*, uint32, uint32, uint32 );
void BASSMOD_MusicFree( void );
int32 BASSMOD_MusicPlay( void );
int32 BASSMOD_MusicStop( void );

int32 main( int32 __FB_ARGC__$0, char** __FB_ARGV__$0 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	int32 fb$result$0;
	__builtin_memset( &fb$result$0, 0, 4ll );
	fb_Init( __FB_ARGC__$0, (char**)__FB_ARGV__$0, 0 );
	label$0:;
	uint32 vr$1 = BASSMOD_GetVersion(  );
	if( (int64)vr$1 >= 2ll) goto label$27;
	{
		FBSTRING* vr$3 = fb_StrAllocTempDescZEx( (char*)"BASSMOD version 2 or above required!", 36ll );
		fb_PrintString( 0, (FBSTRING*)vr$3, 1 );
		fb_End( 1 );
	}
	label$27:;
	label$26:;
	int32 vr$4 = BASSMOD_Init( -1, 44100u, 0u );
	if( (int64)vr$4 != 0ll) goto label$29;
	{
		FBSTRING* vr$6 = fb_StrAllocTempDescZEx( (char*)"Could not initialize BASSMOD", 28ll );
		fb_PrintString( 0, (FBSTRING*)vr$6, 1 );
		fb_End( 1 );
	}
	label$29:;
	label$28:;
	int32 vr$7 = BASSMOD_MusicLoad( 0, (void*)"test.mod", 0u, 0u, 4u );
	if( (int64)vr$7 != 0ll) goto label$31;
	{
		FBSTRING* vr$9 = fb_StrAllocTempDescZEx( (char*)"BASSMOD could not load 'test.mod'", 33ll );
		fb_PrintString( 0, (FBSTRING*)vr$9, 1 );
		BASSMOD_Free(  );
		fb_End( 1 );
	}
	label$31:;
	label$30:;
	BASSMOD_MusicPlay(  );
	FBSTRING* vr$10 = fb_StrAllocTempDescZEx( (char*)"Sound playing; waiting for keypress to stop and exit...", 55ll );
	fb_PrintString( 0, (FBSTRING*)vr$10, 1 );
	fb_Sleep( -1 );
	BASSMOD_MusicStop(  );
	BASSMOD_MusicFree(  );
	BASSMOD_Free(  );
	label$1:;
	fb_End( 0 );
	return fb$result$0;
}
