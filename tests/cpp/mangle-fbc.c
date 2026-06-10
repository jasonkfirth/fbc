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
static void fb_ctor__manglezfbc( void ) __attribute__(( constructor ));
void VARIADIC_LIST_BYVAL( int32, ... );
void VARIADIC_LIST_BYREF( int32, ... );
void VARIADIC_LIST_BYREF_PTR( int32, ... );
void VARIADIC_LIST_PTR( int32, ... );
void VARIADIC_LIST_PTR_PTR( int32, ... );

void VARIADIC_LIST_BYVAL( int32 N$1, ... )
{
	label$2:;
	__builtin_va_list X$1;
	__builtin_va_start( *(__builtin_va_list*)&X$1, N$1);
	__builtin_va_end( *(__builtin_va_list*)&X$1);
	label$3:;
}

void VARIADIC_LIST_BYREF( int32 N$1, ... )
{
	label$4:;
	__builtin_va_list X$1;
	__builtin_va_start( *(__builtin_va_list*)&X$1, N$1);
	__builtin_va_end( *(__builtin_va_list*)&X$1);
	label$5:;
}

void VARIADIC_LIST_BYREF_PTR( int32 N$1, ... )
{
	label$6:;
	__builtin_va_list X$1;
	__builtin_va_list* Y$1;
	Y$1 = &X$1;
	__builtin_va_start( *(__builtin_va_list*)Y$1, N$1);
	__builtin_va_end( *(__builtin_va_list*)Y$1);
	label$7:;
}

void VARIADIC_LIST_PTR( int32 N$1, ... )
{
	label$8:;
	__builtin_va_list X$1;
	__builtin_va_start( *(__builtin_va_list*)&X$1, N$1);
	__builtin_va_end( *(__builtin_va_list*)&X$1);
	label$9:;
}

void VARIADIC_LIST_PTR_PTR( int32 N$1, ... )
{
	label$10:;
	__builtin_va_list X$1;
	__builtin_va_list* Y$1;
	Y$1 = &X$1;
	__builtin_va_start( *(__builtin_va_list*)Y$1, N$1);
	__builtin_va_end( *(__builtin_va_list*)Y$1);
	label$11:;
}

__attribute__(( constructor )) static void fb_ctor__manglezfbc( void )
{
	label$0:;
	{
		boolean B$1;
		__builtin_memset( &B$1, 0, 1ll );
		B$1 = (boolean)0ll;
		B$1 = (boolean)1ll;
	}
	{
		float S$1;
		__builtin_memset( &S$1, 0, 4ll );
		S$1 = 0x0p+0f;
		S$1 = 0x1.4p+3f;
	}
	{
		double D$1;
		__builtin_memset( &D$1, 0, 8ll );
		D$1 = 0x0p+0;
		D$1 = 0x1.4p+3;
	}
	{
		uint8 UB$1;
		UB$1 = (uint8)0u;
		int8 SB$1;
		SB$1 = (int8)0;
		UB$1 = (uint8)127u;
		SB$1 = (int8)127;
	}
	{
		uint16 US$1;
		US$1 = (uint16)0u;
		int16 SS$1;
		SS$1 = (int16)0;
		US$1 = (uint16)32767u;
		SS$1 = (int16)32767;
	}
	{
		uint32 UI$1;
		UI$1 = 0u;
		int32 SI$1;
		SI$1 = 0;
		UI$1 = 2147483647u;
		SI$1 = 2147483647;
	}
	{
		uint64 UL$1;
		UL$1 = 0ull;
		int64 SL$1;
		SL$1 = 0ll;
		UL$1 = 2147483647ull;
		SL$1 = 2147483647ll;
	}
	{
		uint64 ULL$1;
		ULL$1 = 0ull;
		int64 SLL$1;
		SLL$1 = 0ll;
		ULL$1 = 9223372036854775807ull;
		SLL$1 = 9223372036854775807ll;
	}
	{
		double D$1;
		D$1 = 0x1.p+0;
		double* DP$1;
		DP$1 = &D$1;
		double** DPP$1;
		DPP$1 = &DP$1;
	}
	{
		VARIADIC_LIST_BYVAL( 3, 1ll, 2ll, 3ll );
		VARIADIC_LIST_BYREF( 3, 1ll, 2ll, 3ll );
		VARIADIC_LIST_BYREF_PTR( 3, 1ll, 2ll, 3ll );
		VARIADIC_LIST_PTR( 3, 1ll, 2ll, 3ll );
		VARIADIC_LIST_PTR_PTR( 3, 1ll, 2ll, 3ll );
	}
	{
		int32 I$1;
		I$1 = 123;
	}
	label$1:;
}
