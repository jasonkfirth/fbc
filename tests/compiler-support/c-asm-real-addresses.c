/*
    Project: FreeBASIC C backend regressions
    File: c-asm-real-addresses.c
    Purpose: Observe a BASIC parameter while its assembly block is still active.
    Responsibilities: Retain one borrowed slot and check its live value; pass
    floating representations to BASIC without evaluating them through x87.
    This file contains no entry point or floating arithmetic.
*/

static int *published_slot;
static int expected_value;
static int observed_value;

void asm_publish_home(int *slot, int expected);
void asm_observe_home(void);
int asm_home_observed(void);
void asm_clear_home(void);
unsigned asm_read_float(float value);
unsigned long long asm_read_double(double value);
unsigned asm_call_raw_float(unsigned bits);
unsigned long long asm_call_raw_double(unsigned long long bits);

void asm_publish_home(int *slot, int expected)
{
    published_slot = slot;
    expected_value = expected;
    observed_value = 0;
}

void asm_observe_home(void)
{
    /* This call occurs before assembly can return or copy a value back. */
    observed_value = published_slot != 0 && *published_slot == expected_value;
}

int asm_home_observed(void)
{
    return observed_value;
}

void asm_clear_home(void)
{
    /* The BASIC caller owns the slot and is about to return. */
    published_slot = 0;
}

#if defined(__i386__)

#if defined(_WIN32) || defined(__APPLE__) || defined(__DJGPP__)
#define ASM_FLOAT_LABEL "_asm_read_float"
#define ASM_DOUBLE_LABEL "_asm_read_double"
#else
#define ASM_FLOAT_LABEL "asm_read_float"
#define ASM_DOUBLE_LABEL "asm_read_double"
#endif

/* Establish an ordinary outgoing call frame before the explicit stack call.
   Each assembly call consumes 16 bytes, preserving the i386 SysV alignment. */
static void __attribute__((noinline)) asm_float_call_frame(void)
{
    __asm__ __volatile__("" : : : "memory");
}

unsigned asm_call_raw_float(unsigned bits)
{
    unsigned result;
    asm_float_call_frame();
    __asm__ __volatile__(
        "sub esp, 12; push %1; call " ASM_FLOAT_LABEL "; add esp, 16"
        : "=a" (result) : "r" (bits) : "ecx", "edx", "memory", "cc");
    return result;
}

unsigned long long asm_call_raw_double(unsigned long long bits)
{
    unsigned low, high;
    asm_float_call_frame();
    __asm__ __volatile__(
        "sub esp, 8; push %3; push %2; call " ASM_DOUBLE_LABEL "; add esp, 16"
        : "=a" (low), "=d" (high)
        : "r" ((unsigned)bits), "r" ((unsigned)(bits >> 32))
        : "ecx", "memory", "cc");
    return ((unsigned long long)high << 32) | low;
}

#else

/* The x86-64 ABI transports these values with SSE moves. The union provides
   their C object representations without a floating conversion or operation. */
unsigned asm_call_raw_float(unsigned bits)
{
    union { unsigned bits; float value; } argument;
    argument.bits = bits;
    return asm_read_float(argument.value);
}

unsigned long long asm_call_raw_double(unsigned long long bits)
{
    union { unsigned long long bits; double value; } argument;
    argument.bits = bits;
    return asm_read_double(argument.value);
}

#endif

/* end of c-asm-real-addresses.c */
