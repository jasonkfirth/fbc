/*
    Project: FreeBASIC compiler regression tests
    File: windows-vararg-aggregates-helper.c
    Purpose: Read the record values passed through C variadic arguments.
    Responsibilities: Validate each value using the native C ABI.
    This fixture does not implement the FreeBASIC caller.
*/

#include <stdarg.h>
#include <stdint.h>

typedef struct { uint8_t value; } Value1;
typedef struct { uint16_t value; } Value2;
typedef struct { uint32_t value; } Value4;
typedef struct { uint64_t value; } Value8;
typedef struct { uint64_t first_value, second_value; } Value16;

_Static_assert(sizeof(Value1) == 1, "Value1 size");
_Static_assert(sizeof(Value2) == 2, "Value2 size");
_Static_assert(sizeof(Value4) == 4, "Value4 size");
_Static_assert(sizeof(Value8) == 8, "Value8 size");
_Static_assert(sizeof(Value16) == 16, "Value16 size");

int check_aggregate_arguments(int marker, ...);
int check_aggregate_arguments(int marker, ...)
{
    va_list arguments;
    va_start(arguments, marker);
    Value1 a = va_arg(arguments, Value1);
    Value2 b = va_arg(arguments, Value2);
    Value4 c = va_arg(arguments, Value4);
    Value8 d = va_arg(arguments, Value8);
    Value16 e = va_arg(arguments, Value16);
    va_end(arguments);
    return marker == 17 && a.value == 0x11 && b.value == 0x2233 &&
        c.value == UINT32_C(0x44556677) && d.value == UINT64_C(0x1122334455667788) &&
        e.first_value == UINT64_C(0x1020304050607080) &&
        e.second_value == UINT64_C(0x123456789abcdef0) ? -1 : 0;
}

/* end of windows-vararg-aggregates-helper.c */
