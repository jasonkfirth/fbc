/*
 * Project: FreeBASIC CRT contract regression tests
 * File: crt-native-values.c
 * Purpose: Observe CRT layout and stream values through the real native SDK.
 * Responsibilities: Report public type sizes and share ordinary stream operations.
 * This file intentionally does NOT define substitute CRT entry points.
 */
#include <stddef.h>
#include <stdio.h>
#include <errno.h>

struct fb_crt_file_holder {
    unsigned char lead;
    FILE stream;
};

size_t fb_crt_file_size(void);
size_t fb_crt_file_offset(void);
void *fb_crt_stream(unsigned stream_index);
int fb_crt_errno(int *slot, int expected);
int fb_crt_append(void *stream);

size_t fb_crt_file_size(void)
{
    return sizeof(FILE);
}

size_t fb_crt_file_offset(void)
{
    return offsetof(struct fb_crt_file_holder, stream);
}

void *fb_crt_stream(unsigned stream_index)
{
    return __acrt_iob_func(stream_index);
}

int fb_crt_errno(int *slot, int expected)
{
    return slot == _errno() && *slot == expected;
}

int fb_crt_append(void *stream)
{
    return fputc('C', (FILE *)stream) == EOF ? -1 : 0;
}

/* end of crt-native-values.c */
