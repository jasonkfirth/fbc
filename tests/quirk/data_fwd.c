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
#line 14 "QUIRK\\DATA_FWD.BAS"
typedef void (*tmp$5)( void );
#line 40 "QUIRK\\DATA_FWD.BAS"
typedef int32 (*tmp$4)( void );
#line 40 "quirk\\data_fwd.bas"
void fb_DataRestore( void* );
#line 40 "quirk\\data_fwd.bas"
void fb_DataReadStr( void*, int64, int32 );
#line 40 "quirk\\data_fwd.bas"
void fb_DataReadLongint( int64* );
#line 40 "quirk\\data_fwd.bas"
void fb_DataReadDouble( double* );
#line 40 "quirk\\data_fwd.bas"
void fb_StrDelete( FBSTRING* );
#line 40 "quirk\\data_fwd.bas"
int32 fb_StrCompare( void*, int64, void*, int64 );
#line 40 "quirk\\data_fwd.bas"
FBSTRING* fb_LongintToStr( int64 );
#line 40 "quirk\\data_fwd.bas"
static void fb_ctor__data_fwd( void ) __attribute__(( constructor ));
#line 40 "quirk\\data_fwd.bas"
void _ZN4FBCU9ADD_SUITEEPcPFivES2_( char*, tmp$4, tmp$4 );
#line 40 "quirk\\data_fwd.bas"
void _ZN4FBCU8ADD_TESTEPcS0_PFvvEb( char*, char*, tmp$5, boolean );
#line 40 "quirk\\data_fwd.bas"
void _ZN4FBCU10CU_ASSERT_EbPciS0_S0_( boolean, char*, int32, char*, char* );
#line 40 "quirk\\data_fwd.bas"
void _ZN5TESTS9FBC_TESTS5QUIRK8DATA_FWD8INTEGER_Ev( void );
#line 40 "quirk\\data_fwd.bas"
static void _ZN5TESTS9FBC_TESTS5QUIRK8DATA_FWD13INTEGER__CTOREv( void ) __attribute__(( constructor ));
#line 40 "quirk\\data_fwd.bas"
void _ZN5TESTS9FBC_TESTS5QUIRK8DATA_FWD7STRING_Ev( void );
#line 40 "quirk\\data_fwd.bas"
static void _ZN5TESTS9FBC_TESTS5QUIRK8DATA_FWD12STRING__CTOREv( void ) __attribute__(( constructor ));
#line 40 "quirk\\data_fwd.bas"
void _ZN5TESTS9FBC_TESTS5QUIRK8DATA_FWD5FLOATEv( void );
#line 40 "quirk\\data_fwd.bas"
static void _ZN5TESTS9FBC_TESTS5QUIRK8DATA_FWD10FLOAT_CTOREv( void ) __attribute__(( constructor ));
#line 40 "quirk\\data_fwd.bas"
static void _ZN5TESTS9FBC_TESTS5QUIRK8DATA_FWD12SUITE_CTOR40Ev( void ) __attribute__(( constructor ));
struct __attribute__((gcc_struct)) $14__FB_DATADESC$ {
	int16 TYPE __attribute__((packed, aligned(1)));
	void* NODE __attribute__((packed, aligned(1)));
};
#define __FB_STATIC_ASSERT( expr ) extern int __$fb_structsizecheck[(expr) ? 1 : -1]
#line 40 "quirk\\data_fwd.bas"
__FB_STATIC_ASSERT( sizeof( struct $14__FB_DATADESC$ ) == 10 );
#line 40 "quirk\\data_fwd.bas"
static struct $14__FB_DATADESC$ label$25[7] = { { (int16)3, (void*)"0.5" }, { (int16)1, (void*)"1" }, { (int16)3, (void*)"1.5" }, { (int16)1, (void*)"2" }, { (int16)3, (void*)"2.5" }, { (int16)1, (void*)"3" }, { (int16)-1, (void*)0ull } };
#line 40 "quirk\\data_fwd.bas"
static struct $14__FB_DATADESC$ label$41[4] = { { (int16)2, (void*)"-4" }, { (int16)2, (void*)"-5" }, { (int16)2, (void*)"-6" }, { (int16)-1, (void*)label$25 } };
#line 40 "quirk\\data_fwd.bas"
static struct $14__FB_DATADESC$ label$39[3] = { { (int16)2, (void*)"-2" }, { (int16)2, (void*)"-3" }, { (int16)-1, (void*)label$41 } };
#line 40 "quirk\\data_fwd.bas"
static struct $14__FB_DATADESC$ label$15[2] = { { (int16)2, (void*)"-1" }, { (int16)-1, (void*)label$39 } };
#line 40 "quirk\\data_fwd.bas"
static struct $14__FB_DATADESC$ label$37[4] = { { (int16)1, (void*)"4" }, { (int16)1, (void*)"5" }, { (int16)1, (void*)"6" }, { (int16)-1, (void*)label$15 } };
#line 40 "quirk\\data_fwd.bas"
static struct $14__FB_DATADESC$ label$35[3] = { { (int16)1, (void*)"2" }, { (int16)1, (void*)"3" }, { (int16)-1, (void*)label$37 } };
#line 40 "quirk\\data_fwd.bas"
static struct $14__FB_DATADESC$ label$5[2] = { { (int16)1, (void*)"1" }, { (int16)-1, (void*)label$35 } };

#line 5 "QUIRK\\DATA_FWD.BAS"
void _ZN5TESTS9FBC_TESTS5QUIRK8DATA_FWD8INTEGER_Ev( void )
#line 5 "QUIRK\\DATA_FWD.BAS"
{
	#line 5 "QUIRK\\DATA_FWD.BAS"
	label$2:;
	// #else
	// TEST_EMIT( fbcu_global, TMP_FBCUNIT_TEST_NAME integer_ )sub integer_ cdecl () FBCU_TRACE( "TEST" integer_ ) #endif ]
	// 		dim as integer i, v
	#line 6 "QUIRK\\DATA_FWD.BAS"
	int64 I$1;
	#line 6 "QUIRK\\DATA_FWD.BAS"
	__builtin_memset( &I$1, 0, 8ll );
	#line 6 "QUIRK\\DATA_FWD.BAS"
	int64 V$1;
	#line 6 "QUIRK\\DATA_FWD.BAS"
	__builtin_memset( &V$1, 0, 8ll );
	// 		restore data_1
	#line 8 "QUIRK\\DATA_FWD.BAS"
	fb_DataRestore( (void*)label$5 );
	// 		for i = 1 to 6
	{
		#line 9 "QUIRK\\DATA_FWD.BAS"
		I$1 = 1ll;
		#line 9 "QUIRK\\DATA_FWD.BAS"
		label$9:;
		{
			// 			read v
			#line 10 "QUIRK\\DATA_FWD.BAS"
			fb_DataReadLongint( (int64*)&V$1 );
			// 			CU_ASSERT_EQUAL( v, i ) [Macro Expansion: fbcu.CU_ASSERT_( ((v)=(i)), __FILE__,$"quirk\data_fwd.bas", __LINE__,11, __FUNCTION__,$"TESTS.FBC_TESTS.QUIRK.DATA_FWD.INTEGER_", "CU_ASSERT_EQUAL(" $"v" "," $"i" ")" ) ]
			#line 11 "QUIRK\\DATA_FWD.BAS"
			_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(V$1 == I$1) != 0ll), (char*)"quirk\x5C" "data_fwd.bas", 11, (char*)"TESTS.FBC_TESTS.QUIRK.DATA_FWD.INTEGER_", (char*)"CU_ASSERT_EQUAL(v,i)" );
			// 		next
		}
		#line 12 "QUIRK\\DATA_FWD.BAS"
		label$7:;
		#line 12 "QUIRK\\DATA_FWD.BAS"
		I$1 = I$1 + 1ll;
		#line 12 "QUIRK\\DATA_FWD.BAS"
		label$6:;
		#line 12 "QUIRK\\DATA_FWD.BAS"
		if( I$1 <= 6ll) goto label$9;
		#line 12 "QUIRK\\DATA_FWD.BAS"
		label$8:;
	}
	// 	END_TEST [Macro Expansion: #if defined( TMP_FBCUNIT_TEST_NAME )
	// #if defined( TMP_FBCUNIT_SUITE_NAME )
	// #if defined( TMP_FBCUNIT_TEST_GROUP_NAME )
	// END_TEST_EMIT( TMP_FBCUNIT_SUITE_NAME,fbc_tests.quirk.data_fwd, TMP_FBCUNIT_TEST_GROUP_NAME, TMP_FBCUNIT_TEST_NAME,integer_, false )end sub private sub integer__ctor cdecl () constructor #if $"TMP_FBCUNIT_TEST_GROUP_NAME" > "" fbcu.add_test( $"fbc_tests.quirk.data_fwd", $"TMP_FBCUNIT_TEST_GROUP_NAME" + "." + $"integer_", procptr(integer_), false ) #else fbcu.add_test( $"fbc_tests.quirk.data_fwd", $"integer_", procptr(integer_), false ) #endif end sub FBCU_TRACE( "END_TEST" integer_ ) #else
	#line 14 "QUIRK\\DATA_FWD.BAS"
	label$3:;
#line 14 "QUIRK\\DATA_FWD.BAS"
}

#line 16 "QUIRK\\DATA_FWD.BAS"
void _ZN5TESTS9FBC_TESTS5QUIRK8DATA_FWD7STRING_Ev( void )
#line 16 "QUIRK\\DATA_FWD.BAS"
{
	#line 16 "QUIRK\\DATA_FWD.BAS"
	label$12:;
	// #else
	// TEST_EMIT( fbcu_global, TMP_FBCUNIT_TEST_NAME string_ )sub string_ cdecl () FBCU_TRACE( "TEST" string_ ) #endif ]
	// 		dim as integer i
	#line 17 "QUIRK\\DATA_FWD.BAS"
	int64 I$1;
	#line 17 "QUIRK\\DATA_FWD.BAS"
	__builtin_memset( &I$1, 0, 8ll );
	// 		dim as string v
	#line 18 "QUIRK\\DATA_FWD.BAS"
	FBSTRING V$1;
	#line 18 "QUIRK\\DATA_FWD.BAS"
	__builtin_memset( &V$1, 0, 24ll );
	// 		restore data_2
	#line 20 "QUIRK\\DATA_FWD.BAS"
	fb_DataRestore( (void*)label$15 );
	// 		for i = 1 to 6
	{
		#line 21 "QUIRK\\DATA_FWD.BAS"
		I$1 = 1ll;
		#line 21 "QUIRK\\DATA_FWD.BAS"
		label$19:;
		{
			// 			read v
			#line 22 "QUIRK\\DATA_FWD.BAS"
			fb_DataReadStr( (void*)&V$1, -1ll, 0 );
			// 			CU_ASSERT_EQUAL( v, str(-i) ) [Macro Expansion: fbcu.CU_ASSERT_( ((v)=(str(-i))), __FILE__,$"quirk\data_fwd.bas", __LINE__,23, __FUNCTION__,$"TESTS.FBC_TESTS.QUIRK.DATA_FWD.STRING_", "CU_ASSERT_EQUAL(" $"v" "," $"str(-i)" ")" ) ]
			#line 23 "QUIRK\\DATA_FWD.BAS"
			FBSTRING* vr$4 = fb_LongintToStr( -I$1 );
			#line 23 "QUIRK\\DATA_FWD.BAS"
			int32 vr$6 = fb_StrCompare( (void*)&V$1, -1ll, (void*)vr$4, -1ll );
			#line 23 "QUIRK\\DATA_FWD.BAS"
			_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)vr$6 == 0ll) != 0ll), (char*)"quirk\x5C" "data_fwd.bas", 23, (char*)"TESTS.FBC_TESTS.QUIRK.DATA_FWD.STRING_", (char*)"CU_ASSERT_EQUAL(v,str(-i))" );
			// 		next
		}
		#line 24 "QUIRK\\DATA_FWD.BAS"
		label$17:;
		#line 24 "QUIRK\\DATA_FWD.BAS"
		I$1 = I$1 + 1ll;
		#line 24 "QUIRK\\DATA_FWD.BAS"
		label$16:;
		#line 24 "QUIRK\\DATA_FWD.BAS"
		if( I$1 <= 6ll) goto label$19;
		#line 24 "QUIRK\\DATA_FWD.BAS"
		label$18:;
	}
	// 	END_TEST [Macro Expansion: #if defined( TMP_FBCUNIT_TEST_NAME )
	// #if defined( TMP_FBCUNIT_SUITE_NAME )
	// #if defined( TMP_FBCUNIT_TEST_GROUP_NAME )
	// END_TEST_EMIT( TMP_FBCUNIT_SUITE_NAME,fbc_tests.quirk.data_fwd, TMP_FBCUNIT_TEST_GROUP_NAME, TMP_FBCUNIT_TEST_NAME,string_, false )end sub private sub string__ctor cdecl () constructor #if $"TMP_FBCUNIT_TEST_GROUP_NAME" > "" fbcu.add_test( $"fbc_tests.quirk.data_fwd", $"TMP_FBCUNIT_TEST_GROUP_NAME" + "." + $"string_", procptr(string_), false ) #else fbcu.add_test( $"fbc_tests.quirk.data_fwd", $"string_", procptr(string_), false ) #endif end sub FBCU_TRACE( "END_TEST" string_ ) #else
	#line 26 "QUIRK\\DATA_FWD.BAS"
	fb_StrDelete( (FBSTRING*)&V$1 );
	#line 26 "QUIRK\\DATA_FWD.BAS"
	label$13:;
#line 26 "QUIRK\\DATA_FWD.BAS"
}

#line 28 "QUIRK\\DATA_FWD.BAS"
void _ZN5TESTS9FBC_TESTS5QUIRK8DATA_FWD5FLOATEv( void )
#line 28 "QUIRK\\DATA_FWD.BAS"
{
	#line 28 "QUIRK\\DATA_FWD.BAS"
	label$22:;
	// #else
	// TEST_EMIT( fbcu_global, TMP_FBCUNIT_TEST_NAME float )sub float cdecl () FBCU_TRACE( "TEST" float ) #endif ]
	// 		dim as integer i
	#line 29 "QUIRK\\DATA_FWD.BAS"
	int64 I$1;
	#line 29 "QUIRK\\DATA_FWD.BAS"
	__builtin_memset( &I$1, 0, 8ll );
	// 		dim as double v
	#line 30 "QUIRK\\DATA_FWD.BAS"
	double V$1;
	#line 30 "QUIRK\\DATA_FWD.BAS"
	__builtin_memset( &V$1, 0, 8ll );
	// 		restore data_3
	#line 32 "QUIRK\\DATA_FWD.BAS"
	fb_DataRestore( (void*)label$25 );
	// 		for i = 1 to 6
	{
		#line 33 "QUIRK\\DATA_FWD.BAS"
		I$1 = 1ll;
		#line 33 "QUIRK\\DATA_FWD.BAS"
		label$29:;
		{
			// 			read v
			#line 34 "QUIRK\\DATA_FWD.BAS"
			fb_DataReadDouble( &V$1 );
			// 			CU_ASSERT_EQUAL( v, i/2 ) [Macro Expansion: fbcu.CU_ASSERT_( ((v)=(i/2)), __FILE__,$"quirk\data_fwd.bas", __LINE__,35, __FUNCTION__,$"TESTS.FBC_TESTS.QUIRK.DATA_FWD.FLOAT", "CU_ASSERT_EQUAL(" $"v" "," $"i/2" ")" ) ]
			#line 35 "QUIRK\\DATA_FWD.BAS"
			_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(V$1 == ((double)I$1 / 0x1.p+1)) != 0ll), (char*)"quirk\x5C" "data_fwd.bas", 35, (char*)"TESTS.FBC_TESTS.QUIRK.DATA_FWD.FLOAT", (char*)"CU_ASSERT_EQUAL(v,i/2)" );
			// 		next
		}
		#line 36 "QUIRK\\DATA_FWD.BAS"
		label$27:;
		#line 36 "QUIRK\\DATA_FWD.BAS"
		I$1 = I$1 + 1ll;
		#line 36 "QUIRK\\DATA_FWD.BAS"
		label$26:;
		#line 36 "QUIRK\\DATA_FWD.BAS"
		if( I$1 <= 6ll) goto label$29;
		#line 36 "QUIRK\\DATA_FWD.BAS"
		label$28:;
	}
	// 	END_TEST [Macro Expansion: #if defined( TMP_FBCUNIT_TEST_NAME )
	// #if defined( TMP_FBCUNIT_SUITE_NAME )
	// #if defined( TMP_FBCUNIT_TEST_GROUP_NAME )
	// END_TEST_EMIT( TMP_FBCUNIT_SUITE_NAME,fbc_tests.quirk.data_fwd, TMP_FBCUNIT_TEST_GROUP_NAME, TMP_FBCUNIT_TEST_NAME,float, false )end sub private sub float_ctor cdecl () constructor #if $"TMP_FBCUNIT_TEST_GROUP_NAME" > "" fbcu.add_test( $"fbc_tests.quirk.data_fwd", $"TMP_FBCUNIT_TEST_GROUP_NAME" + "." + $"float", procptr(float), false ) #else fbcu.add_test( $"fbc_tests.quirk.data_fwd", $"float", procptr(float), false ) #endif end sub FBCU_TRACE( "END_TEST" float ) #else
	#line 38 "QUIRK\\DATA_FWD.BAS"
	label$23:;
#line 38 "QUIRK\\DATA_FWD.BAS"
}

#line 1 "QUIRK\\DATA_FWD.BAS"
__attribute__(( constructor )) static void fb_ctor__data_fwd( void )
#line 1 "QUIRK\\DATA_FWD.BAS"
{
	#line 1 "QUIRK\\DATA_FWD.BAS"
	label$0:;
	// #include "fbcunit.bi"
	// SUITE( fbc_tests.quirk.data_fwd ) [Macro Expansion: #if defined( TMP_FBCUNIT_SUITE_NAME )
	// #error FBCUNIT: test suites can not be nested, or missing "END_SUITE" before "SUITE" #endif
	// #if $"fbc_tests.quirk.data_fwd" > ""
	// #define TMP_FBCUNIT_SUITE_NAME fbc_tests.quirk.data_fwd
	// #else
	// #define TMP_FBCUNIT_SUITE_NAME fbcu_global #endif
	// SUITE_EMIT( TMP_FBCUNIT_SUITE_NAME fbc_tests.quirk.data_fwd )namespace tests.fbc_tests.quirk.data_fwd
	// 	TEST( integer_ ) [Macro Expansion: #if defined( TMP_FBCUNIT_TEST_NAME )
	// #error FBCUNIT: tests can not be nested or missing "END_TEST" #elseif defined( TMP_FBCUNIT_SUITE_IN_INIT )
	// #error FBCUNIT: missing "END_SUITE_INIT" before "TEST" #elseif defined( TMP_FBCUNIT_SUITE_IN_CLEANUP )
	// #error FBCUNIT: missing "END_SUITE_CLEANUP" before "TEST" #endif
	// #if $"integer_" > ""
	// #define TMP_FBCUNIT_TEST_NAME integer_
	// #else
	// #define TMP_FBCUNIT_TEST_NAME default #endif
	// #if defined( TMP_FBCUNIT_SUITE_NAME )
	// END_TEST_EMIT( TMP_FBCUNIT_SUITE_NAME,fbc_tests.quirk.data_fwd, , TMP_FBCUNIT_TEST_NAME,integer_, false )end sub
	// end sub
	// #endif
	// #else
	// #if defined( TMP_FBCUNIT_TEST_GROUP_NAME ) END_TEST_EMIT( fbcu_global, TMP_FBCUNIT_TEST_GROUP_NAME, TMP_FBCUNIT_TEST_NAME,integer_, true )end sub private sub integer__ctor cdecl () constructor #if $"TMP_FBCUNIT_TEST_GROUP_NAME" > "" fbcu.add_test( $"fbcu_global", $"TMP_FBCUNIT_TEST_GROUP_NAME" + "." + $"integer_", procptr(integer_), true ) #else fbcu.add_test( $"fbcu_global", $"integer_", procptr(integer_), true ) #endif end sub FBCU_TRACE( "END_TEST" integer_ ) #else END_TEST_EMIT( fbcu_global, , TMP_FBCUNIT_TEST_NAME,integer_, true )end sub private sub integer__ctor cdecl () constructor #if "" > "" fbcu.add_test( $"fbcu_global", "" + "." + $"integer_", procptr(integer_), true ) #else fbcu.add_test( $"fbcu_global", $"integer_", procptr(integer_), true ) #endif end sub FBCU_TRACE( "END_TEST" integer_ ) #endif #endif
	// #else
	// #error FBCUNIT: mismatched "END_TEST" #endif
	// #undef TMP_FBCUNIT_TEST_NAME ]
	// 	TEST( string_ ) [Macro Expansion: #if defined( TMP_FBCUNIT_TEST_NAME )
	// #error FBCUNIT: tests can not be nested or missing "END_TEST" #elseif defined( TMP_FBCUNIT_SUITE_IN_INIT )
	// #error FBCUNIT: missing "END_SUITE_INIT" before "TEST" #elseif defined( TMP_FBCUNIT_SUITE_IN_CLEANUP )
	// #error FBCUNIT: missing "END_SUITE_CLEANUP" before "TEST" #endif
	// #if $"string_" > ""
	// #define TMP_FBCUNIT_TEST_NAME string_
	// #else
	// #define TMP_FBCUNIT_TEST_NAME default #endif
	// #if defined( TMP_FBCUNIT_SUITE_NAME )
	// END_TEST_EMIT( TMP_FBCUNIT_SUITE_NAME,fbc_tests.quirk.data_fwd, , TMP_FBCUNIT_TEST_NAME,string_, false )end sub
	// end sub
	// #endif
	// #else
	// #if defined( TMP_FBCUNIT_TEST_GROUP_NAME ) END_TEST_EMIT( fbcu_global, TMP_FBCUNIT_TEST_GROUP_NAME, TMP_FBCUNIT_TEST_NAME,string_, true )end sub private sub string__ctor cdecl () constructor #if $"TMP_FBCUNIT_TEST_GROUP_NAME" > "" fbcu.add_test( $"fbcu_global", $"TMP_FBCUNIT_TEST_GROUP_NAME" + "." + $"string_", procptr(string_), true ) #else fbcu.add_test( $"fbcu_global", $"string_", procptr(string_), true ) #endif end sub FBCU_TRACE( "END_TEST" string_ ) #else END_TEST_EMIT( fbcu_global, , TMP_FBCUNIT_TEST_NAME,string_, true )end sub private sub string__ctor cdecl () constructor #if "" > "" fbcu.add_test( $"fbcu_global", "" + "." + $"string_", procptr(string_), true ) #else fbcu.add_test( $"fbcu_global", $"string_", procptr(string_), true ) #endif end sub FBCU_TRACE( "END_TEST" string_ ) #endif #endif
	// #else
	// #error FBCUNIT: mismatched "END_TEST" #endif
	// #undef TMP_FBCUNIT_TEST_NAME ]
	// 	TEST( float ) [Macro Expansion: #if defined( TMP_FBCUNIT_TEST_NAME )
	// #error FBCUNIT: tests can not be nested or missing "END_TEST" #elseif defined( TMP_FBCUNIT_SUITE_IN_INIT )
	// #error FBCUNIT: missing "END_SUITE_INIT" before "TEST" #elseif defined( TMP_FBCUNIT_SUITE_IN_CLEANUP )
	// #error FBCUNIT: missing "END_SUITE_CLEANUP" before "TEST" #endif
	// #if $"float" > ""
	// #define TMP_FBCUNIT_TEST_NAME float
	// #else
	// #define TMP_FBCUNIT_TEST_NAME default #endif
	// #if defined( TMP_FBCUNIT_SUITE_NAME )
	// END_TEST_EMIT( TMP_FBCUNIT_SUITE_NAME,fbc_tests.quirk.data_fwd, , TMP_FBCUNIT_TEST_NAME,float, false )end sub
	// end sub
	// #endif
	// #else
	// #if defined( TMP_FBCUNIT_TEST_GROUP_NAME ) END_TEST_EMIT( fbcu_global, TMP_FBCUNIT_TEST_GROUP_NAME, TMP_FBCUNIT_TEST_NAME,float, true )end sub private sub float_ctor cdecl () constructor #if $"TMP_FBCUNIT_TEST_GROUP_NAME" > "" fbcu.add_test( $"fbcu_global", $"TMP_FBCUNIT_TEST_GROUP_NAME" + "." + $"float", procptr(float), true ) #else fbcu.add_test( $"fbcu_global", $"float", procptr(float), true ) #endif end sub FBCU_TRACE( "END_TEST" float ) #else END_TEST_EMIT( fbcu_global, , TMP_FBCUNIT_TEST_NAME,float, true )end sub private sub float_ctor cdecl () constructor #if "" > "" fbcu.add_test( $"fbcu_global", "" + "." + $"float", procptr(float), true ) #else fbcu.add_test( $"fbcu_global", $"float", procptr(float), true ) #endif end sub FBCU_TRACE( "END_TEST" float ) #endif #endif
	// #else
	// #error FBCUNIT: mismatched "END_TEST" #endif
	// #undef TMP_FBCUNIT_TEST_NAME ]
	// END_SUITE [Macro Expansion: #if not defined( TMP_FBCUNIT_SUITE_NAME )
	// #error FBCUNIT: unexpected "END_SUITE" #elseif defined( TMP_FBCUNIT_TEST_GROUP_NAME )
	// #error FBCUNIT: missing "END_TEST_GROUP" before "END_SUITE" #elseif defined( TMP_FBCUNIT_TEST_NAME )
	// #error FBCUNIT: missing "END_TEST" before "END_SUITE" #elseif defined( TMP_FBCUNIT_SUITE_IN_INIT )
	// #error FBCUNIT: missing "END_SUITE_INIT" before "END_SUITE" #elseif defined( TMP_FBCUNIT_SUITE_IN_CLEANUP )
	// #error FBCUNIT: missing "END_SUITE_CLEANUP" before "END_SUITE" #endif
	// #if defined( TMP_FBCUNIT_SUITE_NAME )
	// end sub
	// end namespace
	// #else
	// #error FBCUNIT: mismatched "END_SUITE" #endif
	// #undef TMP_FBCUNIT_SUITE_NAME
	// #undef TMP_FBCUNIT_SUITE_IN_INIT
	// #undef TMP_FBCUNIT_SUITE_HAVE_INIT
	// #undef TMP_FBCUNIT_SUITE_IN_CLEANUP
	// #undef TMP_FBCUNIT_SUITE_HAVE_CLEANUP ]
	// data_1:
	#line 42 "QUIRK\\DATA_FWD.BAS"
	label$4:;
	// data 1
	// data 2, 3
	// data 4, 5, 6
	// data_2:
	#line 47 "QUIRK\\DATA_FWD.BAS"
	label$14:;
	// data "-1"
	// data "-2", "-3"
	// data "-4", "-5", "-6"
	// data_3:
	#line 52 "QUIRK\\DATA_FWD.BAS"
	label$24:;
	// data 1/2, 2/2, 3/2, 4/2, 5/2, 6/2
	#line 53 "QUIRK\\DATA_FWD.BAS"
	label$1:;
#line 53 "QUIRK\\DATA_FWD.BAS"
}

#line 14 "QUIRK\\DATA_FWD.BAS"
__attribute__(( constructor )) static void _ZN5TESTS9FBC_TESTS5QUIRK8DATA_FWD13INTEGER__CTOREv( void )
#line 14 "QUIRK\\DATA_FWD.BAS"
{
	#line 14 "QUIRK\\DATA_FWD.BAS"
	label$10:;
	// #if "" > ""
	// fbcu.add_test( $"fbc_tests.quirk.data_fwd", "" + "." + $"integer_", procptr(integer_), false ) #else
	// fbcu.add_test( $"fbc_tests.quirk.data_fwd", $"integer_", procptr(integer_), false )
	#line 14 "QUIRK\\DATA_FWD.BAS"
	_ZN4FBCU8ADD_TESTEPcS0_PFvvEb( (char*)"fbc_tests.quirk.data_fwd", (char*)"integer_", (tmp$5)&_ZN5TESTS9FBC_TESTS5QUIRK8DATA_FWD8INTEGER_Ev, (boolean)0ll );
	// #endif
	#line 14 "QUIRK\\DATA_FWD.BAS"
	label$11:;
#line 14 "QUIRK\\DATA_FWD.BAS"
}

#line 26 "QUIRK\\DATA_FWD.BAS"
__attribute__(( constructor )) static void _ZN5TESTS9FBC_TESTS5QUIRK8DATA_FWD12STRING__CTOREv( void )
#line 26 "QUIRK\\DATA_FWD.BAS"
{
	#line 26 "QUIRK\\DATA_FWD.BAS"
	label$20:;
	// #if "" > ""
	// fbcu.add_test( $"fbc_tests.quirk.data_fwd", "" + "." + $"string_", procptr(string_), false ) #else
	// fbcu.add_test( $"fbc_tests.quirk.data_fwd", $"string_", procptr(string_), false )
	#line 26 "QUIRK\\DATA_FWD.BAS"
	_ZN4FBCU8ADD_TESTEPcS0_PFvvEb( (char*)"fbc_tests.quirk.data_fwd", (char*)"string_", (tmp$5)&_ZN5TESTS9FBC_TESTS5QUIRK8DATA_FWD7STRING_Ev, (boolean)0ll );
	// #endif
	#line 26 "QUIRK\\DATA_FWD.BAS"
	label$21:;
#line 26 "QUIRK\\DATA_FWD.BAS"
}

#line 38 "QUIRK\\DATA_FWD.BAS"
__attribute__(( constructor )) static void _ZN5TESTS9FBC_TESTS5QUIRK8DATA_FWD10FLOAT_CTOREv( void )
#line 38 "QUIRK\\DATA_FWD.BAS"
{
	#line 38 "QUIRK\\DATA_FWD.BAS"
	label$30:;
	// #if "" > ""
	// fbcu.add_test( $"fbc_tests.quirk.data_fwd", "" + "." + $"float", procptr(float), false ) #else
	// fbcu.add_test( $"fbc_tests.quirk.data_fwd", $"float", procptr(float), false )
	#line 38 "QUIRK\\DATA_FWD.BAS"
	_ZN4FBCU8ADD_TESTEPcS0_PFvvEb( (char*)"fbc_tests.quirk.data_fwd", (char*)"float", (tmp$5)&_ZN5TESTS9FBC_TESTS5QUIRK8DATA_FWD5FLOATEv, (boolean)0ll );
	// #endif
	#line 38 "QUIRK\\DATA_FWD.BAS"
	label$31:;
#line 38 "QUIRK\\DATA_FWD.BAS"
}

#line 40 "QUIRK\\DATA_FWD.BAS"
__attribute__(( constructor )) static void _ZN5TESTS9FBC_TESTS5QUIRK8DATA_FWD12SUITE_CTOR40Ev( void )
#line 40 "QUIRK\\DATA_FWD.BAS"
{
	#line 40 "QUIRK\\DATA_FWD.BAS"
	label$32:;
	// #if (defined( TMP_FBCUNIT_SUITE_HAVE_INIT ) andalso defined( TMP_FBCUNIT_SUITE_HAVE_CLEANUP ))
	// fbcu.add_suite( $"fbc_tests.quirk.data_fwd", procptr(tests.fbc_tests.quirk.data_fwd.init), procptr(tests.fbc_tests.quirk.data_fwd.cleanup) ) #elseif defined( TMP_FBCUNIT_SUITE_HAVE_INIT )
	// fbcu.add_suite( $"fbc_tests.quirk.data_fwd", procptr(tests.fbc_tests.quirk.data_fwd.init), FBCU_NULL ) #elseif defined( TMP_FBCUNIT_SUITE_HAVE_CLEANUP )
	// fbcu.add_suite( $"fbc_tests.quirk.data_fwd", FBCU_NULL, procptr(tests.fbc_tests.quirk.data_fwd.cleanup) ) #else
	// fbcu.add_suite( $"fbc_tests.quirk.data_fwd", FBCU_NULL,0, FBCU_NULL 0 )
	#line 40 "QUIRK\\DATA_FWD.BAS"
	_ZN4FBCU9ADD_SUITEEPcPFivES2_( (char*)"fbc_tests.quirk.data_fwd", (tmp$4)0ull, (tmp$4)0ull );
	// #endif
	#line 40 "QUIRK\\DATA_FWD.BAS"
	label$33:;
#line 40 "QUIRK\\DATA_FWD.BAS"
}

static const char __attribute__((used, section(".fbctinf"))) __fbctinf[] = "-l\0fbcunit\0-mt";
