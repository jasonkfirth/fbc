/*
 * Project: FreeBASIC runtime library
 * File: console/con_print_raw_uni.h
 * Purpose: print raw data - no interpretation is done.
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include <limits.h>

void FB_CONPRINTRAW
	(
		fb_ConHooks *handle,
        const FB_TCHAR *pachText,
        size_t textLength
	)
{
    fb_Rect *pBorder;
    fb_Coord *pCoord;
    int64_t BorderWidth;

    if( textLength == 0 )
        return;

    pBorder = &handle->Border;
    pCoord = &handle->Coord;
    /* Coordinates use int, while transfer counts use size_t. Form the
       difference before narrowing; an empty or unrepresentable view has
       no writable cells. */
    BorderWidth = (int64_t)pBorder->Right - pBorder->Left + 1;
    if( BorderWidth <= 0 || BorderWidth > INT_MAX )
        return;

    while( textLength != 0 )
    {
        int64_t RemainingWidth;
        size_t copySize;

        if( pBorder->Bottom != -1 &&
            (int64_t)pCoord->Y - pBorder->Bottom > INT_MAX )
            return;
        fb_hConCheckScroll( handle );
        RemainingWidth = (int64_t)pBorder->Right - pCoord->X + 1;
        if( RemainingWidth < 0 || RemainingWidth > BorderWidth )
            return;

        /* A caller can leave the cursor just past the last column.
           Wrap it without issuing a zero-length write. */
        if( RemainingWidth == 0 ) {
            if( pCoord->Y == INT_MAX )
                return;
            pCoord->X = pBorder->Left;
            pCoord->Y += 1;
            continue;
        }

        /* RemainingWidth is positive and at most INT_MAX here. */
        copySize = (textLength > (size_t)RemainingWidth) ?
                   (size_t)RemainingWidth : textLength;
        if( copySize == (size_t)RemainingWidth && pCoord->Y == INT_MAX )
            return;
        if( handle->FB_CON_HOOK_TWRITE( handle,
                                       (const char *)pachText,
                                       copySize ) != TRUE )
            break;

        textLength -= copySize;
        FB_TCHAR_ADVANCE( pachText, copySize );

        /* Wrap before forming Right + 1, which need not fit in int. */
        if( copySize == (size_t)RemainingWidth ) {
            if( pCoord->Y == INT_MAX )
                return;
            pCoord->X = pBorder->Left;
            pCoord->Y += 1;
        } else {
            int64_t NewX = (int64_t)pCoord->X + (int64_t)copySize;
            if( NewX < INT_MIN || NewX > INT_MAX )
                return;
            pCoord->X = (int)NewX;
        }
    }
}

/* end of console/con_print_raw_uni.h */
