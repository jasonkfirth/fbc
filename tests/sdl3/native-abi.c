/*
    Project: FreeBASIC SDL3 binding tests
    File: native-abi.c
    Purpose: Exercise C calling conventions independently of the BASIC binding.
    Responsibilities: Exchange C values and check native macro semantics.
    This file intentionally does NOT contain: SDL wrappers or application logic.
*/

#include <SDL3/SDL.h>

bool SDL3_test_boolean(bool value)
{
    return value;
}

typedef bool (SDLCALL *SDL3_TestCallback)(bool value, void *userdata);

bool SDL3_test_callback(SDL3_TestCallback callback, void *userdata)
{
    if (callback == NULL)
        return false;

    return callback(false, userdata) && !callback(true, userdata);
}

SDL_FColor SDL3_test_color(SDL_FColor color)
{
    color.r = 1.0f - color.r;
    color.g = 1.0f - color.g;
    color.b = 1.0f - color.b;
    return color;
}

Uint64 SDL3_test_ns_to_seconds(Uint64 value)
{
    return SDL_NS_TO_SECONDS(value);
}

Uint64 SDL3_test_ns_to_ms(Uint64 value)
{
    return SDL_NS_TO_MS(value);
}

Uint64 SDL3_test_ns_to_us(Uint64 value)
{
    return SDL_NS_TO_US(value);
}

int SDL3_test_audio_bytes(SDL_AudioFormat format)
{
    return SDL_AUDIO_BYTESIZE(format);
}

size_t SDL3_test_audio_bytes_width(void)
{
    return sizeof(SDL_AUDIO_BYTESIZE(SDL_AUDIO_F32));
}

int SDL3_test_audio_frame_bytes(const SDL_AudioSpec *spec)
{
    return SDL_AUDIO_FRAMESIZE(*spec);
}

size_t SDL3_test_audio_frame_bytes_width(void)
{
    SDL_AudioSpec spec = { SDL_AUDIO_F32, 2, 8000 };
    return sizeof(SDL_AUDIO_FRAMESIZE(spec));
}

/* Header-defined helpers must be measured through the original C headers;
   looking only for exported SDL symbols would miss all of these functions. */
Uint16 SDL3_test_swap16(Uint16 value)
{
    return SDL_Swap16(value);
}
Uint32 SDL3_test_swap32(Uint32 value)
{
    return SDL_Swap32(value);
}
Uint64 SDL3_test_swap64(Uint64 value)
{
    return SDL_Swap64(value);
}

Uint32 SDL3_test_swap_float_bits(Uint32 bits)
{
    float value;
    SDL_memcpy(&value, &bits, sizeof(value));
    value = SDL_SwapFloat(value);
    SDL_memcpy(&bits, &value, sizeof(bits));
    return bits;
}

int SDL3_test_bit_index(Uint32 value)
{
    return SDL_MostSignificantBitIndex32(value);
}
bool SDL3_test_one_bit(Uint32 value)
{
    return SDL_HasExactlyOneBitSet32(value);
}
void SDL3_test_rect_to_float(const SDL_Rect *r, SDL_FRect *f)
{
    SDL_RectToFRect(r, f);
}
bool SDL3_test_point_in_rect(const SDL_Point *p, const SDL_Rect *r)
{
    return SDL_PointInRect(p, r);
}
bool SDL3_test_rect_empty(const SDL_Rect *r)
{
    return SDL_RectEmpty(r);
}
bool SDL3_test_rect_equal(const SDL_Rect *a, const SDL_Rect *b)
{
    return SDL_RectsEqual(a, b);
}
bool SDL3_test_point_in_float(const SDL_FPoint *p, const SDL_FRect *r)
{
    return SDL_PointInRectFloat(p, r);
}
bool SDL3_test_float_empty(const SDL_FRect *r)
{
    return SDL_RectEmptyFloat(r);
}
bool SDL3_test_float_equal_epsilon(const SDL_FRect *a, const SDL_FRect *b, float epsilon)
{
    return SDL_RectsEqualEpsilon(a, b, epsilon);
}
bool SDL3_test_float_equal(const SDL_FRect *a, const SDL_FRect *b)
{
    return SDL_RectsEqualFloat(a, b);
}
bool SDL3_test_size_mul(size_t a, size_t b, size_t *result)
{
    return SDL_size_mul_check_overflow(a, b, result);
}
bool SDL3_test_size_add(size_t a, size_t b, size_t *result)
{
    return SDL_size_add_check_overflow(a, b, result);
}

/* end of native-abi.c */
