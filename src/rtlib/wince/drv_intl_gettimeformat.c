/* get localized short TIME format */

#include "../fb.h"
#include "fb_private_intl.h"

int fb_DrvIntlGetTimeFormat( char *buffer, size_t len )
{
    char achFormat[90], *pszFormat;
    char achHourZero[8], *pszHourZero;
    char achTimeMark[8], *pszTimeMark;
    char achTimeMarkPos[8], *pszTimeMarkPos;
    int use_timemark, timemark_prefix;
    size_t i;

    DBG_ASSERT(buffer!=NULL);
    if( len==0 )
        return FALSE;

    /* Can I use this? The problem is that it returns the date format
     * with localized separators. */
    pszFormat = fb_hGetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_STIMEFORMAT,
                                   achFormat, sizeof(achFormat) - 1 );
    if( pszFormat!=NULL ) {
        size_t uiNameSize = strlen(pszFormat);
        if( uiNameSize < len ) {
            memcpy( buffer, pszFormat, uiNameSize + 1 );
            return TRUE;
        } else {
            return FALSE;
        }
    }


    /* Fall back for Win95 and WinNT < 4.0 */
    pszTimeMarkPos = fb_hGetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_ITIMEMARKPOSN,
                                        achTimeMarkPos, sizeof(achTimeMarkPos) );
    pszTimeMark = fb_hGetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_ITIME,
                                     achTimeMark, sizeof(achTimeMark) );
    pszHourZero = fb_hGetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_ITLZERO,
                                     achHourZero, sizeof(achHourZero) );

    i = 0;

    use_timemark = ( pszTimeMark!=NULL && pszTimeMark[0]=='1' );
    timemark_prefix = ( pszTimeMarkPos!=NULL && pszTimeMarkPos[0]=='1' );

    if( use_timemark && timemark_prefix ) {
        FB_MEMCPY( achFormat + i, "AM/PM ", 6 );
        i += 6;
    }

    if( pszHourZero!=NULL && pszHourZero[0]=='1' ) {
        if( !use_timemark ) {
            FB_MEMCPY( achFormat + i, "HH:", 3 );
        } else {
            FB_MEMCPY( achFormat + i, "hh:", 3 );
        }
        i += 3;
    }
    FB_MEMCPY( achFormat + i, "mm:ss", 5 );
    i += 5;

    if( use_timemark && !timemark_prefix ) {
        FB_MEMCPY( achFormat + i, " AM/PM", 6 );
        i += 6;
    }

    if( len < (i+1) )
        return FALSE;

    FB_MEMCPY(buffer, achFormat, i);
    buffer[i] = 0;

    return TRUE;
}
