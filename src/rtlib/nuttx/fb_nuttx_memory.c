/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_memory.c

    Purpose:

        Provide raw memory helper routines required by the generated-C
        NuttX smoke target.

    Responsibilities:

        - copy fixed-size memory blocks
        - clear destination tail bytes when the source block is shorter

    This file intentionally does NOT contain:

        - string descriptor ownership logic
        - array allocation logic
        - platform-specific device memory access
*/

#if !defined(FB_NUTTX_USE_GENERIC_MEMORY) || \
    (FB_NUTTX_USE_GENERIC_MEMORY == 0)
void fb_MemCopyClear(void *dst, const uint32 dst_len, const void *src,
    const uint32 src_len)
{
    uint32 copy_len;

    if ((dst == NULL) || (dst_len == 0))
        return;

    copy_len = src_len;

    if (copy_len > dst_len)
        copy_len = dst_len;

    if ((src != NULL) && (copy_len > 0))
        memcpy(dst, src, (size_t)copy_len);

    if (dst_len > copy_len) {
        memset((unsigned char *)dst + copy_len, 0,
            (size_t)(dst_len - copy_len));
    }
}
#endif

/* end of fb_nuttx_memory.c */
