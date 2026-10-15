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
struct __attribute__((gcc_struct)) $N2FB11_OLD_HEADERE {
	uint8 __fb_struct_body[4];
};
#define __FB_STATIC_ASSERT( expr ) extern int __$fb_structsizecheck[(expr) ? 1 : -1]
__FB_STATIC_ASSERT( sizeof( struct $N2FB11_OLD_HEADERE ) == 4 );
struct __attribute__((gcc_struct)) $N2FB5IMAGEE {
	union {
		struct $N2FB11_OLD_HEADERE OLD;
		uint32 TYPE __attribute__((packed, aligned(1)));
	};
	int32 BPP __attribute__((packed, aligned(1)));
	uint32 WIDTH __attribute__((packed, aligned(1)));
	uint32 HEIGHT __attribute__((packed, aligned(1)));
	uint32 PITCH __attribute__((packed, aligned(1)));
	uint8 _RESERVED[12];
};
__FB_STATIC_ASSERT( sizeof( struct $N2FB5IMAGEE ) == 32 );
void fb_GfxPset( void*, const float, const float, const uint32, const int32, const int32 );
int32 fb_GfxPut( void*, const float, const float, const void*, const int32, const int32, const int32, const int32, const int32, const int32, void*, const int32, void*, void* );
int32 fb_GfxScreenRes( const int32, const int32, const int32, const int32, const int32, const int32 );
void fb_GfxSetWindowTitle( const FBSTRING* );
void* fb_GfxImageCreate( const int32, const int32, const uint32, const int32, const int32 );
void fb_GfxImageDestroy( const void* );
int32 fb_GfxImageInfo32( const void*, int32*, int32*, int32*, int32*, void**, int32* );
void fb_GfxImageConvertRow( const void*, const int32, void*, const int32, const int32, const int32 );
void fb_hPutXor( const uint8*, uint8*, const int32, const int32, const int32, const int32, const int32, void*, void* );
void fb_PrintString( const int32, const FBSTRING*, const int32 );
FBSTRING* fb_StrAllocTempDescZEx( const char*, const int64 );
void fb_End( const int32 );
void fb_Sleep( const int32 );
static void fb_ctor__imageconvertrow( void ) __attribute__(( constructor ));
void RELEASEIMAGES( struct $N2FB5IMAGEE**, struct $N2FB5IMAGEE** );

void RELEASEIMAGES( struct $N2FB5IMAGEE** IMAGE8$1, struct $N2FB5IMAGEE** IMAGE32$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$4:;
	if( __extension__ ({ struct $N2FB5IMAGEE* __fb_unaligned; __builtin_memcpy( &__fb_unaligned, (void const*)(IMAGE8$1), sizeof( __fb_unaligned ) ); __fb_unaligned; }) == (struct $N2FB5IMAGEE*)0ull) goto label$7;
	{
		fb_GfxImageDestroy( __extension__ ({ void* __fb_unaligned; __builtin_memcpy( &__fb_unaligned, (void const*)(IMAGE8$1), sizeof( __fb_unaligned ) ); __fb_unaligned; }) );
		__extension__ ({ struct $N2FB5IMAGEE* __fb_unaligned = (struct $N2FB5IMAGEE*)0ull; __builtin_memcpy( (void*)(IMAGE8$1), &__fb_unaligned, sizeof( __fb_unaligned ) ); });
	}
	label$7:;
	label$6:;
	if( __extension__ ({ struct $N2FB5IMAGEE* __fb_unaligned; __builtin_memcpy( &__fb_unaligned, (void const*)(IMAGE32$1), sizeof( __fb_unaligned ) ); __fb_unaligned; }) == (struct $N2FB5IMAGEE*)0ull) goto label$9;
	{
		fb_GfxImageDestroy( __extension__ ({ void* __fb_unaligned; __builtin_memcpy( &__fb_unaligned, (void const*)(IMAGE32$1), sizeof( __fb_unaligned ) ); __fb_unaligned; }) );
		__extension__ ({ struct $N2FB5IMAGEE* __fb_unaligned = (struct $N2FB5IMAGEE*)0ull; __builtin_memcpy( (void*)(IMAGE32$1), &__fb_unaligned, sizeof( __fb_unaligned ) ); });
	}
	label$9:;
	label$8:;
	label$5:;
}

__attribute__(( constructor )) static void fb_ctor__imageconvertrow( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	int32 TMP$15$0;
	int32 TMP$16$0;
	int32 TMP$17$0;
	int32 TMP$18$0;
	int32 TMP$19$0;
	int32 TMP$20$0;
	int32 TMP$21$0;
	int32 TMP$22$0;
	label$0:;
	struct $N2FB5IMAGEE* IMG8$0;
	IMG8$0 = (struct $N2FB5IMAGEE*)0ull;
	struct $N2FB5IMAGEE* IMG32$0;
	IMG32$0 = (struct $N2FB5IMAGEE*)0ull;
	int64 X$0;
	__builtin_memset( &X$0, 0, 8ll );
	int64 Y$0;
	__builtin_memset( &Y$0, 0, 8ll );
	int32 vr$2 = fb_GfxScreenRes( 1, 1, 32, 1, -1, 0 );
	if( (int64)vr$2 == 0ll) goto label$11;
	{
		FBSTRING* vr$4 = fb_StrAllocTempDescZEx( (char*)"Could not set the requested graphics mode", 41ll );
		fb_PrintString( 0, (FBSTRING*)vr$4, 1 );
		fb_Sleep( -1 );
		fb_End( 1 );
	}
	label$11:;
	label$10:;
	void* vr$5 = fb_GfxImageCreate( 64, 64, 0u, 0, (int32)-2147483648u );
	IMG32$0 = (struct $N2FB5IMAGEE*)vr$5;
	if( IMG32$0 != (struct $N2FB5IMAGEE*)0ull) goto label$13;
	{
		FBSTRING* vr$6 = fb_StrAllocTempDescZEx( (char*)"ImageCreate failed on img32!", 28ll );
		fb_PrintString( 0, (FBSTRING*)vr$6, 1 );
		fb_Sleep( -1 );
		fb_End( 1 );
	}
	label$13:;
	label$12:;
	int32 vr$7 = fb_GfxScreenRes( 1, 1, 8, 1, -1, 0 );
	if( (int64)vr$7 == 0ll) goto label$15;
	{
		RELEASEIMAGES( &IMG8$0, &IMG32$0 );
		FBSTRING* vr$11 = fb_StrAllocTempDescZEx( (char*)"Could not set the requested graphics mode", 41ll );
		fb_PrintString( 0, (FBSTRING*)vr$11, 1 );
		fb_Sleep( -1 );
		fb_End( 1 );
	}
	label$15:;
	label$14:;
	void* vr$12 = fb_GfxImageCreate( 64, 64, 0u, 0, (int32)-2147483648u );
	IMG8$0 = (struct $N2FB5IMAGEE*)vr$12;
	if( IMG8$0 != (struct $N2FB5IMAGEE*)0ull) goto label$17;
	{
		RELEASEIMAGES( &IMG8$0, &IMG32$0 );
		FBSTRING* vr$15 = fb_StrAllocTempDescZEx( (char*)"ImageCreate failed on img8!", 27ll );
		fb_PrintString( 0, (FBSTRING*)vr$15, 1 );
		fb_Sleep( -1 );
		fb_End( 1 );
	}
	label$17:;
	label$16:;
	{
		Y$0 = 0ll;
		label$21:;
		{
			{
				X$0 = 0ll;
				label$25:;
				{
					fb_GfxPset( (void*)IMG8$0, (float)X$0, (float)Y$0, (uint32)(((X$0 + Y$0) % 24ll) + 56ll), 4, 0 );
				}
				label$23:;
				X$0 = X$0 + 1ll;
				label$22:;
				if( X$0 <= 63ll) goto label$25;
				label$24:;
			}
		}
		label$19:;
		Y$0 = Y$0 + 1ll;
		label$18:;
		if( Y$0 <= 63ll) goto label$21;
		label$20:;
	}
	int32 vr$24 = fb_GfxScreenRes( 320, 200, 8, 1, 0, 0 );
	if( (int64)vr$24 == 0ll) goto label$27;
	{
		RELEASEIMAGES( &IMG8$0, &IMG32$0 );
		FBSTRING* vr$28 = fb_StrAllocTempDescZEx( (char*)"Could not set the requested graphics mode", 41ll );
		fb_PrintString( 0, (FBSTRING*)vr$28, 1 );
		fb_Sleep( -1 );
		fb_End( 1 );
	}
	label$27:;
	label$26:;
	FBSTRING* vr$29 = fb_StrAllocTempDescZEx( (char*)"8-bit color mode", 16ll );
	fb_GfxSetWindowTitle( (FBSTRING*)vr$29 );
	fb_GfxPut( (void*)0ull, 0x1.4p+3f, 0x1.4p+3f, (void*)IMG8$0, -65536, -65536, -65536, -65536, 0, 5, (void*)&fb_hPutXor, -1, (void*)0ull, (void*)0ull );
	fb_Sleep( -1 );
	int8* P8$0;
	__builtin_memset( &P8$0, 0, 8ll );
	int8* P32$0;
	__builtin_memset( &P32$0, 0, 8ll );
	int32 PITCH8$0;
	__builtin_memset( &PITCH8$0, 0, 4ll );
	int32 PITCH32$0;
	__builtin_memset( &PITCH32$0, 0, 4ll );
	TMP$18$0 = 0;
	TMP$17$0 = 0;
	TMP$16$0 = 0;
	TMP$15$0 = 0;
	fb_GfxImageInfo32( (void*)IMG8$0, &TMP$15$0, &TMP$16$0, &TMP$17$0, &PITCH8$0, (void**)&P8$0, &TMP$18$0 );
	TMP$22$0 = 0;
	TMP$21$0 = 0;
	TMP$20$0 = 0;
	TMP$19$0 = 0;
	fb_GfxImageInfo32( (void*)IMG32$0, &TMP$19$0, &TMP$20$0, &TMP$21$0, &PITCH32$0, (void**)&P32$0, &TMP$22$0 );
	{
		Y$0 = 0ll;
		label$31:;
		{
			fb_GfxImageConvertRow( (void*)((uint8*)P8$0 + (Y$0 * (int64)PITCH8$0)), 8, (void*)((uint8*)P32$0 + (Y$0 * (int64)PITCH32$0)), 32, 64, 1 );
		}
		label$29:;
		Y$0 = Y$0 + 1ll;
		label$28:;
		if( Y$0 <= 63ll) goto label$31;
		label$30:;
	}
	int32 vr$53 = fb_GfxScreenRes( 320, 200, 32, 1, 0, 0 );
	if( (int64)vr$53 == 0ll) goto label$33;
	{
		RELEASEIMAGES( &IMG8$0, &IMG32$0 );
		FBSTRING* vr$57 = fb_StrAllocTempDescZEx( (char*)"Could not set the requested graphics mode", 41ll );
		fb_PrintString( 0, (FBSTRING*)vr$57, 1 );
		fb_Sleep( -1 );
		fb_End( 1 );
	}
	label$33:;
	label$32:;
	FBSTRING* vr$58 = fb_StrAllocTempDescZEx( (char*)"32-bit color mode", 17ll );
	fb_GfxSetWindowTitle( (FBSTRING*)vr$58 );
	fb_GfxPut( (void*)0ull, 0x1.4p+3f, 0x1.4p+3f, (void*)IMG32$0, -65536, -65536, -65536, -65536, 0, 5, (void*)&fb_hPutXor, -1, (void*)0ull, (void*)0ull );
	fb_Sleep( -1 );
	RELEASEIMAGES( &IMG8$0, &IMG32$0 );
	label$1:;
}

static const char __attribute__((used, section(".fbctinf"))) __fbctinf[] = "-gfx";
