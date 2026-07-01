/* get localized short DATE format */

#include "../fb.h"
#include "fb_private_intl.h"

int fb_DrvIntlGetDateFormat( char *buffer, size_t len )
{
    char *pszName;
    char achFormat[90];
    char achOrder[3] = { 0 };
    char achDayZero[2], *pszDayZero;
    char achMonZero[2], *pszMonZero;
    char achDate[2], *pszDate;
    size_t i;

    DBG_ASSERT(buffer!=NULL);
    if( len==0 )
        return FALSE;

    /* Can I use this? The problem is that it returns the date format
     * with localized separators. */
    pszName = fb_hGetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE,
                                 achFormat, sizeof(achFormat) - 1 );
    if( pszName!=NULL ) {
        size_t uiNameSize = strlen(pszName);
        if( uiNameSize < len ) {
            memcpy( buffer, pszName, uiNameSize + 1 );
            return TRUE;
        } else {
            return FALSE;
        }
    }


    /* Fall back for Win95 and WinNT < 4.0 */
    pszDayZero = fb_hGetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_IDAYLZERO,
                                    achDayZero, sizeof(achDayZero) );
    pszMonZero = fb_hGetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_IMONLZERO,
                                    achMonZero, sizeof(achMonZero) );
    pszDate = fb_hGetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_IDATE,
                                 achDate, sizeof(achDate) );
    if( pszDate!=NULL && pszDayZero!=0 && pszMonZero!=0 ) {
        switch( pszDate[0] ) {
        case '0':
            FB_MEMCPY(achOrder, "mdy", 3);
            break;
        case '1':
            FB_MEMCPY(achOrder, "dmy", 3);
            break;
        case '2':
            FB_MEMCPY(achOrder, "ymd", 3);
            break;
        default:
            break;
        }

        if( achOrder[0]!=0 ) {
            size_t remaining = len - 1;
            int day_lead_zero = pszDayZero[0] != '0';
            int mon_lead_zero = pszMonZero[0] != '0';
            for(i=0; i!=3; ++i) {
                const char *pszAdd = NULL;
                size_t add_len;
                switch ( achOrder[i] ) {
                case 'm':
                    if( mon_lead_zero ) {
                        pszAdd = "MM";
                    } else {
                        pszAdd = "M";
                    }
                    break;
                case 'd':
                    if( day_lead_zero ) {
                        pszAdd = "dd";
                    } else {
                        pszAdd = "d";
                    }
                    break;
                case 'y':
                    pszAdd = "yyyy";
                    break;
                default:
                    return FALSE;
                }
                add_len = strlen(pszAdd);
                if( remaining < add_len )
                    return FALSE;
                memcpy( buffer, pszAdd, add_len );
                buffer += add_len;
                remaining -= add_len;
                if( i!=2 ) {
                    if( remaining==0 )
                        return FALSE;
                    *buffer = '/';
                    buffer += 1;
                    remaining -= 1;
                }
            }
            *buffer = '\0';
            return TRUE;
        }
    }

    return FALSE;
}
