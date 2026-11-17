/*
    Project: FreeBASIC runtime library
    -------------------------

    File: drivers/aros/io_printbuff.c

    Purpose:

        Write narrow console output through the native AROS DOS handle.

    Responsibilities:

        - preserve the runtime's console shadow buffer
        - translate control characters which cannot be emitted directly
        - honour shell output redirection through dos.library

    This file intentionally does NOT contain: - console initialization or raw input mode handling
        - wide-character conversion
        - cursor and colour command construction

    AROS output policy:

        Output() returns an AmigaDOS file handle. Passing console output
        through POSIX stdio adds a second handle translation layer. On ARM,
        that layer can lose the FAT handler message port during repeated
        character writes and flushes. Write() accepts the native handle and
        works for both console output and redirected compiler logs.
*/

#include "../../fb.h"
#include "../unix/fb_private_console.h"

#include <dos/dos.h>
#include <proto/dos.h>

#include <limits.h>
#include <string.h>

#define CTRL_ALWAYS 0x0800D101u
#define ENTER_UTF8 "\033%G"
#define EXIT_UTF8 "\033%@"
#define FB_AROS_OUTPUT_BUFFER_SIZE 512

/* ------------------------------------------------------------------------- */
/* Native output                                                             */
/* ------------------------------------------------------------------------- */

static void fb_hArosWrite(const void *buffer, size_t length)
{
    const unsigned char *cursor;
    BPTR output;

    if (buffer == NULL || length == 0)
        return;

    output = Output();
    if (output == BNULL)
        return;

    cursor = buffer;
    while (length > 0) {
        LONG requested;
        LONG written;

        requested = (length > (size_t)INT32_MAX) ? (LONG)INT32_MAX :
            (LONG)length;
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif
        /* Write's legacy AROS API uses APTR although it only reads this data. */
        written = Write(output, (APTR)cursor, requested);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
        if (written <= 0)
            return;

        cursor += written;
        length -= (size_t)written;
    }
}

static void fb_hArosBufferByte(unsigned char byte,
    unsigned char *output_buffer, size_t *output_length)
{
    if (*output_length == FB_AROS_OUTPUT_BUFFER_SIZE) {
        fb_hArosWrite(output_buffer, *output_length);
        *output_length = 0;
    }

    output_buffer[*output_length] = byte;
    ++*output_length;
}

/* ------------------------------------------------------------------------- */
/* FreeBASIC console interface                                               */
/* ------------------------------------------------------------------------- */

void fb_ConsolePrintBufferEx(const void *buffer, size_t length, int mask)
{
    const unsigned char *input;
    unsigned char output_buffer[FB_AROS_OUTPUT_BUFFER_SIZE];
    size_t width;
    size_t height;
    size_t total_cells;
    size_t current_cell;
    size_t available;
    size_t copy_length;
    size_t output_length;
    size_t remaining;

    (void)mask;

    if (buffer == NULL || length == 0)
        return;

    if (fb_console_context.inited == FALSE) {
        fb_hArosWrite(buffer, length);
        return;
    }

    BG_LOCK();
    fb_hRecheckConsoleSize(TRUE);
    BG_UNLOCK();

    if (fb_console_context.w <= 0 || fb_console_context.h <= 0 ||
        fb_console_context.cur_x < 1 || fb_console_context.cur_y < 1 ||
        fb_console_context.cur_x > fb_console_context.w ||
        fb_console_context.cur_y > fb_console_context.h ||
        fb_console_context.char_buffer == NULL ||
        fb_console_context.attr_buffer == NULL)
        return;

    width = (size_t)fb_console_context.w;
    height = (size_t)fb_console_context.h;
    if (width > SIZE_MAX / height)
        return;
    total_cells = width * height;
    current_cell = ((size_t)(fb_console_context.cur_y - 1) * width) +
        (size_t)(fb_console_context.cur_x - 1);
    if (current_cell > total_cells)
        return;

    available = total_cells - current_cell;
    copy_length = (available < length) ? available : length;
    memcpy(fb_console_context.char_buffer +
        current_cell, buffer, copy_length);
    memset(fb_console_context.attr_buffer +
        current_cell,
        (int)(unsigned char)(((unsigned int)fb_console_context.fg_color & 0x0fU) |
        (((unsigned int)fb_console_context.bg_color & 0x0fU) << 4)),
        copy_length);

    input = buffer;
    output_length = 0;
    remaining = length;
    while (remaining > 0) {
        unsigned int character = *input;

        if (character == 0)
            character = ' ';

        if (character < 32 && ((CTRL_ALWAYS >> character) & 1u) != 0) {
            const unsigned char translated[] = {
                (unsigned char)ENTER_UTF8[0],
                (unsigned char)ENTER_UTF8[1],
                (unsigned char)ENTER_UTF8[2],
                0xef, 0x80, (unsigned char)(character | 0x80),
                (unsigned char)EXIT_UTF8[0],
                (unsigned char)EXIT_UTF8[1],
                (unsigned char)EXIT_UTF8[2]
            };
            size_t index;

            for (index = 0; index < sizeof(translated); ++index)
                fb_hArosBufferByte(translated[index], output_buffer,
                    &output_length);
        } else {
            fb_hArosBufferByte((unsigned char)character, output_buffer,
                &output_length);
        }

        ++fb_console_context.cur_x;
        if (character == '\n' || fb_console_context.cur_x >= fb_console_context.w) {
            fb_console_context.cur_x = 1;
            ++fb_console_context.cur_y;
            if (fb_console_context.cur_y > fb_console_context.h)
                fb_console_context.cur_y = fb_console_context.h;
        }

        ++input;
        --remaining;
    }

    fb_hArosWrite(output_buffer, output_length);
}

void fb_ConsolePrintBuffer(const char *buffer, int mask)
{
    if (buffer != NULL)
        fb_ConsolePrintBufferEx(buffer, strlen(buffer), mask);
}

/* end of drivers/aros/io_printbuff.c */
