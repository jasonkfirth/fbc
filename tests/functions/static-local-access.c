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
void fb_End( const int32 );
static void fb_ctor__staticzlocalzaccess( void ) __attribute__(( constructor ));
static int64 NEXTVALUE( void );

__attribute__(( constructor )) static void fb_ctor__staticzlocalzaccess( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$0:;
	int64 vr$0 = NEXTVALUE(  );
	if( vr$0 == 1ll) goto label$5;
	{
		fb_End( 1 );
	}
	label$5:;
	label$4:;
	int64 vr$1 = NEXTVALUE(  );
	if( vr$1 == 2ll) goto label$7;
	{
		fb_End( 1 );
	}
	label$7:;
	label$6:;
	fb_End( 0 );
	label$1:;
}

static int64 NEXTVALUE( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$2:;
	static int64 USED_STATIC$1;
	USED_STATIC$1 = USED_STATIC$1 + 1ll;
	fb$result$1 = USED_STATIC$1;
	goto label$3;
	label$3:;
	return fb$result$1;
}
