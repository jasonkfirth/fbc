/*
 * Project: FreeBASIC runtime library
 * File: drivers/wince/io_printer_private.h
 * Purpose: Provide io printer private runtime support.
 * Responsibilities: Target-specific runtime support for wince.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include <windows.h>
#include <winspool.h>

struct _W32_PRINTER_INFO;

typedef void (*FnEmuPrint)(struct _W32_PRINTER_INFO *pInfo, const void *pText, size_t uiLength, int isunicode);

/* Win32-specific printer information */
typedef struct _W32_PRINTER_INFO {
    HANDLE          hPrinter;
    DWORD           dwJob;
    HDC             hDc;
    struct {
        DWORD       dwFullSizeX;
        DWORD       dwFullSizeY;
        DWORD       dwSizeX;
        DWORD       dwSizeY;
        DWORD       dwOffsetX;
        DWORD       dwOffsetY;
        DWORD       dwDPI_X;
        DWORD       dwDPI_Y;

        DWORD       dwCurrentX;
        DWORD       dwCurrentY;
        HFONT       hFont;
        COLORREF    clFore, clBack;
        DWORD       dwFontSizeX;
        DWORD       dwFontSizeY;
        int         iPageStarted;

        FnEmuPrint  pfnPrint;
    } Emu;
} W32_PRINTER_INFO;

/* end of drivers/wince/io_printer_private.h */
