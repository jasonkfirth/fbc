/*
    Project: FreeBASIC runtime library
    -------------------------

    File: drivers/amiga/hinit.c

    Purpose:

        Initialize the FreeBASIC console and runtime locks on AmigaOS.

    Responsibilities:

        - use dos.library for interactive console input and raw-mode control
        - leave redirected input on the shared buffered stdio fallback
        - translate FreeBASIC text-console operations to AmigaOS CSI commands
        - own runtime and background-thread locks
        - maintain the console geometry used by text-mode operations

    This file intentionally does NOT contain: - POSIX terminal discovery or TERM environment handling
        - Intuition graphics-window lifecycle
        - AmigaOS input events for gfxlib2

    AmigaOS console policy:

        AmigaOS console handles are not Unix tty file descriptors.  In
        particular, the POSIXC environment and termios path may block while
        the runtime is still inside its INIT symbol set.  Use dos.library's
        Input(), WaitForChar(), Read(), and SetMode() interface directly.
*/

#include "../../fb.h"
#include "../../fb_private_thread.h"
#include "../unix/fb_private_console.h"

#include <dos/dos.h>
#include <proto/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <devices/conunit.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FB_AmigaOS_CONSOLE_COLUMNS 80
#define FB_AmigaOS_CONSOLE_ROWS 25
#define FB_AmigaOS_CSI "\233"

FBCONSOLE __fb_con;

static pthread_t __fb_bg_thread;
static pthread_mutex_t __fb_bg_mutex;
static int bgthread_inited = FALSE;
static int console_raw_mode = FALSE;
static volatile int __fb_console_resized;

#ifdef ENABLE_MT
static pthread_mutex_t __fb_global_mutex;
static pthread_mutex_t __fb_string_mutex;
static pthread_mutex_t __fb_graphics_mutex;
static pthread_mutex_t __fb_math_mutex;
static pthread_mutex_t __fb_profile_mutex;
#endif

/* ------------------------------------------------------------------------- */
/* Runtime locks                                                             */
/* ------------------------------------------------------------------------- */

FBCALL void fb_BgLock(void)
{
    pthread_mutex_lock(&__fb_bg_mutex);
}

FBCALL void fb_BgUnlock(void)
{
    pthread_mutex_unlock(&__fb_bg_mutex);
}

#ifdef ENABLE_MT
FBCALL void fb_Lock(void)
{
    pthread_mutex_lock(&__fb_global_mutex);
}

FBCALL void fb_Unlock(void)
{
    pthread_mutex_unlock(&__fb_global_mutex);
}

FBCALL void fb_StrLock(void)
{
    pthread_mutex_lock(&__fb_string_mutex);
}

FBCALL void fb_StrUnlock(void)
{
    pthread_mutex_unlock(&__fb_string_mutex);
}

FBCALL void fb_GraphicsLock(void)
{
    pthread_mutex_lock(&__fb_graphics_mutex);
}

FBCALL void fb_GraphicsUnlock(void)
{
    pthread_mutex_unlock(&__fb_graphics_mutex);
}

FBCALL void fb_MathLock(void)
{
    pthread_mutex_lock(&__fb_math_mutex);
}

FBCALL void fb_MathUnlock(void)
{
    pthread_mutex_unlock(&__fb_math_mutex);
}

FBCALL void fb_ProfileLock(void)
{
    pthread_mutex_lock(&__fb_profile_mutex);
}

FBCALL void fb_ProfileUnlock(void)
{
    pthread_mutex_unlock(&__fb_profile_mutex);
}
#endif

/* ------------------------------------------------------------------------- */
/* Background input service                                                  */
/* ------------------------------------------------------------------------- */

static void *bg_thread(void *unused)
{
    (void)unused;

    while (TRUE) {
        BG_LOCK();
        if (__fb_con.inited == FALSE) {
            BG_UNLOCK();
            break;
        }
        if (__fb_con.keyboard_handler != NULL)
            __fb_con.keyboard_handler();
        if (__fb_con.mouse_handler != NULL)
            __fb_con.mouse_handler();
        BG_UNLOCK();
        Delay(2);
    }

    /* The SDK trampoline treats NULL as a C process-exit request. */
    return (void *)&__fb_bg_thread;
}

void fb_hStartBgThread(void)
{
    if (bgthread_inited == FALSE &&
        pthread_create(&__fb_bg_thread, NULL, bg_thread, NULL) == 0)
    {
        bgthread_inited = TRUE;
    }
}

static int default_getch(void)
{
    BPTR input;
    unsigned char character;

    input = Input();
    if (input == BNULL || WaitForChar(input, 0) == 0)
        return EOF;
    if (Read(input, &character, 1) != 1)
        return EOF;

    return character;
}

/* ------------------------------------------------------------------------- */
/* Native AmigaOS console                                                       */
/* ------------------------------------------------------------------------- */

typedef struct FB_AMIGA_CONSOLE_QUERY {
    struct InfoData information __attribute__((aligned(4)));
    struct MsgPort *handler;
    struct Window *window;
    struct ConUnit *unit;
    BPTR output;
    LONG saved_error;
    int acquired;
} FB_AMIGA_CONSOLE_QUERY;

static void fb_hAmigaConsoleRelease(FB_AMIGA_CONSOLE_QUERY *query)
{
#ifdef ACTION_UNDISK_INFO
    /* Newer handlers retain AUTO/iconified windows for DISK_INFO callers. */
    if (query->acquired)
        DoPkt(query->handler, ACTION_UNDISK_INFO, 0, 0, 0, 0, 0);
#endif
    SetIoErr(query->saved_error);
}

static int fb_hAmigaConsoleAcquire(FB_AMIGA_CONSOLE_QUERY *query)
{
    struct FileHandle *handle;
    struct IOStdReq *request;

    memset(query, 0, sizeof(*query));
    query->saved_error = IoErr();
    query->output = Output();
    if (query->output == BNULL || IsInteractive(query->output) == DOSFALSE)
        goto failed;
    handle = (struct FileHandle *)BADDR(query->output);
    query->handler = handle->fh_Type;
    if (query->handler == NULL)
        goto failed;
    /* The packet requires longword-aligned InfoData on the word-aligned
       68k ABI. CON:/RAW: return a native Window pointer in id_VolumeNode,
       despite that field's BPTR type. AUX: may return no window or unit. */
    if (DoPkt(query->handler, ACTION_DISK_INFO,
              MKBADDR(&query->information), 0, 0, 0, 0) == DOSFALSE)
        goto failed;
    query->acquired = TRUE;
    if ((ULONG)query->information.id_DiskType != 0x434f4e00UL &&
        (ULONG)query->information.id_DiskType != 0x52415700UL)
        goto failed;
    query->window = (struct Window *)query->information.id_VolumeNode;
    if (query->window == NULL || query->window->WScreen == NULL)
        goto failed;
    request = (struct IOStdReq *)query->information.id_InUse;
    if (request != NULL && request->io_Device != NULL &&
        request->io_Device->dd_Library.lib_Node.ln_Name != NULL &&
        strcmp((char *)request->io_Device->dd_Library.lib_Node.ln_Name,
               "console.device") == 0 && request->io_Unit != NULL) {
        query->unit = (struct ConUnit *)request->io_Unit;
        if (query->unit->cu_Window != query->window)
            query->unit = NULL;
    }
    return TRUE;

failed:
    fb_hAmigaConsoleRelease(query);
    return FALSE;
}

static int fb_hAmigaConsoleSize(int *columns, int *rows)
{
    FB_AMIGA_CONSOLE_QUERY query;
    int result = FALSE;
    if (!fb_hAmigaConsoleAcquire(&query))
        return FALSE;
    if (query.unit != NULL && query.unit->cu_XMax >= 0 &&
        query.unit->cu_YMax >= 0) {
        *columns = (int)query.unit->cu_XMax + 1;
        *rows = (int)query.unit->cu_YMax + 1;
        result = TRUE;
    }
    fb_hAmigaConsoleRelease(&query);
    return result;
}

static int fb_hAmigaConsolePen(unsigned int colour)
{
    /* BASIC uses DOS RGBI ordering; Amiga SGR numbers name screen pens. */
    static const unsigned int colours[16] = {
        0x000000U, 0x0000aaU, 0x00aa00U, 0x00aaaaU,
        0xaa0000U, 0xaa00aaU, 0xaa5500U, 0xaaaaaaU,
        0x555555U, 0x5555ffU, 0x55ff55U, 0x55ffffU,
        0xff5555U, 0xff55ffU, 0xffff55U, 0xffffffU
    };
    FB_AMIGA_CONSOLE_QUERY query;
    struct Library *graphics = NULL;
    struct GfxBase *GfxBase;
    ULONG palette[8 * 3];
    int count, index, depth, best = -1;
    int best_distance = 3 * 255 * 255 + 1;
    unsigned int wanted = colours[colour & 15U];

    if (!fb_hAmigaConsoleAcquire(&query))
        return -1;
    if (query.window->WScreen->RastPort.BitMap == NULL ||
        query.window->WScreen->ViewPort.ColorMap == NULL)
        goto cleanup;
    depth = query.window->WScreen->RastPort.BitMap->Depth;
    if (depth < 1 || depth > 32)
        goto cleanup;
    /* SGR30..37/40..47 address the console's first eight pens. */
    count = (depth < 3) ? (1 << depth) : 8;
    graphics = OpenLibrary((CONST_STRPTR)"graphics.library", 39);
    if (graphics == NULL)
        goto cleanup;
    GfxBase = (struct GfxBase *)graphics;
    GetRGB32(query.window->WScreen->ViewPort.ColorMap, 0, (ULONG)count, palette);
    for (index = 0; index < count; ++index) {
        int red = (int)((wanted >> 16) & 255U) - (int)(palette[index * 3] >> 24);
        int green = (int)((wanted >> 8) & 255U) - (int)(palette[index * 3 + 1] >> 24);
        int blue = (int)(wanted & 255U) - (int)(palette[index * 3 + 2] >> 24);
        int distance = red * red + green * green + blue * blue;
        if (distance < best_distance) {
            best = index;
            best_distance = distance;
        }
    }

cleanup:
    if (graphics != NULL)
        CloseLibrary(graphics);
    fb_hAmigaConsoleRelease(&query);
    return best;
}

static int fb_hAmigaConsoleWrite(BPTR output, const char *text, int length)
{
    int offset = 0;
    while (offset < length) {
        LONG written = Write(output, (APTR)(text + offset), length - offset);
        if (written <= 0 || written > length - offset)
            return FALSE;
        offset += written;
    }
    return TRUE;
}

static int fb_hAmigaConsoleClearLine(void)
{
    static const char spaces[] =
        "                                                                ";
    FB_AMIGA_CONSOLE_QUERY query;
    char cursor[32];
    int x, y, remaining, wrapping, result = TRUE;

    if (!fb_hAmigaConsoleAcquire(&query))
        return FALSE;
    if (query.unit == NULL || query.unit->cu_XCP < 0 || query.unit->cu_YCP < 0 ||
        query.unit->cu_XCP > query.unit->cu_XMax ||
        query.unit->cu_YCP > query.unit->cu_YMax) {
        fb_hAmigaConsoleRelease(&query);
        return FALSE;
    }
    x = query.unit->cu_XCP;
    y = query.unit->cu_YCP;
    remaining = (int)query.unit->cu_XMax - x + 1;
    wrapping = query.unit->cu_Modes[PMB_AWM >> 3] & (1U << (PMB_AWM & 7));
    /* V36 erase commands use a separate window background. Paint blank
       character cells instead: this keeps BASIC's current background and
       leaves cells outside VIEW PRINT and the borrowed window theme intact.
       Disable wrapping so clearing the final column cannot scroll a row. */
    if (wrapping && !fb_hAmigaConsoleWrite(query.output, FB_AmigaOS_CSI "?7l", 4))
        result = FALSE;
    while (result && remaining > 0) {
        int chunk = remaining < (int)sizeof(spaces) - 1 ?
            remaining : (int)sizeof(spaces) - 1;
        result = fb_hAmigaConsoleWrite(query.output, spaces, chunk);
        remaining -= chunk;
    }
    /* Restore cursor and mode even after a short/failed output operation. */
    snprintf(cursor, sizeof(cursor), FB_AmigaOS_CSI "%d;%dH", y + 1, x + 1);
    if (!fb_hAmigaConsoleWrite(query.output, cursor, (int)strlen(cursor)))
        result = FALSE;
    if (wrapping && !fb_hAmigaConsoleWrite(query.output, FB_AmigaOS_CSI "?7h", 4))
        result = FALSE;
    fb_hAmigaConsoleRelease(&query);
    return result;
}



void fb_hRecheckCursorPos(void)
{
    FB_AMIGA_CONSOLE_QUERY query;
    if (!fb_hAmigaConsoleAcquire(&query))
        return;
    if (query.unit != NULL && query.unit->cu_XCP >= 0 && query.unit->cu_YCP >= 0 &&
        query.unit->cu_XCP <= query.unit->cu_XMax &&
        query.unit->cu_YCP <= query.unit->cu_YMax) {
        __fb_con.cur_x = (int)query.unit->cu_XCP + 1;
        __fb_con.cur_y = (int)query.unit->cu_YCP + 1;
    }
    fb_hAmigaConsoleRelease(&query);
}

void fb_hRecheckConsoleSize(int requery_cursorpos)
{
    unsigned char *char_buffer;
    unsigned char *attr_buffer;
    int columns;
    int rows;
    int copy_columns;
    int copy_rows;
    int row;

    columns = (__fb_con.w > 0) ? __fb_con.w : FB_AmigaOS_CONSOLE_COLUMNS;
    rows = (__fb_con.h > 0) ? __fb_con.h : FB_AmigaOS_CONSOLE_ROWS;
    fb_hAmigaConsoleSize(&columns, &rows);
    if (columns != __fb_con.w || rows != __fb_con.h)
        __fb_console_resized = TRUE;
    if (__fb_console_resized == FALSE)
        return;
    __fb_console_resized = FALSE;
    char_buffer = calloc((size_t)columns * (size_t)rows, 2);
    if (char_buffer == NULL)
        return;
    attr_buffer = char_buffer + ((size_t)columns * (size_t)rows);

    if (__fb_con.char_buffer != NULL) {
        copy_columns = (__fb_con.w < columns) ? __fb_con.w : columns;
        copy_rows = (__fb_con.h < rows) ? __fb_con.h : rows;
        for (row = 0; row < copy_rows; ++row) {
            memcpy(char_buffer + (row * columns),
                __fb_con.char_buffer + (row * __fb_con.w),
                (size_t)copy_columns);
            memcpy(attr_buffer + (row * columns),
                __fb_con.attr_buffer + (row * __fb_con.w),
                (size_t)copy_columns);
        }
        free(__fb_con.char_buffer);
    }

    __fb_con.char_buffer = char_buffer;
    __fb_con.attr_buffer = attr_buffer;
    __fb_con.w = columns;
    __fb_con.h = rows;

    /* A resized window may leave cached coordinates outside the new cell
       buffer. Keep subsequent PRINT copies inside the successful allocation. */
    if (__fb_con.cur_x < 1) __fb_con.cur_x = 1;
    if (__fb_con.cur_y < 1) __fb_con.cur_y = 1;
    if (__fb_con.cur_x > columns) __fb_con.cur_x = columns;
    if (__fb_con.cur_y > rows) __fb_con.cur_y = rows;

    if (requery_cursorpos != FALSE)
        fb_hRecheckCursorPos();
    fb_DevScrnMaybeUpdateWidth();
}

int fb_hTermOut(int code, int param1, int param2)
{
    char sequence[64];
    int length;
    int pen;
    BPTR output;

    if (__fb_con.inited == FALSE)
        return FALSE;

    switch (code) {
    case SEQ_LOCATE:
        length = snprintf(sequence, sizeof(sequence),
            FB_AmigaOS_CSI "%d;%dH", param2 + 1, param1 + 1);
        break;
    case SEQ_HOME:
        length = snprintf(sequence, sizeof(sequence), FB_AmigaOS_CSI "H");
        break;
    case SEQ_SCROLL_REGION:
        return TRUE;
    case SEQ_CLS:
        length = snprintf(sequence, sizeof(sequence), "\f");
        break;
    case SEQ_CLEOL:
        if (fb_hAmigaConsoleClearLine())
            return TRUE;
        length = snprintf(sequence, sizeof(sequence), FB_AmigaOS_CSI "K");
        break;
    case SEQ_WINDOW_SIZE:
        length = snprintf(sequence, sizeof(sequence),
            FB_AmigaOS_CSI "%dt" FB_AmigaOS_CSI "%du",
            param1, param2);
        break;
    case SEQ_BEEP:
        length = snprintf(sequence, sizeof(sequence), "\a");
        break;
    case SEQ_FG_COLOR:
        pen = fb_hAmigaConsolePen(__fb_con.fg_color);
        length = snprintf(sequence, sizeof(sequence),
            FB_AmigaOS_CSI "%dm", 30 + ((pen >= 0) ? pen : param2));
        break;
    case SEQ_BG_COLOR:
        pen = fb_hAmigaConsolePen(__fb_con.bg_color);
        length = snprintf(sequence, sizeof(sequence),
            FB_AmigaOS_CSI "%dm", 40 + ((pen >= 0) ? pen : param2));
        break;
    case SEQ_RESET_COLOR:
        length = snprintf(sequence, sizeof(sequence), FB_AmigaOS_CSI "0m");
        break;
    case SEQ_BRIGHT_COLOR:
        length = snprintf(sequence, sizeof(sequence), FB_AmigaOS_CSI "1m");
        break;
    case SEQ_SCROLL:
        length = snprintf(sequence, sizeof(sequence),
            FB_AmigaOS_CSI "%dS", param2);
        break;
    case SEQ_SHOW_CURSOR:
        length = snprintf(sequence, sizeof(sequence), FB_AmigaOS_CSI "1 p");
        break;
    case SEQ_HIDE_CURSOR:
        length = snprintf(sequence, sizeof(sequence), FB_AmigaOS_CSI "0 p");
        break;
    case SEQ_DEL_CHAR:
        length = snprintf(sequence, sizeof(sequence), FB_AmigaOS_CSI "P");
        break;
    case SEQ_INIT_KEYPAD:
    case SEQ_EXIT_KEYPAD:
        return TRUE;
    case SEQ_SET_COLOR_EX:
        length = snprintf(sequence, sizeof(sequence),
            FB_AmigaOS_CSI "%dm", param1);
        break;
    default:
        return FALSE;
    }

    if (length < 0 || (size_t)length >= sizeof(sequence))
        return FALSE;

    output = Output();
    if (output == BNULL)
        return FALSE;

    return (Write(output, (APTR)sequence, length) == length) ? TRUE : FALSE;
}

int fb_hInitConsole(void)
{
    BPTR input;
    BPTR output;

    if (__fb_con.inited == FALSE)
        return -1;

    input = Input();
    output = Output();
    /* Match the shared Unix console policy: raw keyboard/terminal handling
       needs an interactive input/output pair. Redirected input must remain
       on stdio, since LINE INPUT may already have buffered the next key.
       WaitForChar() neither observes that buffer nor provides a regular-file
       readiness test. The caller disables console state on this return. */
    if (input == BNULL || output == BNULL ||
        IsInteractive(input) == DOSFALSE || IsInteractive(output) == DOSFALSE)
        return -1;

    if (console_raw_mode == FALSE)
        console_raw_mode = (SetMode(input, 1) == DOSTRUE);

    BG_LOCK();
    if (__fb_con.keyboard_init != NULL)
        __fb_con.keyboard_init();
    if (__fb_con.mouse_init != NULL)
        __fb_con.mouse_init();
    BG_UNLOCK();

    /* Native CON: starts black on grey; BASIC's logical defaults are grey
       on black. Select matching pens without changing the screen palette. */
    {
        static const unsigned char ansi[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };
        fb_hTermOut(SEQ_RESET_COLOR, 0, 0);
        if (__fb_con.fg_color & 8U)
            fb_hTermOut(SEQ_BRIGHT_COLOR, 0, 0);
        fb_hTermOut(SEQ_FG_COLOR, 0, ansi[__fb_con.fg_color & 7U]);
        fb_hTermOut(SEQ_BG_COLOR, 0, ansi[__fb_con.bg_color & 7U]);
    }
    return 0;
}

void fb_hExitConsole(void)
{
    BPTR input;
    BPTR output;

    if (__fb_con.inited == FALSE)
        return;

    if (__fb_con.gfx_exit != NULL)
        __fb_con.gfx_exit();

    BG_LOCK();
    if (__fb_con.keyboard_exit != NULL)
        __fb_con.keyboard_exit();
    if (__fb_con.mouse_exit != NULL)
        __fb_con.mouse_exit();
    BG_UNLOCK();

    /*
        Terminal reset sequences only belong on an interactive output handle;
        writing them to a redirected compiler log corrupts its contents.
    */
    output = Output();
    if (output != BNULL && IsInteractive(output) != DOSFALSE) {
        fb_hTermOut(SEQ_RESET_COLOR, 0, 0);
        fb_hTermOut(SEQ_SHOW_CURSOR, 0, 0);
    }

    input = Input();
    if (input != BNULL && console_raw_mode != FALSE)
        SetMode(input, 0);
    console_raw_mode = FALSE;
}

/* ------------------------------------------------------------------------- */
/* Runtime lifecycle                                                         */
/* ------------------------------------------------------------------------- */

void fb_hInit(void)
{
    pthread_mutexattr_t attributes;

    pthread_mutexattr_init(&attributes);
    pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);

#ifdef ENABLE_MT
    pthread_mutex_init(&__fb_global_mutex, &attributes);
    pthread_mutex_init(&__fb_string_mutex, &attributes);
    pthread_mutex_init(&__fb_graphics_mutex, &attributes);
    pthread_mutex_init(&__fb_math_mutex, &attributes);
    pthread_mutex_init(&__fb_profile_mutex, &attributes);
#endif
    pthread_mutex_init(&__fb_bg_mutex, &attributes);
    pthread_mutexattr_destroy(&attributes);

    memset(&__fb_con, 0, sizeof(__fb_con));
    __fb_con.inited = INIT_CONSOLE;
    __fb_con.term_type = TERM_GENERIC;
    __fb_con.keyboard_getch = default_getch;
    __fb_con.fg_color = 7;
    __fb_con.bg_color = 0;
    __fb_con.cur_x = 1;
    __fb_con.cur_y = 1;
    __fb_console_resized = TRUE;

    if (fb_hInitConsole() != 0)
        __fb_con.inited = FALSE;
}

void fb_hAmigaStopBgThread(void)
{
    BG_LOCK();
    __fb_con.inited = FALSE;
    BG_UNLOCK();

    if (bgthread_inited != FALSE) {
        pthread_join(__fb_bg_thread, NULL);
        bgthread_inited = FALSE;
    }
}

void fb_hEnd(int unused)
{
    (void)unused;
    fb_hExitConsole();
    fb_hAmigaStopBgThread();
    pthread_mutex_destroy(&__fb_bg_mutex);

    if (__fb_con.char_buffer != NULL) {
        free(__fb_con.char_buffer);
        __fb_con.char_buffer = NULL;
        __fb_con.attr_buffer = NULL;
    }

#ifdef ENABLE_MT
    pthread_mutex_destroy(&__fb_global_mutex);
    pthread_mutex_destroy(&__fb_string_mutex);
    pthread_mutex_destroy(&__fb_graphics_mutex);
    pthread_mutex_destroy(&__fb_math_mutex);
    pthread_mutex_destroy(&__fb_profile_mutex);
#endif
}

/* end of drivers/amiga/hinit.c */
