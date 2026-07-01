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
void fb_PrintString( const int32, const FBSTRING*, const int32 );
void fb_sfxSoundLegacy2( const int32, const int32 );
FBSTRING* fb_StrAllocTempDescZEx( const char*, const int32 );
void fb_End( const int32 );
static void fb_ctor__fbsfx_smoke( void ) __attribute__(( constructor ));

__attribute__(( constructor )) static void fb_ctor__fbsfx_smoke( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$0:;
	FBSTRING* vr$0 = fb_StrAllocTempDescZEx( (char*)"fbsfx: starting", 15 );
	fb_PrintString( 0, (FBSTRING*)vr$0, 1 );
	fb_sfxSoundLegacy2( 440, 2 );
	fb_sfxSoundLegacy2( 660, 2 );
	FBSTRING* vr$1 = fb_StrAllocTempDescZEx( (char*)"FB_NUTTX_SFX_SMOKE_OK", 21 );
	fb_PrintString( 0, (FBSTRING*)vr$1, 1 );
	fb_End( 0 );
	label$1:;
}
