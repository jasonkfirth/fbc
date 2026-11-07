/*
 * Project: FreeBASIC runtime tests
 * File: prototype-contracts.c
 * Purpose: Check public C/C++ declarations and their archive-linked exports.
 * Responsibilities: Retain typed symbol references and verify numeric calls.
 * This file intentionally does NOT exercise optional devices or ERROR paths.
 */
#ifdef __cplusplus
extern "C" {
#endif
#include "fb.h"
#include "fb_profile.h"
#include "math/extended_math.h"
#ifdef __cplusplus
}
#endif
#include <math.h>

/* GNU typeof keeps each reference tied to the public header's exact function
   type, including its calling convention. Volatile storage retains the link
   references even when the client is optimized. */
#define API_EXPORTS(X) \
    X(fb_FileAttr) \
    X(fb_DIRCTX_Destructor) \
    X(fb_FileGetWstrEx) \
    X(fb_FileGetWstr) \
    X(fb_FileGetWstrLarge) \
    X(fb_FileGetWstrIOB) \
    X(fb_FileGetWstrLargeIOB) \
    X(fb_FileLenEx) \
    X(fb_FileLockEx) \
    X(fb_FileUnlockEx) \
    X(fb_FilePutWstrEx) \
    X(fb_FilePutWstr) \
    X(fb_FilePutWstrLarge) \
    X(fb_GpioPinOpen) \
    X(fb_GpioPinOpenDevice) \
    X(fb_GpioPinClose) \
    X(fb_GpioPinRead) \
    X(fb_GpioPinWrite) \
    X(fb_GpioPinGetType) \
    X(fb_GpioPinSetType) \
    X(fb_PrintBufferWstr) \
    X(fb_Int128Copy) \
    X(fb_Int128FromU64) \
    X(fb_Int128FromDouble) \
    X(fb_Int128ToU64) \
    X(fb_Int128ToBool) \
    X(fb_Int128ToDouble) \
    X(fb_Int128FromDoubleNative) \
    X(fb_Int128ToDoubleNative) \
    X(fb_Int128Bop) \
    X(fb_Int128Uop) \
    X(fb_ConsoleGetRawXY) \
    X(fb_LPrintFixString) \
    X(fb_PrintUsingBoolean) \
    X(fb_LongDoubleBop) \
    X(fb_LongDoubleUop) \
    X(fb_LongDoubleCompare) \
    X(fb_LongDoubleFromSigned) \
    X(fb_LongDoubleFromUnsigned) \
    X(fb_LongDoubleFromDouble) \
    X(fb_LongDoubleFromString) \
    X(fb_LongDoubleToSigned) \
    X(fb_LongDoubleToUnsigned) \
    X(fb_LongDoubleToDouble) \
    X(fb_LongDoubleVaArgCopy) \
    X(fb_LongDoubleToNative) \
    X(fb_FRACf) \
    X(fb_FRACd) \
    X(fb_SGNb) \
    X(fb_SGNi) \
    X(fb_SGNl) \
    X(fb_IsTypeOf) \
    X(fb_ProfileGetCallsMetrics) \
    X(fb_ProfileGetCallsProfiler) \
    X(fb_ProfileGetCyclesProfiler) \
    X(fb_ChDir) \
    X(fb_MkDir) \
    X(fb_RmDir) \
    X(fb_hGetBeginOfWeek) \
    X(fb_hGetFirstWeekOfYear) \
    X(fb_TextReplace_ssu) \
    X(fb_TextReplace_ssw) \
    X(fb_TextReplace_sus) \
    X(fb_TextReplace_suu) \
    X(fb_TextReplace_suw) \
    X(fb_TextReplace_sws) \
    X(fb_TextReplace_swu) \
    X(fb_TextReplace_sww) \
    X(fb_TextReplace_uss) \
    X(fb_TextReplace_usu) \
    X(fb_TextReplace_usw) \
    X(fb_TextReplace_uus) \
    X(fb_TextReplace_uuu) \
    X(fb_TextReplace_uuw) \
    X(fb_TextReplace_uws) \
    X(fb_TextReplace_uwu) \
    X(fb_TextReplace_uww) \
    X(fb_TextReplace_wss) \
    X(fb_TextReplace_wsu) \
    X(fb_TextReplace_wsw) \
    X(fb_TextReplace_wus) \
    X(fb_TextReplace_wuu) \
    X(fb_TextReplace_wuw) \
    X(fb_TextReplace_wws) \
    X(fb_TextReplace_wwu) \
    X(fb_TextReplace_www) \
    X(fb_TextComp_su) \
    X(fb_TextComp_sw) \
    X(fb_TextComp_us) \
    X(fb_TextComp_uu) \
    X(fb_TextComp_uw) \
    X(fb_TextComp_ws) \
    X(fb_TextComp_wu) \
    X(fb_TextComp_ww) \
    X(fb_TextReverse_w) \
    X(fb_TextFormat_w) \
    X(fb_WideFileExists) \
    X(fb_WideFileLen) \
    X(fb_WideFileDateTime) \
    X(fb_WideFileGetAttr) \
    X(fb_WideFileSetAttr) \
    X(fb_WideFileCopy) \
    X(fb_WideFileCopyFromBytes) \
    X(fb_WideFileCopyToBytes) \
    X(fb_hUTF8ToChar) \
    X(fb_hUTF16ToChar) \
    X(fb_hUTF32ToChar) \
    X(fb_UTFToChar)

#define DECLARE_REFERENCE(name) static __typeof__(&name) volatile reference_##name = &name;
API_EXPORTS(DECLARE_REFERENCE)
#undef DECLARE_REFERENCE

#define CHECK(condition) do { if( !(condition) ) { \
    printf( "Prototype contract failure at line %d\n", __LINE__ ); return 1; \
} } while( 0 )

int main( int argc, char **argv )
{
    uint64_t integer[2] = {0, 0};
    const uint64_t value = UINT64_C(0xfedcba9876543210);
    long double native_value;
    double result;

    fb_Init( argc, argv, 0 );
#define CHECK_REFERENCE(name) CHECK( reference_##name != NULL );
    API_EXPORTS(CHECK_REFERENCE)
#undef CHECK_REFERENCE

    CHECK( fb_Int128FromU64( integer, value, 0 ) == integer );
    CHECK( fb_Int128ToU64( integer ) == value );
    CHECK( fb_Int128ToBool( integer ) != 0 );
    fb_LongDoubleFromDouble( &native_value, 1.25, sizeof( native_value ) );
    result = fb_LongDoubleToDouble( &native_value, sizeof( native_value ) );
    CHECK( fabs( result - 1.25 ) < 1e-15 );
    CHECK( fb_SGNi( -7 ) == -1 );
    CHECK( fabs( fb_FRACd( -1.25 ) + 0.25 ) < 1e-15 );
    puts( "PROTOTYPE_CONTRACTS_PASS exports=108" );
    return 0;
}

/* end of prototype-contracts.c */
