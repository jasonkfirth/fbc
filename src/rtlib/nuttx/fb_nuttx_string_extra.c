/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_string_extra.c

    Purpose:

        Provide one section of the small NuttX runtime used by the
        generated-C RISC-V smoke target.

    This file is included by fb_nuttx_minrt.c while the target is still
    being brought up. Keeping the command bodies in smaller files makes
    the code easier to compare with the normal rtlib layout.
*/

#if !defined(FB_NUTTX_USE_GENERIC_STR_EXTRA) || \
    (FB_NUTTX_USE_GENERIC_STR_EXTRA == 0)

FBSTRING *fb_LEFT(const FBSTRING *s, const int32 len)
{
    int32 copy_len;

    if ((s == NULL) || (s->data == NULL) || (len <= 0))
        return fb_nuttx_temp_string("", 0);

    copy_len = len;

    if (copy_len > s->len)
        copy_len = s->len;

    return fb_nuttx_temp_copy(s->data, copy_len);
}

FBSTRING *fb_RIGHT(const FBSTRING *s, const int32 len)
{
    int32 copy_len;

    if ((s == NULL) || (s->data == NULL) || (len <= 0))
        return fb_nuttx_temp_string("", 0);

    copy_len = len;

    if (copy_len > s->len)
        copy_len = s->len;

    return fb_nuttx_temp_copy(s->data + (s->len - copy_len), copy_len);
}

FBSTRING *fb_StrMid(const FBSTRING *s, const int32 start, const int32 len)
{
    int32 copy_len;
    int32 offset;

    if ((s == NULL) || (s->data == NULL) || (start <= 0) || (len <= 0))
        return fb_nuttx_temp_string("", 0);

    if (start > s->len)
        return fb_nuttx_temp_string("", 0);

    offset = start - 1;
    copy_len = len;

    if ((offset + copy_len) > s->len)
        copy_len = s->len - offset;

    return fb_nuttx_temp_copy(s->data + offset, copy_len);
}

int32 fb_StrInstr(const int32 start, const FBSTRING *haystack,
    const FBSTRING *needle)
{
    const char *found;
    int32 offset;

    if ((haystack == NULL) || (needle == NULL))
        return 0;

    if ((haystack->data == NULL) || (needle->data == NULL))
        return 0;

    if ((needle->len <= 0) || (start <= 0) || (start > haystack->len))
        return 0;

    offset = start - 1;
    found = strstr(haystack->data + offset, needle->data);

    if (found == NULL)
        return 0;

    return (int32)(found - haystack->data) + 1;
}

int32 fb_StrInstrRev(const FBSTRING *haystack, const FBSTRING *needle,
    const int32 start)
{
    int32 last_start;
    int32 pos;

    if ((haystack == NULL) || (needle == NULL))
        return 0;

    if ((haystack->data == NULL) || (needle->data == NULL))
        return 0;

    if ((haystack->len <= 0) || (needle->len <= 0) ||
        (needle->len > haystack->len))
        return 0;

    last_start = haystack->len - needle->len;

    if ((start > 0) && ((start - 1) < last_start))
        last_start = start - 1;

    for (pos = last_start; pos >= 0; pos--) {
        if (memcmp(haystack->data + pos, needle->data,
            (size_t)needle->len) == 0)
            return pos + 1;
    }

    return 0;
}

int32 fb_StrCompare(const void *lhs, const int32 lhs_len,
    const void *rhs, const int32 rhs_len)
{
    const char *left_data;
    const char *right_data;
    int32 left_len;
    int32 right_len;
    int32 min_len;
    int cmp;

    left_len = fb_nuttx_source_length(lhs, lhs_len);
    right_len = fb_nuttx_source_length(rhs, rhs_len);
    left_data = fb_nuttx_source_data(lhs, lhs_len);
    right_data = fb_nuttx_source_data(rhs, rhs_len);

    min_len = left_len;

    if (right_len < min_len)
        min_len = right_len;

    if (min_len > 0) {
        cmp = memcmp(left_data, right_data, (size_t)min_len);

        if (cmp != 0)
            return cmp;
    }

    if (left_len < right_len)
        return -1;

    if (left_len > right_len)
        return 1;

    return 0;
}

int32 fb_StrLen(const void *src, const int32 src_len)
{
    return fb_nuttx_source_length(src, src_len);
}

static FBSTRING *fb_nuttx_case_convert(const FBSTRING *s, int (*convert)(int))
{
    char *buffer;
    int32 i;

    if ((s == NULL) || (s->data == NULL) || (s->len <= 0))
        return fb_nuttx_temp_string("", 0);

    buffer = (char *)malloc((size_t)s->len + 1);

    if (buffer == NULL)
        return fb_nuttx_temp_string("", 0);

    for (i = 0; i < s->len; i++) {
        /*
            The C character conversion functions are only defined for
            unsigned char values and EOF. Cast before conversion so bytes
            above 127 do not become undefined behavior on platforms where
            char is signed.
        */
        buffer[i] = (char)convert((unsigned char)s->data[i]);
    }

    buffer[s->len] = '\0';

    return fb_nuttx_temp_string(buffer, s->len);
}

FBSTRING *fb_StrUcase2(const FBSTRING *s, const int32 mode)
{
    (void)mode;

    return fb_nuttx_case_convert(s, toupper);
}

FBSTRING *fb_StrLcase2(const FBSTRING *s, const int32 mode)
{
    (void)mode;

    return fb_nuttx_case_convert(s, tolower);
}

FBSTRING *fb_TRIM(const FBSTRING *s)
{
    const char *start;
    const char *finish;
    int32 len;

    if ((s == NULL) || (s->data == NULL) || (s->len <= 0))
        return fb_nuttx_temp_string("", 0);

    start = s->data;
    finish = s->data + s->len;

    while ((start < finish) && isspace((unsigned char)*start))
        start++;

    while ((finish > start) && isspace((unsigned char)finish[-1]))
        finish--;

    len = (int32)(finish - start);

    return fb_nuttx_temp_copy(start, len);
}

FBSTRING *fb_LTRIM(const FBSTRING *s)
{
    const char *start;
    const char *finish;
    int32 len;

    if ((s == NULL) || (s->data == NULL) || (s->len <= 0))
        return fb_nuttx_temp_string("", 0);

    start = s->data;
    finish = s->data + s->len;

    while ((start < finish) && isspace((unsigned char)*start))
        start++;

    len = (int32)(finish - start);

    return fb_nuttx_temp_copy(start, len);
}

FBSTRING *fb_RTRIM(const FBSTRING *s)
{
    const char *start;
    const char *finish;
    int32 len;

    if ((s == NULL) || (s->data == NULL) || (s->len <= 0))
        return fb_nuttx_temp_string("", 0);

    start = s->data;
    finish = s->data + s->len;

    while ((finish > start) && isspace((unsigned char)finish[-1]))
        finish--;

    len = (int32)(finish - start);

    return fb_nuttx_temp_copy(start, len);
}

#endif

/* end of fb_nuttx_string_extra.c */
