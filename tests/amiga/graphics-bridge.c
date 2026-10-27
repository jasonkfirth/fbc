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

#include "../../src/gfxlib2/drivers/amiga/fb_gfx_amiga.h"
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
        /* Coordinates supplied by BASIC refer to its client framebuffer. */
        pen = ReadPixel(fb_amiga_gfx.window->RPort,
            x + fb_amiga_gfx.window->BorderLeft,
            y + fb_amiga_gfx.window->BorderTop);
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

int amiga_test_event_queued(int code, int peek)
{
    struct IntuiMessage message;
    struct MsgPort *reply = CreateMsgPort();
    EVENT event;
    int result = -1;
    int expected_type = code & IECODE_UP_PREFIX ? EVENT_KEY_RELEASE : EVENT_KEY_PRESS;

    if (reply == NULL) return -1;
    memset(&message, 0, sizeof(message));
    message.ExecMessage.mn_ReplyPort = reply;
    message.ExecMessage.mn_Length = sizeof(message);
    fb_GfxLock();
    if (fb_amiga_gfx.window != NULL) {
        /* Clear creation/focus events before queuing the test message. */
        fb_amigaGfxPollEvents();
        while (fb_GfxEvent(&event)) { }
        message.Class = IDCMP_RAWKEY;
        message.Code = (UWORD)code;
        message.IDCMPWindow = fb_amiga_gfx.window;
        PutMsg(fb_amiga_gfx.window->UserPort, &message.ExecMessage);
        if ((!peek || fb_GfxEvent(NULL)) && fb_GfxEvent(&event) &&
            event.type == expected_type && event.scancode == SC_A)
            result = 0;
        /* If the query failed, consume the native message before its stack
           storage and reply port disappear. This poll is after the assertion. */
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

int amiga_test_present_cache(int x, int y)
{
    struct IntuiMessage message;
    struct MsgPort *reply = CreateMsgPort();
    unsigned char *cache, *valid;
    LONG original, damaged;
    int result = -1;
    int window_x, window_y;

    if (reply == NULL) return -1;
    memset(&message, 0, sizeof(message));
    message.ExecMessage.mn_ReplyPort = reply;
    message.ExecMessage.mn_Length = sizeof(message);
    fb_GfxLock();
    if (fb_amiga_gfx.present_cache == NULL || fb_amiga_gfx.window == NULL ||
        x < 0 || x >= fb_amiga_gfx.width || y < 0 || y >= fb_amiga_gfx.height)
        goto done;
    window_x = x + fb_amiga_gfx.window->BorderLeft;
    window_y = y + fb_amiga_gfx.window->BorderTop;
    fb_amigaGfxPresent();
    if (!fb_amiga_gfx.present_cache_valid[y]) goto done;

    original = ReadPixel(fb_amiga_gfx.window->RPort, window_x, window_y);
    if (original < 0) goto done;
    damaged = original ^ 1;
    SetAPen(fb_amiga_gfx.window->RPort, (ULONG)damaged);
    WritePixel(fb_amiga_gfx.window->RPort, window_x, window_y);
    __fb_gfx->dirty[y] = TRUE;
    fb_amigaGfxPresent();
    /* A software row which has not changed must use the cache. A real
       refresh event, rather than that identical row, repairs window damage. */
    if ((LONG)ReadPixel(fb_amiga_gfx.window->RPort, window_x, window_y) != damaged) goto done;
    message.Class = IDCMP_REFRESHWINDOW;
    message.IDCMPWindow = fb_amiga_gfx.window;
    PutMsg(fb_amiga_gfx.window->UserPort, &message.ExecMessage);
    fb_amigaGfxPollEvents();
    if (GetMsg(reply) != &message.ExecMessage) goto done;
    if ((LONG)ReadPixel(fb_amiga_gfx.window->RPort, window_x, window_y) != original) goto done;

    /* Exercise the allocation-failure path without exhausting guest RAM.
       Temporarily withhold the optional cache and require an actual upload. */
    cache = fb_amiga_gfx.present_cache;
    valid = fb_amiga_gfx.present_cache_valid;
    fb_amiga_gfx.present_cache = NULL;
    fb_amiga_gfx.present_cache_valid = NULL;
    SetAPen(fb_amiga_gfx.window->RPort, (ULONG)damaged);
    WritePixel(fb_amiga_gfx.window->RPort, window_x, window_y);
    __fb_gfx->dirty[y] = TRUE;
    fb_amigaGfxPresent();
    if ((LONG)ReadPixel(fb_amiga_gfx.window->RPort, window_x, window_y) == original) result = 0;
    fb_amiga_gfx.present_cache = cache;
    fb_amiga_gfx.present_cache_valid = valid;

done:
    fb_amigaGfxInvalidate();
    if (__fb_gfx != NULL && __fb_gfx->dirty != NULL)
        memset(__fb_gfx->dirty, TRUE, (size_t)fb_amiga_gfx.height);
    fb_GfxUnlock(0, 0);
    DeleteMsgPort(reply);
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

/* Window decorations and private palette ownership are independent.
   These observations must describe the entire requested BASIC client area. */
int amiga_test_window_layout(int borderless)
{
    struct Window *window;
    int result = -1;

    fb_GfxLock();
    window = fb_amiga_gfx.window;
    if (window != NULL && fb_amiga_gfx.screen != NULL &&
        !!(window->Flags & WFLG_BORDERLESS) == !!borderless &&
        !!(window->Flags & WFLG_CLOSEGADGET) == !borderless &&
        window->Width - window->BorderLeft - window->BorderRight >= fb_amiga_gfx.width &&
        window->Height - window->BorderTop - window->BorderBottom >= fb_amiga_gfx.height &&
        window->LeftEdge >= 0 && window->TopEdge >= 0 &&
        window->LeftEdge + window->Width <= fb_amiga_gfx.screen->Width &&
        window->TopEdge + window->Height <= fb_amiga_gfx.screen->Height &&
        (!borderless || (window->BorderLeft == 0 && window->BorderTop == 0 &&
            window->BorderRight == 0 && window->BorderBottom == 0)))
        result = 0;
    fb_GfxUnlock(0, 0);
    return result;
}

/* end of graphics-bridge.c */
