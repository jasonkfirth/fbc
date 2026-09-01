/*
    FreeBASIC runtime library
    -------------------------

    File: aros/hinit.c

    Purpose:

        Initialize the FreeBASIC console and runtime locks on AROS.

    Responsibilities:

        - use dos.library for console input and raw-mode control
        - translate FreeBASIC text-console operations to AROS CSI commands
        - own runtime and background-thread locks
        - maintain the console geometry used by text-mode operations

    This file intentionally does NOT contain:

        - POSIX terminal discovery or TERM environment handling
        - Intuition graphics-window lifecycle
        - AROS input events for gfxlib2

    AROS console policy:

        AROS console handles are not Unix tty file descriptors.  In
        particular, the POSIXC environment and termios path may block while
        the runtime is still inside its INIT symbol set.  Use dos.library's
        Input(), WaitForChar(), Read(), and SetMode() interface directly.
*/

#include "../fb.h"
#include "../fb_private_thread.h"
#include "../unix/fb_private_console.h"

#include <dos/dos.h>
#include <proto/dos.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FB_AROS_CONSOLE_COLUMNS 80
#define FB_AROS_CONSOLE_ROWS 25
#define FB_AROS_CSI "\233"

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

    while (__fb_con.inited) {
        BG_LOCK();
        if (__fb_con.keyboard_handler != NULL)
            __fb_con.keyboard_handler();
        if (__fb_con.mouse_handler != NULL)
            __fb_con.mouse_handler();
        BG_UNLOCK();
        usleep(30000);
    }

    return NULL;
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
/* Native AROS console                                                       */
/* ------------------------------------------------------------------------- */

void fb_hRecheckCursorPos(void)
{
    /* AROS has no synchronous cursor-position query required by rtlib. */
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

    if (__fb_console_resized == FALSE)
        return;
    __fb_console_resized = FALSE;

    columns = (__fb_con.w > 0) ? __fb_con.w : FB_AROS_CONSOLE_COLUMNS;
    rows = (__fb_con.h > 0) ? __fb_con.h : FB_AROS_CONSOLE_ROWS;
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

    if (requery_cursorpos != FALSE)
        fb_hRecheckCursorPos();
    fb_DevScrnMaybeUpdateWidth();
}

int fb_hTermOut(int code, int param1, int param2)
{
    char sequence[64];
    int length;
    BPTR output;

    if (__fb_con.inited == FALSE)
        return FALSE;

    switch (code) {
    case SEQ_LOCATE:
        length = snprintf(sequence, sizeof(sequence),
            FB_AROS_CSI "%d;%dH", param2 + 1, param1 + 1);
        break;
    case SEQ_HOME:
        length = snprintf(sequence, sizeof(sequence), FB_AROS_CSI "H");
        break;
    case SEQ_SCROLL_REGION:
        return TRUE;
    case SEQ_CLS:
        length = snprintf(sequence, sizeof(sequence), "\f");
        break;
    case SEQ_CLEOL:
        length = snprintf(sequence, sizeof(sequence), FB_AROS_CSI "K");
        break;
    case SEQ_WINDOW_SIZE:
        length = snprintf(sequence, sizeof(sequence),
            FB_AROS_CSI "%dt" FB_AROS_CSI "%du",
            param1, param2);
        break;
    case SEQ_BEEP:
        length = snprintf(sequence, sizeof(sequence), "\a");
        break;
    case SEQ_FG_COLOR:
        length = snprintf(sequence, sizeof(sequence),
            FB_AROS_CSI "%dm", 30 + param2);
        break;
    case SEQ_BG_COLOR:
        length = snprintf(sequence, sizeof(sequence),
            FB_AROS_CSI "%dm", 40 + param2);
        break;
    case SEQ_RESET_COLOR:
        length = snprintf(sequence, sizeof(sequence), FB_AROS_CSI "0m");
        break;
    case SEQ_BRIGHT_COLOR:
        length = snprintf(sequence, sizeof(sequence), FB_AROS_CSI "1m");
        break;
    case SEQ_SCROLL:
        length = snprintf(sequence, sizeof(sequence),
            FB_AROS_CSI "%dS", param2);
        break;
    case SEQ_SHOW_CURSOR:
        length = snprintf(sequence, sizeof(sequence), FB_AROS_CSI "1 p");
        break;
    case SEQ_HIDE_CURSOR:
        length = snprintf(sequence, sizeof(sequence), FB_AROS_CSI "0 p");
        break;
    case SEQ_DEL_CHAR:
        length = snprintf(sequence, sizeof(sequence), FB_AROS_CSI "P");
        break;
    case SEQ_INIT_KEYPAD:
    case SEQ_EXIT_KEYPAD:
        return TRUE;
    case SEQ_SET_COLOR_EX:
        length = snprintf(sequence, sizeof(sequence),
            FB_AROS_CSI "%dm", param1);
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

    if (__fb_con.inited == FALSE)
        return -1;

    input = Input();
    if (input != BNULL && console_raw_mode == FALSE)
        console_raw_mode = (SetMode(input, 1) == DOSTRUE);

    BG_LOCK();
    if (__fb_con.keyboard_init != NULL)
        __fb_con.keyboard_init();
    if (__fb_con.mouse_init != NULL)
        __fb_con.mouse_init();
    BG_UNLOCK();

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

void fb_hEnd(int unused)
{
    (void)unused;

    fb_hExitConsole();
    __fb_con.inited = FALSE;

    if (bgthread_inited != FALSE) {
        pthread_join(__fb_bg_thread, NULL);
        bgthread_inited = FALSE;
    }
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

/* end of aros/hinit.c */
