/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_print_using.c

    Purpose:

        Provide one section of the small NuttX runtime used by the
        generated-C RISC-V smoke target.

    This file is included by fb_nuttx_minrt.c while the target is still
    being brought up. Keeping the command bodies in smaller files makes
    the code easier to compare with the normal rtlib layout.
*/

static int32 fb_nuttx_using_format_len(void)
{
    if ((fb_nuttx_print_using_format == NULL) ||
        (fb_nuttx_print_using_format->data == NULL) ||
        (fb_nuttx_print_using_format->len <= 0))
        return 0;

    return fb_nuttx_print_using_format->len;
}

static int fb_nuttx_using_contains(const char ch)
{
    int32 i;
    int32 len;

    len = fb_nuttx_using_format_len();

    for (i = 0; i < len; i++) {
        if (fb_nuttx_print_using_format->data[i] == ch)
            return 1;
    }

    return 0;
}

static int32 fb_nuttx_using_decimal_places(void)
{
    int32 i;
    int32 len;
    int32 places;
    const char *format;

    len = fb_nuttx_using_format_len();

    if (len <= 0)
        return -1;

    format = fb_nuttx_print_using_format->data;

    for (i = 0; i < len; i++) {
        if (format[i] != '.')
            continue;

        places = 0;
        i++;

        while ((i < len) && (format[i] == '#')) {
            places++;
            i++;
        }

        return places;
    }

    return -1;
}

static int32 fb_nuttx_using_numeric_width(void)
{
    int32 len;

    len = fb_nuttx_using_format_len();

    if (len > 0)
        return len;

    return 0;
}

static void fb_nuttx_using_print_number(FILE *stream, const double value)
{
    char buffer[96];
    int32 width;
    int32 places;

    if (stream == NULL)
        return;

    width = fb_nuttx_using_numeric_width();
    places = fb_nuttx_using_decimal_places();

    if (width > 80)
        width = 80;

    if (places > 32)
        places = 32;

    if (places >= 0) {
        snprintf(buffer, sizeof(buffer), "%*.*f", (int)width, (int)places,
            value);
    } else if (width > 0) {
        snprintf(buffer, sizeof(buffer), "%*.0f", (int)width, value);
    } else {
        snprintf(buffer, sizeof(buffer), "%g", value);
    }

    fputs(buffer, stream);
}

int32 fb_PrintUsingInit(const FBSTRING *format)
{
    fb_nuttx_print_using_format = format;

    return 0;
}

int32 fb_PrintUsingStr(const int32 file_num, const FBSTRING *s,
    const int32 flags)
{
    FILE *stream;

    (void)flags;

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return -1;

    if (!fb_nuttx_using_contains('&')) {
        fb_PrintString(file_num, s, 0);
        return 0;
    }

    if ((s != NULL) && (s->data != NULL) && (s->len > 0))
        fwrite(s->data, 1, (size_t)s->len, stream);

    fflush(stream);

    return 0;
}

int32 fb_PrintUsingDouble(const int32 file_num, const double value,
    const int32 flags)
{
    FILE *stream;

    (void)flags;

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return -1;

    fb_nuttx_using_print_number(stream, value);
    fflush(stream);

    return 0;
}

int32 fb_PrintUsingSingle(const int32 file_num, const float value,
    const int32 flags)
{
    FILE *stream;

    (void)flags;

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return -1;

    fb_nuttx_using_print_number(stream, (double)value);
    fflush(stream);

    return 0;
}

int32 fb_PrintUsingLongint(const int32 file_num, const int64_t value,
    const int32 flags)
{
    FILE *stream;

    (void)flags;

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return -1;

    fb_nuttx_using_print_number(stream, (double)value);
    fflush(stream);

    return 0;
}

int32 fb_PrintUsingULongint(const int32 file_num, const uint64_t value,
    const int32 flags)
{
    FILE *stream;

    (void)flags;

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return -1;

    fb_nuttx_using_print_number(stream, (double)value);
    fflush(stream);

    return 0;
}

int32 fb_PrintUsingEnd(const int32 file_num)
{
    FILE *stream;

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return -1;

    fputc('\n', stream);
    fflush(stream);

    return 0;
}

/* ------------------------------------------------------------------------- */
/* Numeric string conversion                                                 */
/* ------------------------------------------------------------------------- */
