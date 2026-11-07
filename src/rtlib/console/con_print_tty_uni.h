/*
 * Project: FreeBASIC runtime library
 * File: console/con_print_tty_uni.h
 * Purpose: print text data - using TTY (teletype) interpretation.
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include <limits.h>

/* Commit only representable positions. Pending output can be flushed and
   retried when its scroll hook brings a virtual row back into the view. */
static __inline__ int fb_hConAdvanceCoord( fb_Coord *coord,
                                         const fb_Rect *border,
                                         int width, int chars )
{
    int64_t NewX = (int64_t)coord->X + chars;
    int64_t NewY = coord->Y;

    if( width <= 0 || NewX < border->Left )
        return FALSE;
    if( NewX > border->Right ) {
        int64_t NormalX = NewX - border->Left;
        NewX = (NormalX % width) + border->Left;
        NewY += NormalX / width;
    }
    if( NewX < INT_MIN || NewX > INT_MAX ||
        NewY < INT_MIN || NewY > INT_MAX )
        return FALSE;
    coord->X = (int)NewX;
    coord->Y = (int)NewY;
    return TRUE;
}

#ifndef OUTPUT_BUFFER_SIZE
#define OUTPUT_BUFFER_SIZE 1024
#endif

void FB_CONPRINTTTY
	(
		fb_ConHooks *handle,
        const FB_TCHAR *pachText,
        size_t TextLength,
        int is_text_mode
	)
{
    static const FB_TCHAR achTabSpaces[8] = { 32, 32, 32, 32, 32, 32, 32, 32 };
    FB_TCHAR OutputBuffer[OUTPUT_BUFFER_SIZE];
    size_t OutputBufferLength = 0, OutputBufferChars = 0;
    fb_Rect *pBorder;
    fb_Coord *pCoord;

    fb_Coord dwCurrentCoord;
    fb_Coord dwMoveCoord = { 0, 0 };
    size_t IndexText;
    int fGotNewCoordinate = FALSE;
    int BorderWidth;
    int64_t Width;

    if( TextLength == 0 )
        return;

    pBorder = &handle->Border;
    pCoord = &handle->Coord;
    /* Tiny graphics screens can have no columns. Check the wide
       difference before using an int width or dividing by it. */
    Width = (int64_t)pBorder->Right - pBorder->Left + 1;
    if( Width <= 0 || Width > INT_MAX )
        return;
    BorderWidth = (int)Width;
    if( (int64_t)pCoord->X < pBorder->Left ||
        (int64_t)pCoord->X > (int64_t)pBorder->Right + 1 )
        return;
    if( pBorder->Bottom != -1 &&
        (int64_t)pCoord->Y - pBorder->Bottom > INT_MAX )
        return;

    memcpy( &dwCurrentCoord, pCoord, sizeof( fb_Coord ) );

    for( IndexText=0; IndexText!=TextLength; ++IndexText )
    {
        const FB_TCHAR *pachOutputData = pachText;
        size_t OutputDataLength = 0;
        int OutputDataChars = 0;
        int fDoFlush = FALSE;
        int fSetNewCoord = FALSE;
        FB_TCHAR ch = FB_TCHAR_GET( pachOutputData );

        switch ( ch )
        {
        case FB_TCHAR_LITERAL('\a'):
            /* ALARM */
            fb_Beep();
            break;

        case FB_TCHAR_LITERAL('\b'):
            /* BACKSPACE */
            fSetNewCoord = TRUE;
            if( dwCurrentCoord.X > pBorder->Left ) {
                dwMoveCoord.X = -1;
            } else {
                dwMoveCoord.X = 0;
            }
            dwMoveCoord.Y = 0;
            break;

        case FB_TCHAR_LITERAL('\n'):
            /* LINE FEED / NEW LINE */
            fSetNewCoord = TRUE;
            if( is_text_mode ) {
                dwMoveCoord.X = pBorder->Left - dwCurrentCoord.X;
                dwMoveCoord.Y = 1;
            } else {
                dwMoveCoord.X = 0;
                dwMoveCoord.Y = 1;
            }
            break;

        case FB_TCHAR_LITERAL('\r'):
            /* CARRIAGE RETURN */
            fSetNewCoord = TRUE;
            dwMoveCoord.X = pBorder->Left - dwCurrentCoord.X;
            dwMoveCoord.Y = 0;
            break;

        case FB_TCHAR_LITERAL('\t'):
            /* TAB */
            pachOutputData = achTabSpaces;
            /* Tab stops are eight cells apart; avoid rounding the
               absolute coordinate beyond its representable range. */
            OutputDataChars = 8 - ((dwCurrentCoord.X - pBorder->Left) & 7);
            OutputDataLength = (size_t)OutputDataChars;
            break;

        default:
            OutputDataLength = FB_TCHAR_GET_CHAR_SIZE( pachOutputData );
            OutputDataChars = 1;
            break;
        }

        if( OutputDataLength + OutputBufferLength > OUTPUT_BUFFER_SIZE ) {
            fDoFlush = TRUE;
        } else if( fSetNewCoord ) {
            fDoFlush = TRUE;
        }

        if( fDoFlush ) {
            if( OutputBufferLength!=0 ) {
                FB_CONPRINTRAW( handle,
                                OutputBuffer,
                                OutputBufferChars );
                OutputBufferLength = OutputBufferChars = 0;
                /* Raw output can scroll. Follow the actual hook position
                   instead of accumulating an unbounded virtual row. */
                memcpy( &dwCurrentCoord, pCoord, sizeof( fb_Coord ) );
                fGotNewCoordinate = FALSE;
            }
        }

        if( fSetNewCoord ) {
            int64_t NewX = (int64_t)pCoord->X + dwMoveCoord.X;
            int64_t NewY = (int64_t)pCoord->Y + dwMoveCoord.Y;
            if( NewX < INT_MIN || NewX > INT_MAX ||
                NewY < INT_MIN || NewY > INT_MAX ||
                (pBorder->Bottom != -1 && NewY - pBorder->Bottom > INT_MAX) )
                break;
            pCoord->X = (int)NewX;
            pCoord->Y = (int)NewY;
            memcpy( &dwCurrentCoord, pCoord, sizeof( fb_Coord ) );
            fGotNewCoordinate = TRUE;
        }

        if( OutputDataLength!=0 ) {
            if( fb_hConAdvanceCoord( &dwCurrentCoord, pBorder, BorderWidth, OutputDataChars ) == FALSE ) {
                if( OutputBufferLength != 0 ) {
                    FB_CONPRINTRAW( handle, OutputBuffer, OutputBufferChars );
                    OutputBufferLength = OutputBufferChars = 0;
                } else if( pBorder->Bottom != -1 && pCoord->Y > pBorder->Bottom ) {
                    fb_hConCheckScroll( handle );
                } else {
                    break;
                }
                memcpy( &dwCurrentCoord, pCoord, sizeof( fb_Coord ) );
                fGotNewCoordinate = FALSE;
                if( fb_hConAdvanceCoord( &dwCurrentCoord, pBorder, BorderWidth, OutputDataChars ) == FALSE )
                    break;
            }
            while( OutputDataLength-- ) {
                OutputBuffer[OutputBufferLength++] = *pachOutputData++;
            }
            /* OutputDataChars is zero, one, or at most eight tab cells. */
            OutputBufferChars += (size_t)OutputDataChars;
        }

        FB_TCHAR_ADVANCE( pachText, 1 );
    }

    if( OutputBufferLength!=0 )
    {
        FB_CONPRINTRAW( handle, OutputBuffer, OutputBufferChars );
    }
    else if( fGotNewCoordinate )
    {
        fb_hConCheckScroll( handle );
    }
}

/* end of console/con_print_tty_uni.h */
