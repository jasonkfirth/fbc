/*
    FreeBASIC classic AmigaOS graphics qualification
    -----------------------------------------------
    File: graphics-bridge.c
    Purpose: Observe the native display and submit real Intuition messages.
    Responsibilities: Read displayed colours and check message reply ownership.
    This file intentionally does NOT contain framebuffer or input emulation.

    The graphics lock excludes the runtime poller while a stack-owned test
    message is queued and consumed. Its reply is removed before the stack or
    reply port disappears. ReadPixel and GetRGB32 inspect the native bitmap
    and palette, independently of BASIC's POINT framebuffer operation.
*/

#include "../../src/gfxlib2/amiga/fb_gfx_amiga.h"
#include <proto/exec.h>
#include <proto/graphics.h>

unsigned long amiga_test_display_pixel(int x, int y)
{
    ULONG colours[3];
    LONG pen;
    unsigned long result = ~0UL;

    fb_GfxLock();
    if (fb_amiga_gfx.window != NULL && fb_amiga_gfx.screen != NULL &&
        x >= 0 && x < fb_amiga_gfx.width && y >= 0 && y < fb_amiga_gfx.height) {
        fb_amigaGfxPresent();
        pen = ReadPixel(fb_amiga_gfx.window->RPort, x, y);
        if (pen >= 0) {
            GetRGB32(fb_amiga_gfx.screen->ViewPort.ColorMap, (ULONG)pen, 1, colours);
            result = ((colours[0] >> 24) << 16) |
                     ((colours[1] >> 24) << 8) | (colours[2] >> 24);
        }
    }
    fb_GfxUnlock(0, 0);
    return result;
}

int amiga_test_input(int kind, int code, int x, int y)
{
    struct IntuiMessage message;
    struct MsgPort *reply = CreateMsgPort();
    int result = -1;

    if (reply == NULL) return -1;
    memset(&message, 0, sizeof(message));
    message.ExecMessage.mn_ReplyPort = reply;
    message.ExecMessage.mn_Length = sizeof(message);
    fb_GfxLock();
    if (fb_amiga_gfx.window != NULL) {
        message.Class = kind == 0 ? IDCMP_RAWKEY : IDCMP_MOUSEMOVE;
        message.Code = (UWORD)code;
        message.MouseX = (WORD)(x + fb_amiga_gfx.window->BorderLeft);
        message.MouseY = (WORD)(y + fb_amiga_gfx.window->BorderTop);
        message.IDCMPWindow = fb_amiga_gfx.window;
        PutMsg(fb_amiga_gfx.window->UserPort, &message.ExecMessage);
        fb_amigaGfxPollEvents();
        if (GetMsg(reply) == &message.ExecMessage) result = 0;
    }
    fb_GfxUnlock(0, 0);
    DeleteMsgPort(reply);
    return result;
}

/* end of graphics-bridge.c */
