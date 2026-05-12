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
typedef void (*tmp$5)( void );
typedef int32 (*tmp$4)( void );
uint16* __stdcall fb_WstrAssignFromA( uint16*, int32, void*, int32 ) asm("_fb_WstrAssignFromA@16");
int32 __stdcall fb_WstrCompare( uint16*, uint16* ) asm("_fb_WstrCompare@8");
static void fb_ctor__utf8( void ) __attribute__(( constructor ));
void __stdcall _ZN4FBCU9ADD_SUITEEPcPFivES2_( char*, tmp$4, tmp$4 ) asm("__ZN4FBCU9ADD_SUITEEPcPFivES2_@12");
void __stdcall _ZN4FBCU8ADD_TESTEPcS0_PFvvEb( char*, char*, tmp$5, boolean ) asm("__ZN4FBCU8ADD_TESTEPcS0_PFvvEb@16");
void __stdcall _ZN4FBCU10CU_ASSERT_EbPciS0_S0_( boolean, char*, int32, char*, char* ) asm("__ZN4FBCU10CU_ASSERT_EbPciS0_S0_@20");
void _ZN5TESTS9FBC_TESTS8WSTRING_4UTF87DEFAULTEv( void );
static void _ZN5TESTS9FBC_TESTS8WSTRING_4UTF812DEFAULT_CTOREv( void ) __attribute__(( constructor ));
static void _ZN5TESTS9FBC_TESTS8WSTRING_4UTF812SUITE_CTOR22Ev( void ) __attribute__(( constructor ));

void _ZN5TESTS9FBC_TESTS8WSTRING_4UTF87DEFAULTEv( void )
{
	label$2:;
	uint16 HW1$1[32];
	fb_WstrAssignFromA( (uint16*)HW1$1, 32, (void*)"\xCE\x9A\xCE\xB1\xCE\xBB\xCE\xB7\xCE\xBC\xCE\xAD\xCF\x81\xCE\xB1 \xCE\xBA\xCF\x8C\xCF\x83\xCE\xBC\xCE\xB5!", 29 );
	uint16 HW2$1[32];
	fb_WstrAssignFromA( (uint16*)HW2$1, 32, (void*)"\xCE\x9A\xCE\xB1\xCE\xBB\xCE\xB7\xCE\xBC\xCE\xAD\xCF\x81\xCE\xB1 \xCE\xBA\xCF\x8C\xCF\x83\xCE\xBC\xCE\xB5!", 29 );
	int32 vr$4 = fb_WstrCompare( (uint16*)HW1$1, (uint16*)HW2$1 );
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$4 == 0) != 0), (char*)"tests\x5Cwstring\x5Cutf8.bas", 14, (char*)"TESTS.FBC_TESTS.WSTRING_.UTF8.DEFAULT", (char*)"CU_ASSERT(hw1 = hw2)" );
	int32 vr$8 = fb_WstrCompare( (uint16*)HW1$1, (uint16*)L"\x00CE" L"\x0161" L"\x00CE" L"\x00B1" L"\x00CE" L"\x00BB" L"\x00CE" L"\x00B7" L"\x00CE" L"\x00BC" L"\x00CE" L"\x00AD" L"\x00CF" L"\x0081" L"\x00CE" L"\x00B1" L" \x00CE" L"\x00BA" L"\x00CF" L"\x0152" L"\x00CF" L"\x0192" L"\x00CE" L"\x00BC" L"\x00CE" L"\x00B5" L"!" );
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$8 == 0) != 0), (char*)"tests\x5Cwstring\x5Cutf8.bas", 16, (char*)"TESTS.FBC_TESTS.WSTRING_.UTF8.DEFAULT", (char*)"CU_ASSERT(hw1 = \x22\xCE\x9A\xCE\xB1\xCE\xBB\xCE\xB7\xCE\xBC\xCE\xAD\xCF\x81\xCE\xB1 \x22 + \x22\xCE\xBA\xCF\x8C\xCF\x83\xCE\xBC\xCE\xB5!\x22)" );
	int32 vr$12 = fb_WstrCompare( (uint16*)L"\x00CE" L"\x0161" L"\x00CE" L"\x00B1" L"\x00CE" L"\x00BB" L"\x00CE" L"\x00B7" L"\x00CE" L"\x00BC" L"\x00CE" L"\x00AD" L"\x00CF" L"\x0081" L"\x00CE" L"\x00B1" L" \x00CE" L"\x00BA" L"\x00CF" L"\x0152" L"\x00CF" L"\x0192" L"\x00CE" L"\x00BC" L"\x00CE" L"\x00B5" L"!", (uint16*)HW2$1 );
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$12 == 0) != 0), (char*)"tests\x5Cwstring\x5Cutf8.bas", 18, (char*)"TESTS.FBC_TESTS.WSTRING_.UTF8.DEFAULT", (char*)"CU_ASSERT(\x22\xCE\x9A\xCE\xB1\xCE\xBB\xCE\xB7\xCE\xBC\xCE\xAD\xCF\x81\xCE\xB1 \x22 + \x22\xCE\xBA\xCF\x8C\xCF\x83\xCE\xBC\xCE\xB5!\x22 = hw2)" );
	label$3:;
}

__attribute__(( constructor )) static void _ZN5TESTS9FBC_TESTS8WSTRING_4UTF812DEFAULT_CTOREv( void )
{
	label$4:;
	_ZN4FBCU8ADD_TESTEPcS0_PFvvEb( (char*)"fbc_tests.wstring_.utf8", (char*)"default", (tmp$5)&_ZN5TESTS9FBC_TESTS8WSTRING_4UTF87DEFAULTEv, (boolean)0 );
	label$5:;
}

__attribute__(( constructor )) static void _ZN5TESTS9FBC_TESTS8WSTRING_4UTF812SUITE_CTOR22Ev( void )
{
	label$6:;
	_ZN4FBCU9ADD_SUITEEPcPFivES2_( (char*)"fbc_tests.wstring_.utf8", (tmp$4)0u, (tmp$4)0u );
	label$7:;
}
