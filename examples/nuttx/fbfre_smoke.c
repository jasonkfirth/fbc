typedef   signed char       int8;
typedef unsigned char      uint8;
typedef   signed short      int16;
typedef unsigned short     uint16;
typedef   signed int        int32;
typedef unsigned int       uint32;
typedef   signed long long  int64;
typedef unsigned long long uint64;
typedef struct { char *data; int32 len; int32 size; } FBSTRING;
typedef int8 boolean;
uint32 fb_GetMemAvail( const int32 );
void* calloc( uint32, uint32 );
void free( void* );
void fb_PrintUByte( const int32, const uint8, const int32 );
void fb_PrintInt( const int32, const int32, const int32 );
void fb_PrintString( const int32, const FBSTRING*, const int32 );
FBSTRING* fb_StrAllocTempDescZEx( const char*, const int32 );
void fb_End( const int32 );
static void fb_ctor__fbfre_smoke( void ) __attribute__(( constructor ));

__attribute__(( constructor )) static void fb_ctor__fbfre_smoke( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	int32 TMP$3$0;
	label$0:;
	uint32 FREE_BEFORE$0;
	__builtin_memset( &FREE_BEFORE$0, 0, 4 );
	uint8* PROBE$0;
	__builtin_memset( &PROBE$0, 0, 4 );
	uint32 vr$2 = fb_GetMemAvail( 0 );
	FREE_BEFORE$0 = vr$2;
	FBSTRING* vr$3 = fb_StrAllocTempDescZEx( (char*)"fre initial nonzero =", 21 );
	fb_PrintString( 0, (FBSTRING*)vr$3, 0 );
	if( FREE_BEFORE$0 <= 0u) goto label$2;
	TMP$3$0 = 1;
	goto label$5;
	label$2:;
	TMP$3$0 = 0;
	label$5:;
	fb_PrintInt( 0, TMP$3$0, 1 );
	void* vr$4 = calloc( 256u, 1u );
	PROBE$0 = (uint8*)vr$4;
	if( PROBE$0 != (uint8*)0u) goto label$4;
	{
		FBSTRING* vr$5 = fb_StrAllocTempDescZEx( (char*)"fre allocation failed", 21 );
		fb_PrintString( 0, (FBSTRING*)vr$5, 1 );
		fb_End( 1 );
	}
	label$4:;
	label$3:;
	*PROBE$0 = (uint8)123u;
	FBSTRING* vr$7 = fb_StrAllocTempDescZEx( (char*)"fre allocation sample =", 23 );
	fb_PrintString( 0, (FBSTRING*)vr$7, 0 );
	fb_PrintUByte( 0, *PROBE$0, 1 );
	free( (void*)PROBE$0 );
	FBSTRING* vr$9 = fb_StrAllocTempDescZEx( (char*)"FB_NUTTX_FRE_SMOKE_OK", 21 );
	fb_PrintString( 0, (FBSTRING*)vr$9, 1 );
	label$1:;
}
