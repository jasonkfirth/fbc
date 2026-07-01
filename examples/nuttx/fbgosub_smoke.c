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
void fb_PrintInt( const int32, const int32, const int32 );
void fb_PrintString( const int32, const FBSTRING*, const int32 );
FBSTRING* fb_StrAllocTempDescZEx( const char*, const int32 );
void fb_End( const int32 );
void* fb_GosubPush( void** );
int32 fb_GosubReturn( void** );
void fb_GosubExit( void** );
int32 setjmp( void* );
static void fb_ctor__fbgosub_smoke( void ) __attribute__(( constructor ));
static int32 GOSUB_TOTAL$;

__attribute__(( constructor )) static void fb_ctor__fbgosub_smoke( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$0:;
	void* TMP$2$0;
	__builtin_memset( &TMP$2$0, 0, 4 );
	void* vr$2 = fb_GosubPush( &TMP$2$0 );
	int32 vr$3 = setjmp( vr$2 );
	if( vr$3 != 0) goto label$3;
	goto label$2;
	label$3:;
	void* vr$5 = fb_GosubPush( &TMP$2$0 );
	int32 vr$6 = setjmp( vr$5 );
	if( vr$6 != 0) goto label$5;
	goto label$4;
	label$5:;
	if( GOSUB_TOTAL$ != 42) goto label$7;
	{
		FBSTRING* vr$7 = fb_StrAllocTempDescZEx( (char*)"FB_NUTTX_GOSUB_SMOKE_OK", 23 );
		fb_PrintString( 0, (FBSTRING*)vr$7, 1 );
	}
	goto label$6;
	label$7:;
	{
		FBSTRING* vr$8 = fb_StrAllocTempDescZEx( (char*)"FB_NUTTX_GOSUB_SMOKE_BAD", 24 );
		fb_PrintString( 0, (FBSTRING*)vr$8, 0 );
		fb_PrintInt( 0, GOSUB_TOTAL$, 1 );
		fb_End( 1 );
	}
	label$6:;
	fb_End( 0 );
	label$2:;
	GOSUB_TOTAL$ = 17;
	fb_GosubReturn( &TMP$2$0 );
	label$4:;
	GOSUB_TOTAL$ = GOSUB_TOTAL$ + 25;
	fb_GosubReturn( &TMP$2$0 );
	label$1:;
	fb_GosubExit( &TMP$2$0 );
}
