/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_data.c

    Purpose:

        Provide one section of the small NuttX runtime used by the
        generated-C RISC-V smoke target.

    This file is included by fb_nuttx_minrt.c while the target is still
    being brought up. Keeping the command bodies in smaller files makes
    the code easier to compare with the normal rtlib layout.
*/

void fb_DataRestore(void *data)
{
    fb_nuttx_data_cursor = (const FB_NUTTX_DATA_DESC *)data;
}

static const char *fb_nuttx_data_read_text(void)
{
    const FB_NUTTX_DATA_DESC *item;

    item = fb_nuttx_data_cursor;

    if ((item == NULL) || (item->type == FB_NUTTX_DATA_END))
        return NULL;

    fb_nuttx_data_cursor++;

    return (const char *)item->node;
}

void fb_DataReadBool(char *value)
{
    const char *text;

    if (value == NULL)
        return;

    *value = 0;

    text = fb_nuttx_data_read_text();

    if (text != NULL)
        *value = (char)(strtol(text, NULL, 10) != 0);
}

void fb_DataReadByte(char *value)
{
    const char *text;

    if (value == NULL)
        return;

    *value = 0;

    text = fb_nuttx_data_read_text();

    if (text != NULL)
        *value = (char)strtol(text, NULL, 10);
}

void fb_DataReadUByte(unsigned char *value)
{
    const char *text;

    if (value == NULL)
        return;

    *value = 0;

    text = fb_nuttx_data_read_text();

    if (text != NULL)
        *value = (unsigned char)strtoul(text, NULL, 10);
}

void fb_DataReadShort(short *value)
{
    const char *text;

    if (value == NULL)
        return;

    *value = 0;

    text = fb_nuttx_data_read_text();

    if (text != NULL)
        *value = (short)strtol(text, NULL, 10);
}

void fb_DataReadUShort(unsigned short *value)
{
    const char *text;

    if (value == NULL)
        return;

    *value = 0;

    text = fb_nuttx_data_read_text();

    if (text != NULL)
        *value = (unsigned short)strtoul(text, NULL, 10);
}

void fb_DataReadInt(int32 *value)
{
    const char *text;

    if (value == NULL)
        return;

    *value = 0;

    text = fb_nuttx_data_read_text();

    if (text != NULL)
        *value = (int32)strtol(text, NULL, 10);
}

void fb_DataReadUInt(uint32 *value)
{
    const char *text;

    if (value == NULL)
        return;

    *value = 0;

    text = fb_nuttx_data_read_text();

    if (text != NULL)
        *value = (uint32)strtoul(text, NULL, 10);
}

void fb_DataReadLongint(int64_t *value)
{
    const char *text;

    if (value == NULL)
        return;

    *value = 0;

    text = fb_nuttx_data_read_text();

    if (text != NULL)
        *value = (int64_t)strtoll(text, NULL, 10);
}

void fb_DataReadULongint(uint64_t *value)
{
    const char *text;

    if (value == NULL)
        return;

    *value = 0;

    text = fb_nuttx_data_read_text();

    if (text != NULL)
        *value = (uint64_t)strtoull(text, NULL, 10);
}

void fb_DataReadSingle(float *value)
{
    const char *text;

    if (value == NULL)
        return;

    *value = 0.0f;

    text = fb_nuttx_data_read_text();

    if (text != NULL)
        *value = (float)strtod(text, NULL);
}

void fb_DataReadDouble(double *value)
{
    const char *text;

    if (value == NULL)
        return;

    *value = 0.0;

    text = fb_nuttx_data_read_text();

    if (text != NULL)
        *value = strtod(text, NULL);
}

void fb_DataReadStr(void *dst_void, const int32 dst_len, const int32 fill_rem)
{
    const char *text;

    if (dst_void == NULL)
        return;

    text = fb_nuttx_data_read_text();

    if (text == NULL) {
        fb_StrAssign(dst_void, dst_len, "", 1, fill_rem);
        return;
    }

    fb_StrAssign(dst_void, dst_len, text, (int32)strlen(text), fill_rem);
}

/* ------------------------------------------------------------------------- */
/* Dynamic array support                                                     */
/* ------------------------------------------------------------------------- */

typedef struct FB_NUTTX_ARRAY_DIM {
    int32 elements;
    int32 lbound;
    int32 ubound;
} FB_NUTTX_ARRAY_DIM;

typedef struct FB_NUTTX_ARRAY {
    void *data;
    void *ptr;
    int32 size;
    int32 element_len;
    int32 dimensions;
    int32 flags;
    FB_NUTTX_ARRAY_DIM dimtb[8];
} FB_NUTTX_ARRAY;
