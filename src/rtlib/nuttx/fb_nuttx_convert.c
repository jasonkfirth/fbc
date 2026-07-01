/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_convert.c

    Purpose:

        Provide one section of the small NuttX runtime used by the
        generated-C RISC-V smoke target.

    This file is included by fb_nuttx_minrt.c while the target is still
    being brought up. Keeping the command bodies in smaller files makes
    the code easier to compare with the normal rtlib layout.
*/

FBSTRING *fb_IntToStr(const int32 value)
{
    char buffer[32];

    snprintf(buffer, sizeof(buffer), "%" PRId32, value);

    return fb_nuttx_temp_copy(buffer, fb_nuttx_string_length(buffer, -1));
}

FBSTRING *fb_UIntToStr(const uint32 value)
{
    char buffer[32];

    snprintf(buffer, sizeof(buffer), "%" PRIu32, value);

    return fb_nuttx_temp_copy(buffer, fb_nuttx_string_length(buffer, -1));
}

FBSTRING *fb_IntToStrQB(const int32 value)
{
    char buffer[32];

    snprintf(buffer, sizeof(buffer), "% " PRId32, value);

    return fb_nuttx_temp_copy(buffer, fb_nuttx_string_length(buffer, -1));
}

FBSTRING *fb_UIntToStrQB(const uint32 value)
{
    char buffer[32];

    snprintf(buffer, sizeof(buffer), " %" PRIu32, value);

    return fb_nuttx_temp_copy(buffer, fb_nuttx_string_length(buffer, -1));
}

FBSTRING *fb_LongintToStr(const int64_t value)
{
    char buffer[32];

    snprintf(buffer, sizeof(buffer), "%" PRId64, value);

    return fb_nuttx_temp_copy(buffer, fb_nuttx_string_length(buffer, -1));
}

FBSTRING *fb_LongintToStrQB(const int64_t value)
{
    char buffer[32];

    snprintf(buffer, sizeof(buffer), "% " PRId64, value);

    return fb_nuttx_temp_copy(buffer, fb_nuttx_string_length(buffer, -1));
}

FBSTRING *fb_ULongintToStr(const uint64_t value)
{
    char buffer[32];

    snprintf(buffer, sizeof(buffer), "%" PRIu64, value);

    return fb_nuttx_temp_copy(buffer, fb_nuttx_string_length(buffer, -1));
}

FBSTRING *fb_ULongintToStrQB(const uint64_t value)
{
    char buffer[32];

    snprintf(buffer, sizeof(buffer), " %" PRIu64, value);

    return fb_nuttx_temp_copy(buffer, fb_nuttx_string_length(buffer, -1));
}

static FBSTRING *fb_nuttx_float_string(char *buffer, const size_t buffer_size)
{
    int32 len;

    len = fb_nuttx_string_length(buffer, -1);

    if ((len > 0) && (buffer[len - 1] == '.')) {
        buffer[len - 1] = '\0';
        len--;
    }

    (void)buffer_size;

    return fb_nuttx_temp_copy(buffer, len);
}

FBSTRING *fb_FloatToStr(const float value)
{
    char buffer[32];

    if (snprintf(buffer, sizeof(buffer), "%.7g", value) < 0)
        buffer[0] = '\0';

    return fb_nuttx_float_string(buffer, sizeof(buffer));
}

FBSTRING *fb_FloatToStrQB(const float value)
{
    char buffer[32];

    if (snprintf(buffer, sizeof(buffer), "% .7g", value) < 0)
        buffer[0] = '\0';

    return fb_nuttx_float_string(buffer, sizeof(buffer));
}

FBSTRING *fb_DoubleToStr(const double value)
{
    char buffer[64];

    if (snprintf(buffer, sizeof(buffer), "%.16g", value) < 0)
        buffer[0] = '\0';

    return fb_nuttx_float_string(buffer, sizeof(buffer));
}

FBSTRING *fb_DoubleToStrQB(const double value)
{
    char buffer[64];

    if (snprintf(buffer, sizeof(buffer), "% .16g", value) < 0)
        buffer[0] = '\0';

    return fb_nuttx_float_string(buffer, sizeof(buffer));
}

#if !defined(FB_NUTTX_USE_GENERIC_STR_BASE) || (FB_NUTTX_USE_GENERIC_STR_BASE == 0)
static FBSTRING *fb_nuttx_unsigned_to_base_ex(uint64_t value,
    const uint32 base, int32 digit_count)
{
    static const char digit_chars[] = "0123456789ABCDEF";
    char buffer[65];
    int pos;
    int used;

    if ((base < 2) || (base > 16))
        return fb_StrAllocTempDescZEx("", 0);

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

    return fb_nuttx_temp_copy(&buffer[pos],
        (int32)((int)sizeof(buffer) - 1 - pos));
}

static FBSTRING *fb_nuttx_unsigned_to_base(uint64_t value, const uint32 base)
{
    return fb_nuttx_unsigned_to_base_ex(value, base, 0);
}

FBSTRING *fb_HEX_i(const uint32 value);
FBSTRING *fb_OCT_i(const uint32 value);
FBSTRING *fb_BIN_i(const uint32 value);

FBSTRING *fb_HEX(const int32 value)
{
    return fb_HEX_i((uint32)value);
}

FBSTRING *fb_OCT(const int32 value)
{
    return fb_OCT_i((uint32)value);
}

FBSTRING *fb_BIN(const int32 value)
{
    return fb_BIN_i((uint32)value);
}

FBSTRING *fb_HEX_b(const uint8_t value)
{
    return fb_nuttx_unsigned_to_base(value, 16);
}

FBSTRING *fb_HEX_s(const uint16_t value)
{
    return fb_nuttx_unsigned_to_base(value, 16);
}

FBSTRING *fb_HEX_i(const uint32 value)
{
    return fb_nuttx_unsigned_to_base(value, 16);
}

FBSTRING *fb_HEX_l(const uint64_t value)
{
    return fb_nuttx_unsigned_to_base(value, 16);
}

FBSTRING *fb_HEX_p(const void *value)
{
    return fb_nuttx_unsigned_to_base((uintptr_t)value, 16);
}

FBSTRING *fb_HEXEx_b(const uint8_t value, const int32 digits)
{
    return fb_nuttx_unsigned_to_base_ex(value, 16, digits);
}

FBSTRING *fb_HEXEx_s(const uint16_t value, const int32 digits)
{
    return fb_nuttx_unsigned_to_base_ex(value, 16, digits);
}

FBSTRING *fb_HEXEx_i(const uint32 value, const int32 digits)
{
    return fb_nuttx_unsigned_to_base_ex(value, 16, digits);
}

FBSTRING *fb_HEXEx_l(const uint64_t value, const int32 digits)
{
    return fb_nuttx_unsigned_to_base_ex(value, 16, digits);
}

FBSTRING *fb_HEXEx_p(const void *value, const int32 digits)
{
    return fb_nuttx_unsigned_to_base_ex((uintptr_t)value, 16, digits);
}

FBSTRING *fb_OCT_b(const uint8_t value)
{
    return fb_nuttx_unsigned_to_base(value, 8);
}

FBSTRING *fb_OCT_s(const uint16_t value)
{
    return fb_nuttx_unsigned_to_base(value, 8);
}

FBSTRING *fb_OCT_i(const uint32 value)
{
    return fb_nuttx_unsigned_to_base(value, 8);
}

FBSTRING *fb_OCT_l(const uint64_t value)
{
    return fb_nuttx_unsigned_to_base(value, 8);
}

FBSTRING *fb_OCT_p(const void *value)
{
    return fb_nuttx_unsigned_to_base((uintptr_t)value, 8);
}

FBSTRING *fb_OCTEx_b(const uint8_t value, const int32 digits)
{
    return fb_nuttx_unsigned_to_base_ex(value, 8, digits);
}

FBSTRING *fb_OCTEx_s(const uint16_t value, const int32 digits)
{
    return fb_nuttx_unsigned_to_base_ex(value, 8, digits);
}

FBSTRING *fb_OCTEx_i(const uint32 value, const int32 digits)
{
    return fb_nuttx_unsigned_to_base_ex(value, 8, digits);
}

FBSTRING *fb_OCTEx_l(const uint64_t value, const int32 digits)
{
    return fb_nuttx_unsigned_to_base_ex(value, 8, digits);
}

FBSTRING *fb_OCTEx_p(const void *value, const int32 digits)
{
    return fb_nuttx_unsigned_to_base_ex((uintptr_t)value, 8, digits);
}

FBSTRING *fb_BIN_b(const uint8_t value)
{
    return fb_nuttx_unsigned_to_base(value, 2);
}

FBSTRING *fb_BIN_s(const uint16_t value)
{
    return fb_nuttx_unsigned_to_base(value, 2);
}

FBSTRING *fb_BIN_i(const uint32 value)
{
    return fb_nuttx_unsigned_to_base(value, 2);
}

FBSTRING *fb_BIN_l(const uint64_t value)
{
    return fb_nuttx_unsigned_to_base(value, 2);
}

FBSTRING *fb_BIN_p(const void *value)
{
    return fb_nuttx_unsigned_to_base((uintptr_t)value, 2);
}

FBSTRING *fb_BINEx_b(const uint8_t value, const int32 digits)
{
    return fb_nuttx_unsigned_to_base_ex(value, 2, digits);
}

FBSTRING *fb_BINEx_s(const uint16_t value, const int32 digits)
{
    return fb_nuttx_unsigned_to_base_ex(value, 2, digits);
}

FBSTRING *fb_BINEx_i(const uint32 value, const int32 digits)
{
    return fb_nuttx_unsigned_to_base_ex(value, 2, digits);
}

FBSTRING *fb_BINEx_l(const uint64_t value, const int32 digits)
{
    return fb_nuttx_unsigned_to_base_ex(value, 2, digits);
}

FBSTRING *fb_BINEx_p(const void *value, const int32 digits)
{
    return fb_nuttx_unsigned_to_base_ex((uintptr_t)value, 2, digits);
}
#endif

double fb_VAL(const FBSTRING *s)
{
    if ((s == NULL) || (s->data == NULL))
        return 0.0;

    return strtod(s->data, NULL);
}

int32 fb_VALINT(const FBSTRING *s)
{
    if ((s == NULL) || (s->data == NULL))
        return 0;

    return (int32)strtol(s->data, NULL, 0);
}

uint32 fb_VALUINT(const FBSTRING *s)
{
    if ((s == NULL) || (s->data == NULL))
        return 0;

    return (uint32)strtoul(s->data, NULL, 0);
}

int64_t fb_VALLNG(const FBSTRING *s)
{
    if ((s == NULL) || (s->data == NULL))
        return 0;

    return (int64_t)strtoll(s->data, NULL, 0);
}

uint64_t fb_VALULNG(const FBSTRING *s)
{
    if ((s == NULL) || (s->data == NULL))
        return 0;

    return (uint64_t)strtoull(s->data, NULL, 0);
}

#if !defined(FB_NUTTX_USE_GENERIC_MATH_FIX) || \
    (FB_NUTTX_USE_GENERIC_MATH_FIX == 0)
double fb_FIXDouble(double value)
{
    /*
        BASIC FIX truncates toward zero. A C integer cast has the same behavior
        for values inside the integer range, and avoiding floor()/ceil() keeps
        this seed runtime from depending on extra math-library entry points.
    */
    if (value > 9223372036854775807.0)
        return value;

    if (value < -9223372036854775807.0)
        return value;

    return (double)(int64_t)value;
}
#endif

#if !defined(FB_NUTTX_USE_GENERIC_MATH_CVN) || \
    (FB_NUTTX_USE_GENERIC_MATH_CVN == 0)
double fb_CVDFROMLONGINT(const int64_t value)
{
    double result;

    memcpy(&result, &value, sizeof(result));

    return result;
}

float fb_CVSFROML(const int32 value)
{
    float result;

    memcpy(&result, &value, sizeof(result));

    return result;
}

int32 fb_CVLFROMS(const float value)
{
    int32 result;

    memcpy(&result, &value, sizeof(result));

    return result;
}

int64_t fb_CVLONGINTFROMD(const double value)
{
    int64_t result;

    memcpy(&result, &value, sizeof(result));

    return result;
}
#endif

/* ------------------------------------------------------------------------- */
/* Character string helpers                                                  */
/* ------------------------------------------------------------------------- */

FBSTRING *fb_CHR(const int32 count, ...)
{
    char *buffer;
    va_list args;
    int32 i;

    if (count <= 0)
        return fb_nuttx_temp_string("", 0);

    buffer = (char *)malloc((size_t)count + 1);

    if (buffer == NULL)
        return fb_nuttx_temp_string("", 0);

    va_start(args, count);

    for (i = 0; i < count; i++)
        buffer[i] = (char)(va_arg(args, int) & 255);

    va_end(args);

    buffer[count] = '\0';

    return fb_nuttx_temp_string(buffer, count);
}

uint32 fb_ASC(const FBSTRING *s, const int32 position)
{
    if ((s == NULL) || (s->data == NULL) || (position <= 0))
        return 0;

    if (position > s->len)
        return 0;

    return (uint32)(unsigned char)s->data[position - 1];
}

#if !defined(FB_NUTTX_USE_GENERIC_STR_FILL) || \
    (FB_NUTTX_USE_GENERIC_STR_FILL == 0)
static FBSTRING *fb_nuttx_fill_string(const int32 len, const int32 ch)
{
    char *buffer;

    if (len <= 0)
        return fb_nuttx_temp_string("", 0);

    buffer = (char *)malloc((size_t)len + 1);

    if (buffer == NULL)
        return fb_nuttx_temp_string("", 0);

    memset(buffer, ch & 255, (size_t)len);
    buffer[len] = '\0';

    return fb_nuttx_temp_string(buffer, len);
}

FBSTRING *fb_SPACE(const int32 len)
{
    return fb_nuttx_fill_string(len, ' ');
}

FBSTRING *fb_StrFill1(const int32 len, const int32 ch)
{
    return fb_nuttx_fill_string(len, ch);
}
#endif

/* ------------------------------------------------------------------------- */
/* Pseudo-random numbers                                                     */
/* ------------------------------------------------------------------------- */

#if !defined(FB_NUTTX_USE_GENERIC_MATH_RND) || \
    (FB_NUTTX_USE_GENERIC_MATH_RND == 0)
void fb_Randomize(const double seed, const int32 mode)
{
    uint32_t new_seed;

    /*
        Mode 0 is the compiler's automatic mode.  The generated QB tests use
        it for RANDOMIZE and expect the historical 24-bit QuickBASIC
        sequence.  Other modes keep the earlier compact LCG path for now.
    */
    if (mode == 0) {
        union {
            double d;
            uint64_t i;
        } bits;

        uint32_t upper;

        bits.d = seed;
        upper = (uint32_t)(bits.i >> 32);
        upper ^= (upper >> 16);
        fb_nuttx_random_state = (((upper & 0xffffu) << 8) |
            (fb_nuttx_random_state & 0xffu)) & 0xffffffu;
        fb_nuttx_random_is_qb = 1;
        return;
    }

    if (seed < 0.0)
        new_seed = (uint32_t)(0.0 - seed);
    else
        new_seed = (uint32_t)seed;

    if (new_seed == 0)
        new_seed = 1;

    fb_nuttx_random_state = new_seed;
    fb_nuttx_random_is_qb = 0;
}

double fb_Rnd(const float argument)
{
    if (!fb_nuttx_random_is_qb) {
        if (argument < 0.0f)
            fb_nuttx_random_state = (uint32_t)(0.0f - argument);

        if (argument != 0.0f)
            fb_nuttx_random_state =
                (fb_nuttx_random_state * 1664525u) + 1013904223u;

        return (double)fb_nuttx_random_state / 4294967296.0;
    }

    /*
        QuickBASIC RND uses a 24-bit state.  RND(0) returns the current value,
        negative arguments reseed from the raw IEEE single-precision bits, and
        positive arguments advance the sequence.
    */
    if (argument == 0.0f)
        return (double)((float)fb_nuttx_random_state / (float)0x1000000);

    if (argument < 0.0f) {
        union {
            float f;
            uint32_t i;
        } bits;

        bits.f = argument;
        fb_nuttx_random_state = (bits.i + (bits.i >> 24)) & 0xffffffu;
    }

    fb_nuttx_random_state =
        ((fb_nuttx_random_state * 0xfd43fdu) + 0xc39ec3u) & 0xffffffu;

    return (double)((float)fb_nuttx_random_state / (float)0x1000000);
}
#endif
