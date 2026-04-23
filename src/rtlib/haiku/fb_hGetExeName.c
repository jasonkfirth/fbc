/* get the executable's name (Haiku implementation) */

#include <image.h>
#include <string.h>

char *fb_hGetExeName( char *dst, ssize_t maxlen )
{
    image_info info;
    int32 cookie = 0;
    char *p;

    while (get_next_image_info(0, &cookie, &info) == B_OK) {
        if (info.type == B_APP_IMAGE) {
            strncpy(dst, info.name, maxlen - 1);
            dst[maxlen - 1] = '\0';

            p = strrchr(dst, '/');
            if (p != NULL)
                return p + 1;

            return dst;
        }
    }

    if (maxlen > 0)
        dst[0] = '\0';

    return dst;
}
