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
struct $N7LIBRARY4ITEME {
	int32 VALUE;
};
#define __FB_STATIC_ASSERT( expr ) extern int __$fb_structsizecheck[(expr) ? 1 : -1]
__FB_STATIC_ASSERT( sizeof( struct $N7LIBRARY4ITEME ) == 4 );
void fb_PrintInt( const int32, const int32, const int32 );
void fb_PrintLongint( const int32, const int64, const int32 );
void fb_PrintDouble( const int32, const double, const int32 );
static void fb_ctor__bindings( void ) __attribute__(( constructor ));
int32 _ZN7LIBRARY8READITEMERNS_4ITEME( struct $N7LIBRARY4ITEME* );
void PROBEBINDINGS( void );
static int32 _ZN7LIBRARY8COUNTER$E;

int32 _ZN7LIBRARY8READITEMERNS_4ITEME( struct $N7LIBRARY4ITEME* ITEM$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	int32 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 4ll );
	label$2:;
	fb$result$1 = (int32)((int64)*(int32*)ITEM$1 + 42ll);
	goto label$3;
	label$3:;
	return fb$result$1;
}

void PROBEBINDINGS( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$4:;
	int32 OUTER$1;
	OUTER$1 = 42;
	struct $N7LIBRARY4ITEME ITEM$1;
	__builtin_memset( &ITEM$1, 0, 4ll );
	*(int32*)&ITEM$1 = OUTER$1;
	{
		*(int32*)&ITEM$1 = (int32)((int64)*(int32*)&ITEM$1 + 1ll);
	}
	{
		double OUTER$2;
		OUTER$2 = 0x1.4p+1;
		fb_PrintDouble( 0, OUTER$2, 1 );
	}
	fb_PrintInt( 0, OUTER$1, 2 );
	int32 vr$5 = _ZN7LIBRARY8READITEMERNS_4ITEME( &ITEM$1 );
	fb_PrintInt( 0, vr$5, 2 );
	fb_PrintLongint( 0, 7ll, 1 );
	{
		int32 ITERATOR$2;
		ITERATOR$2 = 0;
		label$9:;
		{
			_ZN7LIBRARY8COUNTER$E = (int32)((int64)_ZN7LIBRARY8COUNTER$E + (int64)ITERATOR$2);
		}
		label$7:;
		ITERATOR$2 = (int32)((int64)ITERATOR$2 + 1ll);
		label$6:;
		if( (int64)ITERATOR$2 <= 2ll) goto label$9;
		label$8:;
	}
	goto label$10;
	label$10:;
	__asm__ __volatile__( "mov eax, %0" : "+m" (OUTER$1) :  : "cc", "memory", "rax", "rbx", "rcx", "rdx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15" );
	__asm__ __volatile__( "jmp asm_local" :  :  : "cc", "memory", "rax", "rbx", "rcx", "rdx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15" );
	__asm__ __volatile__( "asm_local:" :  :  : "cc", "memory", "rax", "rbx", "rcx", "rdx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15" );
	label$5:;
}

__attribute__(( constructor )) static void fb_ctor__bindings( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$0:;
	PROBEBINDINGS(  );
	label$1:;
}
