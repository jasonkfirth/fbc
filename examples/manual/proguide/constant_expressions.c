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
struct $2PT {
	int64 X;
	int64 Y;
};
#define __FB_STATIC_ASSERT( expr ) extern int __$fb_structsizecheck[(expr) ? 1 : -1]
__FB_STATIC_ASSERT( sizeof( struct $2PT ) == 16 );
void fb_PrintLongint( const int32, const int64, const int32 );
void fb_PrintDouble( const int32, const double, const int32 );
void fb_Sleep( const int32 );
static void fb_ctor__constant_expressions( void ) __attribute__(( constructor ));
static void _ZN2PTC1Ev( struct $2PT* );

__attribute__(( constructor )) static void fb_ctor__constant_expressions( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$0:;
	double D$0;
	D$0 = 0x1.6A09E667F3BCDp+0;
	struct $2PT P$0;
	_ZN2PTC1Ev( &P$0 );
	fb_PrintDouble( 0, 0x1.921FB54442D18p+1, 1 );
	fb_PrintDouble( 0, D$0, 1 );
	fb_PrintLongint( 0, *(int64*)&P$0, 2 );
	fb_PrintLongint( 0, *(int64*)((uint8*)&P$0 + 8ll), 1 );
	fb_Sleep( -1 );
	label$1:;
}

static void _ZN2PTC1Ev( struct $2PT* THIS$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	*(int64*)THIS$1 = 260ll;
	*(int64*)((uint8*)THIS$1 + 8ll) = 150ll;
	label$2:;
	label$3:;
}
