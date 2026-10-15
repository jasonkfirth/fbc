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
struct $16__FB_ARRAYDIMTB$ {
	int64 ELEMENTS;
	int64 LBOUND;
	int64 UBOUND;
};
#define __FB_STATIC_ASSERT( expr ) extern int __$fb_structsizecheck[(expr) ? 1 : -1]
__FB_STATIC_ASSERT( sizeof( struct $16__FB_ARRAYDIMTB$ ) == 24 );
struct $8FBARRAY1IiE {
	int32* DATA;
	int32* PTR;
	int64 SIZE;
	int64 ELEMENT_LEN;
	int64 DIMENSIONS;
	int64 FLAGS;
	struct $16__FB_ARRAYDIMTB$ DIMTB[1];
};
__FB_STATIC_ASSERT( sizeof( struct $8FBARRAY1IiE ) == 72 );
#define fb_D2I( value ) ((int32)__builtin_nearbyint( value ))
static void fb_ctor__compoundzexpressionzinputs( void ) __attribute__(( constructor ));
int64 READINDEX( void );

int64 READINDEX( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$2:;
	fb$result$1 = 0ll;
	goto label$3;
	label$3:;
	return fb$result$1;
}

void REVIEW( int32 INPUTVALUE$1, double REALVALUE$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	int32* TMP$3$1;
	label$4:;
	int32 RESULTVALUE$1;
	__builtin_memset( &RESULTVALUE$1, 0, 4ll );
	int32 VALUES$1[2];
	__builtin_memset( (int32*)VALUES$1, 0, 8ll );
	struct $8FBARRAY1IiE tmp$2$1;
	*(int32**)&tmp$2$1 = (int32*)VALUES$1;
	*(int32**)((uint8*)&tmp$2$1 + 8ll) = (int32*)VALUES$1;
	*(int64*)((uint8*)&tmp$2$1 + 16ll) = 8ll;
	*(int64*)((uint8*)&tmp$2$1 + 24ll) = 4ll;
	*(int64*)((uint8*)&tmp$2$1 + 32ll) = 1ll;
	*(int64*)((uint8*)&tmp$2$1 + 40ll) = 49ll;
	*(int64*)((uint8*)&tmp$2$1 + 48ll) = 2ll;
	*(int64*)((uint8*)&tmp$2$1 + 56ll) = 0ll;
	*(int64*)((uint8*)&tmp$2$1 + 64ll) = 1ll;
	RESULTVALUE$1 = (int32)(int64)RESULTVALUE$1;
	RESULTVALUE$1 = 0;
	RESULTVALUE$1 = (int32)((int64)RESULTVALUE$1 + (int64)INPUTVALUE$1);
	RESULTVALUE$1 = fb_D2I( (double)(int64)RESULTVALUE$1 * REALVALUE$1 );
	int64 vr$14 = READINDEX(  );
	TMP$3$1 = (int32*)((uint64)(int32*)VALUES$1 + (vr$14 << (2ll & 63ll)));
	*TMP$3$1 = 0;
	label$5:;
}
