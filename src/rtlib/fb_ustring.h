/*
    FreeBASIC Runtime Library
    File: fb_ustring.h
    Purpose: Define UTF-8 string services using the FBSTRING descriptor ABI.
    Responsibilities: Scalar decoding, checked storage, and text operations.
    This file intentionally does NOT contain compiler type selection.

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

FBSTRING *fb_hUStrFromWstr_NoLock( const FB_WCHAR *src );
ssize_t fb_hUtf8Fold( unsigned int scalar, char *output );
unsigned int fb_hUtf8Decode( const char *data, ssize_t length, ssize_t *offset );
ssize_t fb_hUtf8Encode( char *data, unsigned int scalar );
ssize_t fb_hUtf8Count( const char *data, ssize_t length );
ssize_t fb_hUtf8Offset( const char *data, ssize_t length, ssize_t count );
FBSTRING *fb_hUStrAlloc_NoLock( ssize_t length );
FBSTRING *fb_hUStrCopy_NoLock( const char *data, ssize_t length );
FBSTRING *fb_hUStrNormalize_NoLock( const char *data, ssize_t length );
void fb_hUStrDeletePair_NoLock( FBSTRING *first, FBSTRING *second );
void fb_hUStrMove_NoLock( FBSTRING *dst, FBSTRING *src, int is_init );

FBCALL FBSTRING *fb_UStrToStr( FBSTRING *src );
FBCALL FBSTRING *fb_UStrFromBytes( void *src, ssize_t size );
FBCALL FBSTRING *fb_UStrFromWstr( const FB_WCHAR *src );
FBCALL FB_WCHAR *fb_UStrToWstr( FBSTRING *src );
FBCALL void *fb_UStrAssign( void *dst, ssize_t dst_size, void *src, ssize_t src_size, int fill_rem );
FBCALL void *fb_UStrInit( void *dst, ssize_t dst_size, void *src, ssize_t src_size, int fill_rem );
FBCALL FBSTRING *fb_UStrConcat( FBSTRING *dst, void *first, ssize_t first_size, void *second, ssize_t second_size );
FBCALL void *fb_UStrConcatAssign( void *dst, ssize_t dst_size, void *src, ssize_t src_size, int fill_rem );
FBCALL ssize_t fb_UStrLen( void *src, ssize_t size );
FBCALL unsigned int fb_UStrAsc( FBSTRING *src, ssize_t position );
FBCALL unsigned int fb_UStrIndex( FBSTRING *src, ssize_t index );
FBCALL void fb_UStrSetIndex( FBSTRING *dst, ssize_t index, ssize_t scalar );
FBSTRING *fb_UStrChr( int count, ... );
FBCALL FBSTRING *fb_UStrMid( FBSTRING *src, ssize_t start, ssize_t count );
FBCALL FBSTRING *fb_UStrLeft( FBSTRING *src, ssize_t count );
FBCALL void fb_UStrLeftSelf( FBSTRING *dst, ssize_t count );
FBCALL void fb_WstrLeftSelf( FB_WCHAR *dst, ssize_t count );
FBCALL FBSTRING *fb_UStrRight( FBSTRING *src, ssize_t count );
FBCALL FBSTRING *fb_UStrFill1( ssize_t count, ssize_t scalar );
FBCALL FBSTRING *fb_UStrFill2( ssize_t count, FBSTRING *src );
FBCALL void fb_UStrAssignMid( FBSTRING *dst, ssize_t start, ssize_t count, FBSTRING *src );
FBCALL void fb_UStrLset( FBSTRING *dst, FBSTRING *src );
FBCALL void fb_UStrRset( FBSTRING *dst, FBSTRING *src );
FBCALL ssize_t fb_UStrInstr( ssize_t start, FBSTRING *src, FBSTRING *pattern );
FBCALL ssize_t fb_UStrInstrAny( ssize_t start, FBSTRING *src, FBSTRING *pattern );
FBCALL ssize_t fb_UStrInstrRev( FBSTRING *src, FBSTRING *pattern, ssize_t start );
FBCALL ssize_t fb_UStrInstrRevAny( FBSTRING *src, FBSTRING *pattern, ssize_t start );
FBCALL FBSTRING *fb_UStrTrim( FBSTRING *src );
FBCALL FBSTRING *fb_UStrTrimEx( FBSTRING *src, FBSTRING *pattern );
FBCALL FBSTRING *fb_UStrTrimAny( FBSTRING *src, FBSTRING *pattern );
FBCALL FBSTRING *fb_UStrLTrim( FBSTRING *src );
FBCALL FBSTRING *fb_UStrLTrimEx( FBSTRING *src, FBSTRING *pattern );
FBCALL FBSTRING *fb_UStrLTrimAny( FBSTRING *src, FBSTRING *pattern );
FBCALL FBSTRING *fb_UStrRTrim( FBSTRING *src );
FBCALL FBSTRING *fb_UStrRTrimEx( FBSTRING *src, FBSTRING *pattern );
FBCALL FBSTRING *fb_UStrRTrimAny( FBSTRING *src, FBSTRING *pattern );
FBCALL FBSTRING *fb_UStrUcase( FBSTRING *src, int mode );
FBCALL FBSTRING *fb_UStrLcase( FBSTRING *src, int mode );

FBCALL int fb_UStrFileLineInput( int fnum, void *dst, ssize_t size, int fill );
FBCALL int fb_UStrInput( void *dst, ssize_t size, int fill );
FBCALL void fb_UStrDataRead( void *dst, ssize_t size, int fill );
FBCALL void fb_UStrPrint( int fnum, FBSTRING *src, int mask );
FBCALL void fb_UStrWrite( int fnum, FBSTRING *src, int mask );

FBCALL FBSTRING *fb_UStrReverse( FBSTRING *src );
FBCALL FBSTRING *fb_UStrReplace( FBSTRING *src, FBSTRING *find, FBSTRING *replacement, ssize_t start, ssize_t count, int compare );
FBCALL int fb_UStrComp( FBSTRING *first, FBSTRING *second, int compare );
FBCALL FBSTRING *fb_UStrFormat( double value, FBSTRING *mask );

FBCALL int fb_PrintUsingInitUstr( FBSTRING *format );
FBCALL int fb_LPrintUsingInitUstr( FBSTRING *format );
FBCALL int fb_PrintUsingUstr( int fnum, FBSTRING *source, int mask );

#endif

/* end of fb_ustring.h */
