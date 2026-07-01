/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_input.c

    Purpose:

        Provide one section of the small NuttX runtime used by the
        generated-C RISC-V smoke target.

    This file is included by fb_nuttx_minrt.c while the target is still
    being brought up. Keeping the command bodies in smaller files makes
    the code easier to compare with the normal rtlib layout.
*/

static void fb_nuttx_print_prompt(const FBSTRING *prompt)
{
    if ((prompt == NULL) || (prompt->data == NULL) || (prompt->len <= 0))
        return;

    fwrite(prompt->data, 1, (size_t)prompt->len, stdout);
    fflush(stdout);
}

static int fb_nuttx_read_stream_line(FILE *stream, char *buffer,
    const size_t buffer_size)
{
    size_t len;

    if ((buffer == NULL) || (buffer_size == 0))
        return 0;

    if (stream == NULL)
        stream = stdin;

    if (fgets(buffer, (int)buffer_size, stream) == NULL) {
        buffer[0] = '\0';
        return 0;
    }

    len = strlen(buffer);

    while ((len > 0) && ((buffer[len - 1] == '\n') ||
        (buffer[len - 1] == '\r'))) {
        len--;
        buffer[len] = '\0';
    }

    return 1;
}

static FILE *fb_nuttx_selected_input_stream(void)
{
    FILE *stream;

    if (fb_nuttx_input_file_num <= 0)
        return stdin;

    stream = fb_nuttx_stream_for_file(fb_nuttx_input_file_num);

    if (stream != NULL)
        return stream;

    fb_nuttx_input_file_num = 0;

    return stdin;
}

static int fb_nuttx_read_input_line(char *buffer, const size_t buffer_size)
{
    return fb_nuttx_read_stream_line(fb_nuttx_selected_input_stream(), buffer,
        buffer_size);
}

static void fb_nuttx_input_reset_record(void)
{
    fb_nuttx_input_has_line = 0;
    fb_nuttx_input_pos = 0;
    fb_nuttx_input_line[0] = '\0';
}

static int fb_nuttx_input_load_record(void)
{
    if (fb_nuttx_read_input_line(fb_nuttx_input_line,
        sizeof(fb_nuttx_input_line)) == 0) {
        fb_nuttx_input_reset_record();
        return 0;
    }

    fb_nuttx_input_has_line = 1;
    fb_nuttx_input_pos = 0;

    return 1;
}

static void fb_nuttx_input_skip_blanks(void)
{
    while ((fb_nuttx_input_line[fb_nuttx_input_pos] == ' ') ||
        (fb_nuttx_input_line[fb_nuttx_input_pos] == '\t'))
        fb_nuttx_input_pos++;
}

static int fb_nuttx_input_next_field(char *buffer, const size_t buffer_size)
{
    char *dst;
    char *end;
    char ch;

    if ((buffer == NULL) || (buffer_size == 0))
        return 0;

    buffer[0] = '\0';

    if ((!fb_nuttx_input_has_line) ||
        (fb_nuttx_input_line[fb_nuttx_input_pos] == '\0')) {
        if (fb_nuttx_input_load_record() == 0)
            return 0;
    }

    fb_nuttx_input_skip_blanks();

    dst = buffer;
    end = buffer + buffer_size - 1;

    if (fb_nuttx_input_line[fb_nuttx_input_pos] == '"') {
        fb_nuttx_input_pos++;

        while ((ch = fb_nuttx_input_line[fb_nuttx_input_pos]) != '\0') {
            if (ch == '"') {
                if (fb_nuttx_input_line[fb_nuttx_input_pos + 1] != '"') {
                    fb_nuttx_input_pos++;
                    break;
                }

                fb_nuttx_input_pos++;
            }

            if (dst < end) {
                *dst = fb_nuttx_input_line[fb_nuttx_input_pos];
                dst++;
            }

            fb_nuttx_input_pos++;
        }

        *dst = '\0';
        fb_nuttx_input_skip_blanks();
    } else {
        while ((ch = fb_nuttx_input_line[fb_nuttx_input_pos]) != '\0') {
            if (ch == ',')
                break;

            if (dst < end) {
                *dst = ch;
                dst++;
            }

            fb_nuttx_input_pos++;
        }

        while ((dst > buffer) &&
            ((dst[-1] == ' ') || (dst[-1] == '\t')))
            dst--;

        *dst = '\0';
    }

    if (fb_nuttx_input_line[fb_nuttx_input_pos] == ',') {
        fb_nuttx_input_pos++;
    } else {
        fb_nuttx_input_has_line = 0;
        fb_nuttx_input_pos = 0;
    }

    return 1;
}

static int fb_nuttx_input_bool_value(const char *text)
{
    if (text == NULL)
        return 0;

    if ((text[0] == 't') || (text[0] == 'T') ||
        (text[0] == 'y') || (text[0] == 'Y'))
        return 1;

    if ((text[0] == '-') && (text[1] == '1') && (text[2] == '\0'))
        return 1;

    return strtol(text, NULL, 10) != 0;
}

int32 fb_LineInput(const FBSTRING *prompt, void *dst, const int32 dst_len,
    const int32 add_question, const int32 add_newline, const int32 is_input)
{
    char buffer[FB_NUTTX_INPUT_BUFFER_SIZE];

    (void)add_question;
    (void)add_newline;
    (void)is_input;

    fb_nuttx_input_reset_record();
    fb_nuttx_print_prompt(prompt);

    if (fb_nuttx_read_input_line(buffer, sizeof(buffer)) == 0)
        return -1;

    fb_StrAssign(dst, dst_len, buffer, (int32)strlen(buffer), -1);

    return 0;
}

int32 fb_ConsoleInput(const FBSTRING *prompt, const int32 add_question,
    const int32 add_newline)
{
    (void)add_question;
    (void)add_newline;

    fb_nuttx_input_file_num = 0;
    fb_nuttx_input_reset_record();
    fb_nuttx_print_prompt(prompt);

    return 0;
}

int32 fb_InputInt(int32 *value)
{
    char buffer[FB_NUTTX_INPUT_BUFFER_SIZE];

    if (value == NULL)
        return -1;

    *value = 0;

    if (fb_nuttx_input_next_field(buffer, sizeof(buffer)) == 0)
        return -1;

    *value = (int32)strtol(buffer, NULL, 10);

    return 0;
}

int32 fb_InputUint(uint32_t *value);

int32 fb_InputByte(int8_t *value)
{
    int32 int_value;
    int32 result;

    if (value == NULL)
        return -1;

    int_value = 0;
    result = fb_InputInt(&int_value);
    *value = (int8_t)int_value;

    return result;
}

int32 fb_InputUbyte(uint8_t *value)
{
    uint32_t uint_value;
    int32 result;

    if (value == NULL)
        return -1;

    uint_value = 0;
    result = fb_InputUint(&uint_value);
    *value = (uint8_t)uint_value;

    return result;
}

int32 fb_InputShort(int16_t *value)
{
    int32 int_value;
    int32 result;

    if (value == NULL)
        return -1;

    int_value = 0;
    result = fb_InputInt(&int_value);
    *value = (int16_t)int_value;

    return result;
}

int32 fb_InputUshort(uint16_t *value)
{
    uint32_t uint_value;
    int32 result;

    if (value == NULL)
        return -1;

    uint_value = 0;
    result = fb_InputUint(&uint_value);
    *value = (uint16_t)uint_value;

    return result;
}

int32 fb_InputUint(uint32_t *value)
{
    char buffer[FB_NUTTX_INPUT_BUFFER_SIZE];

    if (value == NULL)
        return -1;

    *value = 0;

    if (fb_nuttx_input_next_field(buffer, sizeof(buffer)) == 0)
        return -1;

    *value = (uint32_t)strtoul(buffer, NULL, 10);

    return 0;
}

int32 fb_InputLongint(int64_t *value)
{
    char buffer[FB_NUTTX_INPUT_BUFFER_SIZE];

    if (value == NULL)
        return -1;

    *value = 0;

    if (fb_nuttx_input_next_field(buffer, sizeof(buffer)) == 0)
        return -1;

    *value = (int64_t)strtoll(buffer, NULL, 10);

    return 0;
}

int32 fb_InputUlongint(uint64_t *value)
{
    char buffer[FB_NUTTX_INPUT_BUFFER_SIZE];

    if (value == NULL)
        return -1;

    *value = 0;

    if (fb_nuttx_input_next_field(buffer, sizeof(buffer)) == 0)
        return -1;

    *value = (uint64_t)strtoull(buffer, NULL, 10);

    return 0;
}

int32 fb_InputBool(uint8_t *value)
{
    char buffer[FB_NUTTX_INPUT_BUFFER_SIZE];

    if (value == NULL)
        return -1;

    *value = 0;

    if (fb_nuttx_input_next_field(buffer, sizeof(buffer)) == 0)
        return -1;

    *value = fb_nuttx_input_bool_value(buffer) ? 1 : 0;

    return 0;
}

int32 fb_InputSingle(float *value)
{
    char buffer[FB_NUTTX_INPUT_BUFFER_SIZE];

    if (value == NULL)
        return -1;

    *value = 0.0f;

    if (fb_nuttx_input_next_field(buffer, sizeof(buffer)) == 0)
        return -1;

    *value = (float)strtod(buffer, NULL);

    return 0;
}

int32 fb_InputDouble(double *value)
{
    char buffer[FB_NUTTX_INPUT_BUFFER_SIZE];

    if (value == NULL)
        return -1;

    *value = 0.0;

    if (fb_nuttx_input_next_field(buffer, sizeof(buffer)) == 0)
        return -1;

    *value = strtod(buffer, NULL);

    return 0;
}

int32 fb_InputString(void *dst_void, const int32 dst_len,
    const int32 add_question)
{
    char buffer[FB_NUTTX_INPUT_BUFFER_SIZE];

    (void)add_question;

    if (dst_void == NULL)
        return -1;

    if (fb_nuttx_input_next_field(buffer, sizeof(buffer)) == 0) {
        fb_StrAssign(dst_void, dst_len, "", 1, 0);
        return -1;
    }

    fb_StrAssign(dst_void, dst_len, buffer, (int32)strlen(buffer), -1);

    return 0;
}

static int32 fb_nuttx_wstr_assign_ascii(uint32_t *dst, int32 dst_chars,
    const char *src)
{
    int32 src_len;
    int32 copy_len;
    int32 i;

    if ((dst == NULL) || (src == NULL))
        return -1;

    src_len = (int32)strlen(src);

    if (dst_chars == 0)
        dst_chars = src_len + 1;

    if (dst_chars <= 0)
        return -1;

    copy_len = src_len;

    if (copy_len >= dst_chars)
        copy_len = dst_chars - 1;

    for (i = 0; i < copy_len; i++)
        dst[i] = (uint32_t)(unsigned char)src[i];

    dst[copy_len] = 0;

    return 0;
}

int32 fb_InputWstr(uint32_t *dst, const int32 dst_chars)
{
    char buffer[FB_NUTTX_INPUT_BUFFER_SIZE];

    if (dst == NULL)
        return -1;

    if (fb_nuttx_input_next_field(buffer, sizeof(buffer)) == 0) {
        dst[0] = 0;
        return -1;
    }

    return fb_nuttx_wstr_assign_ascii(dst, dst_chars, buffer);
}

uint32_t *fb_FileWstrInput(const int32 chars, const int32 file_num)
{
    FILE *stream;
    char *buffer;
    uint32_t *result;
    size_t got;
    int32 i;

    if (chars <= 0)
        return NULL;

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return NULL;

    buffer = (char *)malloc((size_t)chars + 1);

    if (buffer == NULL)
        return NULL;

    got = fread(buffer, 1, (size_t)chars, stream);
    buffer[got] = '\0';

    result = (uint32_t *)calloc((size_t)got + 1, sizeof(uint32_t));

    if (result == NULL) {
        free(buffer);
        return NULL;
    }

    for (i = 0; i < (int32)got; i++)
        result[i] = (uint32_t)(unsigned char)buffer[i];

    free(buffer);

    return result;
}

/* ------------------------------------------------------------------------- */
/* Dynamic string support                                                    */
/* ------------------------------------------------------------------------- */
