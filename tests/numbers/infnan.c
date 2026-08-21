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
static void fb_ctor__infnan( void ) __attribute__(( constructor ));
void _ZN4FBCU9ADD_SUITEEPcPFivES2_( char*, tmp$4, tmp$4 );
void _ZN4FBCU8ADD_TESTEPcS0_PFvvEb( char*, char*, tmp$5, boolean );
void _ZN4FBCU10CU_ASSERT_EbPciS0_S0_( boolean, char*, int32, char*, char* );
static uint32 HREADSINGLEBITS( float* );
static uint64 HREADDOUBLEBITS( double* );
void _ZN5TESTS9FBC_TESTS7NUMBERS6INFNAN7DOUBLE_Ev( void );
static void _ZN5TESTS9FBC_TESTS7NUMBERS6INFNAN12DOUBLE__CTOREv( void ) __attribute__(( constructor ));
void _ZN5TESTS9FBC_TESTS7NUMBERS6INFNAN7SINGLE_Ev( void );
static void _ZN5TESTS9FBC_TESTS7NUMBERS6INFNAN12SINGLE__CTOREv( void ) __attribute__(( constructor ));
static void _ZN5TESTS9FBC_TESTS7NUMBERS6INFNAN13SUITE_CTOR240Ev( void ) __attribute__(( constructor ));

void _ZN5TESTS9FBC_TESTS7NUMBERS6INFNAN7DOUBLE_Ev( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$14:;
	{
		double D$2;
		__builtin_memset( &D$2, 0, 8 );
		D$2 = (-__builtin_nan( "" ));
		uint64 vr$2 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$2 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 80, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		double D$2;
		D$2 = (-__builtin_nan( "" ));
		uint64 vr$7 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$7 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 80, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		static double D$2;
		D$2 = (-__builtin_nan( "" ));
		uint64 vr$11 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$11 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 80, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		static double D$2 = (-__builtin_nan( "" ));
		uint64 vr$15 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$15 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 80, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		double D$2;
		__builtin_memset( &D$2, 0, 8 );
		D$2 = (-__builtin_nan( "" ));
		uint64 vr$21 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$21 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 81, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		double D$2;
		D$2 = (-__builtin_nan( "" ));
		uint64 vr$26 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$26 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 81, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		static double D$2;
		D$2 = (-__builtin_nan( "" ));
		uint64 vr$30 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$30 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 81, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		static double D$2 = (-__builtin_nan( "" ));
		uint64 vr$34 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$34 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 81, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		double D$2;
		__builtin_memset( &D$2, 0, 8 );
		D$2 = (-__builtin_nan( "" ));
		uint64 vr$40 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$40 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 82, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		double D$2;
		D$2 = (-__builtin_nan( "" ));
		uint64 vr$45 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$45 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 82, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		static double D$2;
		D$2 = (-__builtin_nan( "" ));
		uint64 vr$49 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$49 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 82, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		static double D$2 = (-__builtin_nan( "" ));
		uint64 vr$53 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$53 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 82, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		double D$2;
		__builtin_memset( &D$2, 0, 8 );
		D$2 = (-__builtin_nan( "" ));
		uint64 vr$59 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$59 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 83, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		double D$2;
		D$2 = (-__builtin_nan( "" ));
		uint64 vr$64 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$64 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 83, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		static double D$2;
		D$2 = (-__builtin_nan( "" ));
		uint64 vr$68 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$68 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 83, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		static double D$2 = (-__builtin_nan( "" ));
		uint64 vr$72 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$72 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 83, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		double D$2;
		__builtin_memset( &D$2, 0, 8 );
		D$2 = __builtin_inf();
		uint64 vr$78 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$78 == 9218868437227405312ull) != 0), (char*)"numbers/infnan.bas", 84, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = POSINFD)" );
	}
	{
		double D$2;
		D$2 = __builtin_inf();
		uint64 vr$82 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$82 == 9218868437227405312ull) != 0), (char*)"numbers/infnan.bas", 84, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = POSINFD)" );
	}
	{
		static double D$2;
		D$2 = __builtin_inf();
		uint64 vr$85 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$85 == 9218868437227405312ull) != 0), (char*)"numbers/infnan.bas", 84, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = POSINFD)" );
	}
	{
		static double D$2 = __builtin_inf();
		uint64 vr$88 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$88 == 9218868437227405312ull) != 0), (char*)"numbers/infnan.bas", 84, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = POSINFD)" );
	}
	{
		double D$2;
		__builtin_memset( &D$2, 0, 8 );
		D$2 = (-__builtin_inf());
		uint64 vr$93 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$93 == 18442240474082181120ull) != 0), (char*)"numbers/infnan.bas", 85, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = NEGINFD)" );
	}
	{
		double D$2;
		D$2 = (-__builtin_inf());
		uint64 vr$97 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$97 == 18442240474082181120ull) != 0), (char*)"numbers/infnan.bas", 85, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = NEGINFD)" );
	}
	{
		static double D$2;
		D$2 = (-__builtin_inf());
		uint64 vr$100 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$100 == 18442240474082181120ull) != 0), (char*)"numbers/infnan.bas", 85, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = NEGINFD)" );
	}
	{
		static double D$2 = (-__builtin_inf());
		uint64 vr$103 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$103 == 18442240474082181120ull) != 0), (char*)"numbers/infnan.bas", 85, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = NEGINFD)" );
	}
	{
		double D$2;
		__builtin_memset( &D$2, 0, 8 );
		D$2 = (-__builtin_inf());
		uint64 vr$108 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$108 == 18442240474082181120ull) != 0), (char*)"numbers/infnan.bas", 86, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = NEGINFD)" );
	}
	{
		double D$2;
		D$2 = (-__builtin_inf());
		uint64 vr$112 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$112 == 18442240474082181120ull) != 0), (char*)"numbers/infnan.bas", 86, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = NEGINFD)" );
	}
	{
		static double D$2;
		D$2 = (-__builtin_inf());
		uint64 vr$115 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$115 == 18442240474082181120ull) != 0), (char*)"numbers/infnan.bas", 86, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = NEGINFD)" );
	}
	{
		static double D$2 = (-__builtin_inf());
		uint64 vr$118 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$118 == 18442240474082181120ull) != 0), (char*)"numbers/infnan.bas", 86, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = NEGINFD)" );
	}
	{
		double D$2;
		__builtin_memset( &D$2, 0, 8 );
		D$2 = __builtin_inf();
		uint64 vr$123 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$123 == 9218868437227405312ull) != 0), (char*)"numbers/infnan.bas", 87, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = POSINFD)" );
	}
	{
		double D$2;
		D$2 = __builtin_inf();
		uint64 vr$127 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$127 == 9218868437227405312ull) != 0), (char*)"numbers/infnan.bas", 87, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = POSINFD)" );
	}
	{
		static double D$2;
		D$2 = __builtin_inf();
		uint64 vr$130 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$130 == 9218868437227405312ull) != 0), (char*)"numbers/infnan.bas", 87, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = POSINFD)" );
	}
	{
		static double D$2 = __builtin_inf();
		uint64 vr$133 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$133 == 9218868437227405312ull) != 0), (char*)"numbers/infnan.bas", 87, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = POSINFD)" );
	}
	{
		double D$2;
		__builtin_memset( &D$2, 0, 8 );
		D$2 = __builtin_nan( "" );
		uint64 vr$138 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$138 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 89, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		double D$2;
		D$2 = __builtin_nan( "" );
		uint64 vr$143 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$143 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 89, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		static double D$2;
		D$2 = __builtin_nan( "" );
		uint64 vr$147 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$147 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 89, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		static double D$2 = __builtin_nan( "" );
		uint64 vr$151 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$151 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 89, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		double D$2;
		__builtin_memset( &D$2, 0, 8 );
		D$2 = __builtin_nan( "" );
		uint64 vr$157 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$157 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 90, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		double D$2;
		D$2 = __builtin_nan( "" );
		uint64 vr$162 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$162 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 90, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		static double D$2;
		D$2 = __builtin_nan( "" );
		uint64 vr$166 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$166 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 90, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		static double D$2 = __builtin_nan( "" );
		uint64 vr$170 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$170 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 90, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		double D$2;
		__builtin_memset( &D$2, 0, 8 );
		D$2 = __builtin_nan( "" );
		uint64 vr$176 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$176 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 91, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		double D$2;
		D$2 = __builtin_nan( "" );
		uint64 vr$181 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$181 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 91, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		static double D$2;
		D$2 = __builtin_nan( "" );
		uint64 vr$185 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$185 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 91, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		static double D$2 = __builtin_nan( "" );
		uint64 vr$189 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$189 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 91, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		double D$2;
		__builtin_memset( &D$2, 0, 8 );
		D$2 = __builtin_nan( "" );
		uint64 vr$195 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$195 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 92, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		double D$2;
		D$2 = __builtin_nan( "" );
		uint64 vr$200 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$200 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 92, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		static double D$2;
		D$2 = __builtin_nan( "" );
		uint64 vr$204 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$204 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 92, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		static double D$2 = __builtin_nan( "" );
		uint64 vr$208 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$208 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 92, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( d ) and SGNMASK) = POSNAND)" );
	}
	{
		double D$2;
		__builtin_memset( &D$2, 0, 8 );
		D$2 = (-__builtin_inf());
		uint64 vr$214 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$214 == 18442240474082181120ull) != 0), (char*)"numbers/infnan.bas", 93, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = NEGINFD)" );
	}
	{
		double D$2;
		D$2 = (-__builtin_inf());
		uint64 vr$218 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$218 == 18442240474082181120ull) != 0), (char*)"numbers/infnan.bas", 93, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = NEGINFD)" );
	}
	{
		static double D$2;
		D$2 = (-__builtin_inf());
		uint64 vr$221 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$221 == 18442240474082181120ull) != 0), (char*)"numbers/infnan.bas", 93, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = NEGINFD)" );
	}
	{
		static double D$2 = (-__builtin_inf());
		uint64 vr$224 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$224 == 18442240474082181120ull) != 0), (char*)"numbers/infnan.bas", 93, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = NEGINFD)" );
	}
	{
		double D$2;
		__builtin_memset( &D$2, 0, 8 );
		D$2 = __builtin_inf();
		uint64 vr$229 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$229 == 9218868437227405312ull) != 0), (char*)"numbers/infnan.bas", 94, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = POSINFD)" );
	}
	{
		double D$2;
		D$2 = __builtin_inf();
		uint64 vr$233 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$233 == 9218868437227405312ull) != 0), (char*)"numbers/infnan.bas", 94, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = POSINFD)" );
	}
	{
		static double D$2;
		D$2 = __builtin_inf();
		uint64 vr$236 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$236 == 9218868437227405312ull) != 0), (char*)"numbers/infnan.bas", 94, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = POSINFD)" );
	}
	{
		static double D$2 = __builtin_inf();
		uint64 vr$239 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$239 == 9218868437227405312ull) != 0), (char*)"numbers/infnan.bas", 94, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = POSINFD)" );
	}
	{
		double D$2;
		__builtin_memset( &D$2, 0, 8 );
		D$2 = __builtin_inf();
		uint64 vr$244 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$244 == 9218868437227405312ull) != 0), (char*)"numbers/infnan.bas", 95, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = POSINFD)" );
	}
	{
		double D$2;
		D$2 = __builtin_inf();
		uint64 vr$248 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$248 == 9218868437227405312ull) != 0), (char*)"numbers/infnan.bas", 95, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = POSINFD)" );
	}
	{
		static double D$2;
		D$2 = __builtin_inf();
		uint64 vr$251 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$251 == 9218868437227405312ull) != 0), (char*)"numbers/infnan.bas", 95, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = POSINFD)" );
	}
	{
		static double D$2 = __builtin_inf();
		uint64 vr$254 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$254 == 9218868437227405312ull) != 0), (char*)"numbers/infnan.bas", 95, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = POSINFD)" );
	}
	{
		double D$2;
		__builtin_memset( &D$2, 0, 8 );
		D$2 = (-__builtin_inf());
		uint64 vr$259 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$259 == 18442240474082181120ull) != 0), (char*)"numbers/infnan.bas", 96, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = NEGINFD)" );
	}
	{
		double D$2;
		D$2 = (-__builtin_inf());
		uint64 vr$263 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$263 == 18442240474082181120ull) != 0), (char*)"numbers/infnan.bas", 96, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = NEGINFD)" );
	}
	{
		static double D$2;
		D$2 = (-__builtin_inf());
		uint64 vr$266 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$266 == 18442240474082181120ull) != 0), (char*)"numbers/infnan.bas", 96, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = NEGINFD)" );
	}
	{
		static double D$2 = (-__builtin_inf());
		uint64 vr$269 = HREADDOUBLEBITS( &D$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$269 == 18442240474082181120ull) != 0), (char*)"numbers/infnan.bas", 96, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( d ) = NEGINFD)" );
	}
	{
		double A$2;
		__builtin_memset( &A$2, 0, 8 );
		double B$2;
		__builtin_memset( &B$2, 0, 8 );
		double C$2;
		__builtin_memset( &C$2, 0, 8 );
		A$2 = 0x0p+0;
		B$2 = 0x0p+0;
		C$2 = A$2 / B$2;
		uint64 vr$277 = HREADDOUBLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$277 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 108, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( c ) and SGNMASK) = POSNAND)" );
	}
	{
		double A$2;
		__builtin_memset( &A$2, 0, 8 );
		double B$2;
		__builtin_memset( &B$2, 0, 8 );
		double C$2;
		__builtin_memset( &C$2, 0, 8 );
		A$2 = 0x0p+0;
		B$2 = -0x0p+0;
		C$2 = A$2 / B$2;
		uint64 vr$286 = HREADDOUBLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$286 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 109, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( c ) and SGNMASK) = POSNAND)" );
	}
	{
		double A$2;
		__builtin_memset( &A$2, 0, 8 );
		double B$2;
		__builtin_memset( &B$2, 0, 8 );
		double C$2;
		__builtin_memset( &C$2, 0, 8 );
		A$2 = -0x0p+0;
		B$2 = 0x0p+0;
		C$2 = A$2 / B$2;
		uint64 vr$295 = HREADDOUBLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$295 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 110, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( c ) and SGNMASK) = POSNAND)" );
	}
	{
		double A$2;
		__builtin_memset( &A$2, 0, 8 );
		double B$2;
		__builtin_memset( &B$2, 0, 8 );
		double C$2;
		__builtin_memset( &C$2, 0, 8 );
		A$2 = -0x0p+0;
		B$2 = -0x0p+0;
		C$2 = A$2 / B$2;
		uint64 vr$304 = HREADDOUBLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$304 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 111, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( c ) and SGNMASK) = POSNAND)" );
	}
	{
		double A$2;
		__builtin_memset( &A$2, 0, 8 );
		double B$2;
		__builtin_memset( &B$2, 0, 8 );
		double C$2;
		__builtin_memset( &C$2, 0, 8 );
		A$2 = 0x1.p+0;
		B$2 = 0x0p+0;
		C$2 = A$2 / B$2;
		uint64 vr$313 = HREADDOUBLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$313 == 9218868437227405312ull) != 0), (char*)"numbers/infnan.bas", 112, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( c ) = POSINFD)" );
	}
	{
		double A$2;
		__builtin_memset( &A$2, 0, 8 );
		double B$2;
		__builtin_memset( &B$2, 0, 8 );
		double C$2;
		__builtin_memset( &C$2, 0, 8 );
		A$2 = 0x1.p+0;
		B$2 = -0x0p+0;
		C$2 = A$2 / B$2;
		uint64 vr$321 = HREADDOUBLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$321 == 18442240474082181120ull) != 0), (char*)"numbers/infnan.bas", 113, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( c ) = NEGINFD)" );
	}
	{
		double A$2;
		__builtin_memset( &A$2, 0, 8 );
		double B$2;
		__builtin_memset( &B$2, 0, 8 );
		double C$2;
		__builtin_memset( &C$2, 0, 8 );
		A$2 = -0x1.p+0;
		B$2 = 0x0p+0;
		C$2 = A$2 / B$2;
		uint64 vr$329 = HREADDOUBLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$329 == 18442240474082181120ull) != 0), (char*)"numbers/infnan.bas", 114, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( c ) = NEGINFD)" );
	}
	{
		double A$2;
		__builtin_memset( &A$2, 0, 8 );
		double B$2;
		__builtin_memset( &B$2, 0, 8 );
		double C$2;
		__builtin_memset( &C$2, 0, 8 );
		A$2 = -0x1.p+0;
		B$2 = -0x0p+0;
		C$2 = A$2 / B$2;
		uint64 vr$337 = HREADDOUBLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$337 == 9218868437227405312ull) != 0), (char*)"numbers/infnan.bas", 115, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( c ) = POSINFD)" );
	}
	{
		double A$2;
		__builtin_memset( &A$2, 0, 8 );
		double B$2;
		__builtin_memset( &B$2, 0, 8 );
		double C$2;
		__builtin_memset( &C$2, 0, 8 );
		A$2 = 0x0p+0;
		B$2 = 0x0p+0;
		C$2 = -(A$2 / B$2);
		uint64 vr$346 = HREADDOUBLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$346 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 127, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( c ) and SGNMASK) = POSNAND)" );
	}
	{
		double A$2;
		__builtin_memset( &A$2, 0, 8 );
		double B$2;
		__builtin_memset( &B$2, 0, 8 );
		double C$2;
		__builtin_memset( &C$2, 0, 8 );
		A$2 = 0x0p+0;
		B$2 = -0x0p+0;
		C$2 = -(A$2 / B$2);
		uint64 vr$356 = HREADDOUBLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$356 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 128, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( c ) and SGNMASK) = POSNAND)" );
	}
	{
		double A$2;
		__builtin_memset( &A$2, 0, 8 );
		double B$2;
		__builtin_memset( &B$2, 0, 8 );
		double C$2;
		__builtin_memset( &C$2, 0, 8 );
		A$2 = -0x0p+0;
		B$2 = 0x0p+0;
		C$2 = -(A$2 / B$2);
		uint64 vr$366 = HREADDOUBLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$366 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 129, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( c ) and SGNMASK) = POSNAND)" );
	}
	{
		double A$2;
		__builtin_memset( &A$2, 0, 8 );
		double B$2;
		__builtin_memset( &B$2, 0, 8 );
		double C$2;
		__builtin_memset( &C$2, 0, 8 );
		A$2 = -0x0p+0;
		B$2 = -0x0p+0;
		C$2 = -(A$2 / B$2);
		uint64 vr$376 = HREADDOUBLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$376 & 9223372036854775807ull) == 9221120237041090560ull) != 0), (char*)"numbers/infnan.bas", 130, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT((hReadDoubleBits( c ) and SGNMASK) = POSNAND)" );
	}
	{
		double A$2;
		__builtin_memset( &A$2, 0, 8 );
		double B$2;
		__builtin_memset( &B$2, 0, 8 );
		double C$2;
		__builtin_memset( &C$2, 0, 8 );
		A$2 = 0x1.p+0;
		B$2 = 0x0p+0;
		C$2 = -(A$2 / B$2);
		uint64 vr$386 = HREADDOUBLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$386 == 18442240474082181120ull) != 0), (char*)"numbers/infnan.bas", 131, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( c ) = NEGINFD)" );
	}
	{
		double A$2;
		__builtin_memset( &A$2, 0, 8 );
		double B$2;
		__builtin_memset( &B$2, 0, 8 );
		double C$2;
		__builtin_memset( &C$2, 0, 8 );
		A$2 = 0x1.p+0;
		B$2 = -0x0p+0;
		C$2 = -(A$2 / B$2);
		uint64 vr$395 = HREADDOUBLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$395 == 9218868437227405312ull) != 0), (char*)"numbers/infnan.bas", 132, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( c ) = POSINFD)" );
	}
	{
		double A$2;
		__builtin_memset( &A$2, 0, 8 );
		double B$2;
		__builtin_memset( &B$2, 0, 8 );
		double C$2;
		__builtin_memset( &C$2, 0, 8 );
		A$2 = -0x1.p+0;
		B$2 = 0x0p+0;
		C$2 = -(A$2 / B$2);
		uint64 vr$404 = HREADDOUBLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$404 == 9218868437227405312ull) != 0), (char*)"numbers/infnan.bas", 133, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( c ) = POSINFD)" );
	}
	{
		double A$2;
		__builtin_memset( &A$2, 0, 8 );
		double B$2;
		__builtin_memset( &B$2, 0, 8 );
		double C$2;
		__builtin_memset( &C$2, 0, 8 );
		A$2 = -0x1.p+0;
		B$2 = -0x0p+0;
		C$2 = -(A$2 / B$2);
		uint64 vr$413 = HREADDOUBLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$413 == 18442240474082181120ull) != 0), (char*)"numbers/infnan.bas", 134, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.DOUBLE_", (char*)"CU_ASSERT(hReadDoubleBits( c ) = NEGINFD)" );
	}
	label$15:;
}

void _ZN5TESTS9FBC_TESTS7NUMBERS6INFNAN7SINGLE_Ev( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$18:;
	{
		float F$2;
		__builtin_memset( &F$2, 0, 4 );
		F$2 = (-__builtin_nanf( "" ));
		uint32 vr$2 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$2 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 183, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		float F$2;
		F$2 = (-__builtin_nanf( "" ));
		uint32 vr$7 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$7 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 183, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		static float F$2;
		F$2 = (-__builtin_nanf( "" ));
		uint32 vr$11 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$11 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 183, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		static float F$2 = (-__builtin_nanf( "" ));
		uint32 vr$15 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$15 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 183, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		float F$2;
		__builtin_memset( &F$2, 0, 4 );
		F$2 = (-__builtin_nanf( "" ));
		uint32 vr$21 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$21 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 184, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		float F$2;
		F$2 = (-__builtin_nanf( "" ));
		uint32 vr$26 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$26 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 184, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		static float F$2;
		F$2 = (-__builtin_nanf( "" ));
		uint32 vr$30 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$30 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 184, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		static float F$2 = (-__builtin_nanf( "" ));
		uint32 vr$34 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$34 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 184, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		float F$2;
		__builtin_memset( &F$2, 0, 4 );
		F$2 = (-__builtin_nanf( "" ));
		uint32 vr$40 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$40 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 185, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		float F$2;
		F$2 = (-__builtin_nanf( "" ));
		uint32 vr$45 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$45 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 185, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		static float F$2;
		F$2 = (-__builtin_nanf( "" ));
		uint32 vr$49 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$49 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 185, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		static float F$2 = (-__builtin_nanf( "" ));
		uint32 vr$53 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$53 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 185, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		float F$2;
		__builtin_memset( &F$2, 0, 4 );
		F$2 = (-__builtin_nanf( "" ));
		uint32 vr$59 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$59 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 186, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		float F$2;
		F$2 = (-__builtin_nanf( "" ));
		uint32 vr$64 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$64 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 186, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		static float F$2;
		F$2 = (-__builtin_nanf( "" ));
		uint32 vr$68 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$68 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 186, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		static float F$2 = (-__builtin_nanf( "" ));
		uint32 vr$72 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$72 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 186, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		float F$2;
		__builtin_memset( &F$2, 0, 4 );
		F$2 = __builtin_inff();
		uint32 vr$78 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$78 == 2139095040u) != 0), (char*)"numbers/infnan.bas", 187, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = POSINFF)" );
	}
	{
		float F$2;
		F$2 = __builtin_inff();
		uint32 vr$82 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$82 == 2139095040u) != 0), (char*)"numbers/infnan.bas", 187, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = POSINFF)" );
	}
	{
		static float F$2;
		F$2 = __builtin_inff();
		uint32 vr$85 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$85 == 2139095040u) != 0), (char*)"numbers/infnan.bas", 187, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = POSINFF)" );
	}
	{
		static float F$2 = __builtin_inff();
		uint32 vr$88 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$88 == 2139095040u) != 0), (char*)"numbers/infnan.bas", 187, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = POSINFF)" );
	}
	{
		float F$2;
		__builtin_memset( &F$2, 0, 4 );
		F$2 = (-__builtin_inff());
		uint32 vr$93 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$93 == 4286578688u) != 0), (char*)"numbers/infnan.bas", 188, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = NEGINFF)" );
	}
	{
		float F$2;
		F$2 = (-__builtin_inff());
		uint32 vr$97 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$97 == 4286578688u) != 0), (char*)"numbers/infnan.bas", 188, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = NEGINFF)" );
	}
	{
		static float F$2;
		F$2 = (-__builtin_inff());
		uint32 vr$100 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$100 == 4286578688u) != 0), (char*)"numbers/infnan.bas", 188, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = NEGINFF)" );
	}
	{
		static float F$2 = (-__builtin_inff());
		uint32 vr$103 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$103 == 4286578688u) != 0), (char*)"numbers/infnan.bas", 188, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = NEGINFF)" );
	}
	{
		float F$2;
		__builtin_memset( &F$2, 0, 4 );
		F$2 = (-__builtin_inff());
		uint32 vr$108 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$108 == 4286578688u) != 0), (char*)"numbers/infnan.bas", 189, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = NEGINFF)" );
	}
	{
		float F$2;
		F$2 = (-__builtin_inff());
		uint32 vr$112 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$112 == 4286578688u) != 0), (char*)"numbers/infnan.bas", 189, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = NEGINFF)" );
	}
	{
		static float F$2;
		F$2 = (-__builtin_inff());
		uint32 vr$115 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$115 == 4286578688u) != 0), (char*)"numbers/infnan.bas", 189, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = NEGINFF)" );
	}
	{
		static float F$2 = (-__builtin_inff());
		uint32 vr$118 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$118 == 4286578688u) != 0), (char*)"numbers/infnan.bas", 189, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = NEGINFF)" );
	}
	{
		float F$2;
		__builtin_memset( &F$2, 0, 4 );
		F$2 = __builtin_inff();
		uint32 vr$123 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$123 == 2139095040u) != 0), (char*)"numbers/infnan.bas", 190, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = POSINFF)" );
	}
	{
		float F$2;
		F$2 = __builtin_inff();
		uint32 vr$127 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$127 == 2139095040u) != 0), (char*)"numbers/infnan.bas", 190, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = POSINFF)" );
	}
	{
		static float F$2;
		F$2 = __builtin_inff();
		uint32 vr$130 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$130 == 2139095040u) != 0), (char*)"numbers/infnan.bas", 190, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = POSINFF)" );
	}
	{
		static float F$2 = __builtin_inff();
		uint32 vr$133 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$133 == 2139095040u) != 0), (char*)"numbers/infnan.bas", 190, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = POSINFF)" );
	}
	{
		float F$2;
		__builtin_memset( &F$2, 0, 4 );
		F$2 = __builtin_nanf( "" );
		uint32 vr$138 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$138 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 192, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		float F$2;
		F$2 = __builtin_nanf( "" );
		uint32 vr$143 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$143 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 192, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		static float F$2;
		F$2 = __builtin_nanf( "" );
		uint32 vr$147 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$147 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 192, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		static float F$2 = __builtin_nanf( "" );
		uint32 vr$151 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$151 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 192, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		float F$2;
		__builtin_memset( &F$2, 0, 4 );
		F$2 = __builtin_nanf( "" );
		uint32 vr$157 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$157 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 193, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		float F$2;
		F$2 = __builtin_nanf( "" );
		uint32 vr$162 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$162 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 193, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		static float F$2;
		F$2 = __builtin_nanf( "" );
		uint32 vr$166 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$166 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 193, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		static float F$2 = __builtin_nanf( "" );
		uint32 vr$170 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$170 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 193, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		float F$2;
		__builtin_memset( &F$2, 0, 4 );
		F$2 = __builtin_nanf( "" );
		uint32 vr$176 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$176 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 194, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		float F$2;
		F$2 = __builtin_nanf( "" );
		uint32 vr$181 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$181 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 194, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		static float F$2;
		F$2 = __builtin_nanf( "" );
		uint32 vr$185 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$185 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 194, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		static float F$2 = __builtin_nanf( "" );
		uint32 vr$189 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$189 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 194, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		float F$2;
		__builtin_memset( &F$2, 0, 4 );
		F$2 = __builtin_nanf( "" );
		uint32 vr$195 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$195 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 195, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		float F$2;
		F$2 = __builtin_nanf( "" );
		uint32 vr$200 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$200 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 195, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		static float F$2;
		F$2 = __builtin_nanf( "" );
		uint32 vr$204 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$204 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 195, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		static float F$2 = __builtin_nanf( "" );
		uint32 vr$208 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$208 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 195, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( f ) and SGNMASK) = POSNANF)" );
	}
	{
		float F$2;
		__builtin_memset( &F$2, 0, 4 );
		F$2 = (-__builtin_inff());
		uint32 vr$214 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$214 == 4286578688u) != 0), (char*)"numbers/infnan.bas", 196, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = NEGINFF)" );
	}
	{
		float F$2;
		F$2 = (-__builtin_inff());
		uint32 vr$218 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$218 == 4286578688u) != 0), (char*)"numbers/infnan.bas", 196, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = NEGINFF)" );
	}
	{
		static float F$2;
		F$2 = (-__builtin_inff());
		uint32 vr$221 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$221 == 4286578688u) != 0), (char*)"numbers/infnan.bas", 196, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = NEGINFF)" );
	}
	{
		static float F$2 = (-__builtin_inff());
		uint32 vr$224 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$224 == 4286578688u) != 0), (char*)"numbers/infnan.bas", 196, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = NEGINFF)" );
	}
	{
		float F$2;
		__builtin_memset( &F$2, 0, 4 );
		F$2 = __builtin_inff();
		uint32 vr$229 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$229 == 2139095040u) != 0), (char*)"numbers/infnan.bas", 197, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = POSINFF)" );
	}
	{
		float F$2;
		F$2 = __builtin_inff();
		uint32 vr$233 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$233 == 2139095040u) != 0), (char*)"numbers/infnan.bas", 197, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = POSINFF)" );
	}
	{
		static float F$2;
		F$2 = __builtin_inff();
		uint32 vr$236 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$236 == 2139095040u) != 0), (char*)"numbers/infnan.bas", 197, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = POSINFF)" );
	}
	{
		static float F$2 = __builtin_inff();
		uint32 vr$239 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$239 == 2139095040u) != 0), (char*)"numbers/infnan.bas", 197, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = POSINFF)" );
	}
	{
		float F$2;
		__builtin_memset( &F$2, 0, 4 );
		F$2 = __builtin_inff();
		uint32 vr$244 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$244 == 2139095040u) != 0), (char*)"numbers/infnan.bas", 198, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = POSINFF)" );
	}
	{
		float F$2;
		F$2 = __builtin_inff();
		uint32 vr$248 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$248 == 2139095040u) != 0), (char*)"numbers/infnan.bas", 198, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = POSINFF)" );
	}
	{
		static float F$2;
		F$2 = __builtin_inff();
		uint32 vr$251 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$251 == 2139095040u) != 0), (char*)"numbers/infnan.bas", 198, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = POSINFF)" );
	}
	{
		static float F$2 = __builtin_inff();
		uint32 vr$254 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$254 == 2139095040u) != 0), (char*)"numbers/infnan.bas", 198, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = POSINFF)" );
	}
	{
		float F$2;
		__builtin_memset( &F$2, 0, 4 );
		F$2 = (-__builtin_inff());
		uint32 vr$259 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$259 == 4286578688u) != 0), (char*)"numbers/infnan.bas", 199, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = NEGINFF)" );
	}
	{
		float F$2;
		F$2 = (-__builtin_inff());
		uint32 vr$263 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$263 == 4286578688u) != 0), (char*)"numbers/infnan.bas", 199, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = NEGINFF)" );
	}
	{
		static float F$2;
		F$2 = (-__builtin_inff());
		uint32 vr$266 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$266 == 4286578688u) != 0), (char*)"numbers/infnan.bas", 199, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = NEGINFF)" );
	}
	{
		static float F$2 = (-__builtin_inff());
		uint32 vr$269 = HREADSINGLEBITS( &F$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$269 == 4286578688u) != 0), (char*)"numbers/infnan.bas", 199, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( f ) = NEGINFF)" );
	}
	{
		float A$2;
		__builtin_memset( &A$2, 0, 4 );
		float B$2;
		__builtin_memset( &B$2, 0, 4 );
		float C$2;
		__builtin_memset( &C$2, 0, 4 );
		A$2 = 0x0p+0f;
		B$2 = 0x0p+0f;
		C$2 = (float)((double)A$2 / (double)B$2);
		uint32 vr$277 = HREADSINGLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$277 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 211, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( c ) and SGNMASK) = POSNANF)" );
	}
	{
		float A$2;
		__builtin_memset( &A$2, 0, 4 );
		float B$2;
		__builtin_memset( &B$2, 0, 4 );
		float C$2;
		__builtin_memset( &C$2, 0, 4 );
		A$2 = 0x0p+0f;
		B$2 = -0x0p+0f;
		C$2 = (float)((double)A$2 / (double)B$2);
		uint32 vr$286 = HREADSINGLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$286 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 212, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( c ) and SGNMASK) = POSNANF)" );
	}
	{
		float A$2;
		__builtin_memset( &A$2, 0, 4 );
		float B$2;
		__builtin_memset( &B$2, 0, 4 );
		float C$2;
		__builtin_memset( &C$2, 0, 4 );
		A$2 = -0x0p+0f;
		B$2 = 0x0p+0f;
		C$2 = (float)((double)A$2 / (double)B$2);
		uint32 vr$295 = HREADSINGLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$295 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 213, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( c ) and SGNMASK) = POSNANF)" );
	}
	{
		float A$2;
		__builtin_memset( &A$2, 0, 4 );
		float B$2;
		__builtin_memset( &B$2, 0, 4 );
		float C$2;
		__builtin_memset( &C$2, 0, 4 );
		A$2 = -0x0p+0f;
		B$2 = -0x0p+0f;
		C$2 = (float)((double)A$2 / (double)B$2);
		uint32 vr$304 = HREADSINGLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$304 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 214, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( c ) and SGNMASK) = POSNANF)" );
	}
	{
		float A$2;
		__builtin_memset( &A$2, 0, 4 );
		float B$2;
		__builtin_memset( &B$2, 0, 4 );
		float C$2;
		__builtin_memset( &C$2, 0, 4 );
		A$2 = 0x1.p+0f;
		B$2 = 0x0p+0f;
		C$2 = (float)((double)A$2 / (double)B$2);
		uint32 vr$313 = HREADSINGLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$313 == 2139095040u) != 0), (char*)"numbers/infnan.bas", 215, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( c ) = POSINFF)" );
	}
	{
		float A$2;
		__builtin_memset( &A$2, 0, 4 );
		float B$2;
		__builtin_memset( &B$2, 0, 4 );
		float C$2;
		__builtin_memset( &C$2, 0, 4 );
		A$2 = 0x1.p+0f;
		B$2 = -0x0p+0f;
		C$2 = (float)((double)A$2 / (double)B$2);
		uint32 vr$321 = HREADSINGLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$321 == 4286578688u) != 0), (char*)"numbers/infnan.bas", 216, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( c ) = NEGINFF)" );
	}
	{
		float A$2;
		__builtin_memset( &A$2, 0, 4 );
		float B$2;
		__builtin_memset( &B$2, 0, 4 );
		float C$2;
		__builtin_memset( &C$2, 0, 4 );
		A$2 = -0x1.p+0f;
		B$2 = 0x0p+0f;
		C$2 = (float)((double)A$2 / (double)B$2);
		uint32 vr$329 = HREADSINGLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$329 == 4286578688u) != 0), (char*)"numbers/infnan.bas", 217, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( c ) = NEGINFF)" );
	}
	{
		float A$2;
		__builtin_memset( &A$2, 0, 4 );
		float B$2;
		__builtin_memset( &B$2, 0, 4 );
		float C$2;
		__builtin_memset( &C$2, 0, 4 );
		A$2 = -0x1.p+0f;
		B$2 = -0x0p+0f;
		C$2 = (float)((double)A$2 / (double)B$2);
		uint32 vr$337 = HREADSINGLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$337 == 2139095040u) != 0), (char*)"numbers/infnan.bas", 218, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( c ) = POSINFF)" );
	}
	{
		float A$2;
		__builtin_memset( &A$2, 0, 4 );
		float B$2;
		__builtin_memset( &B$2, 0, 4 );
		float C$2;
		__builtin_memset( &C$2, 0, 4 );
		A$2 = 0x0p+0f;
		B$2 = 0x0p+0f;
		C$2 = -(float)((double)A$2 / (double)B$2);
		uint32 vr$346 = HREADSINGLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$346 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 230, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( c ) and SGNMASK) = POSNANF)" );
	}
	{
		float A$2;
		__builtin_memset( &A$2, 0, 4 );
		float B$2;
		__builtin_memset( &B$2, 0, 4 );
		float C$2;
		__builtin_memset( &C$2, 0, 4 );
		A$2 = 0x0p+0f;
		B$2 = -0x0p+0f;
		C$2 = -(float)((double)A$2 / (double)B$2);
		uint32 vr$356 = HREADSINGLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$356 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 231, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( c ) and SGNMASK) = POSNANF)" );
	}
	{
		float A$2;
		__builtin_memset( &A$2, 0, 4 );
		float B$2;
		__builtin_memset( &B$2, 0, 4 );
		float C$2;
		__builtin_memset( &C$2, 0, 4 );
		A$2 = -0x0p+0f;
		B$2 = 0x0p+0f;
		C$2 = -(float)((double)A$2 / (double)B$2);
		uint32 vr$366 = HREADSINGLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$366 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 232, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( c ) and SGNMASK) = POSNANF)" );
	}
	{
		float A$2;
		__builtin_memset( &A$2, 0, 4 );
		float B$2;
		__builtin_memset( &B$2, 0, 4 );
		float C$2;
		__builtin_memset( &C$2, 0, 4 );
		A$2 = -0x0p+0f;
		B$2 = -0x0p+0f;
		C$2 = -(float)((double)A$2 / (double)B$2);
		uint32 vr$376 = HREADSINGLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-((vr$376 & 2147483647u) == 2143289344u) != 0), (char*)"numbers/infnan.bas", 233, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT((hReadSingleBits( c ) and SGNMASK) = POSNANF)" );
	}
	{
		float A$2;
		__builtin_memset( &A$2, 0, 4 );
		float B$2;
		__builtin_memset( &B$2, 0, 4 );
		float C$2;
		__builtin_memset( &C$2, 0, 4 );
		A$2 = 0x1.p+0f;
		B$2 = 0x0p+0f;
		C$2 = -(float)((double)A$2 / (double)B$2);
		uint32 vr$386 = HREADSINGLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$386 == 4286578688u) != 0), (char*)"numbers/infnan.bas", 234, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( c ) = NEGINFF)" );
	}
	{
		float A$2;
		__builtin_memset( &A$2, 0, 4 );
		float B$2;
		__builtin_memset( &B$2, 0, 4 );
		float C$2;
		__builtin_memset( &C$2, 0, 4 );
		A$2 = 0x1.p+0f;
		B$2 = -0x0p+0f;
		C$2 = -(float)((double)A$2 / (double)B$2);
		uint32 vr$395 = HREADSINGLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$395 == 2139095040u) != 0), (char*)"numbers/infnan.bas", 235, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( c ) = POSINFF)" );
	}
	{
		float A$2;
		__builtin_memset( &A$2, 0, 4 );
		float B$2;
		__builtin_memset( &B$2, 0, 4 );
		float C$2;
		__builtin_memset( &C$2, 0, 4 );
		A$2 = -0x1.p+0f;
		B$2 = 0x0p+0f;
		C$2 = -(float)((double)A$2 / (double)B$2);
		uint32 vr$404 = HREADSINGLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$404 == 2139095040u) != 0), (char*)"numbers/infnan.bas", 236, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( c ) = POSINFF)" );
	}
	{
		float A$2;
		__builtin_memset( &A$2, 0, 4 );
		float B$2;
		__builtin_memset( &B$2, 0, 4 );
		float C$2;
		__builtin_memset( &C$2, 0, 4 );
		A$2 = -0x1.p+0f;
		B$2 = -0x0p+0f;
		C$2 = -(float)((double)A$2 / (double)B$2);
		uint32 vr$413 = HREADSINGLEBITS( &C$2 );
		_ZN4FBCU10CU_ASSERT_EbPciS0_S0_( (boolean)(-(vr$413 == 4286578688u) != 0), (char*)"numbers/infnan.bas", 237, (char*)"TESTS.FBC_TESTS.NUMBERS.INFNAN.SINGLE_", (char*)"CU_ASSERT(hReadSingleBits( c ) = NEGINFF)" );
	}
	label$19:;
}

static uint32 HREADSINGLEBITS( float* F$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	uint32 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 4 );
	label$2:;
	uint8* P$1;
	P$1 = (uint8*)F$1;
	uint32 BITS$1;
	BITS$1 = 0u;
	{
		int32 I$2;
		I$2 = 0;
		label$7:;
		{
			BITS$1 = (BITS$1 << (8 & 31)) | (uint32)(int32)*(uint8*)(P$1 + I$2);
		}
		label$5:;
		I$2 = I$2 + 1;
		label$4:;
		if( I$2 <= 3) goto label$7;
		label$6:;
	}
	fb$result$1 = BITS$1;
	label$3:;
	return fb$result$1;
}

static uint64 HREADDOUBLEBITS( double* D$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	uint64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8 );
	label$8:;
	uint8* P$1;
	P$1 = (uint8*)D$1;
	uint64 BITS$1;
	BITS$1 = 0ull;
	{
		int32 I$2;
		I$2 = 0;
		label$13:;
		{
			BITS$1 = (BITS$1 << (8 & 63)) | (uint64)(int32)*(uint8*)(P$1 + I$2);
		}
		label$11:;
		I$2 = I$2 + 1;
		label$10:;
		if( I$2 <= 7) goto label$13;
		label$12:;
	}
	fb$result$1 = BITS$1;
	label$9:;
	return fb$result$1;
}

__attribute__(( constructor )) static void _ZN5TESTS9FBC_TESTS7NUMBERS6INFNAN12DOUBLE__CTOREv( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$16:;
	_ZN4FBCU8ADD_TESTEPcS0_PFvvEb( (char*)"fbc_tests.numbers.infnan", (char*)"double_", (tmp$5)&_ZN5TESTS9FBC_TESTS7NUMBERS6INFNAN7DOUBLE_Ev, (boolean)0 );
	label$17:;
}

__attribute__(( constructor )) static void _ZN5TESTS9FBC_TESTS7NUMBERS6INFNAN12SINGLE__CTOREv( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$20:;
	_ZN4FBCU8ADD_TESTEPcS0_PFvvEb( (char*)"fbc_tests.numbers.infnan", (char*)"single_", (tmp$5)&_ZN5TESTS9FBC_TESTS7NUMBERS6INFNAN7SINGLE_Ev, (boolean)0 );
	label$21:;
}

__attribute__(( constructor )) static void _ZN5TESTS9FBC_TESTS7NUMBERS6INFNAN13SUITE_CTOR240Ev( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$22:;
	_ZN4FBCU9ADD_SUITEEPcPFivES2_( (char*)"fbc_tests.numbers.infnan", (tmp$4)0u, (tmp$4)0u );
	label$23:;
}
