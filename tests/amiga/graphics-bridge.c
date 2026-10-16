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
#include <graphics/displayinfo.h>
#include <graphics/modeid.h>
#include <stddef.h>

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

int amiga_test_multikey_queued(int code, int scancode)
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
        message.Class = IDCMP_RAWKEY;
        message.Code = (UWORD)code;
        message.IDCMPWindow = fb_amiga_gfx.window;
        PutMsg(fb_amiga_gfx.window->UserPort, &message.ExecMessage);
        /* MULTIKEY must consume the queued event itself. Polling afterward
           only guarantees that this stack-owned message is safely replied
           before returning, including when the tested query fails. */
        result = fb_GfxMultikey(scancode) ? 1 : 0;
        fb_amigaGfxPollEvents();
        if (GetMsg(reply) != &message.ExecMessage) result = -1;
    }
    fb_GfxUnlock(0, 0);
    DeleteMsgPort(reply);
    return result;
}

int amiga_test_pointer_hidden(void)
{
    int result = -1;
    int index;

    fb_GfxLock();
    if (fb_amiga_gfx.window != NULL) {
        const UWORD *pointer = fb_amiga_gfx.window->Pointer;
        result = 0;
        if (pointer != NULL && fb_amiga_gfx.window->PtrHeight == 1 &&
            fb_amiga_gfx.window->PtrWidth == 16) {
            /* Inspect the transparent image row. Intuition owns the sprite
               control words, so their contents are not application state. */
            result = 1;
            for (index = 2; index < 4; ++index) {
                if (pointer[index] != 0) result = -1;
            }
        }
    }
    fb_GfxUnlock(0, 0);
    return result;
}

int amiga_test_display_mode(int *width, int *height, ULONG *mode_id)
{
    struct DimensionInfo dimensions;
    ULONG mode, length;
    int result = -1;

    *width = *height = 0;
    *mode_id = (ULONG)INVALID_ID;
    fb_GfxLock();
    if (fb_amiga_gfx.screen != NULL) {
        mode = GetVPModeID(&fb_amiga_gfx.screen->ViewPort);
        length = GetDisplayInfoData(NULL, (UBYTE *)&dimensions,
            sizeof(dimensions), DTAG_DIMS, mode);
        if (length >= offsetof(struct DimensionInfo, Nominal) + sizeof(dimensions.Nominal)) {
            *width = dimensions.Nominal.MaxX - dimensions.Nominal.MinX + 1;
            *height = dimensions.Nominal.MaxY - dimensions.Nominal.MinY + 1;
            *mode_id = mode;
            result = 0;
        }
    }
    fb_GfxUnlock(0, 0);
    return result;
}

/* end of graphics-bridge.c */
