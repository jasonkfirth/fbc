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
void fb_End( const int32 );
static void fb_ctor__compile_and_run_ok( void ) __attribute__(( constructor ));

__attribute__(( constructor )) static void fb_ctor__compile_and_run_ok( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$0:;
	fb_End( 0 );
	label$1:;
}
