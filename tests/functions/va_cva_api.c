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
struct $9__va_list {
	void* __STACK;
	void* __GR_TOP;
	void* __VR_TOP;
	int32 __GR_OFFS;
	int32 __VR_OFFS;
};
#define __FB_STATIC_ASSERT( expr ) extern int __$fb_structsizecheck[(expr) ? 1 : -1]
#line 355 "FUNCTIONS\\VA_CVA_API.BAS"
__FB_STATIC_ASSERT( sizeof( struct $9__va_list ) == 32 );
struct $N5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API10UDT_VARARGE {
	struct $9__va_list F1;
	struct $9__va_list F2;
};
#line 355 "FUNCTIONS\\VA_CVA_API.BAS"
__FB_STATIC_ASSERT( sizeof( struct $N5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API10UDT_VARARGE ) == 64 );
struct $16__FB_ARRAYDIMTB$ {
	int64 ELEMENTS;
	int64 LBOUND;
	int64 UBOUND;
};
#line 371 "FUNCTIONS\\VA_CVA_API.BAS"
__FB_STATIC_ASSERT( sizeof( struct $16__FB_ARRAYDIMTB$ ) == 24 );
#line 61 "FUNCTIONS\\VA_CVA_API.BAS"
typedef void (*tmp$5)( void );
#line 551 "FUNCTIONS\\VA_CVA_API.BAS"
typedef int32 (*tmp$4)( void );
#line 551 "functions\\va_cva_api.bas"
void* calloc( uint64, uint64 );
#line 551 "functions\\va_cva_api.bas"
void free( void* );
#line 551 "functions\\va_cva_api.bas"
static void fb_ctor__va_cva_api( void ) __attribute__(( constructor ));
#line 551 "functions\\va_cva_api.bas"
void _ZN4FBCU9ADD_SUITEEPcPFivES2_( char*, tmp$4, tmp$4 );
#line 551 "functions\\va_cva_api.bas"
void _ZN4FBCU8ADD_TESTEPcS0_PFvvEb( char*, char*, tmp$5, boolean );
#line 551 "functions\\va_cva_api.bas"
void _ZN4FBCU10CU_ASSERT_EbPciS0_S0_( boolean, char*, int32, char*, char* );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API12F_TEST_STARTEu7INTEGERz( int64, ... );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API5STARTEv( void );
#line 551 "functions\\va_cva_api.bas"
static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API10START_CTOREv( void ) __attribute__(( constructor ));
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API11F_TEST_COPYEu7INTEGERz( int64, ... );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API4COPYEv( void );
#line 551 "functions\\va_cva_api.bas"
static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API9COPY_CTOREv( void ) __attribute__(( constructor ));
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API12F_TEST_LIST2Eu7INTEGERSt9__va_list( int64, __builtin_va_list );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API11F_TEST_LISTEu7INTEGERz( int64, ... );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API4LISTEv( void );
#line 551 "functions\\va_cva_api.bas"
static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API9LIST_CTOREv( void ) __attribute__(( constructor ));
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API16F_TEST_ARG_UBYTEEu7INTEGERz( int64, ... );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API15F_TEST_ARG_BYTEEu7INTEGERz( int64, ... );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API17F_TEST_ARG_USHORTEu7INTEGERz( int64, ... );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API16F_TEST_ARG_SHORTEu7INTEGERz( int64, ... );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API16F_TEST_ARG_ULONGEu7INTEGERz( int64, ... );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API15F_TEST_ARG_LONGEu7INTEGERz( int64, ... );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API19F_TEST_ARG_UINTEGEREu7INTEGERz( int64, ... );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API18F_TEST_ARG_INTEGEREu7INTEGERz( int64, ... );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API19F_TEST_ARG_ULONGINTEu7INTEGERz( int64, ... );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API18F_TEST_ARG_LONGINTEu7INTEGERz( int64, ... );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API11BASIC_TYPESEv( void );
#line 551 "functions\\va_cva_api.bas"
static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API16BASIC_TYPES_CTOREv( void ) __attribute__(( constructor ));
#line 551 "functions\\va_cva_api.bas"
int64 _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API11F_ARG_BYVALEu7INTEGERS3_St9__va_list( int64, int64, __builtin_va_list );
#line 551 "functions\\va_cva_api.bas"
int64 _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API11F_ARG_BYREFEu7INTEGERS3_RSt9__va_list( int64, int64, __builtin_va_list* );
#line 551 "functions\\va_cva_api.bas"
int64 _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API15F_ARG_BYREF_PTREu7INTEGERS3_RPSt9__va_list( int64, int64, __builtin_va_list** );
#line 551 "functions\\va_cva_api.bas"
int64 _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API9F_ARG_PTREu7INTEGERS3_PSt9__va_list( int64, int64, __builtin_va_list* );
#line 551 "functions\\va_cva_api.bas"
int64 _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API13F_ARG_PTR_PTREu7INTEGERS3_PPSt9__va_list( int64, int64, __builtin_va_list** );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API14ARGUMENT_TESTSEu7INTEGERS3_z( int64, int64, ... );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API9ARGUMENTSEv( void );
#line 551 "functions\\va_cva_api.bas"
static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API14ARGUMENTS_CTOREv( void ) __attribute__(( constructor ));
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API13COMPLEX_TESTSEu7INTEGERS3_z( int64, int64, ... );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API7COMPLEXEv( void );
#line 551 "functions\\va_cva_api.bas"
static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API12COMPLEX_CTOREv( void ) __attribute__(( constructor ));
#line 551 "functions\\va_cva_api.bas"
__builtin_va_list* _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API14RET_VALIST_PTREPSt9__va_list( __builtin_va_list* );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API17PROC_CVA_LIST_PTREu7INTEGERz( int64, ... );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API19PROC_CVA_LIST_BYVALEu7INTEGERz( int64, ... );
#line 551 "functions\\va_cva_api.bas"
__builtin_va_list* _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API16RET_VALIST_BYREFERSt9__va_list( __builtin_va_list* );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API19PROC_CVA_LIST_BYREFEu7INTEGERz( int64, ... );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API21CVA_LIST_RETURN_BYVALEv( void );
#line 551 "functions\\va_cva_api.bas"
static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API26CVA_LIST_RETURN_BYVAL_CTOREv( void ) __attribute__(( constructor ));
#line 551 "functions\\va_cva_api.bas"
__builtin_va_list* _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API10SIDEFX_PTREPSt9__va_list( __builtin_va_list* );
#line 551 "functions\\va_cva_api.bas"
__builtin_va_list* _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API12SIDEFX_BYREFERSt9__va_list( __builtin_va_list* );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API11PROC_SIDEFXEu7INTEGERz( int64, ... );
#line 551 "functions\\va_cva_api.bas"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API12SIDE_EFFECTSEv( void );
#line 551 "functions\\va_cva_api.bas"
static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API17SIDE_EFFECTS_CTOREv( void ) __attribute__(( constructor ));
#line 551 "functions\\va_cva_api.bas"
static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API13SUITE_CTOR551Ev( void ) __attribute__(( constructor ));

#line 44 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API12F_TEST_STARTEu7INTEGERz( int64 N$1, ... )
#line 44 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 44 "FUNCTIONS\\VA_CVA_API.BAS"
	label$2:;
	// 		dim as cva_list x = any
	#line 46 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list X$1;
	// 		cva_start( x, n )
	#line 48 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_start( *(__builtin_va_list*)&X$1, N$1);
	// 		for i as integer = 1 to n
	{
		#line 50 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 I$2;
		#line 50 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = 1ll;
		#line 50 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 TMP$7$2;
		#line 50 "FUNCTIONS\\VA_CVA_API.BAS"
		TMP$7$2 = N$1;
		#line 50 "FUNCTIONS\\VA_CVA_API.BAS"
		goto label$4;
		#line 50 "FUNCTIONS\\VA_CVA_API.BAS"
		label$7:;
		{
			// 			dim a as integer = cva_arg( x, integer )
			#line 51 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 A$3;
			#line 51 "FUNCTIONS\\VA_CVA_API.BAS"
			A$3 = __builtin_va_arg( *(__builtin_va_list*)&X$1, int64);
			// 			CU_ASSERT( a = i * 100 ) [Macro Expansion: fbcu.CU_ASSERT_( (a = i * 100), __FILE__,$"functions\va_cva_api.bas", __LINE__,52, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_START", "CU_ASSERT(" $"a = i * 100" ")" ) ]
			#line 52 "FUNCTIONS\\VA_CVA_API.BAS"
			_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(A$3 == (I$2 * 100ll)) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 52, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_START", (char*)"CU_ASSERT(a = i * 100)" );
			// 		next i
		}
		#line 53 "FUNCTIONS\\VA_CVA_API.BAS"
		label$5:;
		#line 53 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = I$2 + 1ll;
		#line 53 "FUNCTIONS\\VA_CVA_API.BAS"
		label$4:;
		#line 53 "FUNCTIONS\\VA_CVA_API.BAS"
		if( I$2 <= TMP$7$2) goto label$7;
		#line 53 "FUNCTIONS\\VA_CVA_API.BAS"
		label$6:;
	}
	// 		cva_end( x )
	#line 55 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&X$1);
	#line 57 "FUNCTIONS\\VA_CVA_API.BAS"
	label$3:;
#line 57 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 59 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API5STARTEv( void )
#line 59 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 59 "FUNCTIONS\\VA_CVA_API.BAS"
	label$8:;
	// #else
	// TEST_EMIT( fbcu_global, TMP_FBCUNIT_TEST_NAME start )sub start cdecl () FBCU_TRACE( "TEST" start ) #endif ]
	// 		f_test_start( 10, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000 )
	#line 60 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API12F_TEST_STARTEu7INTEGERz( 10ll, 100ll, 200ll, 300ll, 400ll, 500ll, 600ll, 700ll, 800ll, 900ll, 1000ll );
	// 	END_TEST [Macro Expansion: #if defined( TMP_FBCUNIT_TEST_NAME )
	// #if defined( TMP_FBCUNIT_SUITE_NAME )
	// #if defined( TMP_FBCUNIT_TEST_GROUP_NAME )
	// END_TEST_EMIT( TMP_FBCUNIT_SUITE_NAME,fbc_tests.functions.va_cva_api, TMP_FBCUNIT_TEST_GROUP_NAME, TMP_FBCUNIT_TEST_NAME,start, false )end sub private sub start_ctor cdecl () constructor #if $"TMP_FBCUNIT_TEST_GROUP_NAME" > "" fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"TMP_FBCUNIT_TEST_GROUP_NAME" + "." + $"start", procptr(start), false ) #else fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"start", procptr(start), false ) #endif end sub FBCU_TRACE( "END_TEST" start ) #else
	#line 61 "FUNCTIONS\\VA_CVA_API.BAS"
	label$9:;
#line 61 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 65 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API11F_TEST_COPYEu7INTEGERz( int64 N$1, ... )
#line 65 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 65 "FUNCTIONS\\VA_CVA_API.BAS"
	label$12:;
	// 		dim as cva_list x1 = any, x2 = any, y1 = any, y2 = any
	#line 67 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list X1$1;
	#line 67 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list X2$1;
	#line 67 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list Y1$1;
	#line 67 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list Y2$1;
	// 		cva_start( x1, n )
	#line 69 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_start( *(__builtin_va_list*)&X1$1, N$1);
	// 		cva_copy( x2, x1 )
	#line 70 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_copy( *(__builtin_va_list*)&X2$1, *(__builtin_va_list*)&X1$1);
	// 		cva_copy( y1, x1 )
	#line 71 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_copy( *(__builtin_va_list*)&Y1$1, *(__builtin_va_list*)&X1$1);
	// 		cva_copy( y2, x1 )
	#line 72 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_copy( *(__builtin_va_list*)&Y2$1, *(__builtin_va_list*)&X1$1);
	// 		for i as integer = 1 to n
	{
		#line 74 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 I$2;
		#line 74 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = 1ll;
		#line 74 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 TMP$17$2;
		#line 74 "FUNCTIONS\\VA_CVA_API.BAS"
		TMP$17$2 = N$1;
		#line 74 "FUNCTIONS\\VA_CVA_API.BAS"
		goto label$14;
		#line 74 "FUNCTIONS\\VA_CVA_API.BAS"
		label$17:;
		{
			// 			dim a1 as integer = cva_arg( x1, integer )
			#line 76 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 A1$3;
			#line 76 "FUNCTIONS\\VA_CVA_API.BAS"
			A1$3 = __builtin_va_arg( *(__builtin_va_list*)&X1$1, int64);
			// 			dim a2 as integer = cva_arg( x2, integer )
			#line 77 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 A2$3;
			#line 77 "FUNCTIONS\\VA_CVA_API.BAS"
			A2$3 = __builtin_va_arg( *(__builtin_va_list*)&X2$1, int64);
			// 			CU_ASSERT( a1 = i * 100 ) [Macro Expansion: fbcu.CU_ASSERT_( (a1 = i * 100), __FILE__,$"functions\va_cva_api.bas", __LINE__,79, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_COPY", "CU_ASSERT(" $"a1 = i * 100" ")" ) ]
			#line 79 "FUNCTIONS\\VA_CVA_API.BAS"
			_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(A1$3 == (I$2 * 100ll)) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 79, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_COPY", (char*)"CU_ASSERT(a1 = i * 100)" );
			// 			CU_ASSERT( a2 = i * 100 ) [Macro Expansion: fbcu.CU_ASSERT_( (a2 = i * 100), __FILE__,$"functions\va_cva_api.bas", __LINE__,80, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_COPY", "CU_ASSERT(" $"a2 = i * 100" ")" ) ]
			#line 80 "FUNCTIONS\\VA_CVA_API.BAS"
			_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(A2$3 == (I$2 * 100ll)) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 80, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_COPY", (char*)"CU_ASSERT(a2 = i * 100)" );
			// 		next
		}
		#line 82 "FUNCTIONS\\VA_CVA_API.BAS"
		label$15:;
		#line 82 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = I$2 + 1ll;
		#line 82 "FUNCTIONS\\VA_CVA_API.BAS"
		label$14:;
		#line 82 "FUNCTIONS\\VA_CVA_API.BAS"
		if( I$2 <= TMP$17$2) goto label$17;
		#line 82 "FUNCTIONS\\VA_CVA_API.BAS"
		label$16:;
	}
	// 		cva_end( x1 )
	#line 84 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&X1$1);
	// 		cva_end( x2 )
	#line 85 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&X2$1);
	// 		for i as integer = 1 to n
	{
		#line 87 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 I$2;
		#line 87 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = 1ll;
		#line 87 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 TMP$25$2;
		#line 87 "FUNCTIONS\\VA_CVA_API.BAS"
		TMP$25$2 = N$1;
		#line 87 "FUNCTIONS\\VA_CVA_API.BAS"
		goto label$18;
		#line 87 "FUNCTIONS\\VA_CVA_API.BAS"
		label$21:;
		{
			// 			dim a1 as integer = cva_arg( y1, integer )
			#line 89 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 A1$3;
			#line 89 "FUNCTIONS\\VA_CVA_API.BAS"
			A1$3 = __builtin_va_arg( *(__builtin_va_list*)&Y1$1, int64);
			// 			dim a2 as integer = cva_arg( y2, integer )
			#line 90 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 A2$3;
			#line 90 "FUNCTIONS\\VA_CVA_API.BAS"
			A2$3 = __builtin_va_arg( *(__builtin_va_list*)&Y2$1, int64);
			// 			CU_ASSERT( a1 = i * 100 ) [Macro Expansion: fbcu.CU_ASSERT_( (a1 = i * 100), __FILE__,$"functions\va_cva_api.bas", __LINE__,92, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_COPY", "CU_ASSERT(" $"a1 = i * 100" ")" ) ]
			#line 92 "FUNCTIONS\\VA_CVA_API.BAS"
			_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(A1$3 == (I$2 * 100ll)) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 92, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_COPY", (char*)"CU_ASSERT(a1 = i * 100)" );
			// 			CU_ASSERT( a2 = i * 100 ) [Macro Expansion: fbcu.CU_ASSERT_( (a2 = i * 100), __FILE__,$"functions\va_cva_api.bas", __LINE__,93, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_COPY", "CU_ASSERT(" $"a2 = i * 100" ")" ) ]
			#line 93 "FUNCTIONS\\VA_CVA_API.BAS"
			_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(A2$3 == (I$2 * 100ll)) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 93, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_COPY", (char*)"CU_ASSERT(a2 = i * 100)" );
			// 		next
		}
		#line 95 "FUNCTIONS\\VA_CVA_API.BAS"
		label$19:;
		#line 95 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = I$2 + 1ll;
		#line 95 "FUNCTIONS\\VA_CVA_API.BAS"
		label$18:;
		#line 95 "FUNCTIONS\\VA_CVA_API.BAS"
		if( I$2 <= TMP$25$2) goto label$21;
		#line 95 "FUNCTIONS\\VA_CVA_API.BAS"
		label$20:;
	}
	// 		cva_end( y1 )
	#line 97 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&Y1$1);
	// 		cva_end( y2 )
	#line 98 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&Y2$1);
	#line 100 "FUNCTIONS\\VA_CVA_API.BAS"
	label$13:;
#line 100 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 102 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API4COPYEv( void )
#line 102 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 102 "FUNCTIONS\\VA_CVA_API.BAS"
	label$22:;
	// #else
	// TEST_EMIT( fbcu_global, TMP_FBCUNIT_TEST_NAME copy )sub copy cdecl () FBCU_TRACE( "TEST" copy ) #endif ]
	// 		f_test_copy( 10, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000 )
	#line 103 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API11F_TEST_COPYEu7INTEGERz( 10ll, 100ll, 200ll, 300ll, 400ll, 500ll, 600ll, 700ll, 800ll, 900ll, 1000ll );
	// 	END_TEST [Macro Expansion: #if defined( TMP_FBCUNIT_TEST_NAME )
	// #if defined( TMP_FBCUNIT_SUITE_NAME )
	// #if defined( TMP_FBCUNIT_TEST_GROUP_NAME )
	// END_TEST_EMIT( TMP_FBCUNIT_SUITE_NAME,fbc_tests.functions.va_cva_api, TMP_FBCUNIT_TEST_GROUP_NAME, TMP_FBCUNIT_TEST_NAME,copy, false )end sub private sub copy_ctor cdecl () constructor #if $"TMP_FBCUNIT_TEST_GROUP_NAME" > "" fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"TMP_FBCUNIT_TEST_GROUP_NAME" + "." + $"copy", procptr(copy), false ) #else fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"copy", procptr(copy), false ) #endif end sub FBCU_TRACE( "END_TEST" copy ) #else
	#line 104 "FUNCTIONS\\VA_CVA_API.BAS"
	label$23:;
#line 104 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 106 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API12F_TEST_LIST2Eu7INTEGERSt9__va_list( int64 N$1, __builtin_va_list ARGS$1 )
#line 106 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 106 "FUNCTIONS\\VA_CVA_API.BAS"
	label$26:;
	// 		dim as cva_list x = any
	#line 108 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list X$1;
	// 		cva_copy( x, args )
	#line 110 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_copy( *(__builtin_va_list*)&X$1, *(__builtin_va_list*)&ARGS$1);
	// 		for i as integer = 1 to n
	{
		#line 112 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 I$2;
		#line 112 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = 1ll;
		#line 112 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 TMP$28$2;
		#line 112 "FUNCTIONS\\VA_CVA_API.BAS"
		TMP$28$2 = N$1;
		#line 112 "FUNCTIONS\\VA_CVA_API.BAS"
		goto label$28;
		#line 112 "FUNCTIONS\\VA_CVA_API.BAS"
		label$31:;
		{
			// 			dim a as integer = cva_arg( x, integer )
			#line 113 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 A$3;
			#line 113 "FUNCTIONS\\VA_CVA_API.BAS"
			A$3 = __builtin_va_arg( *(__builtin_va_list*)&X$1, int64);
			// 			CU_ASSERT( a = i * 100 ) [Macro Expansion: fbcu.CU_ASSERT_( (a = i * 100), __FILE__,$"functions\va_cva_api.bas", __LINE__,114, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_LIST2", "CU_ASSERT(" $"a = i * 100" ")" ) ]
			#line 114 "FUNCTIONS\\VA_CVA_API.BAS"
			_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(A$3 == (I$2 * 100ll)) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 114, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_LIST2", (char*)"CU_ASSERT(a = i * 100)" );
			// 		next i
		}
		#line 115 "FUNCTIONS\\VA_CVA_API.BAS"
		label$29:;
		#line 115 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = I$2 + 1ll;
		#line 115 "FUNCTIONS\\VA_CVA_API.BAS"
		label$28:;
		#line 115 "FUNCTIONS\\VA_CVA_API.BAS"
		if( I$2 <= TMP$28$2) goto label$31;
		#line 115 "FUNCTIONS\\VA_CVA_API.BAS"
		label$30:;
	}
	// 		cva_end( x )
	#line 117 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&X$1);
	#line 119 "FUNCTIONS\\VA_CVA_API.BAS"
	label$27:;
#line 119 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 121 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API11F_TEST_LISTEu7INTEGERz( int64 N$1, ... )
#line 121 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 121 "FUNCTIONS\\VA_CVA_API.BAS"
	label$32:;
	// 		dim as cva_list x = any
	#line 123 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list X$1;
	// 		cva_start( x, n )
	#line 125 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_start( *(__builtin_va_list*)&X$1, N$1);
	// 		f_test_list2( n, x )
	#line 127 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API12F_TEST_LIST2Eu7INTEGERSt9__va_list( N$1, X$1 );
	// 		for i as integer = 1 to n
	{
		#line 129 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 I$2;
		#line 129 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = 1ll;
		#line 129 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 TMP$30$2;
		#line 129 "FUNCTIONS\\VA_CVA_API.BAS"
		TMP$30$2 = N$1;
		#line 129 "FUNCTIONS\\VA_CVA_API.BAS"
		goto label$34;
		#line 129 "FUNCTIONS\\VA_CVA_API.BAS"
		label$37:;
		{
			// 			dim a as integer = cva_arg( x, integer )
			#line 130 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 A$3;
			#line 130 "FUNCTIONS\\VA_CVA_API.BAS"
			A$3 = __builtin_va_arg( *(__builtin_va_list*)&X$1, int64);
			// 			CU_ASSERT( a = i * 100 ) [Macro Expansion: fbcu.CU_ASSERT_( (a = i * 100), __FILE__,$"functions\va_cva_api.bas", __LINE__,131, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_LIST", "CU_ASSERT(" $"a = i * 100" ")" ) ]
			#line 131 "FUNCTIONS\\VA_CVA_API.BAS"
			_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(A$3 == (I$2 * 100ll)) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 131, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_LIST", (char*)"CU_ASSERT(a = i * 100)" );
			// 		next i
		}
		#line 132 "FUNCTIONS\\VA_CVA_API.BAS"
		label$35:;
		#line 132 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = I$2 + 1ll;
		#line 132 "FUNCTIONS\\VA_CVA_API.BAS"
		label$34:;
		#line 132 "FUNCTIONS\\VA_CVA_API.BAS"
		if( I$2 <= TMP$30$2) goto label$37;
		#line 132 "FUNCTIONS\\VA_CVA_API.BAS"
		label$36:;
	}
	// 		cva_end( x )
	#line 134 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&X$1);
	#line 136 "FUNCTIONS\\VA_CVA_API.BAS"
	label$33:;
#line 136 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 138 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API4LISTEv( void )
#line 138 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 138 "FUNCTIONS\\VA_CVA_API.BAS"
	label$38:;
	// #else
	// TEST_EMIT( fbcu_global, TMP_FBCUNIT_TEST_NAME list )sub list cdecl () FBCU_TRACE( "TEST" list ) #endif ]
	// 		f_test_list( 10, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000 )
	#line 139 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API11F_TEST_LISTEu7INTEGERz( 10ll, 100ll, 200ll, 300ll, 400ll, 500ll, 600ll, 700ll, 800ll, 900ll, 1000ll );
	// 	END_TEST [Macro Expansion: #if defined( TMP_FBCUNIT_TEST_NAME )
	// #if defined( TMP_FBCUNIT_SUITE_NAME )
	// #if defined( TMP_FBCUNIT_TEST_GROUP_NAME )
	// END_TEST_EMIT( TMP_FBCUNIT_SUITE_NAME,fbc_tests.functions.va_cva_api, TMP_FBCUNIT_TEST_GROUP_NAME, TMP_FBCUNIT_TEST_NAME,list, false )end sub private sub list_ctor cdecl () constructor #if $"TMP_FBCUNIT_TEST_GROUP_NAME" > "" fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"TMP_FBCUNIT_TEST_GROUP_NAME" + "." + $"list", procptr(list), false ) #else fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"list", procptr(list), false ) #endif end sub FBCU_TRACE( "END_TEST" list ) #else
	#line 140 "FUNCTIONS\\VA_CVA_API.BAS"
	label$39:;
#line 140 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 175 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API16F_TEST_ARG_UBYTEEu7INTEGERz( int64 N$1, ... )
#line 175 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 175 "FUNCTIONS\\VA_CVA_API.BAS"
	label$42:;
	// dim as cva_list x = any
	#line 175 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list X$1;
	// cva_start( x, n )
	#line 175 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_start( *(__builtin_va_list*)&X$1, N$1);
	// CU_ASSERT_EQUAL( cva_arg( x, ubyte ), 1 )fbcu.CU_ASSERT_( ((cva_arg( x, ubyte ))=(1)), __FILE__,$"functions\va_cva_api.bas", __LINE__,175, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UBYTE", "CU_ASSERT_EQUAL(" $"cva_arg( x, ubyte )" "," $"1" ")" )
	#line 175 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint8)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 1ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 175, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UBYTE", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ubyte ),1)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ubyte ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, ubyte ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,175, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UBYTE", "CU_ASSERT_EQUAL(" $"cva_arg( x, ubyte )" "," $"0" ")" )
	#line 175 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint8)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 175, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UBYTE", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ubyte ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ubyte ), 127 )fbcu.CU_ASSERT_( ((cva_arg( x, ubyte ))=(127)), __FILE__,$"functions\va_cva_api.bas", __LINE__,175, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UBYTE", "CU_ASSERT_EQUAL(" $"cva_arg( x, ubyte )" "," $"127" ")" )
	#line 175 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint8)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 127ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 175, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UBYTE", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ubyte ),127)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ubyte ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, ubyte ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,175, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UBYTE", "CU_ASSERT_EQUAL(" $"cva_arg( x, ubyte )" "," $"0" ")" )
	#line 175 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint8)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 175, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UBYTE", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ubyte ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ubyte ), 255 )fbcu.CU_ASSERT_( ((cva_arg( x, ubyte ))=(255)), __FILE__,$"functions\va_cva_api.bas", __LINE__,175, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UBYTE", "CU_ASSERT_EQUAL(" $"cva_arg( x, ubyte )" "," $"255" ")" )
	#line 175 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint8)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 255ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 175, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UBYTE", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ubyte ),255)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ubyte ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, ubyte ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,175, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UBYTE", "CU_ASSERT_EQUAL(" $"cva_arg( x, ubyte )" "," $"0" ")" )
	#line 175 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint8)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 175, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UBYTE", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ubyte ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ubyte ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, ubyte ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,175, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UBYTE", "CU_ASSERT_EQUAL(" $"cva_arg( x, ubyte )" "," $"0" ")" )
	#line 175 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint8)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 175, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UBYTE", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ubyte ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ubyte ), 127 )fbcu.CU_ASSERT_( ((cva_arg( x, ubyte ))=(127)), __FILE__,$"functions\va_cva_api.bas", __LINE__,175, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UBYTE", "CU_ASSERT_EQUAL(" $"cva_arg( x, ubyte )" "," $"127" ")" )
	#line 175 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint8)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 127ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 175, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UBYTE", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ubyte ),127)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ubyte ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, ubyte ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,175, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UBYTE", "CU_ASSERT_EQUAL(" $"cva_arg( x, ubyte )" "," $"0" ")" )
	#line 175 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint8)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 175, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UBYTE", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ubyte ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ubyte ), 255 )fbcu.CU_ASSERT_( ((cva_arg( x, ubyte ))=(255)), __FILE__,$"functions\va_cva_api.bas", __LINE__,175, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UBYTE", "CU_ASSERT_EQUAL(" $"cva_arg( x, ubyte )" "," $"255" ")" )
	#line 175 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint8)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 255ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 175, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UBYTE", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ubyte ),255)" );
	// cva_end( x )
	#line 175 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&X$1);
	#line 175 "FUNCTIONS\\VA_CVA_API.BAS"
	label$43:;
#line 175 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 176 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API15F_TEST_ARG_BYTEEu7INTEGERz( int64 N$1, ... )
#line 176 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 176 "FUNCTIONS\\VA_CVA_API.BAS"
	label$44:;
	// dim as cva_list x = any
	#line 176 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list X$1;
	// cva_start( x, n )
	#line 176 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_start( *(__builtin_va_list*)&X$1, N$1);
	// CU_ASSERT_EQUAL( cva_arg( x, byte ), 2 )fbcu.CU_ASSERT_( ((cva_arg( x, byte ))=(2)), __FILE__,$"functions\va_cva_api.bas", __LINE__,176, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_BYTE", "CU_ASSERT_EQUAL(" $"cva_arg( x, byte )" "," $"2" ")" )
	#line 176 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int8)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 2ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 176, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_BYTE", (char*)"CU_ASSERT_EQUAL(cva_arg( x, byte ),2)" );
	// CU_ASSERT_EQUAL( cva_arg( x, byte ), -128 )fbcu.CU_ASSERT_( ((cva_arg( x, byte ))=(-128)), __FILE__,$"functions\va_cva_api.bas", __LINE__,176, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_BYTE", "CU_ASSERT_EQUAL(" $"cva_arg( x, byte )" "," $"-128" ")" )
	#line 176 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int8)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == -128ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 176, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_BYTE", (char*)"CU_ASSERT_EQUAL(cva_arg( x, byte ),-128)" );
	// CU_ASSERT_EQUAL( cva_arg( x, byte ), 127 )fbcu.CU_ASSERT_( ((cva_arg( x, byte ))=(127)), __FILE__,$"functions\va_cva_api.bas", __LINE__,176, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_BYTE", "CU_ASSERT_EQUAL(" $"cva_arg( x, byte )" "," $"127" ")" )
	#line 176 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int8)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 127ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 176, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_BYTE", (char*)"CU_ASSERT_EQUAL(cva_arg( x, byte ),127)" );
	// CU_ASSERT_EQUAL( cva_arg( x, byte ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, byte ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,176, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_BYTE", "CU_ASSERT_EQUAL(" $"cva_arg( x, byte )" "," $"0" ")" )
	#line 176 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int8)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 176, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_BYTE", (char*)"CU_ASSERT_EQUAL(cva_arg( x, byte ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, byte ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, byte ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,176, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_BYTE", "CU_ASSERT_EQUAL(" $"cva_arg( x, byte )" "," $"0" ")" )
	#line 176 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int8)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 176, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_BYTE", (char*)"CU_ASSERT_EQUAL(cva_arg( x, byte ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, byte ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, byte ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,176, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_BYTE", "CU_ASSERT_EQUAL(" $"cva_arg( x, byte )" "," $"0" ")" )
	#line 176 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int8)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 176, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_BYTE", (char*)"CU_ASSERT_EQUAL(cva_arg( x, byte ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, byte ), -128 )fbcu.CU_ASSERT_( ((cva_arg( x, byte ))=(-128)), __FILE__,$"functions\va_cva_api.bas", __LINE__,176, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_BYTE", "CU_ASSERT_EQUAL(" $"cva_arg( x, byte )" "," $"-128" ")" )
	#line 176 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int8)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == -128ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 176, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_BYTE", (char*)"CU_ASSERT_EQUAL(cva_arg( x, byte ),-128)" );
	// CU_ASSERT_EQUAL( cva_arg( x, byte ), 127 )fbcu.CU_ASSERT_( ((cva_arg( x, byte ))=(127)), __FILE__,$"functions\va_cva_api.bas", __LINE__,176, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_BYTE", "CU_ASSERT_EQUAL(" $"cva_arg( x, byte )" "," $"127" ")" )
	#line 176 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int8)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 127ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 176, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_BYTE", (char*)"CU_ASSERT_EQUAL(cva_arg( x, byte ),127)" );
	// CU_ASSERT_EQUAL( cva_arg( x, byte ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, byte ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,176, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_BYTE", "CU_ASSERT_EQUAL(" $"cva_arg( x, byte )" "," $"0" ")" )
	#line 176 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int8)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 176, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_BYTE", (char*)"CU_ASSERT_EQUAL(cva_arg( x, byte ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, byte ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, byte ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,176, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_BYTE", "CU_ASSERT_EQUAL(" $"cva_arg( x, byte )" "," $"0" ")" )
	#line 176 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int8)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 176, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_BYTE", (char*)"CU_ASSERT_EQUAL(cva_arg( x, byte ),0)" );
	// cva_end( x )
	#line 176 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&X$1);
	#line 176 "FUNCTIONS\\VA_CVA_API.BAS"
	label$45:;
#line 176 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 178 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API17F_TEST_ARG_USHORTEu7INTEGERz( int64 N$1, ... )
#line 178 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 178 "FUNCTIONS\\VA_CVA_API.BAS"
	label$46:;
	// dim as cva_list x = any
	#line 178 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list X$1;
	// cva_start( x, n )
	#line 178 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_start( *(__builtin_va_list*)&X$1, N$1);
	// CU_ASSERT_EQUAL( cva_arg( x, ushort ), 3 )fbcu.CU_ASSERT_( ((cva_arg( x, ushort ))=(3)), __FILE__,$"functions\va_cva_api.bas", __LINE__,178, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_USHORT", "CU_ASSERT_EQUAL(" $"cva_arg( x, ushort )" "," $"3" ")" )
	#line 178 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint16)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 3ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 178, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_USHORT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ushort ),3)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ushort ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, ushort ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,178, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_USHORT", "CU_ASSERT_EQUAL(" $"cva_arg( x, ushort )" "," $"0" ")" )
	#line 178 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint16)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 178, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_USHORT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ushort ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ushort ), 32767 )fbcu.CU_ASSERT_( ((cva_arg( x, ushort ))=(32767)), __FILE__,$"functions\va_cva_api.bas", __LINE__,178, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_USHORT", "CU_ASSERT_EQUAL(" $"cva_arg( x, ushort )" "," $"32767" ")" )
	#line 178 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint16)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 32767ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 178, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_USHORT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ushort ),32767)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ushort ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, ushort ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,178, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_USHORT", "CU_ASSERT_EQUAL(" $"cva_arg( x, ushort )" "," $"0" ")" )
	#line 178 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint16)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 178, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_USHORT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ushort ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ushort ), 65535 )fbcu.CU_ASSERT_( ((cva_arg( x, ushort ))=(65535)), __FILE__,$"functions\va_cva_api.bas", __LINE__,178, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_USHORT", "CU_ASSERT_EQUAL(" $"cva_arg( x, ushort )" "," $"65535" ")" )
	#line 178 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint16)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 65535ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 178, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_USHORT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ushort ),65535)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ushort ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, ushort ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,178, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_USHORT", "CU_ASSERT_EQUAL(" $"cva_arg( x, ushort )" "," $"0" ")" )
	#line 178 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint16)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 178, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_USHORT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ushort ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ushort ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, ushort ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,178, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_USHORT", "CU_ASSERT_EQUAL(" $"cva_arg( x, ushort )" "," $"0" ")" )
	#line 178 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint16)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 178, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_USHORT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ushort ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ushort ), 32767 )fbcu.CU_ASSERT_( ((cva_arg( x, ushort ))=(32767)), __FILE__,$"functions\va_cva_api.bas", __LINE__,178, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_USHORT", "CU_ASSERT_EQUAL(" $"cva_arg( x, ushort )" "," $"32767" ")" )
	#line 178 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint16)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 32767ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 178, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_USHORT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ushort ),32767)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ushort ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, ushort ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,178, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_USHORT", "CU_ASSERT_EQUAL(" $"cva_arg( x, ushort )" "," $"0" ")" )
	#line 178 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint16)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 178, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_USHORT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ushort ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ushort ), 65535 )fbcu.CU_ASSERT_( ((cva_arg( x, ushort ))=(65535)), __FILE__,$"functions\va_cva_api.bas", __LINE__,178, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_USHORT", "CU_ASSERT_EQUAL(" $"cva_arg( x, ushort )" "," $"65535" ")" )
	#line 178 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint16)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 65535ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 178, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_USHORT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ushort ),65535)" );
	// cva_end( x )
	#line 178 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&X$1);
	#line 178 "FUNCTIONS\\VA_CVA_API.BAS"
	label$47:;
#line 178 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 179 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API16F_TEST_ARG_SHORTEu7INTEGERz( int64 N$1, ... )
#line 179 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 179 "FUNCTIONS\\VA_CVA_API.BAS"
	label$48:;
	// dim as cva_list x = any
	#line 179 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list X$1;
	// cva_start( x, n )
	#line 179 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_start( *(__builtin_va_list*)&X$1, N$1);
	// CU_ASSERT_EQUAL( cva_arg( x, short ), 4 )fbcu.CU_ASSERT_( ((cva_arg( x, short ))=(4)), __FILE__,$"functions\va_cva_api.bas", __LINE__,179, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_SHORT", "CU_ASSERT_EQUAL(" $"cva_arg( x, short )" "," $"4" ")" )
	#line 179 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int16)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 4ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 179, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_SHORT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, short ),4)" );
	// CU_ASSERT_EQUAL( cva_arg( x, short ), -32768 )fbcu.CU_ASSERT_( ((cva_arg( x, short ))=(-32768)), __FILE__,$"functions\va_cva_api.bas", __LINE__,179, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_SHORT", "CU_ASSERT_EQUAL(" $"cva_arg( x, short )" "," $"-32768" ")" )
	#line 179 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int16)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == -32768ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 179, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_SHORT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, short ),-32768)" );
	// CU_ASSERT_EQUAL( cva_arg( x, short ), 32767 )fbcu.CU_ASSERT_( ((cva_arg( x, short ))=(32767)), __FILE__,$"functions\va_cva_api.bas", __LINE__,179, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_SHORT", "CU_ASSERT_EQUAL(" $"cva_arg( x, short )" "," $"32767" ")" )
	#line 179 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int16)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 32767ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 179, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_SHORT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, short ),32767)" );
	// CU_ASSERT_EQUAL( cva_arg( x, short ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, short ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,179, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_SHORT", "CU_ASSERT_EQUAL(" $"cva_arg( x, short )" "," $"0" ")" )
	#line 179 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int16)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 179, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_SHORT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, short ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, short ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, short ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,179, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_SHORT", "CU_ASSERT_EQUAL(" $"cva_arg( x, short )" "," $"0" ")" )
	#line 179 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int16)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 179, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_SHORT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, short ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, short ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, short ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,179, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_SHORT", "CU_ASSERT_EQUAL(" $"cva_arg( x, short )" "," $"0" ")" )
	#line 179 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int16)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 179, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_SHORT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, short ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, short ), -32768 )fbcu.CU_ASSERT_( ((cva_arg( x, short ))=(-32768)), __FILE__,$"functions\va_cva_api.bas", __LINE__,179, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_SHORT", "CU_ASSERT_EQUAL(" $"cva_arg( x, short )" "," $"-32768" ")" )
	#line 179 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int16)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == -32768ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 179, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_SHORT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, short ),-32768)" );
	// CU_ASSERT_EQUAL( cva_arg( x, short ), 32767 )fbcu.CU_ASSERT_( ((cva_arg( x, short ))=(32767)), __FILE__,$"functions\va_cva_api.bas", __LINE__,179, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_SHORT", "CU_ASSERT_EQUAL(" $"cva_arg( x, short )" "," $"32767" ")" )
	#line 179 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int16)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 32767ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 179, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_SHORT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, short ),32767)" );
	// CU_ASSERT_EQUAL( cva_arg( x, short ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, short ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,179, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_SHORT", "CU_ASSERT_EQUAL(" $"cva_arg( x, short )" "," $"0" ")" )
	#line 179 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int16)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 179, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_SHORT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, short ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, short ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, short ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,179, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_SHORT", "CU_ASSERT_EQUAL(" $"cva_arg( x, short )" "," $"0" ")" )
	#line 179 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int16)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 179, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_SHORT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, short ),0)" );
	// cva_end( x )
	#line 179 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&X$1);
	#line 179 "FUNCTIONS\\VA_CVA_API.BAS"
	label$49:;
#line 179 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 181 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API16F_TEST_ARG_ULONGEu7INTEGERz( int64 N$1, ... )
#line 181 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 181 "FUNCTIONS\\VA_CVA_API.BAS"
	label$50:;
	// dim as cva_list x = any
	#line 181 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list X$1;
	// cva_start( x, n )
	#line 181 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_start( *(__builtin_va_list*)&X$1, N$1);
	// CU_ASSERT_EQUAL( cva_arg( x, ulong ), 5 )fbcu.CU_ASSERT_( ((cva_arg( x, ulong ))=(5)), __FILE__,$"functions\va_cva_api.bas", __LINE__,181, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONG", "CU_ASSERT_EQUAL(" $"cva_arg( x, ulong )" "," $"5" ")" )
	#line 181 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint32)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 5ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 181, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONG", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ulong ),5)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ulong ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, ulong ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,181, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONG", "CU_ASSERT_EQUAL(" $"cva_arg( x, ulong )" "," $"0" ")" )
	#line 181 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint32)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 181, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONG", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ulong ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ulong ), 2147483647 )fbcu.CU_ASSERT_( ((cva_arg( x, ulong ))=(2147483647)), __FILE__,$"functions\va_cva_api.bas", __LINE__,181, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONG", "CU_ASSERT_EQUAL(" $"cva_arg( x, ulong )" "," $"2147483647" ")" )
	#line 181 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint32)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 2147483647ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 181, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONG", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ulong ),2147483647)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ulong ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, ulong ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,181, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONG", "CU_ASSERT_EQUAL(" $"cva_arg( x, ulong )" "," $"0" ")" )
	#line 181 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint32)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 181, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONG", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ulong ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ulong ), 4294967295 )fbcu.CU_ASSERT_( ((cva_arg( x, ulong ))=(4294967295)), __FILE__,$"functions\va_cva_api.bas", __LINE__,181, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONG", "CU_ASSERT_EQUAL(" $"cva_arg( x, ulong )" "," $"4294967295" ")" )
	#line 181 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((uint64)(int64)(uint32)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 4294967295ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 181, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONG", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ulong ),4294967295)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ulong ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, ulong ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,181, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONG", "CU_ASSERT_EQUAL(" $"cva_arg( x, ulong )" "," $"0" ")" )
	#line 181 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint32)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 181, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONG", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ulong ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ulong ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, ulong ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,181, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONG", "CU_ASSERT_EQUAL(" $"cva_arg( x, ulong )" "," $"0" ")" )
	#line 181 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint32)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 181, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONG", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ulong ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ulong ), 2147483647 )fbcu.CU_ASSERT_( ((cva_arg( x, ulong ))=(2147483647)), __FILE__,$"functions\va_cva_api.bas", __LINE__,181, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONG", "CU_ASSERT_EQUAL(" $"cva_arg( x, ulong )" "," $"2147483647" ")" )
	#line 181 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint32)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 2147483647ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 181, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONG", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ulong ),2147483647)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ulong ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, ulong ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,181, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONG", "CU_ASSERT_EQUAL(" $"cva_arg( x, ulong )" "," $"0" ")" )
	#line 181 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(uint32)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 181, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONG", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ulong ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ulong ), 4294967295 )fbcu.CU_ASSERT_( ((cva_arg( x, ulong ))=(4294967295)), __FILE__,$"functions\va_cva_api.bas", __LINE__,181, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONG", "CU_ASSERT_EQUAL(" $"cva_arg( x, ulong )" "," $"4294967295" ")" )
	#line 181 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((uint64)(int64)(uint32)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 4294967295ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 181, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONG", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ulong ),4294967295)" );
	// cva_end( x )
	#line 181 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&X$1);
	#line 181 "FUNCTIONS\\VA_CVA_API.BAS"
	label$51:;
#line 181 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 182 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API15F_TEST_ARG_LONGEu7INTEGERz( int64 N$1, ... )
#line 182 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 182 "FUNCTIONS\\VA_CVA_API.BAS"
	label$52:;
	// dim as cva_list x = any
	#line 182 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list X$1;
	// cva_start( x, n )
	#line 182 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_start( *(__builtin_va_list*)&X$1, N$1);
	// CU_ASSERT_EQUAL( cva_arg( x, long ), 6 )fbcu.CU_ASSERT_( ((cva_arg( x, long ))=(6)), __FILE__,$"functions\va_cva_api.bas", __LINE__,182, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONG", "CU_ASSERT_EQUAL(" $"cva_arg( x, long )" "," $"6" ")" )
	#line 182 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int32)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 6ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 182, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONG", (char*)"CU_ASSERT_EQUAL(cva_arg( x, long ),6)" );
	// CU_ASSERT_EQUAL( cva_arg( x, long ), -2147483648 )fbcu.CU_ASSERT_( ((cva_arg( x, long ))=(-2147483648)), __FILE__,$"functions\va_cva_api.bas", __LINE__,182, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONG", "CU_ASSERT_EQUAL(" $"cva_arg( x, long )" "," $"-2147483648" ")" )
	#line 182 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int32)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == -2147483648ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 182, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONG", (char*)"CU_ASSERT_EQUAL(cva_arg( x, long ),-2147483648)" );
	// CU_ASSERT_EQUAL( cva_arg( x, long ), 2147483647 )fbcu.CU_ASSERT_( ((cva_arg( x, long ))=(2147483647)), __FILE__,$"functions\va_cva_api.bas", __LINE__,182, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONG", "CU_ASSERT_EQUAL(" $"cva_arg( x, long )" "," $"2147483647" ")" )
	#line 182 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int32)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 2147483647ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 182, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONG", (char*)"CU_ASSERT_EQUAL(cva_arg( x, long ),2147483647)" );
	// CU_ASSERT_EQUAL( cva_arg( x, long ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, long ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,182, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONG", "CU_ASSERT_EQUAL(" $"cva_arg( x, long )" "," $"0" ")" )
	#line 182 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int32)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 182, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONG", (char*)"CU_ASSERT_EQUAL(cva_arg( x, long ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, long ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, long ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,182, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONG", "CU_ASSERT_EQUAL(" $"cva_arg( x, long )" "," $"0" ")" )
	#line 182 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int32)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 182, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONG", (char*)"CU_ASSERT_EQUAL(cva_arg( x, long ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, long ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, long ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,182, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONG", "CU_ASSERT_EQUAL(" $"cva_arg( x, long )" "," $"0" ")" )
	#line 182 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int32)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 182, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONG", (char*)"CU_ASSERT_EQUAL(cva_arg( x, long ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, long ), -2147483648 )fbcu.CU_ASSERT_( ((cva_arg( x, long ))=(-2147483648)), __FILE__,$"functions\va_cva_api.bas", __LINE__,182, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONG", "CU_ASSERT_EQUAL(" $"cva_arg( x, long )" "," $"-2147483648" ")" )
	#line 182 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int32)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == -2147483648ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 182, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONG", (char*)"CU_ASSERT_EQUAL(cva_arg( x, long ),-2147483648)" );
	// CU_ASSERT_EQUAL( cva_arg( x, long ), 2147483647 )fbcu.CU_ASSERT_( ((cva_arg( x, long ))=(2147483647)), __FILE__,$"functions\va_cva_api.bas", __LINE__,182, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONG", "CU_ASSERT_EQUAL(" $"cva_arg( x, long )" "," $"2147483647" ")" )
	#line 182 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int32)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 2147483647ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 182, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONG", (char*)"CU_ASSERT_EQUAL(cva_arg( x, long ),2147483647)" );
	// CU_ASSERT_EQUAL( cva_arg( x, long ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, long ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,182, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONG", "CU_ASSERT_EQUAL(" $"cva_arg( x, long )" "," $"0" ")" )
	#line 182 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int32)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 182, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONG", (char*)"CU_ASSERT_EQUAL(cva_arg( x, long ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, long ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, long ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,182, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONG", "CU_ASSERT_EQUAL(" $"cva_arg( x, long )" "," $"0" ")" )
	#line 182 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-((int64)(int32)__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 182, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONG", (char*)"CU_ASSERT_EQUAL(cva_arg( x, long ),0)" );
	// cva_end( x )
	#line 182 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&X$1);
	#line 182 "FUNCTIONS\\VA_CVA_API.BAS"
	label$53:;
#line 182 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 184 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API19F_TEST_ARG_UINTEGEREu7INTEGERz( int64 N$1, ... )
#line 184 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 184 "FUNCTIONS\\VA_CVA_API.BAS"
	label$54:;
	// dim as cva_list x = any
	#line 184 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list X$1;
	// cva_start( x, n )
	#line 184 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_start( *(__builtin_va_list*)&X$1, N$1);
	// CU_ASSERT_EQUAL( cva_arg( x, uinteger ), 7 )fbcu.CU_ASSERT_( ((cva_arg( x, uinteger ))=(7)), __FILE__,$"functions\va_cva_api.bas", __LINE__,184, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UINTEGER", "CU_ASSERT_EQUAL(" $"cva_arg( x, uinteger )" "," $"7" ")" )
	#line 184 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, uint64) == 7ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 184, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UINTEGER", (char*)"CU_ASSERT_EQUAL(cva_arg( x, uinteger ),7)" );
	// CU_ASSERT_EQUAL( cva_arg( x, uinteger ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, uinteger ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,184, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UINTEGER", "CU_ASSERT_EQUAL(" $"cva_arg( x, uinteger )" "," $"0" ")" )
	#line 184 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, uint64) == 0ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 184, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UINTEGER", (char*)"CU_ASSERT_EQUAL(cva_arg( x, uinteger ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, uinteger ), 9223372036854775807ll )fbcu.CU_ASSERT_( ((cva_arg( x, uinteger ))=(9223372036854775807ll)), __FILE__,$"functions\va_cva_api.bas", __LINE__,184, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UINTEGER", "CU_ASSERT_EQUAL(" $"cva_arg( x, uinteger )" "," $"9223372036854775807ll" ")" )
	#line 184 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, uint64) == 9223372036854775807ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 184, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UINTEGER", (char*)"CU_ASSERT_EQUAL(cva_arg( x, uinteger ),9223372036854775807ll)" );
	// CU_ASSERT_EQUAL( cva_arg( x, uinteger ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, uinteger ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,184, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UINTEGER", "CU_ASSERT_EQUAL(" $"cva_arg( x, uinteger )" "," $"0" ")" )
	#line 184 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, uint64) == 0ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 184, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UINTEGER", (char*)"CU_ASSERT_EQUAL(cva_arg( x, uinteger ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, uinteger ), 18446744073709551615ull )fbcu.CU_ASSERT_( ((cva_arg( x, uinteger ))=(18446744073709551615ull)), __FILE__,$"functions\va_cva_api.bas", __LINE__,184, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UINTEGER", "CU_ASSERT_EQUAL(" $"cva_arg( x, uinteger )" "," $"18446744073709551615ull" ")" )
	#line 184 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, uint64) == 18446744073709551615ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 184, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UINTEGER", (char*)"CU_ASSERT_EQUAL(cva_arg( x, uinteger ),18446744073709551615ull)" );
	// CU_ASSERT_EQUAL( cva_arg( x, uinteger ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, uinteger ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,184, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UINTEGER", "CU_ASSERT_EQUAL(" $"cva_arg( x, uinteger )" "," $"0" ")" )
	#line 184 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, uint64) == 0ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 184, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UINTEGER", (char*)"CU_ASSERT_EQUAL(cva_arg( x, uinteger ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, uinteger ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, uinteger ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,184, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UINTEGER", "CU_ASSERT_EQUAL(" $"cva_arg( x, uinteger )" "," $"0" ")" )
	#line 184 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, uint64) == 0ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 184, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UINTEGER", (char*)"CU_ASSERT_EQUAL(cva_arg( x, uinteger ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, uinteger ), 9223372036854775807ll )fbcu.CU_ASSERT_( ((cva_arg( x, uinteger ))=(9223372036854775807ll)), __FILE__,$"functions\va_cva_api.bas", __LINE__,184, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UINTEGER", "CU_ASSERT_EQUAL(" $"cva_arg( x, uinteger )" "," $"9223372036854775807ll" ")" )
	#line 184 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, uint64) == 9223372036854775807ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 184, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UINTEGER", (char*)"CU_ASSERT_EQUAL(cva_arg( x, uinteger ),9223372036854775807ll)" );
	// CU_ASSERT_EQUAL( cva_arg( x, uinteger ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, uinteger ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,184, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UINTEGER", "CU_ASSERT_EQUAL(" $"cva_arg( x, uinteger )" "," $"0" ")" )
	#line 184 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, uint64) == 0ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 184, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UINTEGER", (char*)"CU_ASSERT_EQUAL(cva_arg( x, uinteger ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, uinteger ), 18446744073709551615ull )fbcu.CU_ASSERT_( ((cva_arg( x, uinteger ))=(18446744073709551615ull)), __FILE__,$"functions\va_cva_api.bas", __LINE__,184, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UINTEGER", "CU_ASSERT_EQUAL(" $"cva_arg( x, uinteger )" "," $"18446744073709551615ull" ")" )
	#line 184 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, uint64) == 18446744073709551615ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 184, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_UINTEGER", (char*)"CU_ASSERT_EQUAL(cva_arg( x, uinteger ),18446744073709551615ull)" );
	// cva_end( x )
	#line 184 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&X$1);
	#line 184 "FUNCTIONS\\VA_CVA_API.BAS"
	label$55:;
#line 184 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 185 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API18F_TEST_ARG_INTEGEREu7INTEGERz( int64 N$1, ... )
#line 185 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 185 "FUNCTIONS\\VA_CVA_API.BAS"
	label$56:;
	// dim as cva_list x = any
	#line 185 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list X$1;
	// cva_start( x, n )
	#line 185 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_start( *(__builtin_va_list*)&X$1, N$1);
	// CU_ASSERT_EQUAL( cva_arg( x, integer ), 8 )fbcu.CU_ASSERT_( ((cva_arg( x, integer ))=(8)), __FILE__,$"functions\va_cva_api.bas", __LINE__,185, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_INTEGER", "CU_ASSERT_EQUAL(" $"cva_arg( x, integer )" "," $"8" ")" )
	#line 185 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 8ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 185, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_INTEGER", (char*)"CU_ASSERT_EQUAL(cva_arg( x, integer ),8)" );
	// CU_ASSERT_EQUAL( cva_arg( x, integer ), (-9223372036854775807ll-1ll) )fbcu.CU_ASSERT_( ((cva_arg( x, integer ))=((-9223372036854775807ll-1ll))), __FILE__,$"functions\va_cva_api.bas", __LINE__,185, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_INTEGER", "CU_ASSERT_EQUAL(" $"cva_arg( x, integer )" "," $"(-9223372036854775807ll-1ll)" ")" )
	#line 185 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == (int64)-9223372036854775808ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 185, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_INTEGER", (char*)"CU_ASSERT_EQUAL(cva_arg( x, integer ),(-9223372036854775807ll-1ll))" );
	// CU_ASSERT_EQUAL( cva_arg( x, integer ), 9223372036854775807ll )fbcu.CU_ASSERT_( ((cva_arg( x, integer ))=(9223372036854775807ll)), __FILE__,$"functions\va_cva_api.bas", __LINE__,185, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_INTEGER", "CU_ASSERT_EQUAL(" $"cva_arg( x, integer )" "," $"9223372036854775807ll" ")" )
	#line 185 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 9223372036854775807ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 185, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_INTEGER", (char*)"CU_ASSERT_EQUAL(cva_arg( x, integer ),9223372036854775807ll)" );
	// CU_ASSERT_EQUAL( cva_arg( x, integer ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, integer ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,185, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_INTEGER", "CU_ASSERT_EQUAL(" $"cva_arg( x, integer )" "," $"0" ")" )
	#line 185 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 185, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_INTEGER", (char*)"CU_ASSERT_EQUAL(cva_arg( x, integer ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, integer ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, integer ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,185, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_INTEGER", "CU_ASSERT_EQUAL(" $"cva_arg( x, integer )" "," $"0" ")" )
	#line 185 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 185, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_INTEGER", (char*)"CU_ASSERT_EQUAL(cva_arg( x, integer ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, integer ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, integer ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,185, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_INTEGER", "CU_ASSERT_EQUAL(" $"cva_arg( x, integer )" "," $"0" ")" )
	#line 185 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 185, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_INTEGER", (char*)"CU_ASSERT_EQUAL(cva_arg( x, integer ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, integer ), (-9223372036854775807ll-1ll) )fbcu.CU_ASSERT_( ((cva_arg( x, integer ))=((-9223372036854775807ll-1ll))), __FILE__,$"functions\va_cva_api.bas", __LINE__,185, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_INTEGER", "CU_ASSERT_EQUAL(" $"cva_arg( x, integer )" "," $"(-9223372036854775807ll-1ll)" ")" )
	#line 185 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == (int64)-9223372036854775808ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 185, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_INTEGER", (char*)"CU_ASSERT_EQUAL(cva_arg( x, integer ),(-9223372036854775807ll-1ll))" );
	// CU_ASSERT_EQUAL( cva_arg( x, integer ), 9223372036854775807ll )fbcu.CU_ASSERT_( ((cva_arg( x, integer ))=(9223372036854775807ll)), __FILE__,$"functions\va_cva_api.bas", __LINE__,185, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_INTEGER", "CU_ASSERT_EQUAL(" $"cva_arg( x, integer )" "," $"9223372036854775807ll" ")" )
	#line 185 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 9223372036854775807ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 185, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_INTEGER", (char*)"CU_ASSERT_EQUAL(cva_arg( x, integer ),9223372036854775807ll)" );
	// CU_ASSERT_EQUAL( cva_arg( x, integer ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, integer ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,185, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_INTEGER", "CU_ASSERT_EQUAL(" $"cva_arg( x, integer )" "," $"0" ")" )
	#line 185 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 185, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_INTEGER", (char*)"CU_ASSERT_EQUAL(cva_arg( x, integer ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, integer ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, integer ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,185, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_INTEGER", "CU_ASSERT_EQUAL(" $"cva_arg( x, integer )" "," $"0" ")" )
	#line 185 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 185, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_INTEGER", (char*)"CU_ASSERT_EQUAL(cva_arg( x, integer ),0)" );
	// cva_end( x )
	#line 185 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&X$1);
	#line 185 "FUNCTIONS\\VA_CVA_API.BAS"
	label$57:;
#line 185 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 187 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API19F_TEST_ARG_ULONGINTEu7INTEGERz( int64 N$1, ... )
#line 187 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 187 "FUNCTIONS\\VA_CVA_API.BAS"
	label$58:;
	// dim as cva_list x = any
	#line 187 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list X$1;
	// cva_start( x, n )
	#line 187 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_start( *(__builtin_va_list*)&X$1, N$1);
	// CU_ASSERT_EQUAL( cva_arg( x, ulongint ), 9 )fbcu.CU_ASSERT_( ((cva_arg( x, ulongint ))=(9)), __FILE__,$"functions\va_cva_api.bas", __LINE__,187, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONGINT", "CU_ASSERT_EQUAL(" $"cva_arg( x, ulongint )" "," $"9" ")" )
	#line 187 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, uint64) == 9ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 187, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONGINT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ulongint ),9)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ulongint ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, ulongint ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,187, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONGINT", "CU_ASSERT_EQUAL(" $"cva_arg( x, ulongint )" "," $"0" ")" )
	#line 187 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, uint64) == 0ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 187, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONGINT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ulongint ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ulongint ), 9223372036854775807ll )fbcu.CU_ASSERT_( ((cva_arg( x, ulongint ))=(9223372036854775807ll)), __FILE__,$"functions\va_cva_api.bas", __LINE__,187, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONGINT", "CU_ASSERT_EQUAL(" $"cva_arg( x, ulongint )" "," $"9223372036854775807ll" ")" )
	#line 187 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, uint64) == 9223372036854775807ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 187, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONGINT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ulongint ),9223372036854775807ll)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ulongint ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, ulongint ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,187, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONGINT", "CU_ASSERT_EQUAL(" $"cva_arg( x, ulongint )" "," $"0" ")" )
	#line 187 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, uint64) == 0ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 187, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONGINT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ulongint ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ulongint ), 18446744073709551615ull )fbcu.CU_ASSERT_( ((cva_arg( x, ulongint ))=(18446744073709551615ull)), __FILE__,$"functions\va_cva_api.bas", __LINE__,187, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONGINT", "CU_ASSERT_EQUAL(" $"cva_arg( x, ulongint )" "," $"18446744073709551615ull" ")" )
	#line 187 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, uint64) == 18446744073709551615ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 187, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONGINT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ulongint ),18446744073709551615ull)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ulongint ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, ulongint ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,187, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONGINT", "CU_ASSERT_EQUAL(" $"cva_arg( x, ulongint )" "," $"0" ")" )
	#line 187 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, uint64) == 0ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 187, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONGINT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ulongint ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ulongint ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, ulongint ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,187, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONGINT", "CU_ASSERT_EQUAL(" $"cva_arg( x, ulongint )" "," $"0" ")" )
	#line 187 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, uint64) == 0ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 187, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONGINT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ulongint ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ulongint ), 9223372036854775807ll )fbcu.CU_ASSERT_( ((cva_arg( x, ulongint ))=(9223372036854775807ll)), __FILE__,$"functions\va_cva_api.bas", __LINE__,187, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONGINT", "CU_ASSERT_EQUAL(" $"cva_arg( x, ulongint )" "," $"9223372036854775807ll" ")" )
	#line 187 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, uint64) == 9223372036854775807ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 187, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONGINT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ulongint ),9223372036854775807ll)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ulongint ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, ulongint ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,187, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONGINT", "CU_ASSERT_EQUAL(" $"cva_arg( x, ulongint )" "," $"0" ")" )
	#line 187 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, uint64) == 0ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 187, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONGINT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ulongint ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, ulongint ), 18446744073709551615ull )fbcu.CU_ASSERT_( ((cva_arg( x, ulongint ))=(18446744073709551615ull)), __FILE__,$"functions\va_cva_api.bas", __LINE__,187, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONGINT", "CU_ASSERT_EQUAL(" $"cva_arg( x, ulongint )" "," $"18446744073709551615ull" ")" )
	#line 187 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, uint64) == 18446744073709551615ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 187, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_ULONGINT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, ulongint ),18446744073709551615ull)" );
	// cva_end( x )
	#line 187 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&X$1);
	#line 187 "FUNCTIONS\\VA_CVA_API.BAS"
	label$59:;
#line 187 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 188 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API18F_TEST_ARG_LONGINTEu7INTEGERz( int64 N$1, ... )
#line 188 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 188 "FUNCTIONS\\VA_CVA_API.BAS"
	label$60:;
	// dim as cva_list x = any
	#line 188 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list X$1;
	// cva_start( x, n )
	#line 188 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_start( *(__builtin_va_list*)&X$1, N$1);
	// CU_ASSERT_EQUAL( cva_arg( x, longint ), 10 )fbcu.CU_ASSERT_( ((cva_arg( x, longint ))=(10)), __FILE__,$"functions\va_cva_api.bas", __LINE__,188, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONGINT", "CU_ASSERT_EQUAL(" $"cva_arg( x, longint )" "," $"10" ")" )
	#line 188 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 10ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 188, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONGINT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, longint ),10)" );
	// CU_ASSERT_EQUAL( cva_arg( x, longint ), (-9223372036854775807ll-1ll) )fbcu.CU_ASSERT_( ((cva_arg( x, longint ))=((-9223372036854775807ll-1ll))), __FILE__,$"functions\va_cva_api.bas", __LINE__,188, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONGINT", "CU_ASSERT_EQUAL(" $"cva_arg( x, longint )" "," $"(-9223372036854775807ll-1ll)" ")" )
	#line 188 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == (int64)-9223372036854775808ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 188, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONGINT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, longint ),(-9223372036854775807ll-1ll))" );
	// CU_ASSERT_EQUAL( cva_arg( x, longint ), 9223372036854775807ll )fbcu.CU_ASSERT_( ((cva_arg( x, longint ))=(9223372036854775807ll)), __FILE__,$"functions\va_cva_api.bas", __LINE__,188, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONGINT", "CU_ASSERT_EQUAL(" $"cva_arg( x, longint )" "," $"9223372036854775807ll" ")" )
	#line 188 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 9223372036854775807ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 188, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONGINT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, longint ),9223372036854775807ll)" );
	// CU_ASSERT_EQUAL( cva_arg( x, longint ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, longint ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,188, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONGINT", "CU_ASSERT_EQUAL(" $"cva_arg( x, longint )" "," $"0" ")" )
	#line 188 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 188, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONGINT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, longint ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, longint ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, longint ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,188, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONGINT", "CU_ASSERT_EQUAL(" $"cva_arg( x, longint )" "," $"0" ")" )
	#line 188 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 188, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONGINT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, longint ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, longint ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, longint ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,188, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONGINT", "CU_ASSERT_EQUAL(" $"cva_arg( x, longint )" "," $"0" ")" )
	#line 188 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 188, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONGINT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, longint ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, longint ), (-9223372036854775807ll-1ll) )fbcu.CU_ASSERT_( ((cva_arg( x, longint ))=((-9223372036854775807ll-1ll))), __FILE__,$"functions\va_cva_api.bas", __LINE__,188, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONGINT", "CU_ASSERT_EQUAL(" $"cva_arg( x, longint )" "," $"(-9223372036854775807ll-1ll)" ")" )
	#line 188 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == (int64)-9223372036854775808ull) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 188, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONGINT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, longint ),(-9223372036854775807ll-1ll))" );
	// CU_ASSERT_EQUAL( cva_arg( x, longint ), 9223372036854775807ll )fbcu.CU_ASSERT_( ((cva_arg( x, longint ))=(9223372036854775807ll)), __FILE__,$"functions\va_cva_api.bas", __LINE__,188, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONGINT", "CU_ASSERT_EQUAL(" $"cva_arg( x, longint )" "," $"9223372036854775807ll" ")" )
	#line 188 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 9223372036854775807ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 188, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONGINT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, longint ),9223372036854775807ll)" );
	// CU_ASSERT_EQUAL( cva_arg( x, longint ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, longint ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,188, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONGINT", "CU_ASSERT_EQUAL(" $"cva_arg( x, longint )" "," $"0" ")" )
	#line 188 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 188, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONGINT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, longint ),0)" );
	// CU_ASSERT_EQUAL( cva_arg( x, longint ), 0 )fbcu.CU_ASSERT_( ((cva_arg( x, longint ))=(0)), __FILE__,$"functions\va_cva_api.bas", __LINE__,188, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONGINT", "CU_ASSERT_EQUAL(" $"cva_arg( x, longint )" "," $"0" ")" )
	#line 188 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)&X$1, int64) == 0ll) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 188, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_TEST_ARG_LONGINT", (char*)"CU_ASSERT_EQUAL(cva_arg( x, longint ),0)" );
	// cva_end( x )
	#line 188 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&X$1);
	#line 188 "FUNCTIONS\\VA_CVA_API.BAS"
	label$61:;
#line 188 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 195 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API11BASIC_TYPESEv( void )
#line 195 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 195 "FUNCTIONS\\VA_CVA_API.BAS"
	label$62:;
	// #else
	// TEST_EMIT( fbcu_global, TMP_FBCUNIT_TEST_NAME basic_types )sub basic_types cdecl () FBCU_TRACE( "TEST" basic_types ) #endif ]
	// 		TEST_BASICTYPE ( ubyte, cubyte, 1,   0, SB2, [Macro Expansion: 127 ] , UB1, [Macro Expansion: 0 ] , UB2, [Macro Expansion: 255 ] , 0,   0, SB2, [Macro Expansion: 127 ] , UB1, [Macro Expansion: 0 ] , UB2  [Macro Expansion: 255 ]  ) [Macro Expansion: f_test_arg_ubyte ( 10, cubyte(1), cubyte(0), cubyte(127), cubyte(0), cubyte(255), cubyte(0), cubyte(0), cubyte(127), cubyte(0), cubyte(255) ) ]
	#line 196 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API16F_TEST_ARG_UBYTEEu7INTEGERz( 10ll, 1u, 0u, 127u, 0u, 255u, 0u, 0u, 127u, 0u, 255u );
	// 		TEST_BASICTYPE ( byte , cbyte , 2, SB1, [Macro Expansion: -128 ] , SB2, [Macro Expansion: 127 ] , UB1, [Macro Expansion: 0 ] ,   0, 0, SB1, [Macro Expansion: -128 ] , SB2, [Macro Expansion: 127 ] , UB1, [Macro Expansion: 0 ] ,   0 ) [Macro Expansion: f_test_arg_byte ( 10, cbyte(2), cbyte(-128), cbyte(127), cbyte(0), cbyte(0), cbyte(0), cbyte(-128), cbyte(127), cbyte(0), cbyte(0) ) ]
	#line 197 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API15F_TEST_ARG_BYTEEu7INTEGERz( 10ll, 2, -128, 127, 0, 0, 0, -128, 127, 0, 0 );
	// 		TEST_BASICTYPE ( ushort, cushort, 3,   0, SS2, [Macro Expansion: 32767 ] , US1, [Macro Expansion: 0 ] , US2, [Macro Expansion: 65535 ] , 0,   0, SS2, [Macro Expansion: 32767 ] , US1, [Macro Expansion: 0 ] , US2  [Macro Expansion: 65535 ]  ) [Macro Expansion: f_test_arg_ushort ( 10, cushort(3), cushort(0), cushort(32767), cushort(0), cushort(65535), cushort(0), cushort(0), cushort(32767), cushort(0), cushort(65535) ) ]
	#line 199 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API17F_TEST_ARG_USHORTEu7INTEGERz( 10ll, 3u, 0u, 32767u, 0u, 65535u, 0u, 0u, 32767u, 0u, 65535u );
	// 		TEST_BASICTYPE ( short , cshort , 4, SS1, [Macro Expansion: -32768 ] , SS2, [Macro Expansion: 32767 ] , US1, [Macro Expansion: 0 ] ,   0, 0, SS1, [Macro Expansion: -32768 ] , SS2, [Macro Expansion: 32767 ] , US1, [Macro Expansion: 0 ] ,   0 ) [Macro Expansion: f_test_arg_short ( 10, cshort(4), cshort(-32768), cshort(32767), cshort(0), cshort(0), cshort(0), cshort(-32768), cshort(32767), cshort(0), cshort(0) ) ]
	#line 200 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API16F_TEST_ARG_SHORTEu7INTEGERz( 10ll, 4, -32768, 32767, 0, 0, 0, -32768, 32767, 0, 0 );
	// 		TEST_BASICTYPE ( ulong, culng, 5,   0, SL2, [Macro Expansion: 2147483647 ] , UL1, [Macro Expansion: 0 ] , UL2, [Macro Expansion: 4294967295 ] , 0,   0, SL2, [Macro Expansion: 2147483647 ] , UL1, [Macro Expansion: 0 ] , UL2  [Macro Expansion: 4294967295 ]  ) [Macro Expansion: f_test_arg_ulong ( 10, culng(5), culng(0), culng(2147483647), culng(0), culng(4294967295), culng(0), culng(0), culng(2147483647), culng(0), culng(4294967295) ) ]
	#line 202 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API16F_TEST_ARG_ULONGEu7INTEGERz( 10ll, 5u, 0u, 2147483647u, 0u, 4294967295u, 0u, 0u, 2147483647u, 0u, 4294967295u );
	// 		TEST_BASICTYPE  ( long , clng, 6, SL1, [Macro Expansion: -2147483648 ] , SL2, [Macro Expansion: 2147483647 ] , UL1, [Macro Expansion: 0 ] ,   0, 0, SL1, [Macro Expansion: -2147483648 ] , SL2, [Macro Expansion: 2147483647 ] , UL1, [Macro Expansion: 0 ] ,   0 ) [Macro Expansion: f_test_arg_long ( 10, clng(6), clng(-2147483648), clng(2147483647), clng(0), clng(0), clng(0), clng(-2147483648), clng(2147483647), clng(0), clng(0) ) ]
	#line 203 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API15F_TEST_ARG_LONGEu7INTEGERz( 10ll, 6, (int32)-2147483648u, 2147483647, 0, 0, 0, (int32)-2147483648u, 2147483647, 0, 0 );
	// 		TEST_BASICTYPE ( uinteger, cuint, 7,    0,  SI2, [Macro Expansion: SLL2 ] , [Macro Expansion: 9223372036854775807ll ] ,  UI1, [Macro Expansion: ULL1 ] , [Macro Expansion: 0 ] ,  UI2, [Macro Expansion: ULL2 ] , [Macro Expansion: 18446744073709551615ull ] , 0,    0,  SI2, [Macro Expansion: SLL2 ] , [Macro Expansion: 9223372036854775807ll ] ,  UI1, [Macro Expansion: ULL1 ] , [Macro Expansion: 0 ] ,  UI2  [Macro Expansion: ULL2 ]   [Macro Expansion: 18446744073709551615ull ]  ) [Macro Expansion: f_test_arg_uinteger ( 10, cuint(7), cuint(0), cuint(9223372036854775807ll), cuint(0), cuint(18446744073709551615ull), cuint(0), cuint(0), cuint(9223372036854775807ll), cuint(0), cuint(18446744073709551615ull) ) ]
	#line 205 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API19F_TEST_ARG_UINTEGEREu7INTEGERz( 10ll, 7ull, 0ull, 9223372036854775807ull, 0ull, 18446744073709551615ull, 0ull, 0ull, 9223372036854775807ull, 0ull, 18446744073709551615ull );
	// 		TEST_BASICTYPE ( integer , cint , 8,  SI1, [Macro Expansion: SLL1 ] , [Macro Expansion: (-9223372036854775807ll-1ll) ] ,  SI2, [Macro Expansion: SLL2 ] , [Macro Expansion: 9223372036854775807ll ] ,  UI1, [Macro Expansion: ULL1 ] , [Macro Expansion: 0 ] ,    0, 0,  SI1, [Macro Expansion: SLL1 ] , [Macro Expansion: (-9223372036854775807ll-1ll) ] ,  SI2, [Macro Expansion: SLL2 ] , [Macro Expansion: 9223372036854775807ll ] ,  UI1, [Macro Expansion: ULL1 ] , [Macro Expansion: 0 ] ,    0 ) [Macro Expansion: f_test_arg_integer ( 10, cint(8), cint((-9223372036854775807ll-1ll)), cint(9223372036854775807ll), cint(0), cint(0), cint(0), cint((-9223372036854775807ll-1ll)), cint(9223372036854775807ll), cint(0), cint(0) ) ]
	#line 206 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API18F_TEST_ARG_INTEGEREu7INTEGERz( 10ll, 8ll, (int64)-9223372036854775808ull, 9223372036854775807ll, 0ll, 0ll, 0ll, (int64)-9223372036854775808ull, 9223372036854775807ll, 0ll, 0ll );
	// 		TEST_BASICTYPE ( ulongint, culngint, 9,    0, SLL2, [Macro Expansion: 9223372036854775807ll ] , ULL1, [Macro Expansion: 0 ] , ULL2, [Macro Expansion: 18446744073709551615ull ] , 0,    0, SLL2, [Macro Expansion: 9223372036854775807ll ] , ULL1, [Macro Expansion: 0 ] , ULL2  [Macro Expansion: 18446744073709551615ull ]  ) [Macro Expansion: f_test_arg_ulongint ( 10, culngint(9), culngint(0), culngint(9223372036854775807ll), culngint(0), culngint(18446744073709551615ull), culngint(0), culngint(0), culngint(9223372036854775807ll), culngint(0), culngint(18446744073709551615ull) ) ]
	#line 208 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API19F_TEST_ARG_ULONGINTEu7INTEGERz( 10ll, 9ull, 0ull, 9223372036854775807ull, 0ull, 18446744073709551615ull, 0ull, 0ull, 9223372036854775807ull, 0ull, 18446744073709551615ull );
	// 		TEST_BASICTYPE ( longint , clngint , 10, SLL1, [Macro Expansion: (-9223372036854775807ll-1ll) ] , SLL2, [Macro Expansion: 9223372036854775807ll ] , ULL1, [Macro Expansion: 0 ] ,    0, 0, SLL1, [Macro Expansion: (-9223372036854775807ll-1ll) ] , SLL2, [Macro Expansion: 9223372036854775807ll ] , ULL1, [Macro Expansion: 0 ] ,    0 ) [Macro Expansion: f_test_arg_longint ( 10, clngint(10), clngint((-9223372036854775807ll-1ll)), clngint(9223372036854775807ll), clngint(0), clngint(0), clngint(0), clngint((-9223372036854775807ll-1ll)), clngint(9223372036854775807ll), clngint(0), clngint(0) ) ]
	#line 209 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API18F_TEST_ARG_LONGINTEu7INTEGERz( 10ll, 10ll, (int64)-9223372036854775808ull, 9223372036854775807ll, 0ll, 0ll, 0ll, (int64)-9223372036854775808ull, 9223372036854775807ll, 0ll, 0ll );
	// 	END_TEST [Macro Expansion: #if defined( TMP_FBCUNIT_TEST_NAME )
	// #if defined( TMP_FBCUNIT_SUITE_NAME )
	// #if defined( TMP_FBCUNIT_TEST_GROUP_NAME )
	// END_TEST_EMIT( TMP_FBCUNIT_SUITE_NAME,fbc_tests.functions.va_cva_api, TMP_FBCUNIT_TEST_GROUP_NAME, TMP_FBCUNIT_TEST_NAME,basic_types, false )end sub private sub basic_types_ctor cdecl () constructor #if $"TMP_FBCUNIT_TEST_GROUP_NAME" > "" fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"TMP_FBCUNIT_TEST_GROUP_NAME" + "." + $"basic_types", procptr(basic_types), false ) #else fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"basic_types", procptr(basic_types), false ) #endif end sub FBCU_TRACE( "END_TEST" basic_types ) #else
	#line 210 "FUNCTIONS\\VA_CVA_API.BAS"
	label$63:;
#line 210 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 235 "FUNCTIONS\\VA_CVA_API.BAS"
int64 _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API11F_ARG_BYVALEu7INTEGERS3_St9__va_list( int64 TOTAL$1, int64 N$1, __builtin_va_list ARGS$1 )
#line 235 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 235 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 fb$result$1;
	#line 235 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_memset( &fb$result$1, 0, 8ll );
	#line 235 "FUNCTIONS\\VA_CVA_API.BAS"
	label$66:;
	// 		sum_cva_list_args( args ) [Macro Expansion: dim x as cva_list
	#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list X$1;
	#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_memset( &X$1, 0, 32ll );
	// cva_copy( x, args )
	#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_copy( *(__builtin_va_list*)&X$1, *(__builtin_va_list*)&ARGS$1);
	// dim d as integer = 0
	#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 D$1;
	#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
	D$1 = 0ll;
	// for i as integer = 1 to n
	{
		#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 I$2;
		#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = 1ll;
		#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 TMP$181$2;
		#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
		TMP$181$2 = N$1;
		#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
		goto label$68;
		#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
		label$71:;
		{
			// d += cva_arg( x, integer )
			#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
			D$1 = D$1 + __builtin_va_arg( *(__builtin_va_list*)&X$1, int64);
			// next
		}
		#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
		label$69:;
		#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = I$2 + 1ll;
		#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
		label$68:;
		#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
		if( I$2 <= TMP$181$2) goto label$71;
		#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
		label$70:;
	}
	// cva_end( x )
	#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&X$1);
	// dim c as integer = 0
	#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 C$1;
	#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
	C$1 = 0ll;
	// for i as integer = 1 to n
	{
		#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 I$2;
		#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = 1ll;
		#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 TMP$182$2;
		#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
		TMP$182$2 = N$1;
		#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
		goto label$72;
		#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
		label$75:;
		{
			// c += cva_arg( args, integer )
			#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
			C$1 = C$1 + __builtin_va_arg( *(__builtin_va_list*)&ARGS$1, int64);
			// next
		}
		#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
		label$73:;
		#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = I$2 + 1ll;
		#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
		label$72:;
		#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
		if( I$2 <= TMP$182$2) goto label$75;
		#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
		label$74:;
	}
	// CU_ASSERT( c = d )fbcu.CU_ASSERT_( (c = d), __FILE__,$"functions\va_cva_api.bas", __LINE__,236, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_ARG_BYVAL", "CU_ASSERT(" $"c = d" ")" ) ]
	#line 236 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(C$1 == D$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 236, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_ARG_BYVAL", (char*)"CU_ASSERT(c = d)" );
	// 		function = c
	#line 237 "FUNCTIONS\\VA_CVA_API.BAS"
	fb$result$1 = C$1;
	#line 238 "FUNCTIONS\\VA_CVA_API.BAS"
	label$67:;
	#line 238 "FUNCTIONS\\VA_CVA_API.BAS"
	return fb$result$1;
#line 238 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 240 "FUNCTIONS\\VA_CVA_API.BAS"
int64 _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API11F_ARG_BYREFEu7INTEGERS3_RSt9__va_list( int64 TOTAL$1, int64 N$1, __builtin_va_list* ARGS$1 )
#line 240 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 240 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 fb$result$1;
	#line 240 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_memset( &fb$result$1, 0, 8ll );
	#line 240 "FUNCTIONS\\VA_CVA_API.BAS"
	label$76:;
	// 		sum_cva_list_args( args ) [Macro Expansion: dim x as cva_list
	#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list X$1;
	#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_memset( &X$1, 0, 32ll );
	// cva_copy( x, args )
	#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_copy( *(__builtin_va_list*)&X$1, *(__builtin_va_list*)ARGS$1);
	// dim d as integer = 0
	#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 D$1;
	#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
	D$1 = 0ll;
	// for i as integer = 1 to n
	{
		#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 I$2;
		#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = 1ll;
		#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 TMP$187$2;
		#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
		TMP$187$2 = N$1;
		#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
		goto label$78;
		#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
		label$81:;
		{
			// d += cva_arg( x, integer )
			#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
			D$1 = D$1 + __builtin_va_arg( *(__builtin_va_list*)&X$1, int64);
			// next
		}
		#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
		label$79:;
		#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = I$2 + 1ll;
		#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
		label$78:;
		#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
		if( I$2 <= TMP$187$2) goto label$81;
		#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
		label$80:;
	}
	// cva_end( x )
	#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&X$1);
	// dim c as integer = 0
	#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 C$1;
	#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
	C$1 = 0ll;
	// for i as integer = 1 to n
	{
		#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 I$2;
		#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = 1ll;
		#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 TMP$188$2;
		#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
		TMP$188$2 = N$1;
		#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
		goto label$82;
		#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
		label$85:;
		{
			// c += cva_arg( args, integer )
			#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
			C$1 = C$1 + __builtin_va_arg( *(__builtin_va_list*)ARGS$1, int64);
			// next
		}
		#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
		label$83:;
		#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = I$2 + 1ll;
		#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
		label$82:;
		#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
		if( I$2 <= TMP$188$2) goto label$85;
		#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
		label$84:;
	}
	// CU_ASSERT( c = d )fbcu.CU_ASSERT_( (c = d), __FILE__,$"functions\va_cva_api.bas", __LINE__,241, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_ARG_BYREF", "CU_ASSERT(" $"c = d" ")" ) ]
	#line 241 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(C$1 == D$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 241, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_ARG_BYREF", (char*)"CU_ASSERT(c = d)" );
	// 		function = c
	#line 242 "FUNCTIONS\\VA_CVA_API.BAS"
	fb$result$1 = C$1;
	#line 243 "FUNCTIONS\\VA_CVA_API.BAS"
	label$77:;
	#line 243 "FUNCTIONS\\VA_CVA_API.BAS"
	return fb$result$1;
#line 243 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 245 "FUNCTIONS\\VA_CVA_API.BAS"
int64 _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API15F_ARG_BYREF_PTREu7INTEGERS3_RPSt9__va_list( int64 TOTAL$1, int64 N$1, __builtin_va_list** ARGS$1 )
#line 245 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 245 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 fb$result$1;
	#line 245 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_memset( &fb$result$1, 0, 8ll );
	#line 245 "FUNCTIONS\\VA_CVA_API.BAS"
	label$86:;
	// 		sum_cva_list_args( *args ) [Macro Expansion: dim x as cva_list
	#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list X$1;
	#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_memset( &X$1, 0, 32ll );
	// cva_copy( x, *args )
	#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_copy( *(__builtin_va_list*)&X$1, *(*(__builtin_va_list**)ARGS$1));
	// dim d as integer = 0
	#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 D$1;
	#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
	D$1 = 0ll;
	// for i as integer = 1 to n
	{
		#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 I$2;
		#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = 1ll;
		#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 TMP$190$2;
		#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
		TMP$190$2 = N$1;
		#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
		goto label$88;
		#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
		label$91:;
		{
			// d += cva_arg( x, integer )
			#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
			D$1 = D$1 + __builtin_va_arg( *(__builtin_va_list*)&X$1, int64);
			// next
		}
		#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
		label$89:;
		#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = I$2 + 1ll;
		#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
		label$88:;
		#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
		if( I$2 <= TMP$190$2) goto label$91;
		#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
		label$90:;
	}
	// cva_end( x )
	#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&X$1);
	// dim c as integer = 0
	#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 C$1;
	#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
	C$1 = 0ll;
	// for i as integer = 1 to n
	{
		#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 I$2;
		#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = 1ll;
		#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 TMP$191$2;
		#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
		TMP$191$2 = N$1;
		#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
		goto label$92;
		#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
		label$95:;
		{
			// c += cva_arg( *args, integer )
			#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
			C$1 = C$1 + __builtin_va_arg( *(*(__builtin_va_list**)ARGS$1), int64);
			// next
		}
		#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
		label$93:;
		#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = I$2 + 1ll;
		#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
		label$92:;
		#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
		if( I$2 <= TMP$191$2) goto label$95;
		#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
		label$94:;
	}
	// CU_ASSERT( c = d )fbcu.CU_ASSERT_( (c = d), __FILE__,$"functions\va_cva_api.bas", __LINE__,246, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_ARG_BYREF_PTR", "CU_ASSERT(" $"c = d" ")" ) ]
	#line 246 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(C$1 == D$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 246, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_ARG_BYREF_PTR", (char*)"CU_ASSERT(c = d)" );
	// 		function = c
	#line 247 "FUNCTIONS\\VA_CVA_API.BAS"
	fb$result$1 = C$1;
	#line 248 "FUNCTIONS\\VA_CVA_API.BAS"
	label$87:;
	#line 248 "FUNCTIONS\\VA_CVA_API.BAS"
	return fb$result$1;
#line 248 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 250 "FUNCTIONS\\VA_CVA_API.BAS"
int64 _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API9F_ARG_PTREu7INTEGERS3_PSt9__va_list( int64 TOTAL$1, int64 N$1, __builtin_va_list* ARGS$1 )
#line 250 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 250 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 fb$result$1;
	#line 250 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_memset( &fb$result$1, 0, 8ll );
	#line 250 "FUNCTIONS\\VA_CVA_API.BAS"
	label$96:;
	// 		sum_cva_list_args( *args ) [Macro Expansion: dim x as cva_list
	#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list X$1;
	#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_memset( &X$1, 0, 32ll );
	// cva_copy( x, *args )
	#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_copy( *(__builtin_va_list*)&X$1, *(__builtin_va_list*)ARGS$1);
	// dim d as integer = 0
	#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 D$1;
	#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
	D$1 = 0ll;
	// for i as integer = 1 to n
	{
		#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 I$2;
		#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = 1ll;
		#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 TMP$193$2;
		#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
		TMP$193$2 = N$1;
		#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
		goto label$98;
		#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
		label$101:;
		{
			// d += cva_arg( x, integer )
			#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
			D$1 = D$1 + __builtin_va_arg( *(__builtin_va_list*)&X$1, int64);
			// next
		}
		#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
		label$99:;
		#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = I$2 + 1ll;
		#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
		label$98:;
		#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
		if( I$2 <= TMP$193$2) goto label$101;
		#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
		label$100:;
	}
	// cva_end( x )
	#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&X$1);
	// dim c as integer = 0
	#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 C$1;
	#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
	C$1 = 0ll;
	// for i as integer = 1 to n
	{
		#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 I$2;
		#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = 1ll;
		#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 TMP$194$2;
		#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
		TMP$194$2 = N$1;
		#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
		goto label$102;
		#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
		label$105:;
		{
			// c += cva_arg( *args, integer )
			#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
			C$1 = C$1 + __builtin_va_arg( *(__builtin_va_list*)ARGS$1, int64);
			// next
		}
		#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
		label$103:;
		#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = I$2 + 1ll;
		#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
		label$102:;
		#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
		if( I$2 <= TMP$194$2) goto label$105;
		#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
		label$104:;
	}
	// CU_ASSERT( c = d )fbcu.CU_ASSERT_( (c = d), __FILE__,$"functions\va_cva_api.bas", __LINE__,251, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_ARG_PTR", "CU_ASSERT(" $"c = d" ")" ) ]
	#line 251 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(C$1 == D$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 251, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_ARG_PTR", (char*)"CU_ASSERT(c = d)" );
	// 		function = c
	#line 252 "FUNCTIONS\\VA_CVA_API.BAS"
	fb$result$1 = C$1;
	#line 253 "FUNCTIONS\\VA_CVA_API.BAS"
	label$97:;
	#line 253 "FUNCTIONS\\VA_CVA_API.BAS"
	return fb$result$1;
#line 253 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 255 "FUNCTIONS\\VA_CVA_API.BAS"
int64 _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API13F_ARG_PTR_PTREu7INTEGERS3_PPSt9__va_list( int64 TOTAL$1, int64 N$1, __builtin_va_list** ARGS$1 )
#line 255 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 255 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 fb$result$1;
	#line 255 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_memset( &fb$result$1, 0, 8ll );
	#line 255 "FUNCTIONS\\VA_CVA_API.BAS"
	label$106:;
	// 		sum_cva_list_args( **args ) [Macro Expansion: dim x as cva_list
	#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list X$1;
	#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_memset( &X$1, 0, 32ll );
	// cva_copy( x, **args )
	#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_copy( *(__builtin_va_list*)&X$1, *(*(__builtin_va_list**)ARGS$1));
	// dim d as integer = 0
	#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 D$1;
	#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
	D$1 = 0ll;
	// for i as integer = 1 to n
	{
		#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 I$2;
		#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = 1ll;
		#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 TMP$196$2;
		#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
		TMP$196$2 = N$1;
		#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
		goto label$108;
		#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
		label$111:;
		{
			// d += cva_arg( x, integer )
			#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
			D$1 = D$1 + __builtin_va_arg( *(__builtin_va_list*)&X$1, int64);
			// next
		}
		#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
		label$109:;
		#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = I$2 + 1ll;
		#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
		label$108:;
		#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
		if( I$2 <= TMP$196$2) goto label$111;
		#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
		label$110:;
	}
	// cva_end( x )
	#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&X$1);
	// dim c as integer = 0
	#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 C$1;
	#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
	C$1 = 0ll;
	// for i as integer = 1 to n
	{
		#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 I$2;
		#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = 1ll;
		#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 TMP$197$2;
		#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
		TMP$197$2 = N$1;
		#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
		goto label$112;
		#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
		label$115:;
		{
			// c += cva_arg( **args, integer )
			#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
			C$1 = C$1 + __builtin_va_arg( *(*(__builtin_va_list**)ARGS$1), int64);
			// next
		}
		#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
		label$113:;
		#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
		I$2 = I$2 + 1ll;
		#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
		label$112:;
		#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
		if( I$2 <= TMP$197$2) goto label$115;
		#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
		label$114:;
	}
	// CU_ASSERT( c = d )fbcu.CU_ASSERT_( (c = d), __FILE__,$"functions\va_cva_api.bas", __LINE__,256, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_ARG_PTR_PTR", "CU_ASSERT(" $"c = d" ")" ) ]
	#line 256 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(C$1 == D$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 256, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.F_ARG_PTR_PTR", (char*)"CU_ASSERT(c = d)" );
	// 		function = c
	#line 257 "FUNCTIONS\\VA_CVA_API.BAS"
	fb$result$1 = C$1;
	#line 258 "FUNCTIONS\\VA_CVA_API.BAS"
	label$107:;
	#line 258 "FUNCTIONS\\VA_CVA_API.BAS"
	return fb$result$1;
#line 258 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 260 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API14ARGUMENT_TESTSEu7INTEGERS3_z( int64 TOTAL$1, int64 N$1, ... )
#line 260 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 260 "FUNCTIONS\\VA_CVA_API.BAS"
	label$116:;
	// 		dim as cva_list args
	#line 262 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list ARGS$1;
	#line 262 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_memset( &ARGS$1, 0, 32ll );
	// 		scope
	{
		// 			cva_start( args, n )
		#line 266 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_start( *(__builtin_va_list*)&ARGS$1, N$1);
		// 			CU_ASSERT( f_arg_byval( total, n, args ) = total ) [Macro Expansion: fbcu.CU_ASSERT_( (f_arg_byval( total, n, args ) = total), __FILE__,$"functions\va_cva_api.bas", __LINE__,267, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.ARGUMENT_TESTS", "CU_ASSERT(" $"f_arg_byval( total, n, args ) = total" ")" ) ]
		#line 267 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 vr$1 = _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API11F_ARG_BYVALEu7INTEGERS3_St9__va_list( TOTAL$1, N$1, ARGS$1 );
		#line 267 "FUNCTIONS\\VA_CVA_API.BAS"
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(vr$1 == TOTAL$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 267, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.ARGUMENT_TESTS", (char*)"CU_ASSERT(f_arg_byval( total, n, args ) = total)" );
		// 			cva_end( args )
		#line 268 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_end( *(__builtin_va_list*)&ARGS$1);
		// 		end scope
	}
	// 		scope
	{
		// 			dim byref as cva_list r_args = args
		#line 273 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_list* R_ARGS$2;
		#line 273 "FUNCTIONS\\VA_CVA_API.BAS"
		R_ARGS$2 = &ARGS$1;
		// 			cva_start( r_args, n )
		#line 274 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_start( *(__builtin_va_list*)R_ARGS$2, N$1);
		// 			CU_ASSERT( f_arg_byval( total, n, r_args ) = total ) [Macro Expansion: fbcu.CU_ASSERT_( (f_arg_byval( total, n, r_args ) = total), __FILE__,$"functions\va_cva_api.bas", __LINE__,275, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.ARGUMENT_TESTS", "CU_ASSERT(" $"f_arg_byval( total, n, r_args ) = total" ")" ) ]
		#line 275 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 vr$7 = _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API11F_ARG_BYVALEu7INTEGERS3_St9__va_list( TOTAL$1, N$1, *R_ARGS$2 );
		#line 275 "FUNCTIONS\\VA_CVA_API.BAS"
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(vr$7 == TOTAL$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 275, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.ARGUMENT_TESTS", (char*)"CU_ASSERT(f_arg_byval( total, n, r_args ) = total)" );
		// 			cva_end( r_args )
		#line 276 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_end( *(__builtin_va_list*)R_ARGS$2);
		// 		end scope
	}
	// 		scope
	{
		// 			cva_start( args, n )
		#line 281 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_start( *(__builtin_va_list*)&ARGS$1, N$1);
		// 			CU_ASSERT( f_arg_byref( total, n, args ) = total ) [Macro Expansion: fbcu.CU_ASSERT_( (f_arg_byref( total, n, args ) = total), __FILE__,$"functions\va_cva_api.bas", __LINE__,282, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.ARGUMENT_TESTS", "CU_ASSERT(" $"f_arg_byref( total, n, args ) = total" ")" ) ]
		#line 282 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 vr$12 = _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API11F_ARG_BYREFEu7INTEGERS3_RSt9__va_list( TOTAL$1, N$1, &ARGS$1 );
		#line 282 "FUNCTIONS\\VA_CVA_API.BAS"
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(vr$12 == TOTAL$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 282, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.ARGUMENT_TESTS", (char*)"CU_ASSERT(f_arg_byref( total, n, args ) = total)" );
		// 			cva_end( args )
		#line 283 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_end( *(__builtin_va_list*)&ARGS$1);
		// 		end scope
	}
	// 		scope
	{
		// 			dim byref as cva_list r_args = args
		#line 288 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_list* R_ARGS$2;
		#line 288 "FUNCTIONS\\VA_CVA_API.BAS"
		R_ARGS$2 = &ARGS$1;
		// 			cva_start( r_args, n )
		#line 289 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_start( *(__builtin_va_list*)R_ARGS$2, N$1);
		// 			CU_ASSERT( f_arg_byref( total, n, r_args ) = total ) [Macro Expansion: fbcu.CU_ASSERT_( (f_arg_byref( total, n, r_args ) = total), __FILE__,$"functions\va_cva_api.bas", __LINE__,290, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.ARGUMENT_TESTS", "CU_ASSERT(" $"f_arg_byref( total, n, r_args ) = total" ")" ) ]
		#line 290 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 vr$17 = _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API11F_ARG_BYREFEu7INTEGERS3_RSt9__va_list( TOTAL$1, N$1, R_ARGS$2 );
		#line 290 "FUNCTIONS\\VA_CVA_API.BAS"
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(vr$17 == TOTAL$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 290, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.ARGUMENT_TESTS", (char*)"CU_ASSERT(f_arg_byref( total, n, r_args ) = total)" );
		// 			cva_end( r_args )
		#line 291 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_end( *(__builtin_va_list*)R_ARGS$2);
		// 		end scope
	}
	// 		scope
	{
		// 			dim as cva_list ptr pargs = @args
		#line 296 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_list* PARGS$2;
		#line 296 "FUNCTIONS\\VA_CVA_API.BAS"
		PARGS$2 = &ARGS$1;
		// 			cva_start( *pargs, n )
		#line 297 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_start( *(__builtin_va_list*)PARGS$2, N$1);
		// 			CU_ASSERT( f_arg_byref_ptr( total, n, pargs ) = total ) [Macro Expansion: fbcu.CU_ASSERT_( (f_arg_byref_ptr( total, n, pargs ) = total), __FILE__,$"functions\va_cva_api.bas", __LINE__,298, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.ARGUMENT_TESTS", "CU_ASSERT(" $"f_arg_byref_ptr( total, n, pargs ) = total" ")" ) ]
		#line 298 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 vr$24 = _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API15F_ARG_BYREF_PTREu7INTEGERS3_RPSt9__va_list( TOTAL$1, N$1, &PARGS$2 );
		#line 298 "FUNCTIONS\\VA_CVA_API.BAS"
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(vr$24 == TOTAL$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 298, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.ARGUMENT_TESTS", (char*)"CU_ASSERT(f_arg_byref_ptr( total, n, pargs ) = total)" );
		// 			cva_end( *pargs )
		#line 299 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_end( *(__builtin_va_list*)PARGS$2);
		// 		end scope
	}
	// 		scope
	{
		// 			dim as cva_list ptr pargs = @args
		#line 304 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_list* PARGS$2;
		#line 304 "FUNCTIONS\\VA_CVA_API.BAS"
		PARGS$2 = &ARGS$1;
		// 			cva_start( *pargs, n )
		#line 305 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_start( *(__builtin_va_list*)PARGS$2, N$1);
		// 			CU_ASSERT( f_arg_ptr( total, n, pargs ) = total ) [Macro Expansion: fbcu.CU_ASSERT_( (f_arg_ptr( total, n, pargs ) = total), __FILE__,$"functions\va_cva_api.bas", __LINE__,306, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.ARGUMENT_TESTS", "CU_ASSERT(" $"f_arg_ptr( total, n, pargs ) = total" ")" ) ]
		#line 306 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 vr$30 = _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API9F_ARG_PTREu7INTEGERS3_PSt9__va_list( TOTAL$1, N$1, PARGS$2 );
		#line 306 "FUNCTIONS\\VA_CVA_API.BAS"
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(vr$30 == TOTAL$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 306, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.ARGUMENT_TESTS", (char*)"CU_ASSERT(f_arg_ptr( total, n, pargs ) = total)" );
		// 			cva_end( *pargs )
		#line 307 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_end( *(__builtin_va_list*)PARGS$2);
		// 		end scope
	}
	// 		scope
	{
		// 			cva_start( args, n )
		#line 312 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_start( *(__builtin_va_list*)&ARGS$1, N$1);
		// 			CU_ASSERT( f_arg_ptr( total, n, @args ) = total ) [Macro Expansion: fbcu.CU_ASSERT_( (f_arg_ptr( total, n, @args ) = total), __FILE__,$"functions\va_cva_api.bas", __LINE__,313, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.ARGUMENT_TESTS", "CU_ASSERT(" $"f_arg_ptr( total, n, @args ) = total" ")" ) ]
		#line 313 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 vr$35 = _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API9F_ARG_PTREu7INTEGERS3_PSt9__va_list( TOTAL$1, N$1, &ARGS$1 );
		#line 313 "FUNCTIONS\\VA_CVA_API.BAS"
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(vr$35 == TOTAL$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 313, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.ARGUMENT_TESTS", (char*)"CU_ASSERT(f_arg_ptr( total, n, @args ) = total)" );
		// 			cva_end( args )
		#line 314 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_end( *(__builtin_va_list*)&ARGS$1);
		// 		end scope
	}
	// 		scope
	{
		// 			dim as cva_list ptr pargs = @args
		#line 319 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_list* PARGS$2;
		#line 319 "FUNCTIONS\\VA_CVA_API.BAS"
		PARGS$2 = &ARGS$1;
		// 			dim as cva_list ptr ptr ppargs = @pargs
		#line 320 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_list** PPARGS$2;
		#line 320 "FUNCTIONS\\VA_CVA_API.BAS"
		PPARGS$2 = &PARGS$2;
		// 			cva_start( **ppargs, n )
		#line 321 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_start( *(*(__builtin_va_list**)PPARGS$2), N$1);
		// 			CU_ASSERT( f_arg_ptr( total, n, *ppargs ) = total ) [Macro Expansion: fbcu.CU_ASSERT_( (f_arg_ptr( total, n, *ppargs ) = total), __FILE__,$"functions\va_cva_api.bas", __LINE__,322, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.ARGUMENT_TESTS", "CU_ASSERT(" $"f_arg_ptr( total, n, *ppargs ) = total" ")" ) ]
		#line 322 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 vr$43 = _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API9F_ARG_PTREu7INTEGERS3_PSt9__va_list( TOTAL$1, N$1, *PPARGS$2 );
		#line 322 "FUNCTIONS\\VA_CVA_API.BAS"
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(vr$43 == TOTAL$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 322, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.ARGUMENT_TESTS", (char*)"CU_ASSERT(f_arg_ptr( total, n, *ppargs ) = total)" );
		// 			cva_end( **ppargs )
		#line 323 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_end( *(*(__builtin_va_list**)PPARGS$2));
		// 		end scope
	}
	// 		scope
	{
		// 			dim as cva_list ptr pargs = @args
		#line 328 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_list* PARGS$2;
		#line 328 "FUNCTIONS\\VA_CVA_API.BAS"
		PARGS$2 = &ARGS$1;
		// 			cva_start( *pargs, n )
		#line 329 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_start( *(__builtin_va_list*)PARGS$2, N$1);
		// 			CU_ASSERT( f_arg_ptr_ptr( total, n, @pargs ) = total ) [Macro Expansion: fbcu.CU_ASSERT_( (f_arg_ptr_ptr( total, n, @pargs ) = total), __FILE__,$"functions\va_cva_api.bas", __LINE__,330, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.ARGUMENT_TESTS", "CU_ASSERT(" $"f_arg_ptr_ptr( total, n, @pargs ) = total" ")" ) ]
		#line 330 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 vr$51 = _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API13F_ARG_PTR_PTREu7INTEGERS3_PPSt9__va_list( TOTAL$1, N$1, &PARGS$2 );
		#line 330 "FUNCTIONS\\VA_CVA_API.BAS"
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(vr$51 == TOTAL$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 330, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.ARGUMENT_TESTS", (char*)"CU_ASSERT(f_arg_ptr_ptr( total, n, @pargs ) = total)" );
		// 			cva_end( *pargs )
		#line 331 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_end( *(__builtin_va_list*)PARGS$2);
		// 		end scope
	}
	// 		scope
	{
		// 			dim as cva_list ptr pargs = @args
		#line 336 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_list* PARGS$2;
		#line 336 "FUNCTIONS\\VA_CVA_API.BAS"
		PARGS$2 = &ARGS$1;
		// 			dim as cva_list ptr ptr ppargs = @pargs
		#line 337 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_list** PPARGS$2;
		#line 337 "FUNCTIONS\\VA_CVA_API.BAS"
		PPARGS$2 = &PARGS$2;
		// 			cva_start( **ppargs, n )
		#line 338 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_start( *(*(__builtin_va_list**)PPARGS$2), N$1);
		// 			CU_ASSERT( f_arg_ptr_ptr( total, n, ppargs ) = total ) [Macro Expansion: fbcu.CU_ASSERT_( (f_arg_ptr_ptr( total, n, ppargs ) = total), __FILE__,$"functions\va_cva_api.bas", __LINE__,339, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.ARGUMENT_TESTS", "CU_ASSERT(" $"f_arg_ptr_ptr( total, n, ppargs ) = total" ")" ) ]
		#line 339 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 vr$59 = _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API13F_ARG_PTR_PTREu7INTEGERS3_PPSt9__va_list( TOTAL$1, N$1, PPARGS$2 );
		#line 339 "FUNCTIONS\\VA_CVA_API.BAS"
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(vr$59 == TOTAL$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 339, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.ARGUMENT_TESTS", (char*)"CU_ASSERT(f_arg_ptr_ptr( total, n, ppargs ) = total)" );
		// 			cva_end( **ppargs )
		#line 340 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_end( *(*(__builtin_va_list**)PPARGS$2));
		// 		end scope
	}
	#line 343 "FUNCTIONS\\VA_CVA_API.BAS"
	label$117:;
#line 343 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 345 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API9ARGUMENTSEv( void )
#line 345 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 345 "FUNCTIONS\\VA_CVA_API.BAS"
	label$118:;
	// #else
	// TEST_EMIT( fbcu_global, TMP_FBCUNIT_TEST_NAME arguments )sub arguments cdecl () FBCU_TRACE( "TEST" arguments ) #endif ]
	// 		argument_tests( 4321, 4, 4000, 300, 20, 1 )
	#line 346 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API14ARGUMENT_TESTSEu7INTEGERS3_z( 4321ll, 4ll, 4000ll, 300ll, 20ll, 1ll );
	// 	END_TEST [Macro Expansion: #if defined( TMP_FBCUNIT_TEST_NAME )
	// #if defined( TMP_FBCUNIT_SUITE_NAME )
	// #if defined( TMP_FBCUNIT_TEST_GROUP_NAME )
	// END_TEST_EMIT( TMP_FBCUNIT_SUITE_NAME,fbc_tests.functions.va_cva_api, TMP_FBCUNIT_TEST_GROUP_NAME, TMP_FBCUNIT_TEST_NAME,arguments, false )end sub private sub arguments_ctor cdecl () constructor #if $"TMP_FBCUNIT_TEST_GROUP_NAME" > "" fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"TMP_FBCUNIT_TEST_GROUP_NAME" + "." + $"arguments", procptr(arguments), false ) #else fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"arguments", procptr(arguments), false ) #endif end sub FBCU_TRACE( "END_TEST" arguments ) #else
	#line 347 "FUNCTIONS\\VA_CVA_API.BAS"
	label$119:;
#line 347 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 354 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API13COMPLEX_TESTSEu7INTEGERS3_z( int64 TOTAL$1, int64 N$1, ... )
#line 354 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 354 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list* TMP$242$1;
	#line 354 "FUNCTIONS\\VA_CVA_API.BAS"
	label$122:;
	// 		dim u as udt_vararg
	#line 355 "FUNCTIONS\\VA_CVA_API.BAS"
	struct $N5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API10UDT_VARARGE U$1;
	#line 355 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_memset( &U$1, 0, 64ll );
	// 		scope
	{
		// 			cva_start( u.f1, n )
		#line 359 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_start( *(__builtin_va_list*)&U$1, N$1);
		// 			sum_cva_list_args( u.f1 ) [Macro Expansion: dim x as cva_list
		#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_list X$2;
		#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_memset( &X$2, 0, 32ll );
		// cva_copy( x, u.f1 )
		#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_copy( *(__builtin_va_list*)&X$2, *(__builtin_va_list*)&U$1);
		// dim d as integer = 0
		#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 D$2;
		#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
		D$2 = 0ll;
		// for i as integer = 1 to n
		{
			#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 I$3;
			#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
			I$3 = 1ll;
			#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 TMP$232$3;
			#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
			TMP$232$3 = N$1;
			#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
			goto label$124;
			#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
			label$127:;
			{
				// d += cva_arg( x, integer )
				#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
				D$2 = D$2 + __builtin_va_arg( *(__builtin_va_list*)&X$2, int64);
				// next
			}
			#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
			label$125:;
			#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
			I$3 = I$3 + 1ll;
			#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
			label$124:;
			#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
			if( I$3 <= TMP$232$3) goto label$127;
			#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
			label$126:;
		}
		// cva_end( x )
		#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_end( *(__builtin_va_list*)&X$2);
		// dim c as integer = 0
		#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 C$2;
		#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
		C$2 = 0ll;
		// for i as integer = 1 to n
		{
			#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 I$3;
			#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
			I$3 = 1ll;
			#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 TMP$233$3;
			#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
			TMP$233$3 = N$1;
			#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
			goto label$128;
			#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
			label$131:;
			{
				// c += cva_arg( u.f1, integer )
				#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
				C$2 = C$2 + __builtin_va_arg( *(__builtin_va_list*)&U$1, int64);
				// next
			}
			#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
			label$129:;
			#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
			I$3 = I$3 + 1ll;
			#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
			label$128:;
			#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
			if( I$3 <= TMP$233$3) goto label$131;
			#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
			label$130:;
		}
		// CU_ASSERT( c = d )fbcu.CU_ASSERT_( (c = d), __FILE__,$"functions\va_cva_api.bas", __LINE__,360, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.COMPLEX_TESTS", "CU_ASSERT(" $"c = d" ")" ) ]
		#line 360 "FUNCTIONS\\VA_CVA_API.BAS"
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(C$2 == D$2) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 360, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.COMPLEX_TESTS", (char*)"CU_ASSERT(c = d)" );
		// 			cva_end( u.f1 )
		#line 361 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_end( *(__builtin_va_list*)&U$1);
		// 		end scope
	}
	// 		scope
	{
		// 			cva_start( u.f2, n )
		#line 365 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_start( *(__builtin_va_list*)((uint8*)&U$1 + 32ll), N$1);
		// 			sum_cva_list_args( u.f2 ) [Macro Expansion: dim x as cva_list
		#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_list X$2;
		#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_memset( &X$2, 0, 32ll );
		// cva_copy( x, u.f2 )
		#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_copy( *(__builtin_va_list*)&X$2, *(__builtin_va_list*)((uint8*)&U$1 + 32ll));
		// dim d as integer = 0
		#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 D$2;
		#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
		D$2 = 0ll;
		// for i as integer = 1 to n
		{
			#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 I$3;
			#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
			I$3 = 1ll;
			#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 TMP$235$3;
			#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
			TMP$235$3 = N$1;
			#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
			goto label$132;
			#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
			label$135:;
			{
				// d += cva_arg( x, integer )
				#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
				D$2 = D$2 + __builtin_va_arg( *(__builtin_va_list*)&X$2, int64);
				// next
			}
			#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
			label$133:;
			#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
			I$3 = I$3 + 1ll;
			#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
			label$132:;
			#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
			if( I$3 <= TMP$235$3) goto label$135;
			#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
			label$134:;
		}
		// cva_end( x )
		#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_end( *(__builtin_va_list*)&X$2);
		// dim c as integer = 0
		#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 C$2;
		#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
		C$2 = 0ll;
		// for i as integer = 1 to n
		{
			#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 I$3;
			#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
			I$3 = 1ll;
			#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 TMP$236$3;
			#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
			TMP$236$3 = N$1;
			#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
			goto label$136;
			#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
			label$139:;
			{
				// c += cva_arg( u.f2, integer )
				#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
				C$2 = C$2 + __builtin_va_arg( *(__builtin_va_list*)((uint8*)&U$1 + 32ll), int64);
				// next
			}
			#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
			label$137:;
			#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
			I$3 = I$3 + 1ll;
			#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
			label$136:;
			#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
			if( I$3 <= TMP$236$3) goto label$139;
			#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
			label$138:;
		}
		// CU_ASSERT( c = d )fbcu.CU_ASSERT_( (c = d), __FILE__,$"functions\va_cva_api.bas", __LINE__,366, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.COMPLEX_TESTS", "CU_ASSERT(" $"c = d" ")" ) ]
		#line 366 "FUNCTIONS\\VA_CVA_API.BAS"
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(C$2 == D$2) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 366, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.COMPLEX_TESTS", (char*)"CU_ASSERT(c = d)" );
		// 			cva_end( u.f2 )
		#line 367 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_end( *(__builtin_va_list*)((uint8*)&U$1 + 32ll));
		// 		end scope
	}
	// 		dim a(1 to 2) as cva_list
	#line 371 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list A$1[2];
	#line 371 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_memset( (__builtin_va_list*)A$1, 0, 64ll );
	struct $N5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API10FBARRAY1$1ISt9__va_listEE {
		struct $9__va_list* DATA;
		struct $9__va_list* PTR;
		int64 SIZE;
		int64 ELEMENT_LEN;
		int64 DIMENSIONS;
		int64 FLAGS;
		struct $16__FB_ARRAYDIMTB$ DIMTB[1];
	};
	#line 371 "FUNCTIONS\\VA_CVA_API.BAS"
	__FB_STATIC_ASSERT( sizeof( struct $N5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API10FBARRAY1$1ISt9__va_listEE ) == 72 );
	#line 371 "FUNCTIONS\\VA_CVA_API.BAS"
	struct $N5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API10FBARRAY1$1ISt9__va_listEE tmp$237$1;
	#line 371 "FUNCTIONS\\VA_CVA_API.BAS"
	*(__builtin_va_list**)&tmp$237$1 = (__builtin_va_list*)((uint64)(struct $9__va_list*)A$1 + -32ll);
	#line 371 "FUNCTIONS\\VA_CVA_API.BAS"
	*(__builtin_va_list**)((uint8*)&tmp$237$1 + 8ll) = (__builtin_va_list*)A$1;
	#line 371 "FUNCTIONS\\VA_CVA_API.BAS"
	*(int64*)((uint8*)&tmp$237$1 + 16ll) = 64ll;
	#line 371 "FUNCTIONS\\VA_CVA_API.BAS"
	*(int64*)((uint8*)&tmp$237$1 + 24ll) = 32ll;
	#line 371 "FUNCTIONS\\VA_CVA_API.BAS"
	*(int64*)((uint8*)&tmp$237$1 + 32ll) = 1ll;
	#line 371 "FUNCTIONS\\VA_CVA_API.BAS"
	*(int64*)((uint8*)&tmp$237$1 + 40ll) = 49ll;
	#line 371 "FUNCTIONS\\VA_CVA_API.BAS"
	*(int64*)((uint8*)&tmp$237$1 + 48ll) = 2ll;
	#line 371 "FUNCTIONS\\VA_CVA_API.BAS"
	*(int64*)((uint8*)&tmp$237$1 + 56ll) = 1ll;
	#line 371 "FUNCTIONS\\VA_CVA_API.BAS"
	*(int64*)((uint8*)&tmp$237$1 + 64ll) = 2ll;
	// 		scope
	{
		// 			cva_start( a(1), n )
		#line 374 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_start( *(__builtin_va_list*)A$1, N$1);
		// 			sum_cva_list_args( a(1) ) [Macro Expansion: dim x as cva_list
		#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_list X$2;
		#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_memset( &X$2, 0, 32ll );
		// cva_copy( x, a(1) )
		#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_copy( *(__builtin_va_list*)&X$2, *(__builtin_va_list*)A$1);
		// dim d as integer = 0
		#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 D$2;
		#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
		D$2 = 0ll;
		// for i as integer = 1 to n
		{
			#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 I$3;
			#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
			I$3 = 1ll;
			#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 TMP$238$3;
			#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
			TMP$238$3 = N$1;
			#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
			goto label$140;
			#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
			label$143:;
			{
				// d += cva_arg( x, integer )
				#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
				D$2 = D$2 + __builtin_va_arg( *(__builtin_va_list*)&X$2, int64);
				// next
			}
			#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
			label$141:;
			#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
			I$3 = I$3 + 1ll;
			#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
			label$140:;
			#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
			if( I$3 <= TMP$238$3) goto label$143;
			#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
			label$142:;
		}
		// cva_end( x )
		#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_end( *(__builtin_va_list*)&X$2);
		// dim c as integer = 0
		#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 C$2;
		#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
		C$2 = 0ll;
		// for i as integer = 1 to n
		{
			#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 I$3;
			#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
			I$3 = 1ll;
			#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 TMP$239$3;
			#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
			TMP$239$3 = N$1;
			#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
			goto label$144;
			#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
			label$147:;
			{
				// c += cva_arg( a(1), integer )
				#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
				C$2 = C$2 + __builtin_va_arg( *(__builtin_va_list*)A$1, int64);
				// next
			}
			#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
			label$145:;
			#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
			I$3 = I$3 + 1ll;
			#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
			label$144:;
			#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
			if( I$3 <= TMP$239$3) goto label$147;
			#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
			label$146:;
		}
		// CU_ASSERT( c = d )fbcu.CU_ASSERT_( (c = d), __FILE__,$"functions\va_cva_api.bas", __LINE__,375, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.COMPLEX_TESTS", "CU_ASSERT(" $"c = d" ")" ) ]
		#line 375 "FUNCTIONS\\VA_CVA_API.BAS"
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(C$2 == D$2) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 375, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.COMPLEX_TESTS", (char*)"CU_ASSERT(c = d)" );
		// 			cva_end( a(1) )
		#line 376 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_end( *(__builtin_va_list*)A$1);
		// 		end scope
	}
	// 		scope
	{
		// 			cva_start( a(2), n )
		#line 380 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_start( *(__builtin_va_list*)((uint64)(struct $9__va_list*)A$1 + 32ll), N$1);
		// 			sum_cva_list_args( a(2) ) [Macro Expansion: dim x as cva_list
		#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_list X$2;
		#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_memset( &X$2, 0, 32ll );
		// cva_copy( x, a(2) )
		#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_copy( *(__builtin_va_list*)&X$2, *(__builtin_va_list*)((uint64)(struct $9__va_list*)A$1 + 32ll));
		// dim d as integer = 0
		#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 D$2;
		#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
		D$2 = 0ll;
		// for i as integer = 1 to n
		{
			#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 I$3;
			#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
			I$3 = 1ll;
			#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 TMP$240$3;
			#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
			TMP$240$3 = N$1;
			#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
			goto label$148;
			#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
			label$151:;
			{
				// d += cva_arg( x, integer )
				#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
				D$2 = D$2 + __builtin_va_arg( *(__builtin_va_list*)&X$2, int64);
				// next
			}
			#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
			label$149:;
			#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
			I$3 = I$3 + 1ll;
			#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
			label$148:;
			#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
			if( I$3 <= TMP$240$3) goto label$151;
			#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
			label$150:;
		}
		// cva_end( x )
		#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_end( *(__builtin_va_list*)&X$2);
		// dim c as integer = 0
		#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 C$2;
		#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
		C$2 = 0ll;
		// for i as integer = 1 to n
		{
			#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 I$3;
			#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
			I$3 = 1ll;
			#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 TMP$241$3;
			#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
			TMP$241$3 = N$1;
			#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
			goto label$152;
			#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
			label$155:;
			{
				// c += cva_arg( a(2), integer )
				#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
				C$2 = C$2 + __builtin_va_arg( *(__builtin_va_list*)((uint64)(struct $9__va_list*)A$1 + 32ll), int64);
				// next
			}
			#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
			label$153:;
			#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
			I$3 = I$3 + 1ll;
			#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
			label$152:;
			#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
			if( I$3 <= TMP$241$3) goto label$155;
			#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
			label$154:;
		}
		// CU_ASSERT( c = d )fbcu.CU_ASSERT_( (c = d), __FILE__,$"functions\va_cva_api.bas", __LINE__,381, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.COMPLEX_TESTS", "CU_ASSERT(" $"c = d" ")" ) ]
		#line 381 "FUNCTIONS\\VA_CVA_API.BAS"
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(C$2 == D$2) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 381, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.COMPLEX_TESTS", (char*)"CU_ASSERT(c = d)" );
		// 			cva_end( a(2) )
		#line 382 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_end( *(__builtin_va_list*)((uint64)(struct $9__va_list*)A$1 + 32ll));
		// 		end scope
	}
	// 		dim p as cva_list ptr = new cva_list
	#line 386 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list* P$1;
	#line 386 "FUNCTIONS\\VA_CVA_API.BAS"
	void* vr$40 = calloc( 32ull, 1ull );
	#line 386 "FUNCTIONS\\VA_CVA_API.BAS"
	TMP$242$1 = (__builtin_va_list*)vr$40;
	#line 386 "FUNCTIONS\\VA_CVA_API.BAS"
	if( (uint8*)TMP$242$1 == (uint8*)0ull) goto label$156;
	#line 386 "FUNCTIONS\\VA_CVA_API.BAS"
	label$156:;
	#line 386 "FUNCTIONS\\VA_CVA_API.BAS"
	P$1 = TMP$242$1;
	// 		scope
	{
		// 			cva_start( *p, n )
		#line 388 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_start( *(__builtin_va_list*)P$1, N$1);
		// 			sum_cva_list_args( *p ) [Macro Expansion: dim x as cva_list
		#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_list X$2;
		#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_memset( &X$2, 0, 32ll );
		// cva_copy( x, *p )
		#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_copy( *(__builtin_va_list*)&X$2, *(__builtin_va_list*)P$1);
		// dim d as integer = 0
		#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 D$2;
		#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
		D$2 = 0ll;
		// for i as integer = 1 to n
		{
			#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 I$3;
			#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
			I$3 = 1ll;
			#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 TMP$243$3;
			#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
			TMP$243$3 = N$1;
			#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
			goto label$157;
			#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
			label$160:;
			{
				// d += cva_arg( x, integer )
				#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
				D$2 = D$2 + __builtin_va_arg( *(__builtin_va_list*)&X$2, int64);
				// next
			}
			#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
			label$158:;
			#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
			I$3 = I$3 + 1ll;
			#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
			label$157:;
			#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
			if( I$3 <= TMP$243$3) goto label$160;
			#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
			label$159:;
		}
		// cva_end( x )
		#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_end( *(__builtin_va_list*)&X$2);
		// dim c as integer = 0
		#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
		int64 C$2;
		#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
		C$2 = 0ll;
		// for i as integer = 1 to n
		{
			#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 I$3;
			#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
			I$3 = 1ll;
			#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
			int64 TMP$244$3;
			#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
			TMP$244$3 = N$1;
			#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
			goto label$161;
			#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
			label$164:;
			{
				// c += cva_arg( *p, integer )
				#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
				C$2 = C$2 + __builtin_va_arg( *(__builtin_va_list*)P$1, int64);
				// next
			}
			#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
			label$162:;
			#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
			I$3 = I$3 + 1ll;
			#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
			label$161:;
			#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
			if( I$3 <= TMP$244$3) goto label$164;
			#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
			label$163:;
		}
		// CU_ASSERT( c = d )fbcu.CU_ASSERT_( (c = d), __FILE__,$"functions\va_cva_api.bas", __LINE__,389, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.COMPLEX_TESTS", "CU_ASSERT(" $"c = d" ")" ) ]
		#line 389 "FUNCTIONS\\VA_CVA_API.BAS"
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(C$2 == D$2) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 389, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.COMPLEX_TESTS", (char*)"CU_ASSERT(c = d)" );
		// 			cva_end( *p )
		#line 390 "FUNCTIONS\\VA_CVA_API.BAS"
		__builtin_va_end( *(__builtin_va_list*)P$1);
		// 		end scope
	}
	// 		delete p
	#line 392 "FUNCTIONS\\VA_CVA_API.BAS"
	if( (uint8*)P$1 == (uint8*)0ull) goto label$165;
	#line 392 "FUNCTIONS\\VA_CVA_API.BAS"
	free( (void*)P$1 );
	#line 392 "FUNCTIONS\\VA_CVA_API.BAS"
	label$165:;
	#line 394 "FUNCTIONS\\VA_CVA_API.BAS"
	label$123:;
#line 394 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 396 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API7COMPLEXEv( void )
#line 396 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 396 "FUNCTIONS\\VA_CVA_API.BAS"
	label$166:;
	// #else
	// TEST_EMIT( fbcu_global, TMP_FBCUNIT_TEST_NAME complex )sub complex cdecl () FBCU_TRACE( "TEST" complex ) #endif ]
	// 		complex_tests( 1234, 4, 1000, 200, 30, 4 )
	#line 397 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API13COMPLEX_TESTSEu7INTEGERS3_z( 1234ll, 4ll, 1000ll, 200ll, 30ll, 4ll );
	// 	END_TEST [Macro Expansion: #if defined( TMP_FBCUNIT_TEST_NAME )
	// #if defined( TMP_FBCUNIT_SUITE_NAME )
	// #if defined( TMP_FBCUNIT_TEST_GROUP_NAME )
	// END_TEST_EMIT( TMP_FBCUNIT_SUITE_NAME,fbc_tests.functions.va_cva_api, TMP_FBCUNIT_TEST_GROUP_NAME, TMP_FBCUNIT_TEST_NAME,complex, false )end sub private sub complex_ctor cdecl () constructor #if $"TMP_FBCUNIT_TEST_GROUP_NAME" > "" fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"TMP_FBCUNIT_TEST_GROUP_NAME" + "." + $"complex", procptr(complex), false ) #else fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"complex", procptr(complex), false ) #endif end sub FBCU_TRACE( "END_TEST" complex ) #else
	#line 398 "FUNCTIONS\\VA_CVA_API.BAS"
	label$167:;
#line 398 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 421 "FUNCTIONS\\VA_CVA_API.BAS"
__builtin_va_list* _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API14RET_VALIST_PTREPSt9__va_list( __builtin_va_list* ARGS$1 )
#line 421 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 421 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list* fb$result$1;
	#line 421 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_memset( &fb$result$1, 0, 8ll );
	#line 421 "FUNCTIONS\\VA_CVA_API.BAS"
	label$170:;
	// 		function = args
	#line 422 "FUNCTIONS\\VA_CVA_API.BAS"
	fb$result$1 = ARGS$1;
	#line 423 "FUNCTIONS\\VA_CVA_API.BAS"
	label$171:;
	#line 423 "FUNCTIONS\\VA_CVA_API.BAS"
	return fb$result$1;
#line 423 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 425 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API17PROC_CVA_LIST_PTREu7INTEGERz( int64 N$1, ... )
#line 425 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 425 "FUNCTIONS\\VA_CVA_API.BAS"
	label$172:;
	// 		dim args as cva_list
	#line 426 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list ARGS$1;
	#line 426 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_memset( &ARGS$1, 0, 32ll );
	// 		dim x as cva_list ptr = @args
	#line 427 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list* X$1;
	#line 427 "FUNCTIONS\\VA_CVA_API.BAS"
	X$1 = &ARGS$1;
	// 		dim i as integer
	#line 428 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 I$1;
	#line 428 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_memset( &I$1, 0, 8ll );
	// 		cva_start( args, n )
	#line 430 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_start( *(__builtin_va_list*)&ARGS$1, N$1);
	// 		dim arg1 as integer = cva_arg( args, integer )
	#line 431 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 ARG1$1;
	#line 431 "FUNCTIONS\\VA_CVA_API.BAS"
	ARG1$1 = __builtin_va_arg( *(__builtin_va_list*)&ARGS$1, int64);
	// 		dim arg2 as integer = cva_arg( args, integer )
	#line 432 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 ARG2$1;
	#line 432 "FUNCTIONS\\VA_CVA_API.BAS"
	ARG2$1 = __builtin_va_arg( *(__builtin_va_list*)&ARGS$1, int64);
	// 		dim arg3 as integer = cva_arg( args, integer )
	#line 433 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 ARG3$1;
	#line 433 "FUNCTIONS\\VA_CVA_API.BAS"
	ARG3$1 = __builtin_va_arg( *(__builtin_va_list*)&ARGS$1, int64);
	// 		dim arg4 as integer = cva_arg( args, integer )
	#line 434 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 ARG4$1;
	#line 434 "FUNCTIONS\\VA_CVA_API.BAS"
	ARG4$1 = __builtin_va_arg( *(__builtin_va_list*)&ARGS$1, int64);
	// 		cva_end( args )
	#line 435 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&ARGS$1);
	// 		cva_start( args, n )
	#line 437 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_start( *(__builtin_va_list*)&ARGS$1, N$1);
	// 		x = ret_valist_ptr( @args )
	#line 438 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list* vr$8 = _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API14RET_VALIST_PTREPSt9__va_list( &ARGS$1 );
	#line 438 "FUNCTIONS\\VA_CVA_API.BAS"
	X$1 = vr$8;
	// 		CU_ASSERT( cva_arg( *x, integer ) = arg1 ) [Macro Expansion: fbcu.CU_ASSERT_( (cva_arg( *x, integer ) = arg1), __FILE__,$"functions\va_cva_api.bas", __LINE__,439, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_CVA_LIST_PTR", "CU_ASSERT(" $"cva_arg( *x, integer ) = arg1" ")" ) ]
	#line 439 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)X$1, int64) == ARG1$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 439, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_CVA_LIST_PTR", (char*)"CU_ASSERT(cva_arg( *x, integer ) = arg1)" );
	// 		CU_ASSERT( cva_arg( *x, integer ) = arg2 ) [Macro Expansion: fbcu.CU_ASSERT_( (cva_arg( *x, integer ) = arg2), __FILE__,$"functions\va_cva_api.bas", __LINE__,440, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_CVA_LIST_PTR", "CU_ASSERT(" $"cva_arg( *x, integer ) = arg2" ")" ) ]
	#line 440 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)X$1, int64) == ARG2$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 440, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_CVA_LIST_PTR", (char*)"CU_ASSERT(cva_arg( *x, integer ) = arg2)" );
	// 		CU_ASSERT( cva_arg( *x, integer ) = arg3 ) [Macro Expansion: fbcu.CU_ASSERT_( (cva_arg( *x, integer ) = arg3), __FILE__,$"functions\va_cva_api.bas", __LINE__,441, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_CVA_LIST_PTR", "CU_ASSERT(" $"cva_arg( *x, integer ) = arg3" ")" ) ]
	#line 441 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)X$1, int64) == ARG3$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 441, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_CVA_LIST_PTR", (char*)"CU_ASSERT(cva_arg( *x, integer ) = arg3)" );
	// 		CU_ASSERT( cva_arg( *x, integer ) = arg4 ) [Macro Expansion: fbcu.CU_ASSERT_( (cva_arg( *x, integer ) = arg4), __FILE__,$"functions\va_cva_api.bas", __LINE__,442, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_CVA_LIST_PTR", "CU_ASSERT(" $"cva_arg( *x, integer ) = arg4" ")" ) ]
	#line 442 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)X$1, int64) == ARG4$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 442, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_CVA_LIST_PTR", (char*)"CU_ASSERT(cva_arg( *x, integer ) = arg4)" );
	// 		cva_end( args )
	#line 443 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&ARGS$1);
	#line 444 "FUNCTIONS\\VA_CVA_API.BAS"
	label$173:;
#line 444 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 472 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API19PROC_CVA_LIST_BYVALEu7INTEGERz( int64 N$1, ... )
#line 472 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 472 "FUNCTIONS\\VA_CVA_API.BAS"
	label$174:;
	#line 473 "FUNCTIONS\\VA_CVA_API.BAS"
	label$175:;
#line 473 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 476 "FUNCTIONS\\VA_CVA_API.BAS"
__builtin_va_list* _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API16RET_VALIST_BYREFERSt9__va_list( __builtin_va_list* ARGS$1 )
#line 476 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 476 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list* fb$result$1;
	#line 476 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_memset( &fb$result$1, 0, 8ll );
	#line 476 "FUNCTIONS\\VA_CVA_API.BAS"
	label$176:;
	// 		function = args
	#line 477 "FUNCTIONS\\VA_CVA_API.BAS"
	fb$result$1 = ARGS$1;
	#line 478 "FUNCTIONS\\VA_CVA_API.BAS"
	label$177:;
	#line 478 "FUNCTIONS\\VA_CVA_API.BAS"
	return fb$result$1;
#line 478 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 480 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API19PROC_CVA_LIST_BYREFEu7INTEGERz( int64 N$1, ... )
#line 480 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 480 "FUNCTIONS\\VA_CVA_API.BAS"
	label$178:;
	// 		dim args as cva_list
	#line 481 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list ARGS$1;
	#line 481 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_memset( &ARGS$1, 0, 32ll );
	// 		dim byref x as cva_list = args
	#line 482 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list* X$1;
	#line 482 "FUNCTIONS\\VA_CVA_API.BAS"
	X$1 = &ARGS$1;
	// 		dim i as integer
	#line 483 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 I$1;
	#line 483 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_memset( &I$1, 0, 8ll );
	// 		cva_start( args, n )
	#line 485 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_start( *(__builtin_va_list*)&ARGS$1, N$1);
	// 		dim arg1 as integer = cva_arg( args, integer )
	#line 486 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 ARG1$1;
	#line 486 "FUNCTIONS\\VA_CVA_API.BAS"
	ARG1$1 = __builtin_va_arg( *(__builtin_va_list*)&ARGS$1, int64);
	// 		dim arg2 as integer = cva_arg( args, integer )
	#line 487 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 ARG2$1;
	#line 487 "FUNCTIONS\\VA_CVA_API.BAS"
	ARG2$1 = __builtin_va_arg( *(__builtin_va_list*)&ARGS$1, int64);
	// 		dim arg3 as integer = cva_arg( args, integer )
	#line 488 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 ARG3$1;
	#line 488 "FUNCTIONS\\VA_CVA_API.BAS"
	ARG3$1 = __builtin_va_arg( *(__builtin_va_list*)&ARGS$1, int64);
	// 		dim arg4 as integer = cva_arg( args, integer )
	#line 489 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 ARG4$1;
	#line 489 "FUNCTIONS\\VA_CVA_API.BAS"
	ARG4$1 = __builtin_va_arg( *(__builtin_va_list*)&ARGS$1, int64);
	// 		cva_end( args )
	#line 490 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&ARGS$1);
	// 		cva_start( args, n )
	#line 492 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_start( *(__builtin_va_list*)&ARGS$1, N$1);
	// 		x = ret_valist_byref( args )
	#line 493 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list* vr$8 = _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API16RET_VALIST_BYREFERSt9__va_list( &ARGS$1 );
	#line 493 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_memcpy( X$1, vr$8, 32 );
	// 		CU_ASSERT( cva_arg( x, integer ) = arg1 ) [Macro Expansion: fbcu.CU_ASSERT_( (cva_arg( x, integer ) = arg1), __FILE__,$"functions\va_cva_api.bas", __LINE__,494, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_CVA_LIST_BYREF", "CU_ASSERT(" $"cva_arg( x, integer ) = arg1" ")" ) ]
	#line 494 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)X$1, int64) == ARG1$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 494, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_CVA_LIST_BYREF", (char*)"CU_ASSERT(cva_arg( x, integer ) = arg1)" );
	// 		CU_ASSERT( cva_arg( x, integer ) = arg2 ) [Macro Expansion: fbcu.CU_ASSERT_( (cva_arg( x, integer ) = arg2), __FILE__,$"functions\va_cva_api.bas", __LINE__,495, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_CVA_LIST_BYREF", "CU_ASSERT(" $"cva_arg( x, integer ) = arg2" ")" ) ]
	#line 495 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)X$1, int64) == ARG2$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 495, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_CVA_LIST_BYREF", (char*)"CU_ASSERT(cva_arg( x, integer ) = arg2)" );
	// 		CU_ASSERT( cva_arg( x, integer ) = arg3 ) [Macro Expansion: fbcu.CU_ASSERT_( (cva_arg( x, integer ) = arg3), __FILE__,$"functions\va_cva_api.bas", __LINE__,496, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_CVA_LIST_BYREF", "CU_ASSERT(" $"cva_arg( x, integer ) = arg3" ")" ) ]
	#line 496 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)X$1, int64) == ARG3$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 496, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_CVA_LIST_BYREF", (char*)"CU_ASSERT(cva_arg( x, integer ) = arg3)" );
	// 		CU_ASSERT( cva_arg( x, integer ) = arg4 ) [Macro Expansion: fbcu.CU_ASSERT_( (cva_arg( x, integer ) = arg4), __FILE__,$"functions\va_cva_api.bas", __LINE__,497, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_CVA_LIST_BYREF", "CU_ASSERT(" $"cva_arg( x, integer ) = arg4" ")" ) ]
	#line 497 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)X$1, int64) == ARG4$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 497, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_CVA_LIST_BYREF", (char*)"CU_ASSERT(cva_arg( x, integer ) = arg4)" );
	// 		cva_end( args )
	#line 498 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&ARGS$1);
	#line 499 "FUNCTIONS\\VA_CVA_API.BAS"
	label$179:;
#line 499 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 501 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API21CVA_LIST_RETURN_BYVALEv( void )
#line 501 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 501 "FUNCTIONS\\VA_CVA_API.BAS"
	label$180:;
	// #else
	// TEST_EMIT( fbcu_global, TMP_FBCUNIT_TEST_NAME cva_list_return_byval )sub cva_list_return_byval cdecl () FBCU_TRACE( "TEST" cva_list_return_byval ) #endif ]
	// 		proc_cva_list_byval( 4, 4000, 300, 200, 1 )
	#line 502 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API19PROC_CVA_LIST_BYVALEu7INTEGERz( 4ll, 4000ll, 300ll, 200ll, 1ll );
	// 		proc_cva_list_byref( 4, 4000, 300, 200, 1 )
	#line 503 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API19PROC_CVA_LIST_BYREFEu7INTEGERz( 4ll, 4000ll, 300ll, 200ll, 1ll );
	// 		proc_cva_list_ptr( 4, 4000, 300, 200, 1 )
	#line 504 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API17PROC_CVA_LIST_PTREu7INTEGERz( 4ll, 4000ll, 300ll, 200ll, 1ll );
	// 	END_TEST [Macro Expansion: #if defined( TMP_FBCUNIT_TEST_NAME )
	// #if defined( TMP_FBCUNIT_SUITE_NAME )
	// #if defined( TMP_FBCUNIT_TEST_GROUP_NAME )
	// END_TEST_EMIT( TMP_FBCUNIT_SUITE_NAME,fbc_tests.functions.va_cva_api, TMP_FBCUNIT_TEST_GROUP_NAME, TMP_FBCUNIT_TEST_NAME,cva_list_return_byval, false )end sub private sub cva_list_return_byval_ctor cdecl () constructor #if $"TMP_FBCUNIT_TEST_GROUP_NAME" > "" fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"TMP_FBCUNIT_TEST_GROUP_NAME" + "." + $"cva_list_return_byval", procptr(cva_list_return_byval), false ) #else fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"cva_list_return_byval", procptr(cva_list_return_byval), false ) #endif end sub FBCU_TRACE( "END_TEST" cva_list_return_byval ) #else
	#line 505 "FUNCTIONS\\VA_CVA_API.BAS"
	label$181:;
#line 505 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 513 "FUNCTIONS\\VA_CVA_API.BAS"
__builtin_va_list* _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API10SIDEFX_PTREPSt9__va_list( __builtin_va_list* ARGS$1 )
#line 513 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 513 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list* fb$result$1;
	#line 513 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_memset( &fb$result$1, 0, 8ll );
	#line 513 "FUNCTIONS\\VA_CVA_API.BAS"
	label$184:;
	// 		function = args
	#line 514 "FUNCTIONS\\VA_CVA_API.BAS"
	fb$result$1 = ARGS$1;
	#line 515 "FUNCTIONS\\VA_CVA_API.BAS"
	label$185:;
	#line 515 "FUNCTIONS\\VA_CVA_API.BAS"
	return fb$result$1;
#line 515 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 517 "FUNCTIONS\\VA_CVA_API.BAS"
__builtin_va_list* _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API12SIDEFX_BYREFERSt9__va_list( __builtin_va_list* ARGS$1 )
#line 517 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 517 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list* fb$result$1;
	#line 517 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_memset( &fb$result$1, 0, 8ll );
	#line 517 "FUNCTIONS\\VA_CVA_API.BAS"
	label$186:;
	// 		function = args
	#line 518 "FUNCTIONS\\VA_CVA_API.BAS"
	fb$result$1 = ARGS$1;
	#line 519 "FUNCTIONS\\VA_CVA_API.BAS"
	label$187:;
	#line 519 "FUNCTIONS\\VA_CVA_API.BAS"
	return fb$result$1;
#line 519 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 521 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API11PROC_SIDEFXEu7INTEGERz( int64 N$1, ... )
#line 521 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 521 "FUNCTIONS\\VA_CVA_API.BAS"
	label$188:;
	// 		dim args as cva_list
	#line 522 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list ARGS$1;
	#line 522 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_memset( &ARGS$1, 0, 32ll );
	// 		cva_start( args, n )
	#line 524 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_start( *(__builtin_va_list*)&ARGS$1, N$1);
	// 		dim arg1 as integer = cva_arg( args, integer )
	#line 525 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 ARG1$1;
	#line 525 "FUNCTIONS\\VA_CVA_API.BAS"
	ARG1$1 = __builtin_va_arg( *(__builtin_va_list*)&ARGS$1, int64);
	// 		dim arg2 as integer = cva_arg( args, integer )
	#line 526 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 ARG2$1;
	#line 526 "FUNCTIONS\\VA_CVA_API.BAS"
	ARG2$1 = __builtin_va_arg( *(__builtin_va_list*)&ARGS$1, int64);
	// 		dim arg3 as integer = cva_arg( args, integer )
	#line 527 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 ARG3$1;
	#line 527 "FUNCTIONS\\VA_CVA_API.BAS"
	ARG3$1 = __builtin_va_arg( *(__builtin_va_list*)&ARGS$1, int64);
	// 		dim arg4 as integer = cva_arg( args, integer )
	#line 528 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 ARG4$1;
	#line 528 "FUNCTIONS\\VA_CVA_API.BAS"
	ARG4$1 = __builtin_va_arg( *(__builtin_va_list*)&ARGS$1, int64);
	// 		dim arg5 as integer = cva_arg( args, integer )
	#line 529 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 ARG5$1;
	#line 529 "FUNCTIONS\\VA_CVA_API.BAS"
	ARG5$1 = __builtin_va_arg( *(__builtin_va_list*)&ARGS$1, int64);
	// 		dim arg6 as integer = cva_arg( args, integer )
	#line 530 "FUNCTIONS\\VA_CVA_API.BAS"
	int64 ARG6$1;
	#line 530 "FUNCTIONS\\VA_CVA_API.BAS"
	ARG6$1 = __builtin_va_arg( *(__builtin_va_list*)&ARGS$1, int64);
	// 		cva_end( args )
	#line 531 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&ARGS$1);
	// 		cva_start( args, n )
	#line 533 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_start( *(__builtin_va_list*)&ARGS$1, N$1);
	// #if VALIST_CAN_RETURN_BYVAL [Macro Expansion: 0 ]
	// 		CU_ASSERT_EQUAL( cva_arg( sidefx_byval( args ), integer ), arg1 ) [Macro Expansion: fbcu.CU_ASSERT_( ((cva_arg( sidefx_byval( args ), integer ))=(arg1)), __FILE__, __LINE__, __FUNCTION__, "CU_ASSERT_EQUAL(" $"cva_arg( sidefx_byval( args ), integer )" "," $"arg1" ")" ) ] 		CU_ASSERT_EQUAL( cva_arg( sidefx_byval( args ), integer ), arg1 ) [Macro Expansion: fbcu.CU_ASSERT_( ((cva_arg( sidefx_byval( args ), integer ))=(arg1)), __FILE__, __LINE__, __FUNCTION__, "CU_ASSERT_EQUAL(" $"cva_arg( sidefx_byval( args ), integer )" "," $"arg1" ")" ) ] #endif
	// 		CU_ASSERT_EQUAL( cva_arg( sidefx_byref( args ), integer ), arg1 ) [Macro Expansion: fbcu.CU_ASSERT_( ((cva_arg( sidefx_byref( args ), integer ))=(arg1)), __FILE__,$"functions\va_cva_api.bas", __LINE__,538, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_SIDEFX", "CU_ASSERT_EQUAL(" $"cva_arg( sidefx_byref( args ), integer )" "," $"arg1" ")" ) ]
	#line 538 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list* vr$8 = _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API12SIDEFX_BYREFERSt9__va_list( (__builtin_va_list*)&ARGS$1 );
	#line 538 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)vr$8, int64) == ARG1$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 538, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_SIDEFX", (char*)"CU_ASSERT_EQUAL(cva_arg( sidefx_byref( args ), integer ),arg1)" );
	// 		CU_ASSERT_EQUAL( cva_arg( sidefx_byref( args ), integer ), arg2 ) [Macro Expansion: fbcu.CU_ASSERT_( ((cva_arg( sidefx_byref( args ), integer ))=(arg2)), __FILE__,$"functions\va_cva_api.bas", __LINE__,539, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_SIDEFX", "CU_ASSERT_EQUAL(" $"cva_arg( sidefx_byref( args ), integer )" "," $"arg2" ")" ) ]
	#line 539 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list* vr$13 = _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API12SIDEFX_BYREFERSt9__va_list( (__builtin_va_list*)&ARGS$1 );
	#line 539 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)vr$13, int64) == ARG2$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 539, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_SIDEFX", (char*)"CU_ASSERT_EQUAL(cva_arg( sidefx_byref( args ), integer ),arg2)" );
	// 		CU_ASSERT_EQUAL( cva_arg( sidefx_byref( args ), integer ), arg3 ) [Macro Expansion: fbcu.CU_ASSERT_( ((cva_arg( sidefx_byref( args ), integer ))=(arg3)), __FILE__,$"functions\va_cva_api.bas", __LINE__,540, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_SIDEFX", "CU_ASSERT_EQUAL(" $"cva_arg( sidefx_byref( args ), integer )" "," $"arg3" ")" ) ]
	#line 540 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list* vr$18 = _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API12SIDEFX_BYREFERSt9__va_list( (__builtin_va_list*)&ARGS$1 );
	#line 540 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)vr$18, int64) == ARG3$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 540, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_SIDEFX", (char*)"CU_ASSERT_EQUAL(cva_arg( sidefx_byref( args ), integer ),arg3)" );
	// 		CU_ASSERT_EQUAL( cva_arg( *sidefx_ptr( @args ), integer ), arg4 ) [Macro Expansion: fbcu.CU_ASSERT_( ((cva_arg( *sidefx_ptr( @args ), integer ))=(arg4)), __FILE__,$"functions\va_cva_api.bas", __LINE__,541, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_SIDEFX", "CU_ASSERT_EQUAL(" $"cva_arg( *sidefx_ptr( @args ), integer )" "," $"arg4" ")" ) ]
	#line 541 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list* vr$23 = _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API10SIDEFX_PTREPSt9__va_list( (__builtin_va_list*)&ARGS$1 );
	#line 541 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)vr$23, int64) == ARG4$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 541, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_SIDEFX", (char*)"CU_ASSERT_EQUAL(cva_arg( *sidefx_ptr( @args ), integer ),arg4)" );
	// 		CU_ASSERT_EQUAL( cva_arg( *sidefx_ptr( @args ), integer ), arg5 ) [Macro Expansion: fbcu.CU_ASSERT_( ((cva_arg( *sidefx_ptr( @args ), integer ))=(arg5)), __FILE__,$"functions\va_cva_api.bas", __LINE__,542, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_SIDEFX", "CU_ASSERT_EQUAL(" $"cva_arg( *sidefx_ptr( @args ), integer )" "," $"arg5" ")" ) ]
	#line 542 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list* vr$28 = _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API10SIDEFX_PTREPSt9__va_list( (__builtin_va_list*)&ARGS$1 );
	#line 542 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)vr$28, int64) == ARG5$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 542, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_SIDEFX", (char*)"CU_ASSERT_EQUAL(cva_arg( *sidefx_ptr( @args ), integer ),arg5)" );
	// 		CU_ASSERT_EQUAL( cva_arg( *sidefx_ptr( @args ), integer ), arg6 ) [Macro Expansion: fbcu.CU_ASSERT_( ((cva_arg( *sidefx_ptr( @args ), integer ))=(arg6)), __FILE__,$"functions\va_cva_api.bas", __LINE__,543, __FUNCTION__,$"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_SIDEFX", "CU_ASSERT_EQUAL(" $"cva_arg( *sidefx_ptr( @args ), integer )" "," $"arg6" ")" ) ]
	#line 543 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_list* vr$33 = _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API10SIDEFX_PTREPSt9__va_list( (__builtin_va_list*)&ARGS$1 );
	#line 543 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)((int64)-(__builtin_va_arg( *(__builtin_va_list*)vr$33, int64) == ARG6$1) != 0ll), (char*)"functions\x5Cva_cva_api.bas", 543, (char*)"TESTS.FBC_TESTS.FUNCTIONS.VA_CVA_API.PROC_SIDEFX", (char*)"CU_ASSERT_EQUAL(cva_arg( *sidefx_ptr( @args ), integer ),arg6)" );
	// 		cva_end( args )
	#line 544 "FUNCTIONS\\VA_CVA_API.BAS"
	__builtin_va_end( *(__builtin_va_list*)&ARGS$1);
	#line 545 "FUNCTIONS\\VA_CVA_API.BAS"
	label$189:;
#line 545 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 547 "FUNCTIONS\\VA_CVA_API.BAS"
void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API12SIDE_EFFECTSEv( void )
#line 547 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 547 "FUNCTIONS\\VA_CVA_API.BAS"
	label$190:;
	// #else
	// TEST_EMIT( fbcu_global, TMP_FBCUNIT_TEST_NAME side_effects )sub side_effects cdecl () FBCU_TRACE( "TEST" side_effects ) #endif ]
	// 		proc_sidefx( 6, 600000, 50000, 4000, 300, 20, 1 )
	#line 548 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API11PROC_SIDEFXEu7INTEGERz( 6ll, 600000ll, 50000ll, 4000ll, 300ll, 20ll, 1ll );
	// 	END_TEST [Macro Expansion: #if defined( TMP_FBCUNIT_TEST_NAME )
	// #if defined( TMP_FBCUNIT_SUITE_NAME )
	// #if defined( TMP_FBCUNIT_TEST_GROUP_NAME )
	// END_TEST_EMIT( TMP_FBCUNIT_SUITE_NAME,fbc_tests.functions.va_cva_api, TMP_FBCUNIT_TEST_GROUP_NAME, TMP_FBCUNIT_TEST_NAME,side_effects, false )end sub private sub side_effects_ctor cdecl () constructor #if $"TMP_FBCUNIT_TEST_GROUP_NAME" > "" fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"TMP_FBCUNIT_TEST_GROUP_NAME" + "." + $"side_effects", procptr(side_effects), false ) #else fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"side_effects", procptr(side_effects), false ) #endif end sub FBCU_TRACE( "END_TEST" side_effects ) #else
	#line 549 "FUNCTIONS\\VA_CVA_API.BAS"
	label$191:;
#line 549 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 1 "FUNCTIONS\\VA_CVA_API.BAS"
__attribute__(( constructor )) static void fb_ctor__va_cva_api( void )
#line 1 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 1 "FUNCTIONS\\VA_CVA_API.BAS"
	label$0:;
	// # include "fbcunit.bi"
	// #define SB1 -128
	// #define SB2 127
	// #define UB1 0
	// #define UB2 255
	// #define SS1 -32768
	// #define SS2 32767
	// #define US1 0
	// #define US2 65535
	// #define SL1 -2147483648
	// #define SL2 2147483647
	// #define UL1 0
	// #define UL2 4294967295
	// #define SLL1 (-9223372036854775807ll-1ll)
	// #define SLL2 9223372036854775807ll
	// #define ULL1 0
	// #define ULl2 18446744073709551615ull
	// #if sizeof(integer) = 4
	// 	#define UI1 UL1	#define UI2 UL2	#define SI1 SL1	#define SI2 SL2#else
	// 	#define UI1 ULL1
	// 	#define UI2 ULL2
	// 	#define SI1 SLL1
	// 	#define SI2 SLL2
	// #endif
	// SUITE( fbc_tests.functions.va_cva_api ) [Macro Expansion: #if defined( TMP_FBCUNIT_SUITE_NAME )
	// #error FBCUNIT: test suites can not be nested, or missing "END_SUITE" before "SUITE" #endif
	// #if $"fbc_tests.functions.va_cva_api" > ""
	// #define TMP_FBCUNIT_SUITE_NAME fbc_tests.functions.va_cva_api
	// #else
	// #define TMP_FBCUNIT_SUITE_NAME fbcu_global #endif
	// SUITE_EMIT( TMP_FBCUNIT_SUITE_NAME fbc_tests.functions.va_cva_api )namespace tests.fbc_tests.functions.va_cva_api
	// 	end sub
	// 	TEST( start ) [Macro Expansion: #if defined( TMP_FBCUNIT_TEST_NAME )
	// #error FBCUNIT: tests can not be nested or missing "END_TEST" #elseif defined( TMP_FBCUNIT_SUITE_IN_INIT )
	// #error FBCUNIT: missing "END_SUITE_INIT" before "TEST" #elseif defined( TMP_FBCUNIT_SUITE_IN_CLEANUP )
	// #error FBCUNIT: missing "END_SUITE_CLEANUP" before "TEST" #endif
	// #if $"start" > ""
	// #define TMP_FBCUNIT_TEST_NAME start
	// #else
	// #define TMP_FBCUNIT_TEST_NAME default #endif
	// #if defined( TMP_FBCUNIT_SUITE_NAME )
	// END_TEST_EMIT( TMP_FBCUNIT_SUITE_NAME,fbc_tests.functions.va_cva_api, , TMP_FBCUNIT_TEST_NAME,start, false )end sub
	// end sub
	// #endif
	// #else
	// #if defined( TMP_FBCUNIT_TEST_GROUP_NAME ) END_TEST_EMIT( fbcu_global, TMP_FBCUNIT_TEST_GROUP_NAME, TMP_FBCUNIT_TEST_NAME,start, true )end sub private sub start_ctor cdecl () constructor #if $"TMP_FBCUNIT_TEST_GROUP_NAME" > "" fbcu.add_test( $"fbcu_global", $"TMP_FBCUNIT_TEST_GROUP_NAME" + "." + $"start", procptr(start), true ) #else fbcu.add_test( $"fbcu_global", $"start", procptr(start), true ) #endif end sub FBCU_TRACE( "END_TEST" start ) #else END_TEST_EMIT( fbcu_global, , TMP_FBCUNIT_TEST_NAME,start, true )end sub private sub start_ctor cdecl () constructor #if "" > "" fbcu.add_test( $"fbcu_global", "" + "." + $"start", procptr(start), true ) #else fbcu.add_test( $"fbcu_global", $"start", procptr(start), true ) #endif end sub FBCU_TRACE( "END_TEST" start ) #endif #endif
	// #else
	// #error FBCUNIT: mismatched "END_TEST" #endif
	// #undef TMP_FBCUNIT_TEST_NAME ]
	// 	end sub
	// 	TEST( copy ) [Macro Expansion: #if defined( TMP_FBCUNIT_TEST_NAME )
	// #error FBCUNIT: tests can not be nested or missing "END_TEST" #elseif defined( TMP_FBCUNIT_SUITE_IN_INIT )
	// #error FBCUNIT: missing "END_SUITE_INIT" before "TEST" #elseif defined( TMP_FBCUNIT_SUITE_IN_CLEANUP )
	// #error FBCUNIT: missing "END_SUITE_CLEANUP" before "TEST" #endif
	// #if $"copy" > ""
	// #define TMP_FBCUNIT_TEST_NAME copy
	// #else
	// #define TMP_FBCUNIT_TEST_NAME default #endif
	// #if defined( TMP_FBCUNIT_SUITE_NAME )
	// END_TEST_EMIT( TMP_FBCUNIT_SUITE_NAME,fbc_tests.functions.va_cva_api, , TMP_FBCUNIT_TEST_NAME,copy, false )end sub
	// end sub
	// #endif
	// #else
	// #if defined( TMP_FBCUNIT_TEST_GROUP_NAME ) END_TEST_EMIT( fbcu_global, TMP_FBCUNIT_TEST_GROUP_NAME, TMP_FBCUNIT_TEST_NAME,copy, true )end sub private sub copy_ctor cdecl () constructor #if $"TMP_FBCUNIT_TEST_GROUP_NAME" > "" fbcu.add_test( $"fbcu_global", $"TMP_FBCUNIT_TEST_GROUP_NAME" + "." + $"copy", procptr(copy), true ) #else fbcu.add_test( $"fbcu_global", $"copy", procptr(copy), true ) #endif end sub FBCU_TRACE( "END_TEST" copy ) #else END_TEST_EMIT( fbcu_global, , TMP_FBCUNIT_TEST_NAME,copy, true )end sub private sub copy_ctor cdecl () constructor #if "" > "" fbcu.add_test( $"fbcu_global", "" + "." + $"copy", procptr(copy), true ) #else fbcu.add_test( $"fbcu_global", $"copy", procptr(copy), true ) #endif end sub FBCU_TRACE( "END_TEST" copy ) #endif #endif
	// #else
	// #error FBCUNIT: mismatched "END_TEST" #endif
	// #undef TMP_FBCUNIT_TEST_NAME ]
	// 	end sub
	// 	end sub
	// 	TEST( list ) [Macro Expansion: #if defined( TMP_FBCUNIT_TEST_NAME )
	// #error FBCUNIT: tests can not be nested or missing "END_TEST" #elseif defined( TMP_FBCUNIT_SUITE_IN_INIT )
	// #error FBCUNIT: missing "END_SUITE_INIT" before "TEST" #elseif defined( TMP_FBCUNIT_SUITE_IN_CLEANUP )
	// #error FBCUNIT: missing "END_SUITE_CLEANUP" before "TEST" #endif
	// #if $"list" > ""
	// #define TMP_FBCUNIT_TEST_NAME list
	// #else
	// #define TMP_FBCUNIT_TEST_NAME default #endif
	// #if defined( TMP_FBCUNIT_SUITE_NAME )
	// END_TEST_EMIT( TMP_FBCUNIT_SUITE_NAME,fbc_tests.functions.va_cva_api, , TMP_FBCUNIT_TEST_NAME,list, false )end sub
	// end sub
	// #endif
	// #else
	// #if defined( TMP_FBCUNIT_TEST_GROUP_NAME ) END_TEST_EMIT( fbcu_global, TMP_FBCUNIT_TEST_GROUP_NAME, TMP_FBCUNIT_TEST_NAME,list, true )end sub private sub list_ctor cdecl () constructor #if $"TMP_FBCUNIT_TEST_GROUP_NAME" > "" fbcu.add_test( $"fbcu_global", $"TMP_FBCUNIT_TEST_GROUP_NAME" + "." + $"list", procptr(list), true ) #else fbcu.add_test( $"fbcu_global", $"list", procptr(list), true ) #endif end sub FBCU_TRACE( "END_TEST" list ) #else END_TEST_EMIT( fbcu_global, , TMP_FBCUNIT_TEST_NAME,list, true )end sub private sub list_ctor cdecl () constructor #if "" > "" fbcu.add_test( $"fbcu_global", "" + "." + $"list", procptr(list), true ) #else fbcu.add_test( $"fbcu_global", $"list", procptr(list), true ) #endif end sub FBCU_TRACE( "END_TEST" list ) #endif #endif
	// #else
	// #error FBCUNIT: mismatched "END_TEST" #endif
	// #undef TMP_FBCUNIT_TEST_NAME ]
	// 	#macro DEFN_BASICTYPE( T, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10 )		sub f_test_arg_##t cdecl( byval n as integer, ... )						dim as cva_list x = any			cva_start( x, n )			'' ignore n, there is always 10 arguments			CU_ASSERT_EQUAL( cva_arg( x, T ), ARG1 )			CU_ASSERT_EQUAL( cva_arg( x, T ), ARG2 )			CU_ASSERT_EQUAL( cva_arg( x, T ), ARG3 )			CU_ASSERT_EQUAL( cva_arg( x, T ), ARG4 )			CU_ASSERT_EQUAL( cva_arg( x, T ), ARG5 )			CU_ASSERT_EQUAL( cva_arg( x, T ), ARG6 )			CU_ASSERT_EQUAL( cva_arg( x, T ), ARG7 )			CU_ASSERT_EQUAL( cva_arg( x, T ), ARG8 )			CU_ASSERT_EQUAL( cva_arg( x, T ), ARG9 )			CU_ASSERT_EQUAL( cva_arg( x, T ), ARG10 )			cva_end( x )		end sub	#endmacro
	// end sub ]
	// end sub ]
	// end sub ]
	// end sub ]
	// end sub ]
	// end sub ]
	// end sub ]
	// end sub ]
	// end sub ]
	// end sub ]
	// 	#macro TEST_BASICTYPE( T, CV, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10 )		f_test_arg_##T ( 10, CV(ARG1), CV(ARG2), CV(ARG3), CV(ARG4), CV(ARG5), CV(ARG6), CV(ARG7), CV(ARG8), CV(ARG9), CV(ARG10) )	#endmacro
	// 	TEST( basic_types ) [Macro Expansion: #if defined( TMP_FBCUNIT_TEST_NAME )
	// #error FBCUNIT: tests can not be nested or missing "END_TEST" #elseif defined( TMP_FBCUNIT_SUITE_IN_INIT )
	// #error FBCUNIT: missing "END_SUITE_INIT" before "TEST" #elseif defined( TMP_FBCUNIT_SUITE_IN_CLEANUP )
	// #error FBCUNIT: missing "END_SUITE_CLEANUP" before "TEST" #endif
	// #if $"basic_types" > ""
	// #define TMP_FBCUNIT_TEST_NAME basic_types
	// #else
	// #define TMP_FBCUNIT_TEST_NAME default #endif
	// #if defined( TMP_FBCUNIT_SUITE_NAME )
	// END_TEST_EMIT( TMP_FBCUNIT_SUITE_NAME,fbc_tests.functions.va_cva_api, , TMP_FBCUNIT_TEST_NAME,basic_types, false )end sub
	// end sub
	// #endif
	// #else
	// #if defined( TMP_FBCUNIT_TEST_GROUP_NAME ) END_TEST_EMIT( fbcu_global, TMP_FBCUNIT_TEST_GROUP_NAME, TMP_FBCUNIT_TEST_NAME,basic_types, true )end sub private sub basic_types_ctor cdecl () constructor #if $"TMP_FBCUNIT_TEST_GROUP_NAME" > "" fbcu.add_test( $"fbcu_global", $"TMP_FBCUNIT_TEST_GROUP_NAME" + "." + $"basic_types", procptr(basic_types), true ) #else fbcu.add_test( $"fbcu_global", $"basic_types", procptr(basic_types), true ) #endif end sub FBCU_TRACE( "END_TEST" basic_types ) #else END_TEST_EMIT( fbcu_global, , TMP_FBCUNIT_TEST_NAME,basic_types, true )end sub private sub basic_types_ctor cdecl () constructor #if "" > "" fbcu.add_test( $"fbcu_global", "" + "." + $"basic_types", procptr(basic_types), true ) #else fbcu.add_test( $"fbcu_global", $"basic_types", procptr(basic_types), true ) #endif end sub FBCU_TRACE( "END_TEST" basic_types ) #endif #endif
	// #else
	// #error FBCUNIT: mismatched "END_TEST" #endif
	// #undef TMP_FBCUNIT_TEST_NAME ]
	// 	#macro sum_cva_list_args( expr )		'' iterate using a copy		dim x as cva_list		cva_copy( x, expr )		dim d as integer = 0		for i as integer = 1 to n			d += cva_arg( x, integer )		next		cva_end( x )		'' iterate using argument passed		dim c as integer = 0		for i as integer = 1 to n			c += cva_arg( expr, integer )		next		CU_ASSERT( c = d )	#endmacro
	// 	end function
	// 	end function
	// 	end function
	// 	end function
	// 	end function
	// 	end sub
	// 	TEST( arguments ) [Macro Expansion: #if defined( TMP_FBCUNIT_TEST_NAME )
	// #error FBCUNIT: tests can not be nested or missing "END_TEST" #elseif defined( TMP_FBCUNIT_SUITE_IN_INIT )
	// #error FBCUNIT: missing "END_SUITE_INIT" before "TEST" #elseif defined( TMP_FBCUNIT_SUITE_IN_CLEANUP )
	// #error FBCUNIT: missing "END_SUITE_CLEANUP" before "TEST" #endif
	// #if $"arguments" > ""
	// #define TMP_FBCUNIT_TEST_NAME arguments
	// #else
	// #define TMP_FBCUNIT_TEST_NAME default #endif
	// #if defined( TMP_FBCUNIT_SUITE_NAME )
	// END_TEST_EMIT( TMP_FBCUNIT_SUITE_NAME,fbc_tests.functions.va_cva_api, , TMP_FBCUNIT_TEST_NAME,arguments, false )end sub
	// end sub
	// #endif
	// #else
	// #if defined( TMP_FBCUNIT_TEST_GROUP_NAME ) END_TEST_EMIT( fbcu_global, TMP_FBCUNIT_TEST_GROUP_NAME, TMP_FBCUNIT_TEST_NAME,arguments, true )end sub private sub arguments_ctor cdecl () constructor #if $"TMP_FBCUNIT_TEST_GROUP_NAME" > "" fbcu.add_test( $"fbcu_global", $"TMP_FBCUNIT_TEST_GROUP_NAME" + "." + $"arguments", procptr(arguments), true ) #else fbcu.add_test( $"fbcu_global", $"arguments", procptr(arguments), true ) #endif end sub FBCU_TRACE( "END_TEST" arguments ) #else END_TEST_EMIT( fbcu_global, , TMP_FBCUNIT_TEST_NAME,arguments, true )end sub private sub arguments_ctor cdecl () constructor #if "" > "" fbcu.add_test( $"fbcu_global", "" + "." + $"arguments", procptr(arguments), true ) #else fbcu.add_test( $"fbcu_global", $"arguments", procptr(arguments), true ) #endif end sub FBCU_TRACE( "END_TEST" arguments ) #endif #endif
	// #else
	// #error FBCUNIT: mismatched "END_TEST" #endif
	// #undef TMP_FBCUNIT_TEST_NAME ]
	// 	end type
	// 	type udt_vararg
	// 		f1 as cva_list
	// 		f2 as cva_list
	// 	end sub
	// 	TEST( complex ) [Macro Expansion: #if defined( TMP_FBCUNIT_TEST_NAME )
	// #error FBCUNIT: tests can not be nested or missing "END_TEST" #elseif defined( TMP_FBCUNIT_SUITE_IN_INIT )
	// #error FBCUNIT: missing "END_SUITE_INIT" before "TEST" #elseif defined( TMP_FBCUNIT_SUITE_IN_CLEANUP )
	// #error FBCUNIT: missing "END_SUITE_CLEANUP" before "TEST" #endif
	// #if $"complex" > ""
	// #define TMP_FBCUNIT_TEST_NAME complex
	// #else
	// #define TMP_FBCUNIT_TEST_NAME default #endif
	// #if defined( TMP_FBCUNIT_SUITE_NAME )
	// END_TEST_EMIT( TMP_FBCUNIT_SUITE_NAME,fbc_tests.functions.va_cva_api, , TMP_FBCUNIT_TEST_NAME,complex, false )end sub
	// end sub
	// #endif
	// #else
	// #if defined( TMP_FBCUNIT_TEST_GROUP_NAME ) END_TEST_EMIT( fbcu_global, TMP_FBCUNIT_TEST_GROUP_NAME, TMP_FBCUNIT_TEST_NAME,complex, true )end sub private sub complex_ctor cdecl () constructor #if $"TMP_FBCUNIT_TEST_GROUP_NAME" > "" fbcu.add_test( $"fbcu_global", $"TMP_FBCUNIT_TEST_GROUP_NAME" + "." + $"complex", procptr(complex), true ) #else fbcu.add_test( $"fbcu_global", $"complex", procptr(complex), true ) #endif end sub FBCU_TRACE( "END_TEST" complex ) #else END_TEST_EMIT( fbcu_global, , TMP_FBCUNIT_TEST_NAME,complex, true )end sub private sub complex_ctor cdecl () constructor #if "" > "" fbcu.add_test( $"fbcu_global", "" + "." + $"complex", procptr(complex), true ) #else fbcu.add_test( $"fbcu_global", $"complex", procptr(complex), true ) #endif end sub FBCU_TRACE( "END_TEST" complex ) #endif #endif
	// #else
	// #error FBCUNIT: mismatched "END_TEST" #endif
	// #undef TMP_FBCUNIT_TEST_NAME ]
	// #if (defined(__FB_UNIX__) and defined(__FB_64BIT__)) or defined(__FB_ARM__) or (defined(__FB_PPC__) and (not defined(__FB_64BIT__)))
	// 	#if ENABLE_CHECK_BUGS
	// 		#define VALIST_CAN_RETURN_BYVAL 1	#else
	// 		#define VALIST_CAN_RETURN_BYVAL 0
	// 	#endif
	// #else
	// 	'' otherwise, assume it's OK for the platform (needs	'' testing on all target/arch, though).	#define VALIST_CAN_RETURN_BYVAL 1#endif
	// 	end function
	// 	end sub
	// #if VALIST_CAN_RETURN_BYVAL [Macro Expansion: 0 ]
	// 	function ret_valist_byval( byval args as cva_list ) as cva_list		function = args	end function	sub proc_cva_list_byval cdecl( byval n as integer, ... )		dim args as cva_list		dim x as cva_list		dim i as integer		cva_start( args, n )		dim arg1 as integer = cva_arg( args, integer )		dim arg2 as integer = cva_arg( args, integer )		dim arg3 as integer = cva_arg( args, integer )		dim arg4 as integer = cva_arg( args, integer )		cva_end( args )		cva_start( args, n )		x = ret_valist_byval( args )		CU_ASSERT( cva_arg( x, integer ) = arg1 ) [Macro Expansion: fbcu.CU_ASSERT_( (cva_arg( x, integer ) = arg1), __FILE__, __LINE__, __FUNCTION__, "CU_ASSERT(" $"cva_arg( x, integer ) = arg1" ")" ) ] 		CU_ASSERT( cva_arg( x, integer ) = arg2 ) [Macro Expansion: fbcu.CU_ASSERT_( (cva_arg( x, integer ) = arg2), __FILE__, __LINE__, __FUNCTION__, "CU_ASSERT(" $"cva_arg( x, integer ) = arg2" ")" ) ] 		CU_ASSERT( cva_arg( x, integer ) = arg3 ) [Macro Expansion: fbcu.CU_ASSERT_( (cva_arg( x, integer ) = arg3), __FILE__, __LINE__, __FUNCTION__, "CU_ASSERT(" $"cva_arg( x, integer ) = arg3" ")" ) ] 		CU_ASSERT( cva_arg( x, integer ) = arg4 ) [Macro Expansion: fbcu.CU_ASSERT_( (cva_arg( x, integer ) = arg4), __FILE__, __LINE__, __FUNCTION__, "CU_ASSERT(" $"cva_arg( x, integer ) = arg4" ")" ) ] 		cva_end( args )	end sub#else
	// 	end sub
	// #endif
	// 	end function
	// 	end sub
	// 	TEST( cva_list_return_byval ) [Macro Expansion: #if defined( TMP_FBCUNIT_TEST_NAME )
	// #error FBCUNIT: tests can not be nested or missing "END_TEST" #elseif defined( TMP_FBCUNIT_SUITE_IN_INIT )
	// #error FBCUNIT: missing "END_SUITE_INIT" before "TEST" #elseif defined( TMP_FBCUNIT_SUITE_IN_CLEANUP )
	// #error FBCUNIT: missing "END_SUITE_CLEANUP" before "TEST" #endif
	// #if $"cva_list_return_byval" > ""
	// #define TMP_FBCUNIT_TEST_NAME cva_list_return_byval
	// #else
	// #define TMP_FBCUNIT_TEST_NAME default #endif
	// #if defined( TMP_FBCUNIT_SUITE_NAME )
	// END_TEST_EMIT( TMP_FBCUNIT_SUITE_NAME,fbc_tests.functions.va_cva_api, , TMP_FBCUNIT_TEST_NAME,cva_list_return_byval, false )end sub
	// end sub
	// #endif
	// #else
	// #if defined( TMP_FBCUNIT_TEST_GROUP_NAME ) END_TEST_EMIT( fbcu_global, TMP_FBCUNIT_TEST_GROUP_NAME, TMP_FBCUNIT_TEST_NAME,cva_list_return_byval, true )end sub private sub cva_list_return_byval_ctor cdecl () constructor #if $"TMP_FBCUNIT_TEST_GROUP_NAME" > "" fbcu.add_test( $"fbcu_global", $"TMP_FBCUNIT_TEST_GROUP_NAME" + "." + $"cva_list_return_byval", procptr(cva_list_return_byval), true ) #else fbcu.add_test( $"fbcu_global", $"cva_list_return_byval", procptr(cva_list_return_byval), true ) #endif end sub FBCU_TRACE( "END_TEST" cva_list_return_byval ) #else END_TEST_EMIT( fbcu_global, , TMP_FBCUNIT_TEST_NAME,cva_list_return_byval, true )end sub private sub cva_list_return_byval_ctor cdecl () constructor #if "" > "" fbcu.add_test( $"fbcu_global", "" + "." + $"cva_list_return_byval", procptr(cva_list_return_byval), true ) #else fbcu.add_test( $"fbcu_global", $"cva_list_return_byval", procptr(cva_list_return_byval), true ) #endif end sub FBCU_TRACE( "END_TEST" cva_list_return_byval ) #endif #endif
	// #else
	// #error FBCUNIT: mismatched "END_TEST" #endif
	// #undef TMP_FBCUNIT_TEST_NAME ]
	// #if VALIST_CAN_RETURN_BYVAL [Macro Expansion: 0 ]
	// 	function sidefx_byval( byval args as cva_list ) as cva_list		function = args	end function#endif
	// 	end function
	// 	end function
	// 	end sub
	// 	TEST( side_effects ) [Macro Expansion: #if defined( TMP_FBCUNIT_TEST_NAME )
	// #error FBCUNIT: tests can not be nested or missing "END_TEST" #elseif defined( TMP_FBCUNIT_SUITE_IN_INIT )
	// #error FBCUNIT: missing "END_SUITE_INIT" before "TEST" #elseif defined( TMP_FBCUNIT_SUITE_IN_CLEANUP )
	// #error FBCUNIT: missing "END_SUITE_CLEANUP" before "TEST" #endif
	// #if $"side_effects" > ""
	// #define TMP_FBCUNIT_TEST_NAME side_effects
	// #else
	// #define TMP_FBCUNIT_TEST_NAME default #endif
	// #if defined( TMP_FBCUNIT_SUITE_NAME )
	// END_TEST_EMIT( TMP_FBCUNIT_SUITE_NAME,fbc_tests.functions.va_cva_api, , TMP_FBCUNIT_TEST_NAME,side_effects, false )end sub
	// end sub
	// #endif
	// #else
	// #if defined( TMP_FBCUNIT_TEST_GROUP_NAME ) END_TEST_EMIT( fbcu_global, TMP_FBCUNIT_TEST_GROUP_NAME, TMP_FBCUNIT_TEST_NAME,side_effects, true )end sub private sub side_effects_ctor cdecl () constructor #if $"TMP_FBCUNIT_TEST_GROUP_NAME" > "" fbcu.add_test( $"fbcu_global", $"TMP_FBCUNIT_TEST_GROUP_NAME" + "." + $"side_effects", procptr(side_effects), true ) #else fbcu.add_test( $"fbcu_global", $"side_effects", procptr(side_effects), true ) #endif end sub FBCU_TRACE( "END_TEST" side_effects ) #else END_TEST_EMIT( fbcu_global, , TMP_FBCUNIT_TEST_NAME,side_effects, true )end sub private sub side_effects_ctor cdecl () constructor #if "" > "" fbcu.add_test( $"fbcu_global", "" + "." + $"side_effects", procptr(side_effects), true ) #else fbcu.add_test( $"fbcu_global", $"side_effects", procptr(side_effects), true ) #endif end sub FBCU_TRACE( "END_TEST" side_effects ) #endif #endif
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
	#line 551 "FUNCTIONS\\VA_CVA_API.BAS"
	label$1:;
#line 551 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 61 "FUNCTIONS\\VA_CVA_API.BAS"
__attribute__(( constructor )) static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API10START_CTOREv( void )
#line 61 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 61 "FUNCTIONS\\VA_CVA_API.BAS"
	label$10:;
	// #if "" > ""
	// fbcu.add_test( $"fbc_tests.functions.va_cva_api", "" + "." + $"start", procptr(start), false ) #else
	// fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"start", procptr(start), false )
	#line 61 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU8ADD_TESTEPcS0_PFvvEb( (char*)"fbc_tests.functions.va_cva_api", (char*)"start", (tmp$5)&_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API5STARTEv, (boolean)0ll );
	// #endif
	#line 61 "FUNCTIONS\\VA_CVA_API.BAS"
	label$11:;
#line 61 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 104 "FUNCTIONS\\VA_CVA_API.BAS"
__attribute__(( constructor )) static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API9COPY_CTOREv( void )
#line 104 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 104 "FUNCTIONS\\VA_CVA_API.BAS"
	label$24:;
	// #if "" > ""
	// fbcu.add_test( $"fbc_tests.functions.va_cva_api", "" + "." + $"copy", procptr(copy), false ) #else
	// fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"copy", procptr(copy), false )
	#line 104 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU8ADD_TESTEPcS0_PFvvEb( (char*)"fbc_tests.functions.va_cva_api", (char*)"copy", (tmp$5)&_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API4COPYEv, (boolean)0ll );
	// #endif
	#line 104 "FUNCTIONS\\VA_CVA_API.BAS"
	label$25:;
#line 104 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 140 "FUNCTIONS\\VA_CVA_API.BAS"
__attribute__(( constructor )) static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API9LIST_CTOREv( void )
#line 140 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 140 "FUNCTIONS\\VA_CVA_API.BAS"
	label$40:;
	// #if "" > ""
	// fbcu.add_test( $"fbc_tests.functions.va_cva_api", "" + "." + $"list", procptr(list), false ) #else
	// fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"list", procptr(list), false )
	#line 140 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU8ADD_TESTEPcS0_PFvvEb( (char*)"fbc_tests.functions.va_cva_api", (char*)"list", (tmp$5)&_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API4LISTEv, (boolean)0ll );
	// #endif
	#line 140 "FUNCTIONS\\VA_CVA_API.BAS"
	label$41:;
#line 140 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 210 "FUNCTIONS\\VA_CVA_API.BAS"
__attribute__(( constructor )) static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API16BASIC_TYPES_CTOREv( void )
#line 210 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 210 "FUNCTIONS\\VA_CVA_API.BAS"
	label$64:;
	// #if "" > ""
	// fbcu.add_test( $"fbc_tests.functions.va_cva_api", "" + "." + $"basic_types", procptr(basic_types), false ) #else
	// fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"basic_types", procptr(basic_types), false )
	#line 210 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU8ADD_TESTEPcS0_PFvvEb( (char*)"fbc_tests.functions.va_cva_api", (char*)"basic_types", (tmp$5)&_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API11BASIC_TYPESEv, (boolean)0ll );
	// #endif
	#line 210 "FUNCTIONS\\VA_CVA_API.BAS"
	label$65:;
#line 210 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 347 "FUNCTIONS\\VA_CVA_API.BAS"
__attribute__(( constructor )) static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API14ARGUMENTS_CTOREv( void )
#line 347 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 347 "FUNCTIONS\\VA_CVA_API.BAS"
	label$120:;
	// #if "" > ""
	// fbcu.add_test( $"fbc_tests.functions.va_cva_api", "" + "." + $"arguments", procptr(arguments), false ) #else
	// fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"arguments", procptr(arguments), false )
	#line 347 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU8ADD_TESTEPcS0_PFvvEb( (char*)"fbc_tests.functions.va_cva_api", (char*)"arguments", (tmp$5)&_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API9ARGUMENTSEv, (boolean)0ll );
	// #endif
	#line 347 "FUNCTIONS\\VA_CVA_API.BAS"
	label$121:;
#line 347 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 398 "FUNCTIONS\\VA_CVA_API.BAS"
__attribute__(( constructor )) static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API12COMPLEX_CTOREv( void )
#line 398 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 398 "FUNCTIONS\\VA_CVA_API.BAS"
	label$168:;
	// #if "" > ""
	// fbcu.add_test( $"fbc_tests.functions.va_cva_api", "" + "." + $"complex", procptr(complex), false ) #else
	// fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"complex", procptr(complex), false )
	#line 398 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU8ADD_TESTEPcS0_PFvvEb( (char*)"fbc_tests.functions.va_cva_api", (char*)"complex", (tmp$5)&_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API7COMPLEXEv, (boolean)0ll );
	// #endif
	#line 398 "FUNCTIONS\\VA_CVA_API.BAS"
	label$169:;
#line 398 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 505 "FUNCTIONS\\VA_CVA_API.BAS"
__attribute__(( constructor )) static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API26CVA_LIST_RETURN_BYVAL_CTOREv( void )
#line 505 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 505 "FUNCTIONS\\VA_CVA_API.BAS"
	label$182:;
	// #if "" > ""
	// fbcu.add_test( $"fbc_tests.functions.va_cva_api", "" + "." + $"cva_list_return_byval", procptr(cva_list_return_byval), false ) #else
	// fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"cva_list_return_byval", procptr(cva_list_return_byval), false )
	#line 505 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU8ADD_TESTEPcS0_PFvvEb( (char*)"fbc_tests.functions.va_cva_api", (char*)"cva_list_return_byval", (tmp$5)&_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API21CVA_LIST_RETURN_BYVALEv, (boolean)0ll );
	// #endif
	#line 505 "FUNCTIONS\\VA_CVA_API.BAS"
	label$183:;
#line 505 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 549 "FUNCTIONS\\VA_CVA_API.BAS"
__attribute__(( constructor )) static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API17SIDE_EFFECTS_CTOREv( void )
#line 549 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 549 "FUNCTIONS\\VA_CVA_API.BAS"
	label$192:;
	// #if "" > ""
	// fbcu.add_test( $"fbc_tests.functions.va_cva_api", "" + "." + $"side_effects", procptr(side_effects), false ) #else
	// fbcu.add_test( $"fbc_tests.functions.va_cva_api", $"side_effects", procptr(side_effects), false )
	#line 549 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU8ADD_TESTEPcS0_PFvvEb( (char*)"fbc_tests.functions.va_cva_api", (char*)"side_effects", (tmp$5)&_ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API12SIDE_EFFECTSEv, (boolean)0ll );
	// #endif
	#line 549 "FUNCTIONS\\VA_CVA_API.BAS"
	label$193:;
#line 549 "FUNCTIONS\\VA_CVA_API.BAS"
}

#line 551 "FUNCTIONS\\VA_CVA_API.BAS"
__attribute__(( constructor )) static void _ZN5TESTS9FBC_TESTS9FUNCTIONS10VA_CVA_API13SUITE_CTOR551Ev( void )
#line 551 "FUNCTIONS\\VA_CVA_API.BAS"
{
	#line 551 "FUNCTIONS\\VA_CVA_API.BAS"
	label$194:;
	// #if (defined( TMP_FBCUNIT_SUITE_HAVE_INIT ) andalso defined( TMP_FBCUNIT_SUITE_HAVE_CLEANUP ))
	// fbcu.add_suite( $"fbc_tests.functions.va_cva_api", procptr(tests.fbc_tests.functions.va_cva_api.init), procptr(tests.fbc_tests.functions.va_cva_api.cleanup) ) #elseif defined( TMP_FBCUNIT_SUITE_HAVE_INIT )
	// fbcu.add_suite( $"fbc_tests.functions.va_cva_api", procptr(tests.fbc_tests.functions.va_cva_api.init), FBCU_NULL ) #elseif defined( TMP_FBCUNIT_SUITE_HAVE_CLEANUP )
	// fbcu.add_suite( $"fbc_tests.functions.va_cva_api", FBCU_NULL, procptr(tests.fbc_tests.functions.va_cva_api.cleanup) ) #else
	// fbcu.add_suite( $"fbc_tests.functions.va_cva_api", FBCU_NULL,0, FBCU_NULL 0 )
	#line 551 "FUNCTIONS\\VA_CVA_API.BAS"
	_ZN4FBCU9ADD_SUITEEPcPFivES2_( (char*)"fbc_tests.functions.va_cva_api", (tmp$4)0ull, (tmp$4)0ull );
	// #endif
	#line 551 "FUNCTIONS\\VA_CVA_API.BAS"
	label$195:;
#line 551 "FUNCTIONS\\VA_CVA_API.BAS"
}

static const char __attribute__((used, section(".fbctinf"))) __fbctinf[] = "-l\0fbcunit\0-mt";
