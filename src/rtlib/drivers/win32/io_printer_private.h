/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/io_printer_private.h
 * Purpose: Provide io printer private runtime support.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include <windows.h>
#include <winspool.h>

struct FB_W32_PRINTER_INFO;

typedef void (*FnEmuPrint)(struct FB_W32_PRINTER_INFO *pInfo, const void *pText, size_t uiLength, int isunicode);

/* Win32-specific printer information */
typedef struct FB_W32_PRINTER_INFO {
    HANDLE          hPrinter;
    DWORD           dwJob;
    HDC             hDc;
    /* GDI reports signed logical coordinates and font sizes.
       Spooler job identifiers and enumeration counts remain DWORD. */
    struct {
        LONG        dwFullSizeX;
        LONG        dwFullSizeY;
        LONG        dwSizeX;
        LONG        dwSizeY;
        LONG        dwOffsetX;
        LONG        dwOffsetY;
        LONG        dwDPI_X;
        LONG        dwDPI_Y;

        LONG        dwCurrentX;
        LONG        dwCurrentY;
        HFONT       hFont;
        COLORREF    clFore, clBack;
        LONG        dwFontSizeX;
        LONG        dwFontSizeY;
        int         iPageStarted;

        FnEmuPrint  pfnPrint;
    } Emu;
} W32_PRINTER_INFO;

/* end of drivers/win32/io_printer_private.h */
