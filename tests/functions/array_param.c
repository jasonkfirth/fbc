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
struct $16__FB_ARRAYDIMTB$ {
	int32 ELEMENTS;
	int32 LBOUND;
	int32 UBOUND;
};
#define __FB_STATIC_ASSERT( expr ) extern int __$fb_structsizecheck[(expr) ? 1 : -1]
__FB_STATIC_ASSERT( sizeof( struct $16__FB_ARRAYDIMTB$ ) == 12 );
struct $7FBARRAYIdE {
	double* DATA;
	double* PTR;
	int32 SIZE;
	int32 ELEMENT_LEN;
	int32 DIMENSIONS;
	int32 FLAGS;
	struct $16__FB_ARRAYDIMTB$ DIMTB[8];
};
__FB_STATIC_ASSERT( sizeof( struct $7FBARRAYIdE ) == 120 );
void fb_PrintDouble( const int32, const double, const int32 );
void fb_Sleep( const int32 );
static void fb_ctor__array_param( void ) __attribute__(( constructor ));
void FUNK( struct $7FBARRAYIdE* );

void FUNK( struct $7FBARRAYIdE* A$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$2:;
	fb_PrintDouble( 0, *(double*)*(int32*)A$1, 1 );
	label$3:;
}
