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
void fb_Assert( const char*, const int32, const char*, const char* );
FBSTRING* fb_StrAssign( void*, const int32, const void*, const int32, const int32 );
void fb_StrDelete( const FBSTRING* );
int32 fb_StrCompare( const void*, const int32, const void*, const int32 );
int32 fb_StrLen( const void*, const int32 );
void fb_End( const int32 );
static void fb_ctor__commandzsweep( void ) __attribute__(( constructor ));
struct $7INNER_T {
	FBSTRING MEMBER;
};
#define __FB_STATIC_ASSERT( expr ) extern int __$fb_structsizecheck[(expr) ? 1 : -1]
__FB_STATIC_ASSERT( sizeof( struct $7INNER_T ) == 12 );
static void _ZN7INNER_TC1Ev( struct $7INNER_T* );
static void _ZN7INNER_TaSERKS_( struct $7INNER_T*, const struct $7INNER_T* );
static void _ZN7INNER_TD1Ev( struct $7INNER_T* );
struct $7OUTER_T {
	struct $7INNER_T INNER;
	int32 VALUE;
};
__FB_STATIC_ASSERT( sizeof( struct $7OUTER_T ) == 16 );
static void _ZN7OUTER_TC1Ev( struct $7OUTER_T* );
static void _ZN7OUTER_TaSERKS_( struct $7OUTER_T*, const struct $7OUTER_T* );
struct $N8SWEEP_NS5BOX_TE {
	struct $7INNER_T INNER;
};
__FB_STATIC_ASSERT( sizeof( struct $N8SWEEP_NS5BOX_TE ) == 12 );
static void _ZN8SWEEP_NS5BOX_TaSERKS0_( struct $N8SWEEP_NS5BOX_TE*, const struct $N8SWEEP_NS5BOX_TE* );
void _ZN8SWEEP_NS7SET_BOXER8FBSTRING( FBSTRING* );
static struct $N8SWEEP_NS5BOX_TE _ZN8SWEEP_NS4BOX$E;

void _ZN8SWEEP_NS7SET_BOXER8FBSTRING( FBSTRING* TEXT$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$26:;
	fb_StrAssign( (void*)&_ZN8SWEEP_NS4BOX$E, -1, (void*)TEXT$1, -1, 0 );
	label$27:;
}
