/*
    FreeBASIC Windows import regression
    File: import-data.c
    Purpose: Export real C data for FreeBASIC's IMPORT attribute checks.
    Responsibilities: Scalars, arrays, pointers and independent write checks.
    This file contains no FreeBASIC implementation or import-name aliases.
*/
#include <stdint.h>

struct guarded_value {
    int32_t before;
    int32_t value;
    int32_t after;
};

__declspec(dllexport) int32_t import_scalar = -2147483000;
__declspec(dllexport) int32_t import_values[3] = { 17, -9, 0x1234567 };
__declspec(dllexport) int32_t *import_pointer = &import_values[1];
__declspec(dllexport) uint64_t import_wide = UINT64_C(0xFEDCBA9876543210);
__declspec(dllexport) struct guarded_value import_record = {
    0x13579BDF, -7, 0x2468ACE
};

__declspec(dllexport) int32_t *import_address(void)
{
    return &import_scalar;
}

__declspec(dllexport) int32_t import_verify(void)
{
    return import_scalar == 12345 && import_values[0] == -123 &&
           import_values[1] == 42 && import_values[2] == 0x1234567 &&
           import_pointer == &import_values[2] &&
           import_wide == UINT64_C(0x8000000000000001) &&
           import_record.before == 0x13579BDF && import_record.value == -99 &&
           import_record.after == 0x2468ACE;
}

__declspec(dllexport) void import_change(void)
{
    import_scalar = -333;
    import_values[0] = 888;
    import_record.value = 555;
}
/* end of import-data.c */
