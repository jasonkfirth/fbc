/*
    Project: FreeBASIC Runtime Library
    File: fb_ustring.h
    Purpose: Define UTF-8 string services using the FBSTRING descriptor ABI.
    Responsibilities: Scalar decoding, checked storage, and text operations.
    This file intentionally does NOT contain: compiler type selection.

    Descriptor lengths and capacities are bytes, including embedded NULs.
    Public text positions are one-based Unicode scalar positions. Indexing
    uses zero-based positions. No normalization of combining characters is
    performed. The _NoLock helpers require the runtime string lock; callers
    must also synchronize concurrent writes to the same string variable.
*/

#ifndef FB_USTRING_H
#define FB_USTRING_H

#define FB_UTF8_REPLACEMENT 0xFFFDu
/* Leave room for the existing string allocator's rounding and growth. */
#define FB_USTRING_MAX_BYTES ((ssize_t)(FB_STRSIZEMSK / 2))

FBSTRING_ACCESS *fb_hUStrFromWstr_NoLock( const FB_WCHAR *src );
ssize_t fb_hUtf8Fold( unsigned int scalar, char *output );
unsigned int fb_hUtf8Decode( const char *data, ssize_t length, ssize_t *offset );
ssize_t fb_hUtf8Encode( char *data, unsigned int scalar );
ssize_t fb_hUtf8Count( const char *data, ssize_t length );
ssize_t fb_hUtf8Offset( const char *data, ssize_t length, ssize_t count );
FBSTRING_ACCESS *fb_hUStrAlloc_NoLock( ssize_t length );
FBSTRING_ACCESS *fb_hUStrCopy_NoLock( const char *data, ssize_t length );
FBSTRING_ACCESS *fb_hUStrNormalize_NoLock( const char *data, ssize_t length );
void fb_hUStrDeletePair_NoLock( FBSTRING_ACCESS *first, FBSTRING_ACCESS *second );
void fb_hUStrMove_NoLock( FBSTRING_ACCESS *dst, FBSTRING_ACCESS *src, int is_init );

FBCALL FBSTRING_ACCESS *fb_UStrToStr( FBSTRING_ACCESS *src );
FBCALL FBSTRING_ACCESS *fb_UStrFromBytes( void *src, ssize_t size );
FBCALL FBSTRING_ACCESS *fb_UStrFromWstr( const FB_WCHAR *src );
FBCALL FB_WCHAR *fb_UStrToWstr( FBSTRING_ACCESS *src );
FBCALL void *fb_UStrAssign( void *dst, ssize_t dst_size, void *src, ssize_t src_size, int fill_rem );
FBCALL void *fb_UStrInit( void *dst, ssize_t dst_size, void *src, ssize_t src_size, int fill_rem );
FBCALL FBSTRING_ACCESS *fb_UStrConcat( FBSTRING_ACCESS *dst, void *first, ssize_t first_size, void *second, ssize_t second_size );
FBCALL void *fb_UStrConcatAssign( void *dst, ssize_t dst_size, void *src, ssize_t src_size, int fill_rem );
FBCALL ssize_t fb_UStrLen( void *src, ssize_t size );
FBCALL unsigned int fb_UStrAsc( FBSTRING_ACCESS *src, ssize_t position );
FBCALL unsigned int fb_UStrIndex( FBSTRING_ACCESS *src, ssize_t index );
FBCALL void fb_UStrSetIndex( FBSTRING_ACCESS *dst, ssize_t index, ssize_t scalar );
FBSTRING_ACCESS *fb_UStrChr( int count, ... );
FBCALL FBSTRING_ACCESS *fb_UStrMid( FBSTRING_ACCESS *src, ssize_t start, ssize_t count );
FBCALL FBSTRING_ACCESS *fb_UStrLeft( FBSTRING_ACCESS *src, ssize_t count );
FBCALL void fb_UStrLeftSelf( FBSTRING_ACCESS *dst, ssize_t count );
FBCALL void fb_WstrLeftSelf( FB_WCHAR *dst, ssize_t count );
FBCALL FBSTRING_ACCESS *fb_UStrRight( FBSTRING_ACCESS *src, ssize_t count );
FBCALL FBSTRING_ACCESS *fb_UStrFill1( ssize_t count, ssize_t scalar );
FBCALL FBSTRING_ACCESS *fb_UStrFill2( ssize_t count, FBSTRING_ACCESS *src );
FBCALL void fb_UStrAssignMid( FBSTRING_ACCESS *dst, ssize_t start, ssize_t count, FBSTRING_ACCESS *src );
FBCALL void fb_UStrLset( FBSTRING_ACCESS *dst, FBSTRING_ACCESS *src );
FBCALL void fb_UStrRset( FBSTRING_ACCESS *dst, FBSTRING_ACCESS *src );
FBCALL ssize_t fb_UStrInstr( ssize_t start, FBSTRING_ACCESS *src, FBSTRING_ACCESS *pattern );
FBCALL ssize_t fb_UStrInstrAny( ssize_t start, FBSTRING_ACCESS *src, FBSTRING_ACCESS *pattern );
FBCALL ssize_t fb_UStrInstrRev( FBSTRING_ACCESS *src, FBSTRING_ACCESS *pattern, ssize_t start );
FBCALL ssize_t fb_UStrInstrRevAny( FBSTRING_ACCESS *src, FBSTRING_ACCESS *pattern, ssize_t start );
FBCALL FBSTRING_ACCESS *fb_UStrTrim( FBSTRING_ACCESS *src );
FBCALL FBSTRING_ACCESS *fb_UStrTrimEx( FBSTRING_ACCESS *src, FBSTRING_ACCESS *pattern );
FBCALL FBSTRING_ACCESS *fb_UStrTrimAny( FBSTRING_ACCESS *src, FBSTRING_ACCESS *pattern );
FBCALL FBSTRING_ACCESS *fb_UStrLTrim( FBSTRING_ACCESS *src );
FBCALL FBSTRING_ACCESS *fb_UStrLTrimEx( FBSTRING_ACCESS *src, FBSTRING_ACCESS *pattern );
FBCALL FBSTRING_ACCESS *fb_UStrLTrimAny( FBSTRING_ACCESS *src, FBSTRING_ACCESS *pattern );
FBCALL FBSTRING_ACCESS *fb_UStrRTrim( FBSTRING_ACCESS *src );
FBCALL FBSTRING_ACCESS *fb_UStrRTrimEx( FBSTRING_ACCESS *src, FBSTRING_ACCESS *pattern );
FBCALL FBSTRING_ACCESS *fb_UStrRTrimAny( FBSTRING_ACCESS *src, FBSTRING_ACCESS *pattern );
FBCALL FBSTRING_ACCESS *fb_UStrUcase( FBSTRING_ACCESS *src, int mode );
FBCALL FBSTRING_ACCESS *fb_UStrLcase( FBSTRING_ACCESS *src, int mode );

FBCALL int fb_UStrFileLineInput( int fnum, void *dst, ssize_t size, int fill );
FBCALL int fb_UStrInput( void *dst, ssize_t size, int fill );
FBCALL void fb_UStrDataRead( void *dst, ssize_t size, int fill );
FBCALL void fb_UStrPrint( int fnum, FBSTRING_ACCESS *src, int mask );
FBCALL void fb_UStrWrite( int fnum, FBSTRING_ACCESS *src, int mask );

FBCALL FBSTRING_ACCESS *fb_UStrReverse( FBSTRING_ACCESS *src );
FBCALL FBSTRING_ACCESS *fb_UStrReplace( FBSTRING_ACCESS *src, FBSTRING_ACCESS *find, FBSTRING_ACCESS *replacement, ssize_t start, ssize_t count, int compare );
FBCALL int fb_UStrComp( FBSTRING_ACCESS *first, FBSTRING_ACCESS *second, int compare );
FBCALL FBSTRING_ACCESS *fb_UStrFormat( double value, FBSTRING_ACCESS *mask );

FBCALL int fb_PrintUsingInitUstr( FBSTRING_ACCESS *format );
FBCALL int fb_LPrintUsingInitUstr( FBSTRING_ACCESS *format );
FBCALL int fb_PrintUsingUstr( int fnum, FBSTRING_ACCESS *source, int mask );

/* Mixed text suffixes distinguish byte strings (s), UTF-8 strings (u)
   and wide strings (w). The result representation follows the source operand.
   Path helpers accept wide input while retaining the narrow API result types. */
FBCALL FBSTRING_ACCESS *fb_TextReplace_ssu( FBSTRING_ACCESS *source, FBSTRING_ACCESS *find, FBSTRING_ACCESS *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FBSTRING_ACCESS *fb_TextReplace_ssw( FBSTRING_ACCESS *source, FBSTRING_ACCESS *find, FB_WCHAR *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FBSTRING_ACCESS *fb_TextReplace_sus( FBSTRING_ACCESS *source, FBSTRING_ACCESS *find, FBSTRING_ACCESS *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FBSTRING_ACCESS *fb_TextReplace_suu( FBSTRING_ACCESS *source, FBSTRING_ACCESS *find, FBSTRING_ACCESS *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FBSTRING_ACCESS *fb_TextReplace_suw( FBSTRING_ACCESS *source, FBSTRING_ACCESS *find, FB_WCHAR *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FBSTRING_ACCESS *fb_TextReplace_sws( FBSTRING_ACCESS *source, FB_WCHAR *find, FBSTRING_ACCESS *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FBSTRING_ACCESS *fb_TextReplace_swu( FBSTRING_ACCESS *source, FB_WCHAR *find, FBSTRING_ACCESS *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FBSTRING_ACCESS *fb_TextReplace_sww( FBSTRING_ACCESS *source, FB_WCHAR *find, FB_WCHAR *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FBSTRING_ACCESS *fb_TextReplace_uss( FBSTRING_ACCESS *source, FBSTRING_ACCESS *find, FBSTRING_ACCESS *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FBSTRING_ACCESS *fb_TextReplace_usu( FBSTRING_ACCESS *source, FBSTRING_ACCESS *find, FBSTRING_ACCESS *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FBSTRING_ACCESS *fb_TextReplace_usw( FBSTRING_ACCESS *source, FBSTRING_ACCESS *find, FB_WCHAR *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FBSTRING_ACCESS *fb_TextReplace_uus( FBSTRING_ACCESS *source, FBSTRING_ACCESS *find, FBSTRING_ACCESS *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FBSTRING_ACCESS *fb_TextReplace_uuu( FBSTRING_ACCESS *source, FBSTRING_ACCESS *find, FBSTRING_ACCESS *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FBSTRING_ACCESS *fb_TextReplace_uuw( FBSTRING_ACCESS *source, FBSTRING_ACCESS *find, FB_WCHAR *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FBSTRING_ACCESS *fb_TextReplace_uws( FBSTRING_ACCESS *source, FB_WCHAR *find, FBSTRING_ACCESS *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FBSTRING_ACCESS *fb_TextReplace_uwu( FBSTRING_ACCESS *source, FB_WCHAR *find, FBSTRING_ACCESS *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FBSTRING_ACCESS *fb_TextReplace_uww( FBSTRING_ACCESS *source, FB_WCHAR *find, FB_WCHAR *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FB_WCHAR *fb_TextReplace_wss( FB_WCHAR *source, FBSTRING_ACCESS *find, FBSTRING_ACCESS *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FB_WCHAR *fb_TextReplace_wsu( FB_WCHAR *source, FBSTRING_ACCESS *find, FBSTRING_ACCESS *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FB_WCHAR *fb_TextReplace_wsw( FB_WCHAR *source, FBSTRING_ACCESS *find, FB_WCHAR *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FB_WCHAR *fb_TextReplace_wus( FB_WCHAR *source, FBSTRING_ACCESS *find, FBSTRING_ACCESS *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FB_WCHAR *fb_TextReplace_wuu( FB_WCHAR *source, FBSTRING_ACCESS *find, FBSTRING_ACCESS *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FB_WCHAR *fb_TextReplace_wuw( FB_WCHAR *source, FBSTRING_ACCESS *find, FB_WCHAR *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FB_WCHAR *fb_TextReplace_wws( FB_WCHAR *source, FB_WCHAR *find, FBSTRING_ACCESS *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FB_WCHAR *fb_TextReplace_wwu( FB_WCHAR *source, FB_WCHAR *find, FBSTRING_ACCESS *replacement, ssize_t start, ssize_t count, int compare );
FBCALL FB_WCHAR *fb_TextReplace_www( FB_WCHAR *source, FB_WCHAR *find, FB_WCHAR *replacement, ssize_t start, ssize_t count, int compare );
FBCALL int fb_TextComp_su( FBSTRING_ACCESS *first, FBSTRING_ACCESS *second, int compare );
FBCALL int fb_TextComp_sw( FBSTRING_ACCESS *first, FB_WCHAR *second, int compare );
FBCALL int fb_TextComp_us( FBSTRING_ACCESS *first, FBSTRING_ACCESS *second, int compare );
FBCALL int fb_TextComp_uu( FBSTRING_ACCESS *first, FBSTRING_ACCESS *second, int compare );
FBCALL int fb_TextComp_uw( FBSTRING_ACCESS *first, FB_WCHAR *second, int compare );
FBCALL int fb_TextComp_ws( FB_WCHAR *first, FBSTRING_ACCESS *second, int compare );
FBCALL int fb_TextComp_wu( FB_WCHAR *first, FBSTRING_ACCESS *second, int compare );
FBCALL int fb_TextComp_ww( FB_WCHAR *first, FB_WCHAR *second, int compare );
FBCALL FB_WCHAR *fb_TextReverse_w( FB_WCHAR *source );
FBCALL FB_WCHAR *fb_TextFormat_w( double value, FB_WCHAR *mask );
FBCALL int fb_WideFileExists( const FB_WCHAR *path );
FBCALL long long fb_WideFileLen( const FB_WCHAR *path );
FBCALL double fb_WideFileDateTime( const FB_WCHAR *path );
FBCALL int fb_WideFileGetAttr( const FB_WCHAR *path );
FBCALL int fb_WideFileSetAttr( const FB_WCHAR *path, int attributes );
FBCALL int fb_WideFileCopy( const FB_WCHAR *source, const FB_WCHAR *destination );
FBCALL int fb_WideFileCopyFromBytes( const char *source, const FB_WCHAR *destination );
FBCALL int fb_WideFileCopyToBytes( const FB_WCHAR *source, const char *destination );

#endif

/* end of fb_ustring.h */
