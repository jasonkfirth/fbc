/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_string.c

    Purpose:

        Provide one section of the small NuttX runtime used by the
        generated-C RISC-V smoke target.

    This file is included by fb_nuttx_minrt.c while the target is still
    being brought up. Keeping the command bodies in smaller files makes
    the code easier to compare with the normal rtlib layout.
*/

static int32 fb_nuttx_source_length(const void *src, const int32 src_len)
{
    const char *text;
    const FBSTRING *s;
    int32 len;

    if (src == NULL)
        return 0;

    if (src_len == -1) {
        s = (const FBSTRING *)src;

        if ((s->data == NULL) || (s->len <= 0))
            return 0;

        return s->len;
    }

    if (fb_nuttx_is_fixed_length(src_len))
        return fb_nuttx_decode_fixed_length(src_len);

    if (src_len == 0) {
        size_t zlen;

        /*
            The normal rtlib treats a size of zero as a ZSTRING pointer
            whose length is not known at compile time.  Generated C uses
            this form for expressions such as *zstring_ptr, so measuring
            it here keeps comparison and LEN() behavior aligned with the
            main runtime.
        */
        zlen = strlen((const char *)src);

        if (zlen > (size_t)INT32_MAX)
            return INT32_MAX;

        return (int32)zlen;
    }

    if (src_len < 0)
        return 0;

    text = (const char *)src;
    len = src_len;

    /*
        A positive length is a bounded ZSTRING.  Generated C uses this
        form both for string literals and for ZSTRING * N variables.
        The visible string ends at the first NUL inside the bound, not
        necessarily at the final byte of the buffer.
    */
    while ((len > 0) && (*text != '\0')) {
        ++text;
        --len;
    }

    return src_len - len;
}

static const char *fb_nuttx_source_data(const void *src, const int32 src_len)
{
    const FBSTRING *s;

    if (src == NULL)
        return "";

    if (src_len == -1) {
        s = (const FBSTRING *)src;

        if (s->data == NULL)
            return "";

        return s->data;
    }

    return (const char *)src;
}

static int fb_nuttx_string_resize(FBSTRING *dst, const int32 len)
{
    char *new_data;
    size_t alloc_size;

    if (dst == NULL)
        return 0;

    if (len < 0)
        return 0;

    alloc_size = (size_t)len + 1;

    new_data = (char *)realloc(dst->data, alloc_size);

    if (new_data == NULL)
        return 0;

    dst->data = new_data;
    dst->len = len;
    dst->size = (int32)alloc_size;
    dst->data[len] = '\0';

    return 1;
}

static FBSTRING *fb_nuttx_zstring_assign(void *dst_void, const int32 dst_len,
    const void *src, const int32 src_len, const int32 fill_rem)
{
    char *dst;
    const char *src_data;
    int32 len;
    int32 copy_len;

    /*
        Generated C uses positive destination lengths for bounded zstring
        buffers.  These are not FBSTRING descriptors and must never be handed
        to realloc().

        A destination length of zero is the zstring-pointer case used by the
        normal rtlib.  As in C, the caller is assumed to have supplied enough
        storage for the result.
    */
    if ((dst_void == NULL) || (dst_len < 0))
        return NULL;

    dst = (char *)dst_void;
    len = fb_nuttx_source_length(src, src_len);
    copy_len = len;

    if ((dst_len > 0) && (copy_len >= dst_len))
        copy_len = dst_len - 1;

    if (copy_len < 0)
        copy_len = 0;

    if (copy_len > 0) {
        src_data = fb_nuttx_source_data(src, src_len);
        memcpy(dst, src_data, (size_t)copy_len);
    }

    dst[copy_len] = '\0';

    if ((fill_rem != 0) && (dst_len > 0) && ((copy_len + 1) < dst_len))
        memset(dst + copy_len + 1, 0, (size_t)(dst_len - copy_len - 1));

    return fb_nuttx_temp_string(dst, copy_len);
}

FBSTRING *fb_StrAssign(void *dst_void, const int32 dst_len, const void *src,
    const int32 src_len, const int32 fill_rem)
{
    FBSTRING *dst;
    const char *src_data;
    int32 len;
    int32 fixed_len;
    int32 copy_len;

    (void)fill_rem;

    if (dst_len >= 0)
        return fb_nuttx_zstring_assign(dst_void, dst_len, src, src_len,
            fill_rem);

    if (fb_nuttx_is_fixed_length(dst_len)) {
        if (dst_void == NULL)
            return NULL;

        fixed_len = fb_nuttx_decode_fixed_length(dst_len);
        len = fb_nuttx_source_length(src, src_len);
        copy_len = len;

        if (copy_len > fixed_len)
            copy_len = fixed_len;

        memset(dst_void, ' ', (size_t)fixed_len);

        if (copy_len > 0) {
            src_data = fb_nuttx_source_data(src, src_len);
            memcpy(dst_void, src_data, (size_t)copy_len);
        }

        return fb_nuttx_temp_string((char *)dst_void, fixed_len);
    }

    dst = (FBSTRING *)dst_void;
    len = fb_nuttx_source_length(src, src_len);

    if (fb_nuttx_string_resize(dst, len) == 0)
        return dst;

    src_data = fb_nuttx_source_data(src, src_len);

    if (len > 0)
        memcpy(dst->data, src_data, (size_t)len);

    return dst;
}

FBSTRING *fb_StrInit(void *dst_void, const int32 dst_len, const void *src,
    const int32 src_len, const int32 fill_rem)
{
    FBSTRING *dst;

    dst = (FBSTRING *)dst_void;

    if (dst == NULL)
        return NULL;

    dst->data = NULL;
    dst->len = 0;
    dst->size = 0;

    return fb_StrAssign(dst_void, dst_len, src, src_len, fill_rem);
}

void fb_StrAssignMid(FBSTRING *dst, const int32 start, const int32 count,
    const FBSTRING *src)
{
    const char *src_data;
    int32 src_len;
    int32 start_index;
    int32 copy_len;

    if ((dst == NULL) || (dst->data == NULL) || (src == NULL) ||
        (src->data == NULL))
        return;

    if ((start <= 0) || (start > dst->len))
        return;

    start_index = start - 1;
    src_data = src->data;
    src_len = src->len;

    if (src_len <= 0)
        return;

    copy_len = src_len;

    if ((count >= 0) && (count < copy_len))
        copy_len = count;

    if (copy_len > (dst->len - start_index))
        copy_len = dst->len - start_index;

    if (copy_len > 0)
        memcpy(dst->data + start_index, src_data, (size_t)copy_len);
}

FBSTRING *fb_StrConcatAssign(void *dst_void, const int32 dst_len,
    const void *src, const int32 src_len, const int32 fill_rem)
{
    FBSTRING *dst;
    const char *src_data;
    int32 old_len;
    int32 add_len;

    (void)dst_len;
    (void)fill_rem;

    dst = (FBSTRING *)dst_void;
    add_len = fb_nuttx_source_length(src, src_len);

    if ((dst == NULL) || (add_len <= 0))
        return dst;

    old_len = dst->len;

    if (old_len < 0)
        old_len = 0;

    if (fb_nuttx_string_resize(dst, old_len + add_len) == 0)
        return dst;

    src_data = fb_nuttx_source_data(src, src_len);
    memcpy(dst->data + old_len, src_data, (size_t)add_len);

    return dst;
}

FBSTRING *fb_StrConcat(FBSTRING *dst, void *str1, const int32 str1_len,
    void *str2, const int32 str2_len)
{
    const char *str1_data;
    const char *str2_data;
    int32 len1;
    int32 len2;

    if (dst == NULL)
        return fb_nuttx_temp_string("", 0);

    len1 = fb_nuttx_source_length(str1, str1_len);
    len2 = fb_nuttx_source_length(str2, str2_len);

    dst->data = NULL;
    dst->len = 0;
    dst->size = 0;

    if ((len1 > INT32_MAX - len2) || ((len1 + len2) <= 0))
        return dst;

    if (fb_nuttx_string_resize(dst, len1 + len2) == 0)
        return dst;

    str1_data = fb_nuttx_source_data(str1, str1_len);
    str2_data = fb_nuttx_source_data(str2, str2_len);

    if (len1 > 0)
        memcpy(dst->data, str1_data, (size_t)len1);

    if (len2 > 0)
        memcpy(dst->data + len1, str2_data, (size_t)len2);

    return dst;
}

FBSTRING *fb_StrAllocTempDescZ(const char *text)
{
    return fb_StrAllocTempDescZEx(text, -1);
}

FBSTRING *fb_StrAllocTempResult(FBSTRING *src)
{
    FBSTRING *desc;

    if (src == NULL)
        return fb_nuttx_temp_string("", 0);

    desc = fb_nuttx_temp_string(src->data, src->len);
    src->data = NULL;
    src->len = 0;
    src->size = 0;

    return desc;
}

#if !defined(FB_NUTTX_USE_GENERIC_STR_FILL) || \
    (FB_NUTTX_USE_GENERIC_STR_FILL == 0)
FBSTRING *fb_StrFill2(const ssize_t count, FBSTRING *src)
{
    FBSTRING result;
    const char *src_data;
    int32 src_len;
    ssize_t i;

    result.data = NULL;
    result.len = 0;
    result.size = 0;

    if ((count <= 0) || (src == NULL))
        return fb_StrAllocTempResult(&result);

    src_data = fb_nuttx_source_data(src, -1);
    src_len = fb_nuttx_source_length(src, -1);

    if (src_len <= 0)
        return fb_StrAllocTempResult(&result);

    if (count > (INT32_MAX / src_len))
        return fb_StrAllocTempResult(&result);

    if (fb_nuttx_string_resize(&result, (int32)(count * src_len)) == 0)
        return fb_StrAllocTempResult(&result);

    for (i = 0; i < count; i++)
        memcpy(result.data + (i * src_len), src_data, (size_t)src_len);

    return fb_StrAllocTempResult(&result);
}
#endif

static void fb_nuttx_fixed_set(void *dst_void, const int32 dst_len,
    const FBSTRING *src, const int right_align)
{
    char *dst;
    const char *src_data;
    int32 dst_actual_len;
    int32 copy_len;
    int32 start;

    if ((dst_void == NULL) || !fb_nuttx_is_fixed_length(dst_len))
        return;

    dst = (char *)dst_void;
    dst_actual_len = fb_nuttx_decode_fixed_length(dst_len);

    if (dst_actual_len <= 0)
        return;

    memset(dst, ' ', (size_t)dst_actual_len);

    if ((src == NULL) || (src->data == NULL) || (src->len <= 0))
        return;

    src_data = src->data;
    copy_len = src->len;

    if (copy_len > dst_actual_len)
        copy_len = dst_actual_len;

    start = 0;

    if (right_align != 0)
        start = dst_actual_len - copy_len;

    if (copy_len > 0)
        memcpy(dst + start, src_data, (size_t)copy_len);
}

void fb_StrLset(FBSTRING *dst, FBSTRING *src)
{
    int32 copy_len;

    if ((dst == NULL) || (dst->data == NULL) || (src == NULL) ||
        (dst->len <= 0))
        return;

    copy_len = src->len;

    if (copy_len < 0)
        copy_len = 0;

    if (copy_len > dst->len)
        copy_len = dst->len;

    if ((copy_len > 0) && (src->data != NULL))
        memcpy(dst->data, src->data, (size_t)copy_len);

    if (copy_len < dst->len)
        memset(dst->data + copy_len, ' ', (size_t)(dst->len - copy_len));

    dst->data[dst->len] = '\0';
}

void fb_StrRset(FBSTRING *dst, FBSTRING *src)
{
    int32 copy_len;
    int32 pad_len;

    if ((dst == NULL) || (dst->data == NULL) || (src == NULL) ||
        (dst->len <= 0))
        return;

    copy_len = src->len;

    if (copy_len < 0)
        copy_len = 0;

    if (copy_len > dst->len)
        copy_len = dst->len;

    pad_len = dst->len - copy_len;

    if (pad_len > 0)
        memset(dst->data, ' ', (size_t)pad_len);

    if ((copy_len > 0) && (src->data != NULL))
        memcpy(dst->data + pad_len, src->data, (size_t)copy_len);

    dst->data[dst->len] = '\0';
}

void fb_StrLsetANA(void *dst_void, const int32 dst_len, const FBSTRING *src)
{
    fb_nuttx_fixed_set(dst_void, dst_len, src, 0);
}

void fb_StrRsetANA(void *dst_void, const int32 dst_len, const FBSTRING *src)
{
    fb_nuttx_fixed_set(dst_void, dst_len, src, 1);
}

void fb_StrDelete(const FBSTRING *s)
{
    FBSTRING *owned;

    owned = (FBSTRING *)s;

    if ((owned != NULL) && (owned->data != NULL) && (owned->size > 0)) {
        free(owned->data);
        owned->data = NULL;
        owned->len = 0;
        owned->size = 0;
    }
}

void fb_hStrDelTemp(const FBSTRING *s)
{
    (void)s;
}

FBSTRING *fb_hStrAllocTemp_NoLock(FBSTRING *str, const ssize_t size)
{
    FBSTRING result;

    (void)str;

    result.data = NULL;
    result.len = 0;
    result.size = 0;

    if (size < 0)
        return fb_StrAllocTempResult(&result);

    if (size > INT32_MAX)
        return fb_StrAllocTempResult(&result);

    if (fb_nuttx_string_resize(&result, (int32)size) == 0)
        return fb_StrAllocTempResult(&result);

    return fb_StrAllocTempResult(&result);
}

FBSTRING *fb_hStrAllocTemp(FBSTRING *str, const ssize_t size)
{
    return fb_hStrAllocTemp_NoLock(str, size);
}

int fb_hStrDelTemp_NoLock(FBSTRING *s)
{
    fb_hStrDelTemp(s);

    return 0;
}

void fb_hStrCopy(char *dst, const char *src, const ssize_t bytes)
{
    if ((dst == NULL) || (bytes <= 0)) {
        if (dst != NULL)
            dst[0] = '\0';

        return;
    }

    if (src != NULL)
        memcpy(dst, src, (size_t)bytes);

    dst[bytes] = '\0';
}

static FBSTRING *fb_nuttx_temp_copy(const char *src, const int32 len)
{
    char *buffer;

    if (len <= 0)
        return fb_nuttx_temp_string("", 0);

    buffer = (char *)malloc((size_t)len + 1);

    if (buffer == NULL)
        return fb_nuttx_temp_string("", 0);

    memcpy(buffer, src, (size_t)len);
    buffer[len] = '\0';

    /*
        Temporary expression results are intentionally leaked in this seed
        runtime. The real runtime will replace this with normal temporary
        string lifetime tracking.
    */
    return fb_nuttx_temp_string(buffer, len);
}

static int32 fb_nuttx_trim_pattern_len(const FBSTRING *pattern)
{
    if ((pattern == NULL) || (pattern->data == NULL) || (pattern->len <= 0))
        return 0;

    return pattern->len;
}

static int fb_nuttx_trim_any_has_char(const FBSTRING *pattern, const char ch)
{
    int32 i;
    int32 len;

    len = fb_nuttx_trim_pattern_len(pattern);

    for (i = 0; i < len; i++) {
        if (pattern->data[i] == ch)
            return 1;
    }

    return 0;
}

static FBSTRING *fb_nuttx_trim_result(const char *data, const int32 len)
{
    if ((data == NULL) || (len <= 0))
        return fb_nuttx_temp_string("", 0);

    return fb_nuttx_temp_copy(data, len);
}

FBSTRING *fb_LTrimEx(FBSTRING *src, FBSTRING *pattern)
{
    const char *data;
    int32 len;
    int32 pattern_len;

    if ((src == NULL) || (src->data == NULL))
        return fb_nuttx_temp_string("", 0);

    data = src->data;
    len = src->len;
    pattern_len = fb_nuttx_trim_pattern_len(pattern);

    if ((len <= 0) || (pattern_len <= 0))
        return fb_nuttx_trim_result(data, len);

    if (pattern_len == 1) {
        while ((len > 0) && (*data == pattern->data[0])) {
            data++;
            len--;
        }
    } else {
        while ((len >= pattern_len) &&
            (memcmp(data, pattern->data, (size_t)pattern_len) == 0)) {
            data += pattern_len;
            len -= pattern_len;
        }
    }

    return fb_nuttx_trim_result(data, len);
}

FBSTRING *fb_LTrimAny(FBSTRING *src, FBSTRING *pattern)
{
    const char *data;
    int32 len;

    if ((src == NULL) || (src->data == NULL))
        return fb_nuttx_temp_string("", 0);

    data = src->data;
    len = src->len;

    while ((len > 0) && fb_nuttx_trim_any_has_char(pattern, *data)) {
        data++;
        len--;
    }

    return fb_nuttx_trim_result(data, len);
}

FBSTRING *fb_RTrimEx(FBSTRING *src, FBSTRING *pattern)
{
    const char *data;
    int32 len;
    int32 pattern_len;

    if ((src == NULL) || (src->data == NULL))
        return fb_nuttx_temp_string("", 0);

    data = src->data;
    len = src->len;
    pattern_len = fb_nuttx_trim_pattern_len(pattern);

    if ((len <= 0) || (pattern_len <= 0))
        return fb_nuttx_trim_result(data, len);

    if (pattern_len == 1) {
        while ((len > 0) && (data[len - 1] == pattern->data[0]))
            len--;
    } else {
        while ((len >= pattern_len) &&
            (memcmp(data + len - pattern_len, pattern->data,
                (size_t)pattern_len) == 0)) {
            len -= pattern_len;
        }
    }

    return fb_nuttx_trim_result(data, len);
}

FBSTRING *fb_RTrimAny(FBSTRING *src, FBSTRING *pattern)
{
    const char *data;
    int32 len;

    if ((src == NULL) || (src->data == NULL))
        return fb_nuttx_temp_string("", 0);

    data = src->data;
    len = src->len;

    while ((len > 0) && fb_nuttx_trim_any_has_char(pattern, data[len - 1]))
        len--;

    return fb_nuttx_trim_result(data, len);
}

FBSTRING *fb_TrimEx(FBSTRING *src, FBSTRING *pattern)
{
    FBSTRING *left;

    left = fb_LTrimEx(src, pattern);

    return fb_RTrimEx(left, pattern);
}

FBSTRING *fb_TrimAny(FBSTRING *src, FBSTRING *pattern)
{
    FBSTRING *left;

    left = fb_LTrimAny(src, pattern);

    return fb_RTrimAny(left, pattern);
}

static void fb_nuttx_read_binary_string(const FBSTRING *s, void *dst,
    const size_t bytes)
{
    size_t copy_len;

    if ((dst == NULL) || (bytes == 0))
        return;

    memset(dst, 0, bytes);

    if ((s == NULL) || (s->data == NULL) || (s->len <= 0))
        return;

    copy_len = (size_t)s->len;

    if (copy_len > bytes)
        copy_len = bytes;

    memcpy(dst, s->data, copy_len);
}

FBSTRING *fb_MKI(const int32 value)
{
    int16_t stored;

    stored = (int16_t)value;

    return fb_nuttx_temp_copy((const char *)&stored, (int32)sizeof(stored));
}

FBSTRING *fb_MKSHORT(const int16_t value)
{
    int16_t stored;

    stored = value;

    return fb_nuttx_temp_copy((const char *)&stored, (int32)sizeof(stored));
}

FBSTRING *fb_MKL(const int32 value)
{
    int32 stored;

    stored = value;

    return fb_nuttx_temp_copy((const char *)&stored, (int32)sizeof(stored));
}

FBSTRING *fb_MKLONGINT(const int64_t value)
{
    int64_t stored;

    stored = value;

    return fb_nuttx_temp_copy((const char *)&stored, (int32)sizeof(stored));
}

FBSTRING *fb_MKS(const float value)
{
    float stored;

    stored = value;

    return fb_nuttx_temp_copy((const char *)&stored, (int32)sizeof(stored));
}

FBSTRING *fb_MKD(const double value)
{
    double stored;

    stored = value;

    return fb_nuttx_temp_copy((const char *)&stored, (int32)sizeof(stored));
}

int16_t fb_CVSHORT(const FBSTRING *s)
{
    int16_t value;

    fb_nuttx_read_binary_string(s, &value, sizeof(value));

    return value;
}

int32 fb_CVL(const FBSTRING *s)
{
    int32 value;

    fb_nuttx_read_binary_string(s, &value, sizeof(value));

    return value;
}

int64_t fb_CVLONGINT(const FBSTRING *s)
{
    int64_t value;

    fb_nuttx_read_binary_string(s, &value, sizeof(value));

    return value;
}

float fb_CVS(const FBSTRING *s)
{
    float value;

    fb_nuttx_read_binary_string(s, &value, sizeof(value));

    return value;
}

double fb_CVD(const FBSTRING *s)
{
    double value;

    fb_nuttx_read_binary_string(s, &value, sizeof(value));

    return value;
}

ssize_t fb_StrInstrAny(ssize_t start, FBSTRING *src, FBSTRING *patt)
{
    ssize_t size_src;
    ssize_t size_patt;
    ssize_t i;
    ssize_t j;

    /*
        INSTR with ANY searches for the first character from the pattern
        string that appears in the source string.  FreeBASIC string search
        functions return one-based positions, with zero meaning "not found".
    */
    if ((src == NULL) || (src->data == NULL) ||
        (patt == NULL) || (patt->data == NULL))
        return 0;

    size_src = src->len;
    size_patt = patt->len;

    if ((size_src <= 0) || (size_patt <= 0) ||
        (start < 1) || (start > size_src))
        return 0;

    for (i = start - 1; i < size_src; i++) {
        for (j = 0; j < size_patt; j++) {
            if (src->data[i] == patt->data[j])
                return i + 1;
        }
    }

    return 0;
}

/* end of fb_nuttx_string.c */
