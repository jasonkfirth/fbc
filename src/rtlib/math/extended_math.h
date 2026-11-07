/*
 * Project: FreeBASIC runtime library
 * File: math/extended_math.h
 * Purpose: Declare the C ABI for byte-backed extended numeric operations.
 * Responsibilities: Preserve native integer/va_list types and opaque storage.
 * This file intentionally does NOT contain: arithmetic algorithms or record layouts.
 */
#ifndef FB_RT_EXTENDED_MATH_H
#define FB_RT_EXTENDED_MATH_H

#include <stdint.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Storage belongs to the caller. Long-double operations receive the native
   storage size explicitly; integer operations use the compiler's 16-byte value.
   These exports retain the native C convention rather than FBCALL. */
void *fb_Int128Copy(void *destination, const void *source);
void *fb_Int128FromU64(void *destination, uint64_t value, int sign_extend);
void *fb_Int128FromDouble(void *destination, double source, int is_signed);
uint64_t fb_Int128ToU64(const void *source);
int fb_Int128ToBool(const void *source);
double fb_Int128ToDouble(const void *source, int is_signed);
void *fb_Int128FromDoubleNative(void *destination, double source, int is_signed);
double fb_Int128ToDoubleNative(const void *source, int is_signed);
void *fb_Int128Bop(void *destination, const void *left_address, const void *right_address, int operation, int is_signed, uint64_t shift_count);
void *fb_Int128Uop(void *destination, const void *source, int operation, int is_signed);
void fb_LongDoubleBop(void *destination, const void *lhs, const void *rhs, int operation, unsigned int storage_size);
void fb_LongDoubleUop(void *destination, const void *source, int operation, unsigned int storage_size);
int fb_LongDoubleCompare(const void *lhs, const void *rhs, int operation, unsigned int storage_size);
void fb_LongDoubleFromSigned(void *destination, int64_t value, unsigned int storage_size);
void fb_LongDoubleFromUnsigned(void *destination, uint64_t value, unsigned int storage_size);
void fb_LongDoubleFromDouble(void *destination, double value, unsigned int storage_size);
void fb_LongDoubleFromString(void *destination, const char *text, unsigned int storage_size);
int64_t fb_LongDoubleToSigned(const void *source, unsigned int storage_size);
uint64_t fb_LongDoubleToUnsigned(const void *source, unsigned int storage_size);
double fb_LongDoubleToDouble(const void *source, unsigned int storage_size);
void fb_LongDoubleVaArgCopy(void *destination, unsigned int storage_size, va_list arguments);
long double fb_LongDoubleToNative(const void *source, unsigned int storage_size);

#ifdef __cplusplus
}
#endif

#endif

/* end of math/extended_math.h */
