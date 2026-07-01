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
struct $7FBARRAYIvE {
	void* DATA;
	void* PTR;
	int64 SIZE;
	int64 ELEMENT_LEN;
	int64 DIMENSIONS;
	int64 FLAGS;
	struct $16__FB_ARRAYDIMTB$ DIMTB[8];
};
__FB_STATIC_ASSERT( sizeof( struct $7FBARRAYIvE ) == 240 );
typedef void (*tmp$5)( void );
typedef int32 (*tmp$4)( void );
static void fb_ctor__void_param( void ) __attribute__(( constructor ));
void _ZN4FBCU9ADD_SUITEEPcPFivES2_( char*, tmp$4, tmp$4 );
void _ZN4FBCU8ADD_TESTEPcS0_PFvvEb( char*, char*, tmp$5, boolean );
void _ZN4FBCU10CU_ASSERT_EbPciS0_S0_( boolean, char*, int32, char*, char* );
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM10TEST_CONSTERv( int64* );
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM10TEST_BYVALERv( int64* );
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM8TEST_STRERv( char** );
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM16TEST_AFTER_BYREFERu7INTEGER( int64* );
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM17TEST_AFTER_BYDESCER7FBARRAYIvE( struct $7FBARRAYIvE* );
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM7DEFAULTEv( void );
static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM12DEFAULT_CTOREv( void ) __attribute__(( constructor ));
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM16AFTER_DEFINITIONEv( void );
static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM21AFTER_DEFINITION_CTOREv( void ) __attribute__(( constructor ));
static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM12SUITE_CTOR54Ev( void ) __attribute__(( constructor ));

void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM7DEFAULTEv( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$2:;
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM10TEST_CONSTERv( (void*)1234ll );
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM10TEST_CONSTERv( (void*)1234ll );
	int64 I$1;
	I$1 = 5678ll;
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM10TEST_BYVALERv( (void*)I$1 );
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM8TEST_STRERv( (void*)"abcd" );
	label$3:;
}

void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM10TEST_CONSTERv( int64* P$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$6:;
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(P$1 == (int64*)1234ull) != 0ll), (char*)"tests\x5C" "functions\x5Cvoid_param.bas", 24, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VOID_PARAM.TEST_CONST", (char*)"CU_ASSERT_EQUAL(@p,1234)" );
	label$7:;
}

void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM10TEST_BYVALERv( int64* P$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$8:;
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(P$1 == (int64*)5678ull) != 0ll), (char*)"tests\x5C" "functions\x5Cvoid_param.bas", 28, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VOID_PARAM.TEST_BYVAL", (char*)"CU_ASSERT_EQUAL(@p,5678)" );
	label$9:;
}

void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM8TEST_STRERv( char** P$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$10:;
	char* EXPECTED$1;
	EXPECTED$1 = (char*)"abcd";
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((uint64)P$1 == (uint64)EXPECTED$1) != 0ll), (char*)"tests\x5C" "functions\x5Cvoid_param.bas", 33, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VOID_PARAM.TEST_STR", (char*)"CU_ASSERT_EQUAL(@p,expected)" );
	label$11:;
}

void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM16TEST_AFTER_BYREFERu7INTEGER( int64* P$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$12:;
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)1ll, (char*)"tests\x5C" "functions\x5Cvoid_param.bas", 37, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VOID_PARAM.TEST_AFTER_BYREF", (char*)"CU_ASSERT_EQUAL(sizeof( p ),sizeof( integer ))" );
	label$13:;
}

void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM17TEST_AFTER_BYDESCER7FBARRAYIvE( struct $7FBARRAYIvE* P$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$14:;
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)1ll, (char*)"tests\x5C" "functions\x5Cvoid_param.bas", 41, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VOID_PARAM.TEST_AFTER_BYDESC", (char*)"CU_ASSERT_EQUAL(sizeof( p(0) ),sizeof( integer ))" );
	label$15:;
}

void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM16AFTER_DEFINITIONEv( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$16:;
	float S$1;
	S$1 = 0x1.p+0f;
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM16TEST_AFTER_BYREFERu7INTEGER( (int64*)&S$1 );
	float A$1[2];
	struct $N5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM10FBARRAY1$1IfEE {
		float* DATA;
		float* PTR;
		int64 SIZE;
		int64 ELEMENT_LEN;
		int64 DIMENSIONS;
		int64 FLAGS;
		struct $16__FB_ARRAYDIMTB$ DIMTB[1];
	};
	__FB_STATIC_ASSERT( sizeof( struct $N5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM10FBARRAY1$1IfEE ) == 72 );
	struct $N5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM10FBARRAY1$1IfEE tmp$43$1;
	*(float**)&tmp$43$1 = (float*)A$1;
	*(float**)((uint8*)&tmp$43$1 + 8ll) = (float*)A$1;
	*(int64*)((uint8*)&tmp$43$1 + 16ll) = 8ll;
	*(int64*)((uint8*)&tmp$43$1 + 24ll) = 4ll;
	*(int64*)((uint8*)&tmp$43$1 + 32ll) = 1ll;
	*(int64*)((uint8*)&tmp$43$1 + 40ll) = 49ll;
	*(int64*)((uint8*)&tmp$43$1 + 48ll) = 2ll;
	*(int64*)((uint8*)&tmp$43$1 + 56ll) = 0ll;
	*(int64*)((uint8*)&tmp$43$1 + 64ll) = 1ll;
	*(float*)A$1 = 0x1.p+0f;
	*(float*)((uint64)(float*)A$1 + 4ll) = 0x1.p+1f;
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM17TEST_AFTER_BYDESCER7FBARRAYIvE( (struct $7FBARRAYIvE*)&tmp$43$1 );
	label$17:;
}

__attribute__(( constructor )) static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM12DEFAULT_CTOREv( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$4:;
	_ZN4FBCU8ADD_TESTEPcS0_PFvvEb( (char*)"fbc_tests.functions.void_param", (char*)"default", (tmp$5)&_ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM7DEFAULTEv, (boolean)0ll );
	label$5:;
}

__attribute__(( constructor )) static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM21AFTER_DEFINITION_CTOREv( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$18:;
	_ZN4FBCU8ADD_TESTEPcS0_PFvvEb( (char*)"fbc_tests.functions.void_param", (char*)"after_definition", (tmp$5)&_ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM16AFTER_DEFINITIONEv, (boolean)0ll );
	label$19:;
}

__attribute__(( constructor )) static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VOID_PARAM12SUITE_CTOR54Ev( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$20:;
	_ZN4FBCU9ADD_SUITEEPcPFivES2_( (char*)"fbc_tests.functions.void_param", (tmp$4)0ull, (tmp$4)0ull );
	label$21:;
}

static const char __attribute__((used, section(".fbctinf"))) __fbctinf[] = "-l\0fbcunit";
