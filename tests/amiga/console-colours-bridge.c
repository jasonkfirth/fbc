/*
    FreeBASIC Amiga console qualification
    File: console-colours-bridge.c
    Purpose: Own an isolated real CON: and inspect its native pixels.
    Responsibilities: Initialize the real runtime on interactive handles,
        read console geometry/colours, and restore temporary test pens.
    This file does not emulate the runtime or replace keyboard results.
*/
#include "../../src/rtlib/fb.h"
#include <dos/dosextens.h>
#include <devices/conunit.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>

static BPTR console_handle, previous_input, previous_output;
static struct Window *console_window;
static struct ConUnit *console_unit;
static struct Library *graphics_library;
static ULONG original_palette[8 * 3];
static int palette_count, ready, swapped;

extern void __real_fb_hInit(void);
extern void __real_fb_hEnd(int);

void __wrap_fb_hInit(void)
{
    struct InfoData information __attribute__((aligned(4)));
    struct FileHandle *handle;
    struct IOStdReq *request;
    struct GfxBase *GfxBase;

    console_handle = Open((CONST_STRPTR)"CON:0/12/640/200/FBC console regression/CLOSE", MODE_NEWFILE);
    if (console_handle != BNULL) {
        previous_input = SelectInput(console_handle);
        previous_output = SelectOutput(console_handle);
    }
    __real_fb_hInit();
    if (console_handle == BNULL)
        return;
    handle = (struct FileHandle *)BADDR(console_handle);
    memset(&information, 0, sizeof(information));
    if (!DoPkt(handle->fh_Type, ACTION_DISK_INFO, MKBADDR(&information), 0, 0, 0, 0))
        return;
    console_window = (struct Window *)information.id_VolumeNode;
    request = (struct IOStdReq *)information.id_InUse;
    if (console_window == NULL || request == NULL || request->io_Unit == NULL)
        return;
    console_unit = (struct ConUnit *)request->io_Unit;
    if (console_unit->cu_Window != console_window)
        return;
    graphics_library = OpenLibrary((CONST_STRPTR)"graphics.library", 39);
    if (graphics_library == NULL)
        return;
    GfxBase = (struct GfxBase *)graphics_library;
    palette_count = console_window->WScreen->RastPort.BitMap->Depth;
    palette_count = palette_count < 3 ? (1 << palette_count) : 8;
    GetRGB32(console_window->WScreen->ViewPort.ColorMap, 0,
             (ULONG)palette_count, original_palette);
    ready = TRUE;
}

void __wrap_fb_hEnd(int code)
{
    if (swapped && graphics_library != NULL && console_window != NULL) {
        struct GfxBase *GfxBase = (struct GfxBase *)graphics_library;
        int index;
        for (index = 0; index < palette_count; ++index)
            SetRGB32(&console_window->WScreen->ViewPort, (ULONG)index,
                     original_palette[index * 3], original_palette[index * 3 + 1],
                     original_palette[index * 3 + 2]);
    }
    __real_fb_hEnd(code);
    if (graphics_library != NULL)
        CloseLibrary(graphics_library);
    if (console_handle != BNULL) {
        SelectInput(previous_input);
        SelectOutput(previous_output);
        Close(console_handle);
    }
}

int amiga_console_ready(void) { return ready; }
int amiga_console_columns(void) { return ready ? console_unit->cu_XMax + 1 : 0; }
int amiga_console_rows(void) { return ready ? console_unit->cu_YMax + 1 : 0; }
int amiga_console_wrap(void)
{
    return ready ? !!(console_unit->cu_Modes[PMB_AWM >> 3] & (1U << (PMB_AWM & 7))) : -1;
}
int amiga_console_cursor(int row, int column)
{
    return ready && console_unit->cu_YCP + 1 == row && console_unit->cu_XCP + 1 == column;
}
int amiga_console_cell(int row, int column, unsigned int foreground,
                       unsigned int background, int need_foreground)
{
    struct GfxBase *GfxBase = (struct GfxBase *)graphics_library;
    ULONG palette[8 * 3];
    int x, y, left, top, seen_foreground = FALSE, seen_background = FALSE;
    if (!ready || row < 1 || column < 1 || row > amiga_console_rows() ||
        column > amiga_console_columns())
        return 1;
    GetRGB32(console_window->WScreen->ViewPort.ColorMap, 0,
             (ULONG)palette_count, palette);
    left = console_unit->cu_XROrigin + (column - 1) * console_unit->cu_XRSize;
    top = console_unit->cu_YROrigin + (row - 1) * console_unit->cu_YRSize;
    for (y = 0; y < console_unit->cu_YRSize; ++y)
        for (x = 0; x < console_unit->cu_XRSize; ++x) {
            LONG pen = ReadPixel(console_window->RPort, left + x, top + y);
            unsigned int colour;
            if (pen < 0 || pen >= palette_count)
                return 2;
            colour = ((palette[pen * 3] >> 24) << 16) |
                     ((palette[pen * 3 + 1] >> 24) << 8) | (palette[pen * 3 + 2] >> 24);
            if (colour == background)
                seen_background = TRUE;
            else if (need_foreground && colour == foreground)
                seen_foreground = TRUE;
            else
                return 3;
        }
    return (!seen_background || (need_foreground && !seen_foreground)) ? 4 : 0;
}
int amiga_console_palette_unchanged(void)
{
    struct GfxBase *GfxBase = (struct GfxBase *)graphics_library;
    ULONG palette[8 * 3];
    if (!ready) return FALSE;
    GetRGB32(console_window->WScreen->ViewPort.ColorMap, 0, (ULONG)palette_count, palette);
    return memcmp(palette, original_palette, (size_t)palette_count * 3 * sizeof(ULONG)) == 0;
}
int amiga_console_swap(void)
{
    struct GfxBase *GfxBase = (struct GfxBase *)graphics_library;
    if (!ready || palette_count < 3) return FALSE;
    SetRGB32(&console_window->WScreen->ViewPort, 1,
             original_palette[6], original_palette[7], original_palette[8]);
    SetRGB32(&console_window->WScreen->ViewPort, 2,
             original_palette[3], original_palette[4], original_palette[5]);
    swapped = TRUE;
    return TRUE;
}
int amiga_console_resize(void)
{
    struct Library *intuition;
    struct IntuitionBase *IntuitionBase;
    int columns, rows, attempt, changed = FALSE;
    if (!ready) return FALSE;
    columns = amiga_console_columns(); rows = amiga_console_rows();
    intuition = OpenLibrary((CONST_STRPTR)"intuition.library", 37);
    if (intuition == NULL) return FALSE;
    IntuitionBase = (struct IntuitionBase *)intuition;
    SizeWindow(console_window, -120, -45);
    /* Wait for the genuine console handler's resize event, not a fabricated
       runtime notification. The original test window stays owned until exit. */
    for (attempt = 0; attempt < 50; ++attempt) {
        if (amiga_console_columns() < columns && amiga_console_rows() < rows) {
            changed = TRUE;
            break;
        }
        Delay(1);
    }
    CloseLibrary(intuition);
    return changed;
}
/* end of console-colours-bridge.c */
