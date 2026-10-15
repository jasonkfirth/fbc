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
void fb_PrintLongint( const int32, const int64, const int32 );
void fb_PrintDouble( const int32, const double, const int32 );
void fb_PrintString( const int32, const FBSTRING*, const int32 );
void fb_StrDelete( const FBSTRING* );
FBSTRING* fb_StrConcatAssign( void*, const int64, const void*, const int64, const int32 );
FBSTRING* fb_StrAllocTempDescZEx( const char*, const int64 );
FBSTRING* fb_StrMid( const FBSTRING*, const int64, const int64 );
void fb_Sleep( const int32 );
static void fb_ctor__macro( void ) __attribute__(( constructor ));

__attribute__(( constructor )) static void fb_ctor__macro( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$0:;
	fb_PrintLongint( 0, 2ll, 1 );
	fb_PrintLongint( 0, 3ll, 1 );
	fb_PrintDouble( 0, 0x1.2p+5, 1 );
	FBSTRING S$0;
	__builtin_memset( &S$0, 0, 24ll );
	{
		int64 I$1;
		I$1 = 9ll;
		label$5:;
		{
			FBSTRING* vr$1 = fb_StrAllocTempDescZEx( (char*)"CISABeerF", 9ll );
			FBSTRING* vr$2 = fb_StrMid( (FBSTRING*)vr$1, I$1, 1ll );
			fb_StrConcatAssign( (void*)&S$0, -1ll, (void*)vr$2, -1ll, 0 );
		}
		label$3:;
		I$1 = I$1 + -1ll;
		label$2:;
		if( I$1 >= 1ll) goto label$5;
		label$4:;
	}
	fb_PrintString( 0, (FBSTRING*)&S$0, 1 );
	fb_Sleep( -1 );
	fb_StrDelete( (FBSTRING*)&S$0 );
	label$1:;
}
