/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_console.c

    Purpose:

        Provide one section of the small NuttX runtime used by the
        generated-C RISC-V smoke target.

    This file is included by fb_nuttx_minrt.c while the target is still
    being brought up. Keeping the command bodies in smaller files makes
    the code easier to compare with the normal rtlib layout.
*/

int32 fb_Width(const int32 columns, const int32 rows)
{
    /*
        NuttX's NSH console does not expose a portable way for an application
        to resize the host terminal. Accept the request so old BASIC programs
        can keep running, but leave the terminal size unchanged.
    */
    (void)columns;
    (void)rows;

    return 0;
}

int32 fb_ConsoleView(const int32 top_row, const int32 bottom_row)
    __attribute__((weak));
int32 fb_ConsoleView(const int32 top_row, const int32 bottom_row)
{
    /*
        VIEW changes the active text viewport on console targets with a real
        screen buffer.  The NuttX NSH console is a serial-style stream, so
        there is no portable viewport to adjust.  Accept the request as a
        harmless no-op.
    */
    (void)top_row;
    (void)bottom_row;

    return 0;
}

int32 fb_GfxScreenList(const int32 depth) __attribute__((weak));
int32 fb_GfxScreenList(const int32 depth)
{
    /*
        The non-gfx NuttX smoke runtime can still compile examples that query
        SCREENLIST.  With no graphics backend linked, return an empty list.
        A real gfxlib2 implementation can override this weak fallback.
    */
    (void)depth;

    return 0;
}

void fb_GfxLine(void *target, const float x1, const float y1, const float x2,
    const float y2, const uint32 color, const int32 style,
    const uint32 pattern, const int32 flags) __attribute__((weak));
void fb_GfxLine(void *target, const float x1, const float y1, const float x2,
    const float y2, const uint32 color, const int32 style,
    const uint32 pattern, const int32 flags)
{
    /*
        Headless smoke runs can compile graphics examples that do not
        explicitly open a screen.  Treat drawing as a no-op unless a real
        gfxlib2 backend is linked.
    */
    (void)target;
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
    (void)color;
    (void)style;
    (void)pattern;
    (void)flags;
}

#if defined(FB_NUTTX_WITH_GFXLIB)
extern int fb_nuttx_gfx_active(void);
extern int fb_nuttx_gfx_print_buffer(const void *buffer, size_t len, int mask);
extern FBSTRING *fb_nuttx_gfx_inkey(void);
extern int fb_nuttx_gfx_cls(int mode);
extern int fb_nuttx_gfx_locate(int row, int col, int cursor);
extern int fb_nuttx_gfx_get_x(void);
extern int fb_nuttx_gfx_get_y(void);
extern int fb_nuttx_gfx_get_size(int *cols, int *rows);
#else
#define FB_NUTTX_GFX_FALLBACK __attribute__((visibility("default"), used, weak))

int fb_nuttx_gfx_active(void) FB_NUTTX_GFX_FALLBACK;
int fb_nuttx_gfx_active(void)
{
    /*
        NuttX loadable modules should not depend on unresolved symbols.
        Built-in firmware images can still link a real gfx bridge later; these
        weak fallbacks keep non-gfx programs working until that bridge exists.
    */
    return 0;
}

int fb_nuttx_gfx_print_buffer(const void *buffer, size_t len, int mask)
    FB_NUTTX_GFX_FALLBACK;
int fb_nuttx_gfx_print_buffer(const void *buffer, size_t len, int mask)
{
    (void)buffer;
    (void)len;
    (void)mask;

    return 0;
}

FBSTRING *fb_nuttx_gfx_inkey(void) FB_NUTTX_GFX_FALLBACK;
FBSTRING *fb_nuttx_gfx_inkey(void)
{
    return fb_nuttx_temp_string("", 0);
}

int fb_nuttx_gfx_cls(int mode) FB_NUTTX_GFX_FALLBACK;
int fb_nuttx_gfx_cls(int mode)
{
    (void)mode;

    return 0;
}

int fb_nuttx_gfx_locate(int row, int col, int cursor)
    FB_NUTTX_GFX_FALLBACK;
int fb_nuttx_gfx_locate(int row, int col, int cursor)
{
    (void)row;
    (void)col;
    (void)cursor;

    return 0;
}

int fb_nuttx_gfx_get_x(void) FB_NUTTX_GFX_FALLBACK;
int fb_nuttx_gfx_get_x(void)
{
    return 0;
}

int fb_nuttx_gfx_get_y(void) FB_NUTTX_GFX_FALLBACK;
int fb_nuttx_gfx_get_y(void)
{
    return 0;
}

int fb_nuttx_gfx_get_size(int *cols, int *rows) FB_NUTTX_GFX_FALLBACK;
int fb_nuttx_gfx_get_size(int *cols, int *rows)
{
    (void)cols;
    (void)rows;

    return 0;
}

#undef FB_NUTTX_GFX_FALLBACK
#endif

#ifndef FB_PRINT_NEWLINE
#define FB_PRINT_NEWLINE 0x00000001
#endif

#ifndef FB_PRINT_PAD
#define FB_PRINT_PAD 0x00000002
#endif

#ifndef FB_PRINT_BIN_NEWLINE
#define FB_PRINT_BIN_NEWLINE 0x00000004
#endif

#ifndef FB_PRINT_HLMASK
#define FB_PRINT_HLMASK 0x00000003
#endif

#ifndef FB_BINARY_NEWLINE
#define FB_BINARY_NEWLINE "\r\n"
#endif

static void fb_nuttx_console_write_stdout(const void *buffer, size_t len)
{
    const unsigned char *src;

    if ((buffer == NULL) || (len == 0))
        return;

    src = (const unsigned char *)buffer;

    while (len > 0) {
        size_t chunk;
        ssize_t written;

        /*
            The NSH console is a device stream, not a hosted terminal.
            Keep writes small and retry EINTR so a BASIC PRINT does not
            depend on stdio buffering behavior inside the NuttX console.
        */
        chunk = 1;

        do {
            written = write(STDOUT_FILENO, src, chunk);
        } while ((written < 0) && (errno == EINTR));

        if (written <= 0)
            break;

        src += written;
        len -= (size_t)written;

        usleep(1000);
    }
}

static int fb_nuttx_have_gfx_hooks(void)
{
    return fb_nuttx_gfx_active();
}

int32 fb_Locate(const int32 row, const int32 column, const int32 cursor,
    const int32 start, const int32 stop)
{
    (void)start;
    (void)stop;

    if (fb_nuttx_have_gfx_hooks())
        return fb_nuttx_gfx_locate((int)row, (int)column, (int)cursor);

    if (row > 0)
        fb_nuttx_cursor_row = row;

    if (column > 0)
        fb_nuttx_cursor_column = column;

    if ((row > 0) && (column > 0))
        printf("\033[%" PRId32 ";%" PRId32 "H", row, column);

    fflush(stdout);

    return 0;
}

void fb_Cls(const int32 mode)
{
    if (fb_nuttx_have_gfx_hooks()) {
        if (fb_nuttx_gfx_cls((int)mode))
            return;
    }

    (void)mode;

    fputs("\033[2J\033[H", stdout);
    fflush(stdout);

    fb_nuttx_cursor_row = 1;
    fb_nuttx_cursor_column = 1;
}

int32 fb_Pos(const int32 file_num)
{
    if (file_num != 0)
        return 0;

    if (fb_nuttx_have_gfx_hooks())
        return fb_nuttx_gfx_get_x();

    return fb_nuttx_cursor_column;
}

int32 fb_GetX(void)
{
    if (fb_nuttx_have_gfx_hooks())
        return fb_nuttx_gfx_get_x();

    return fb_nuttx_cursor_column;
}

int32 fb_GetY(void)
{
    if (fb_nuttx_have_gfx_hooks())
        return fb_nuttx_gfx_get_y();

    return fb_nuttx_cursor_row;
}

void fb_GetXY(int *column, int *row)
{
    if (column != NULL)
        *column = fb_GetX();

    if (row != NULL)
        *row = fb_GetY();
}

void fb_GetSize(int *columns, int *rows)
{
    if (fb_nuttx_have_gfx_hooks()) {
        if (fb_nuttx_gfx_get_size(columns, rows))
            return;
    }

    if (columns != NULL)
        *columns = 80;

    if (rows != NULL)
        *rows = 25;
}

uint32 fb_Color(const int32 foreground, const int32 background,
    const uint32 flags)
{
    static const int ansi_colors[8] = {
        30, 34, 32, 36, 31, 35, 33, 37
    };
    int fg;
    int bg;

    (void)flags;

    fg = foreground & 7;
    bg = background & 7;

    printf("\033[%d;%dm", ansi_colors[fg], ansi_colors[bg] + 10);
    fflush(stdout);

    return ((uint32)(background & 15) << 4) | (uint32)(foreground & 15);
}

FBSTRING *fb_Inkey(void)
{
    if (fb_nuttx_have_gfx_hooks())
        return fb_nuttx_gfx_inkey();

    /*
        Nonblocking keyboard reads need target-specific terminal state. The
        seed runtime keeps INKEY safe and predictable by reporting no pending
        key until the console layer grows that support.
    */
    return fb_nuttx_temp_string("", 0);
}

/* ------------------------------------------------------------------------- */
/* Console PRINT support                                                     */
/* ------------------------------------------------------------------------- */

void fb_PrintSPC(const int32 file_num, const int32 count);

void fb_PrintString(const int32 file_num, const FBSTRING *s, const int32 add_newline)
{
    FILE *stream;

    if (file_num == 0) {
        if (fb_nuttx_have_gfx_hooks()) {
            if ((s != NULL) && (s->data != NULL) && (s->len > 0))
                (void)fb_nuttx_gfx_print_buffer(s->data, (size_t)s->len, 0);

            if (add_newline != 0)
                (void)fb_nuttx_gfx_print_buffer("\n", 1, 0x00000001);

            return;
        }

        if ((s != NULL) && (s->data != NULL) && (s->len > 0))
            fb_nuttx_console_write_stdout(s->data, (size_t)s->len);

        if (add_newline != 0)
            fb_nuttx_console_write_stdout("\n", 1);

        return;
    }

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return;

    if ((s != NULL) && (s->data != NULL) && (s->len > 0))
        fwrite(s->data, 1, (size_t)s->len, stream);

    if (add_newline != 0)
        fputc('\n', stream);

    fflush(stream);
}

void fb_PrintVoid(const int32 file_num, const int32 mask)
{
    FILE *stream;

    if ((file_num == 0) && fb_nuttx_have_gfx_hooks()) {
        if ((mask & FB_PRINT_BIN_NEWLINE) != 0)
            (void)fb_nuttx_gfx_print_buffer(FB_BINARY_NEWLINE,
                sizeof(FB_BINARY_NEWLINE) - 1, mask);
        else if ((mask & FB_PRINT_NEWLINE) != 0)
            (void)fb_nuttx_gfx_print_buffer("\n", 1, mask);
        else if ((mask & FB_PRINT_PAD) != 0)
            fb_PrintSPC(file_num, mask & ~FB_PRINT_HLMASK);

        return;
    }

    if (file_num == 0) {
        if ((mask & FB_PRINT_BIN_NEWLINE) != 0)
            fb_nuttx_console_write_stdout(FB_BINARY_NEWLINE,
                sizeof(FB_BINARY_NEWLINE) - 1);
        else if ((mask & FB_PRINT_NEWLINE) != 0)
            fb_nuttx_console_write_stdout("\n", 1);
        else if ((mask & FB_PRINT_PAD) != 0)
            fb_PrintSPC(file_num, mask & ~FB_PRINT_HLMASK);

        return;
    }

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return;

    if ((mask & FB_PRINT_BIN_NEWLINE) != 0)
        fputs(FB_BINARY_NEWLINE, stream);
    else if ((mask & FB_PRINT_NEWLINE) != 0)
        fputc('\n', stream);
    else if ((mask & FB_PRINT_PAD) != 0)
        fb_PrintSPC(file_num, mask & ~FB_PRINT_HLMASK);

    fflush(stream);
}

void fb_PrintWstr(const int32 file_num, const uint32_t *s,
    const int32 add_newline)
{
    FILE *stream;
    char ch;
    int32 i;

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return;

    if (s != NULL) {
        i = 0;

        while (s[i] != 0) {
            ch = (char)(s[i] & 0xffu);
            fwrite(&ch, 1, 1, stream);
            i++;
        }
    }

    if (add_newline != 0)
        fputc('\n', stream);

    fflush(stream);
}

void fb_PrintInt(const int32 file_num, const int32 value, const int32 add_newline)
{
    FILE *stream;
    char buffer[32];
    int len;

    if (file_num == 0) {
        len = snprintf(buffer, sizeof(buffer), "%" PRId32, value);

        if (len > 0)
            fb_nuttx_console_write_stdout(buffer, (size_t)len);

        if (add_newline != 0)
            fb_nuttx_console_write_stdout("\n", 1);

        return;
    }

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return;

    fprintf(stream, "%" PRId32, value);

    if (add_newline != 0)
        fputc('\n', stream);

    fflush(stream);
}

void fb_PrintByte(const int32 file_num, const int8_t value,
    const int32 add_newline)
{
    fb_PrintInt(file_num, (int32)value, add_newline);
}

void fb_PrintUByte(const int32 file_num, const uint8_t value,
    const int32 add_newline)
{
    fb_PrintInt(file_num, (int32)value, add_newline);
}

void fb_PrintShort(const int32 file_num, const int16_t value,
    const int32 add_newline)
{
    fb_PrintInt(file_num, (int32)value, add_newline);
}

void fb_PrintUShort(const int32 file_num, const uint16_t value,
    const int32 add_newline)
{
    fb_PrintInt(file_num, (int32)value, add_newline);
}

void fb_PrintLongint(const int32 file_num, const int64_t value,
    const int32 add_newline)
{
    FILE *stream;
    char buffer[32];
    int len;

    if (file_num == 0) {
        len = snprintf(buffer, sizeof(buffer), "%" PRId64, value);

        if (len > 0)
            fb_nuttx_console_write_stdout(buffer, (size_t)len);

        if (add_newline)
            fb_nuttx_console_write_stdout("\n", 1);

        return;
    }

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return;

    fprintf(stream, "%" PRId64, value);

    if (add_newline)
        fputc('\n', stream);

    fflush(stream);
}

void fb_PrintULongint(const int32 file_num, const uint64_t value,
    const int32 add_newline)
{
    FILE *stream;
    char buffer[32];
    int len;

    if (file_num == 0) {
        len = snprintf(buffer, sizeof(buffer), "%" PRIu64, value);

        if (len > 0)
            fb_nuttx_console_write_stdout(buffer, (size_t)len);

        if (add_newline)
            fb_nuttx_console_write_stdout("\n", 1);

        return;
    }

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return;

    fprintf(stream, "%" PRIu64, value);

    if (add_newline)
        fputc('\n', stream);

    fflush(stream);
}

void fb_PrintBool(const int32 file_num, const uint8_t value,
    const int32 add_newline)
{
    FILE *stream;
    const char *text;

    if (file_num == 0) {
        text = value ? "true" : "false";
        fb_nuttx_console_write_stdout(text, strlen(text));

        if (add_newline)
            fb_nuttx_console_write_stdout("\n", 1);

        return;
    }

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return;

    fputs(value ? "true" : "false", stream);

    if (add_newline)
        fputc('\n', stream);

    fflush(stream);
}

void fb_PrintDouble(const int32 file_num, const double value, const int32 add_newline)
{
    FILE *stream;
    char buffer[64];
    int len;

    if (file_num == 0) {
        len = snprintf(buffer, sizeof(buffer), "%g", value);

        if (len > 0)
            fb_nuttx_console_write_stdout(buffer, (size_t)len);

        if (add_newline != 0)
            fb_nuttx_console_write_stdout("\n", 1);

        return;
    }

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return;

    fprintf(stream, "%g", value);

    if (add_newline != 0)
        fputc('\n', stream);

    fflush(stream);
}

void fb_PrintSingle(const int32 file_num, const float value,
    const int32 add_newline)
{
    fb_PrintDouble(file_num, (double)value, add_newline);
}

void fb_PrintUInt(const int32 file_num, const uint32 value, const int32 add_newline)
{
    FILE *stream;
    char buffer[32];
    int len;

    if (file_num == 0) {
        len = snprintf(buffer, sizeof(buffer), "%" PRIu32, value);

        if (len > 0)
            fb_nuttx_console_write_stdout(buffer, (size_t)len);

        if (add_newline != 0)
            fb_nuttx_console_write_stdout("\n", 1);

        return;
    }

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return;

    fprintf(stream, "%" PRIu32, value);

    if (add_newline != 0)
        fputc('\n', stream);

    fflush(stream);
}

void fb_PrintSPC(const int32 file_num, const int32 count)
{
    FILE *stream;
    int32 i;

    if (file_num == 0) {
        for (i = 0; i < count; i++)
            fb_nuttx_console_write_stdout(" ", 1);

        return;
    }

    stream = fb_nuttx_stream_for_file(file_num);

    if ((stream == NULL) || (count <= 0))
        return;

    for (i = 0; i < count; i++)
        fputc(' ', stream);

    fflush(stream);
}

void fb_PrintTab(const int32 file_num, const int32 column)
{
    FILE *stream;
    char buffer[32];
    int len;

    stream = fb_nuttx_stream_for_file(file_num);

    if ((stream == NULL) || (column <= 0))
        return;

    if (file_num == 0) {
        len = snprintf(buffer, sizeof(buffer), "\033[%" PRId32 "G", column);

        if (len > 0)
            fb_nuttx_console_write_stdout(buffer, (size_t)len);

        return;
    } else {
        fb_PrintSPC(file_num, column - 1);
    }

    fflush(stream);
}

void fb_Beep(void)
{
    fb_nuttx_console_write_stdout("\a", 1);
}

/* ------------------------------------------------------------------------- */
/* WRITE support                                                             */
/* ------------------------------------------------------------------------- */
