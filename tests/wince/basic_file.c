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
int32 fb_FileOpen( const FBSTRING*, const uint32, const uint32, const uint32, const int32, const int32 );
int32 fb_FileClose( const int32 );
int32 fb_FileFree( void );
void fb_PrintString( const int32, const FBSTRING*, const int32 );
FBSTRING* fb_StrAllocTempDescZEx( const char*, const int32 );
void fb_End( const int32 );
static void fb_ctor__basic_file( void ) __attribute__(( constructor ));

__attribute__(( constructor )) static void fb_ctor__basic_file( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	int32 TMP$3$0;
	label$0:;
	int32 OUTPUT_FILE$0;
	int32 vr$0 = fb_FileFree(  );
	OUTPUT_FILE$0 = vr$0;
	FBSTRING* vr$1 = fb_StrAllocTempDescZEx( (char*)"\x5CStorage Card\x5C" "fb-wince-smoke.txt", 32 );
	int32 vr$2 = fb_FileOpen( (FBSTRING*)vr$1, 3u, 0u, 0u, OUTPUT_FILE$0, 0 );
	if( vr$2 == 0) goto label$3;
	{
		fb_End( 1 );
		label$3:;
	}
	TMP$3$0 = OUTPUT_FILE$0;
	FBSTRING* vr$3 = fb_StrAllocTempDescZEx( (char*)"FreeBASIC Windows CE runtime smoke passed", 41 );
	fb_PrintString( TMP$3$0, (FBSTRING*)vr$3, 1 );
	fb_FileClose( OUTPUT_FILE$0 );
	fb_End( 0 );
	label$1:;
}
