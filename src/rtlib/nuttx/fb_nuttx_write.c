/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_write.c

    Purpose:

        Provide one section of the small NuttX runtime used by the
        generated-C RISC-V smoke target.

    This file is included by fb_nuttx_minrt.c while the target is still
    being brought up. Keeping the command bodies in smaller files makes
    the code easier to compare with the normal rtlib layout.
*/

#ifndef FB_PRINT_NEWLINE
#define FB_PRINT_NEWLINE 0x00000001
#endif

#ifndef FB_PRINT_PAD
#define FB_PRINT_PAD 0x00000002
#endif

static void fb_nuttx_write_finish(FILE *stream, const int32 terminator)
{
    if (terminator == 1)
        fputc('\n', stream);
    else if (terminator == 2)
        fputc(',', stream);

    fflush(stream);
}

void fb_WriteVoid(const int32 file_num, const int32 mask)
{
    FILE *stream;

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return;

    if ((mask & FB_PRINT_NEWLINE) != 0)
        fputc('\n', stream);
    else if ((mask & FB_PRINT_PAD) != 0)
        fputc('\t', stream);

    fflush(stream);
}

void fb_WriteString(const int32 file_num, const FBSTRING *s,
    const int32 terminator)
{
    FILE *stream;
    int32 i;

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return;

    /*
        WRITE produces BASIC-readable text.  Strings are quoted, and embedded
        quotes are doubled so the value can be read back by a parser that
        understands BASIC-style quoted fields.
    */
    fputc('"', stream);

    if ((s != NULL) && (s->data != NULL) && (s->len > 0)) {
        for (i = 0; i < s->len; i++) {
            if (s->data[i] == '"')
                fputc('"', stream);

            fputc(s->data[i], stream);
        }
    }

    fputc('"', stream);
    fb_nuttx_write_finish(stream, terminator);
}

void fb_WriteWstr(const int32 file_num, const uint32_t *s,
    const int32 terminator)
{
    FILE *stream;
    char ch;
    int32 i;

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return;

    fputc('"', stream);

    if (s != NULL) {
        i = 0;

        while (s[i] != 0) {
            ch = (char)(s[i] & 0xffu);

            if (ch == '"')
                fputc('"', stream);

            fputc(ch, stream);
            i++;
        }
    }

    fputc('"', stream);
    fb_nuttx_write_finish(stream, terminator);
}

void fb_WriteInt(const int32 file_num, const int32 value,
    const int32 terminator)
{
    FILE *stream;

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return;

    fprintf(stream, "%" PRId32, value);
    fb_nuttx_write_finish(stream, terminator);
}

void fb_WriteUInt(const int32 file_num, const uint32 value,
    const int32 terminator);

void fb_WriteByte(const int32 file_num, const int8_t value,
    const int32 terminator)
{
    fb_WriteInt(file_num, (int32)value, terminator);
}

void fb_WriteUByte(const int32 file_num, const uint8_t value,
    const int32 terminator)
{
    fb_WriteUInt(file_num, (uint32)value, terminator);
}

void fb_WriteShort(const int32 file_num, const int16_t value,
    const int32 terminator)
{
    fb_WriteInt(file_num, (int32)value, terminator);
}

void fb_WriteUShort(const int32 file_num, const uint16_t value,
    const int32 terminator)
{
    fb_WriteUInt(file_num, (uint32)value, terminator);
}

void fb_WriteUInt(const int32 file_num, const uint32 value,
    const int32 terminator)
{
    FILE *stream;

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return;

    fprintf(stream, "%" PRIu32, value);
    fb_nuttx_write_finish(stream, terminator);
}

void fb_WriteLongint(const int32 file_num, const int64_t value,
    const int32 terminator)
{
    FILE *stream;

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return;

    fprintf(stream, "%" PRId64, value);
    fb_nuttx_write_finish(stream, terminator);
}

void fb_WriteULongint(const int32 file_num, const uint64_t value,
    const int32 terminator)
{
    FILE *stream;

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return;

    fprintf(stream, "%" PRIu64, value);
    fb_nuttx_write_finish(stream, terminator);
}

void fb_WriteBool(const int32 file_num, const uint8_t value,
    const int32 terminator)
{
    FILE *stream;

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return;

    fputs(value ? "true" : "false", stream);
    fb_nuttx_write_finish(stream, terminator);
}

void fb_WriteSingle(const int32 file_num, const float value,
    const int32 terminator)
{
    FILE *stream;

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return;

    fprintf(stream, "%.7g", (double)value);
    fb_nuttx_write_finish(stream, terminator);
}

void fb_WriteDouble(const int32 file_num, const double value,
    const int32 terminator)
{
    FILE *stream;

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return;

    fprintf(stream, "%.16g", value);
    fb_nuttx_write_finish(stream, terminator);
}

/* ------------------------------------------------------------------------- */
/* PRINT USING support                                                       */
/* ------------------------------------------------------------------------- */
