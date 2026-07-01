/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_wstring.c

    Purpose:

        Provide the first small WSTRING runtime surface for the NuttX
        generated-C smoke target.

    Responsibilities:

        - assign ASCII and WSTRING data into fixed WSTRING buffers
        - concatenate WSTRING and ASCII sources into temporary heap buffers
        - convert WSTRING data back into FBSTRING values for tests

    This file intentionally does NOT contain:

        - locale-aware Unicode conversion
        - shared runtime temporary-string lifetime tracking
        - filesystem, console, or graphics WSTRING helpers
*/

static int32 fb_nuttx_wstr_len(const uint32_t *text)
{
    int32 len;

    if (text == NULL)
        return 0;

    len = 0;

    while ((text[len] != 0) && (len < INT32_MAX))
        len++;

    return len;
}

static int32 fb_nuttx_astr_len(const void *src, const int32 src_len)
{
    const char *text;
    size_t measured_len;

    if (src == NULL)
        return 0;

    if (src_len == 0) {
        text = (const char *)src;
        measured_len = strlen(text);

        if (measured_len > (size_t)INT32_MAX)
            return INT32_MAX;

        return (int32)measured_len;
    }

    return fb_nuttx_source_length(src, src_len);
}

static const char *fb_nuttx_astr_data(const void *src, const int32 src_len)
{
    if (src == NULL)
        return "";

    if (src_len == 0)
        return (const char *)src;

    return fb_nuttx_source_data(src, src_len);
}

static uint32_t *fb_nuttx_wstr_alloc(const int32 len);
ssize_t fb_wstr_ConvToA(char *dst, ssize_t dst_chars, const uint32_t *src);

uint32_t *fb_wstr_AllocTemp(ssize_t chars)
{
    if ((chars <= 0) || (chars > INT32_MAX))
        return NULL;

    return fb_nuttx_wstr_alloc((int32)chars);
}

void fb_wstr_Del(uint32_t *text)
{
    free(text);
}

ssize_t fb_wstr_Len(const uint32_t *text)
{
    return (ssize_t)fb_nuttx_wstr_len(text);
}

void fb_wstr_Copy(uint32_t *dst, const uint32_t *src, ssize_t chars)
{
    if (dst == NULL)
        return;

    if (chars < 0)
        chars = 0;

    if ((src != NULL) && (chars > 0))
        memcpy(dst, src, (size_t)chars * sizeof(uint32_t));

    dst[chars] = 0;
}

uint32_t *fb_wstr_Move(uint32_t *dst, const uint32_t *src, ssize_t chars)
{
    if (dst == NULL)
        return NULL;

    if (chars < 0)
        chars = 0;

    if ((src != NULL) && (chars > 0))
        memmove(dst, src, (size_t)chars * sizeof(uint32_t));

    return dst + chars;
}

void fb_wstr_Fill(uint32_t *dst, uint32_t ch, ssize_t chars)
{
    ssize_t i;

    if (dst == NULL)
        return;

    if (chars < 0)
        chars = 0;

    for (i = 0; i < chars; i++)
        dst[i] = ch;

    dst[chars] = 0;
}

int fb_wstr_Compare(const uint32_t *str1, const uint32_t *str2,
    ssize_t chars)
{
    uint32_t ch1;
    uint32_t ch2;
    ssize_t i;

    if (str1 == str2)
        return 0;

    if (str1 == NULL)
        str1 = (const uint32_t []){ 0 };

    if (str2 == NULL)
        str2 = (const uint32_t []){ 0 };

    for (i = 0; i < chars; i++) {
        ch1 = str1[i];
        ch2 = str2[i];

        if (ch1 != ch2)
            return (ch1 < ch2) ? -1 : 1;

        if (ch1 == 0)
            return 0;
    }

    return 0;
}

ssize_t fb_wstr_CalcDiff(const uint32_t *first, const uint32_t *last)
{
    return last - first;
}

const uint32_t *fb_wstr_SkipChar(const uint32_t *text, ssize_t chars,
    uint32_t ch)
{
    if (text == NULL)
        return NULL;

    while ((chars > 0) && (*text == ch)) {
        text++;
        chars--;
    }

    return text;
}

const uint32_t *fb_wstr_SkipCharRev(const uint32_t *text, ssize_t chars,
    uint32_t ch)
{
    const uint32_t *cursor;

    if ((text == NULL) || (chars <= 0))
        return text;

    cursor = text + chars;

    while (chars > 0) {
        cursor--;

        if (*cursor != ch)
            return cursor + 1;

        chars--;
    }

    return cursor;
}

int fb_wstr_IsLower(uint32_t ch)
{
    return (ch >= (uint32_t)'a') && (ch <= (uint32_t)'z');
}

int fb_wstr_IsUpper(uint32_t ch)
{
    return (ch >= (uint32_t)'A') && (ch <= (uint32_t)'Z');
}

uint32_t fb_wstr_ToLower(uint32_t ch)
{
    if (fb_wstr_IsUpper(ch))
        return ch + ((uint32_t)'a' - (uint32_t)'A');

    return ch;
}

uint32_t fb_wstr_ToUpper(uint32_t ch)
{
    if (fb_wstr_IsLower(ch))
        return ch - ((uint32_t)'a' - (uint32_t)'A');

    return ch;
}

ssize_t fb_wstr_ConvFromA(uint32_t *dst, ssize_t dst_chars,
    const char *src)
{
    ssize_t i;

    if (dst == NULL)
        return 0;

    if (src == NULL) {
        dst[0] = 0;
        return 0;
    }

    for (i = 0; (i < dst_chars) && (src[i] != '\0'); i++)
        dst[i] = (uint32_t)(unsigned char)src[i];

    dst[i] = 0;

    return i;
}

#if !defined(FB_NUTTX_USE_GENERIC_WSTRING) || \
    (FB_NUTTX_USE_GENERIC_WSTRING == 0)
uint32_t *fb_WstrAssign(uint32_t *dst, ssize_t dst_chars,
    uint32_t *src)
{
    int32 len;
    int32 copy_len;

    if ((dst == NULL) || (dst_chars <= 0))
        return dst;

    len = fb_nuttx_wstr_len(src);
    copy_len = len;

    if (copy_len >= dst_chars)
        copy_len = dst_chars - 1;

    if (copy_len > 0)
        memcpy(dst, src, (size_t)copy_len * sizeof(uint32_t));

    dst[copy_len] = 0;

    return dst;
}

uint32 fb_WstrAsc(const uint32_t *text, ssize_t position)
{
    if ((text == NULL) || (position <= 0))
        return 0;

    if (position > fb_nuttx_wstr_len(text))
        return 0;

    return (uint32)text[position - 1];
}

#endif

uint32_t *fb_WstrAssignFromA(uint32_t *dst, ssize_t dst_chars,
    void *src, ssize_t src_len)
{
    const char *src_data;
    int32 len;
    int32 copy_len;
    int32 i;

    if ((dst == NULL) || (dst_chars <= 0))
        return dst;

    src_data = fb_nuttx_astr_data(src, src_len);
    len = fb_nuttx_astr_len(src, src_len);
    copy_len = len;

    if (copy_len >= dst_chars)
        copy_len = dst_chars - 1;

    for (i = 0; i < copy_len; i++)
        dst[i] = (uint32_t)(unsigned char)src_data[i];

    dst[copy_len] = 0;

    return dst;
}

uint32_t *fb_StrToWstr(const char *src)
{
    uint32_t *result;
    size_t measured_len;
    int32 len;
    int32 i;

    /*
        The full runtime uses locale or UTF conversion here depending on
        platform.  This NuttX seed runtime only promises byte-for-byte ASCII
        widening for now, which is enough for generated tests and simple
        embedded programs.
    */
    if (src == NULL)
        return NULL;

    measured_len = strlen(src);

    if (measured_len == 0)
        return NULL;

    if (measured_len > (size_t)INT32_MAX)
        return NULL;

    len = (int32)measured_len;
    result = fb_nuttx_wstr_alloc(len);

    if (result == NULL)
        return NULL;

    for (i = 0; i < len; i++)
        result[i] = (uint32_t)(unsigned char)src[i];

    return result;
}

static uint32_t *fb_nuttx_wstr_alloc(const int32 len)
{
    uint32_t *result;

    if (len < 0)
        return NULL;

    result = (uint32_t *)calloc((size_t)len + 1, sizeof(uint32_t));

    return result;
}

#if !defined(FB_NUTTX_USE_GENERIC_WSTRING) || \
    (FB_NUTTX_USE_GENERIC_WSTRING == 0)
ssize_t fb_WstrLen(uint32_t *text)
{
    return (ssize_t)fb_nuttx_wstr_len(text);
}

uint32_t *fb_WstrAlloc(ssize_t chars)
{
    if ((chars < 0) || (chars > INT32_MAX))
        return NULL;

    return fb_nuttx_wstr_alloc((int32)chars);
}

static uint32_t *fb_nuttx_wstr_fill(const int32 len, const uint32_t ch)
{
    uint32_t *result;
    int32 i;

    if (len <= 0)
        return fb_nuttx_wstr_alloc(0);

    result = fb_nuttx_wstr_alloc(len);

    if (result == NULL)
        return NULL;

    for (i = 0; i < len; i++)
        result[i] = ch;

    return result;
}

uint32_t *fb_WstrFill1(ssize_t len, int ch)
{
    return fb_nuttx_wstr_fill(len, (uint32_t)ch);
}

uint32_t *fb_WstrFill2(ssize_t len, const uint32_t *text)
{
    uint32_t ch;

    ch = 0;

    if (text != NULL)
        ch = text[0];

    return fb_nuttx_wstr_fill(len, ch);
}

uint32_t *fb_WstrSpace(ssize_t len)
{
    return fb_nuttx_wstr_fill(len, (uint32_t)' ');
}

static uint32_t fb_nuttx_wstr_lcase_char(const uint32_t ch)
{
    /*
        NuttX smoke tests are using the runtime without the normal locale
        tables.  Keep the first implementation intentionally ASCII-only so
        the behaviour is predictable on small embedded targets.
    */
    if ((ch >= (uint32_t)'A') && (ch <= (uint32_t)'Z'))
        return ch + ((uint32_t)'a' - (uint32_t)'A');

    return ch;
}

static uint32_t fb_nuttx_wstr_ucase_char(const uint32_t ch)
{
    if ((ch >= (uint32_t)'a') && (ch <= (uint32_t)'z'))
        return ch - ((uint32_t)'a' - (uint32_t)'A');

    return ch;
}

int fb_WstrCompare(const uint32_t *str1, const uint32_t *str2)
{
    static const uint32_t empty_wstr[1] = { 0 };
    uint32_t ch1;
    uint32_t ch2;
    int32 i;

    /*
        The shared runtime returns a lexical comparison result, not merely
        true/false.  Matching that contract matters because generated tests
        compare the result against zero.
    */
    if (str1 == str2)
        return 0;

    if (str1 == NULL)
        str1 = empty_wstr;

    if (str2 == NULL)
        str2 = empty_wstr;

    i = 0;

    for (;;) {
        ch1 = str1[i];
        ch2 = str2[i];

        if (ch1 != ch2)
            return (ch1 < ch2) ? -1 : 1;

        if (ch1 == 0)
            return 0;

        i++;
    }
}

uint32_t *fb_WstrLcase2(const uint32_t *src, int mode)
{
    uint32_t *result;
    int32 len;
    int32 i;

    (void)mode;

    len = fb_nuttx_wstr_len(src);
    result = fb_nuttx_wstr_alloc(len);

    if (result == NULL)
        return NULL;

    for (i = 0; i < len; i++)
        result[i] = fb_nuttx_wstr_lcase_char(src[i]);

    return result;
}

uint32_t *fb_WstrUcase2(const uint32_t *src, int mode)
{
    uint32_t *result;
    int32 len;
    int32 i;

    (void)mode;

    len = fb_nuttx_wstr_len(src);
    result = fb_nuttx_wstr_alloc(len);

    if (result == NULL)
        return NULL;

    for (i = 0; i < len; i++)
        result[i] = fb_nuttx_wstr_ucase_char(src[i]);

    return result;
}

uint32_t *fb_WstrConcat(const uint32_t *str1, const uint32_t *str2)
{
    uint32_t *result;
    int32 len1;
    int32 len2;

    len1 = fb_nuttx_wstr_len(str1);
    len2 = fb_nuttx_wstr_len(str2);

    if (len1 > INT32_MAX - len2)
        return fb_nuttx_wstr_alloc(0);

    result = fb_nuttx_wstr_alloc(len1 + len2);

    if (result == NULL)
        return NULL;

    if (len1 > 0)
        memcpy(result, str1, (size_t)len1 * sizeof(uint32_t));

    if (len2 > 0)
        memcpy(result + len1, str2, (size_t)len2 * sizeof(uint32_t));

    return result;
}

uint32_t *fb_WstrConcatWA(const uint32_t *str1, const void *str2,
    const int32 str2_size)
{
    uint32_t *result;
    const char *str2_data;
    int32 len1;
    int32 len2;
    int32 i;

    len1 = fb_nuttx_wstr_len(str1);
    len2 = fb_nuttx_astr_len(str2, str2_size);

    if (len1 > INT32_MAX - len2)
        return fb_nuttx_wstr_alloc(0);

    result = fb_nuttx_wstr_alloc(len1 + len2);

    if (result == NULL)
        return NULL;

    if (len1 > 0)
        memcpy(result, str1, (size_t)len1 * sizeof(uint32_t));

    str2_data = fb_nuttx_astr_data(str2, str2_size);

    for (i = 0; i < len2; i++)
        result[len1 + i] = (uint32_t)(unsigned char)str2_data[i];

    return result;
}

uint32_t *fb_WstrConcatAW(const void *str1, const int32 str1_size,
    const uint32_t *str2)
{
    uint32_t *result;
    const char *str1_data;
    int32 len1;
    int32 len2;
    int32 i;

    len1 = fb_nuttx_astr_len(str1, str1_size);
    len2 = fb_nuttx_wstr_len(str2);

    if (len1 > INT32_MAX - len2)
        return fb_nuttx_wstr_alloc(0);

    result = fb_nuttx_wstr_alloc(len1 + len2);

    if (result == NULL)
        return NULL;

    str1_data = fb_nuttx_astr_data(str1, str1_size);

    for (i = 0; i < len1; i++)
        result[i] = (uint32_t)(unsigned char)str1_data[i];

    if (len2 > 0)
        memcpy(result + len1, str2, (size_t)len2 * sizeof(uint32_t));

    return result;
}
#endif

void *fb_WstrAssignToA_Init(void *dst_void, ssize_t dst_len,
    uint32_t *src, int fill_rem)
{
    char *buffer;
    int32 len;
    FBSTRING *result;

    len = fb_nuttx_wstr_len(src);
    buffer = (char *)malloc((size_t)len + 1);

    if (buffer == NULL)
        return fb_StrInit(dst_void, dst_len, "", 1, fill_rem);

    fb_wstr_ConvToA(buffer, (ssize_t)len, src);
    result = fb_StrInit(dst_void, dst_len, buffer, len + 1, fill_rem);
    free(buffer);

    return result;
}

void *fb_WstrAssignToA(void *dst_void, ssize_t dst_len, uint32_t *src,
    int fill_rem)
{
    char *buffer;
    int32 len;
    void *result;

    len = fb_nuttx_wstr_len(src);
    buffer = (char *)malloc((size_t)len + 1);

    if (buffer == NULL)
        return fb_StrAssign(dst_void, dst_len, "", 1, fill_rem);

    fb_wstr_ConvToA(buffer, (ssize_t)len, src);
    result = fb_StrAssign(dst_void, dst_len, buffer, len + 1, fill_rem);
    free(buffer);

    return result;
}

ssize_t fb_wstr_ConvToA(char *dst, ssize_t dst_chars, const uint32_t *src)
{
    ssize_t i;

    if (dst == NULL)
        return 0;

    if (dst_chars <= 0) {
        dst[0] = '\0';
        return 0;
    }

    if (src == NULL) {
        dst[0] = '\0';
        return 0;
    }

    /*
        The seed runtime does not have locale-aware Unicode conversion yet.
        Keep the same low-byte behavior used by the existing NuttX WSTRING
        assignment helpers so gfx PRINT sees stable byte output.
    */
    for (i = 0; (i < dst_chars) && (src[i] != 0); i++)
        dst[i] = (char)(src[i] & 0xffu);

    dst[i] = '\0';

    return i;
}

FBSTRING *fb_WstrToStr(const uint32_t *src)
{
    char *buffer;
    int32 len;

    len = fb_nuttx_wstr_len(src);
    buffer = (char *)malloc((size_t)len + 1);

    if (buffer == NULL)
        return fb_nuttx_temp_string("", 0);

    fb_wstr_ConvToA(buffer, (ssize_t)len, src);

    return fb_nuttx_temp_string(buffer, len);
}

#if !defined(FB_NUTTX_USE_GENERIC_WSTRING) || \
    (FB_NUTTX_USE_GENERIC_WSTRING == 0)
static uint32_t *fb_nuttx_wstr_unsigned_to_base_ex(uint64_t value,
    const uint32 base, int32 digit_count)
{
    static const char digit_chars[] = "0123456789ABCDEF";
    char buffer[65];
    int pos;
    int used;
    uint32_t *result;
    int32 len;
    int32 i;

    if ((base < 2) || (base > 16))
        return fb_nuttx_wstr_alloc(0);

    if (digit_count < 0)
        digit_count = 0;

    if (digit_count > 64)
        digit_count = 64;

    pos = (int)sizeof(buffer) - 1;
    buffer[pos] = '\0';

    do {
        pos--;
        buffer[pos] = digit_chars[value % base];
        value /= base;
    } while ((value != 0) && (pos > 0));

    used = ((int)sizeof(buffer) - 1) - pos;

    while ((used < digit_count) && (pos > 0)) {
        pos--;
        buffer[pos] = '0';
        used++;
    }

    len = (int32)((int)sizeof(buffer) - 1 - pos);
    result = fb_nuttx_wstr_alloc(len);

    if (result == NULL)
        return NULL;

    for (i = 0; i < len; i++)
        result[i] = (uint32_t)(unsigned char)buffer[pos + i];

    return result;
}

uint32_t *fb_WstrHex_i(const unsigned int num)
{
    return fb_nuttx_wstr_unsigned_to_base_ex(num, 16, 0);
}

uint32_t *fb_WstrBin_i(const uint32 value)
{
    return fb_nuttx_wstr_unsigned_to_base_ex(value, 2, 0);
}

uint32_t *fb_WstrBinEx_i(const uint32 value, const int32 digits)
{
    return fb_nuttx_wstr_unsigned_to_base_ex(value, 2, digits);
}

uint32_t *fb_WstrOct_i(const uint32 value)
{
    return fb_nuttx_wstr_unsigned_to_base_ex(value, 8, 0);
}

uint32_t *fb_WstrOctEx_i(const uint32 value, const int32 digits)
{
    return fb_nuttx_wstr_unsigned_to_base_ex(value, 8, digits);
}

void fb_WstrDelete(uint32_t *text)
{
    free(text);
}
#endif

/* end of fb_nuttx_wstring.c */
